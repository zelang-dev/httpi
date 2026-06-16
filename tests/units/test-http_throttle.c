#include "../test_assert.h"

static int s_total_tests = 0;
static int s_failed_tests = 0;

void check_func(int condition, string_t cond_txt, unsigned line) {
	++s_total_tests;
	if (!condition) {
		printf("Fail on line %d: [%s]"CLR_LN, line, cond_txt);
		++s_failed_tests;
	}
}

#define ASSERT(expr)                                                           \
	do {                                                                       \
		check_func(expr, #expr, __LINE__);                                     \
	} while (0)

const char *lastMessage;
static int log_msg_func(http_t *conn, const char *message) {
	http_ini_t *ctx;
	char *ud;

	ck_assert(conn != NULL);
	ctx = httpi_context(conn);
	ck_assert(ctx != NULL);
	ud = (char *)httpi_user_data(ctx);

	strncpy(ud, message, 255);
	ud[255] = 0;

	printf("LOG_MSG_FUNC: %s\n", message);
	lastMessage = message;

	return 1; /* Return 1 means "already handled" */
}

static int test_log_message(http_t *conn, const char *message) {
	(void)conn;

	printf("LOG_MESSAGE: %s\n", message);
	lastMessage = message;

	return 0; /* Return 0 means "not yet handled" */
}

void main_main(http_ini_t *ctx) {
	use_ca_certificate("cert.pem");
	tls_selfserver_set();
}

static int test_throttle_begin_request(struct mg_connection *conn) {
	const struct mg_request_info *ri;
	long unsigned len = 1024 * 10;
	const char *block = "0123456789";
	unsigned long i, blocklen;

	ck_assert(conn != NULL);
	ri = mg_get_request_info(conn);
	ck_assert(ri != NULL);

	ck_assert_str_eq(ri->request_method, "GET");
	ck_assert_str_eq(ri->request_uri, "/throttle");
	ck_assert_str_eq(ri->local_uri, "/throttle");
	ck_assert_str_eq(ri->http_version, "1.0");
	ck_assert_str_eq(ri->query_string, "q");
	ck_assert_str_eq(ri->remote_addr, "127.0.0.1");

	mg_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: %lu\r\n"
		"Connection: close\r\n\r\n",
		len);

	blocklen = (unsigned long)strlen(block);

	for (i = 0; i < len; i += blocklen) {
		mg_write(conn, block, blocklen);
	}

	mark_point();

	return 987; /* Not a valid HTTP response code,
				 * but it should be written to the log and passed to
				 * end_request. */
}


static void
test_throttle_end_request(const struct mg_connection *conn,
	int reply_status_code) {
	const struct mg_request_info *ri;

	ck_assert(conn != NULL);
	ri = mg_get_request_info(conn);
	ck_assert(ri != NULL);

	ck_assert_str_eq(ri->request_method, "GET");
	ck_assert_str_eq(ri->request_uri, "/throttle");
	ck_assert_str_eq(ri->local_uri, "/throttle");
	ck_assert_str_eq(ri->http_version, "1.0");
	ck_assert_str_eq(ri->query_string, "q");
	ck_assert_str_eq(ri->remote_addr, "127.0.0.1");

	ck_assert_int_eq(reply_status_code, 987);
}


TEST(throttle) {
	/* Server var */
	struct mg_context *ctx;
	struct mg_callbacks callbacks;
	const char *OPTIONS[32];
	int opt_cnt = 0;

	/* Client var */
	struct mg_connection *client;
	char client_err_buf[256];
	char client_data_buf[256];
	const struct mg_response_info *client_ri;

	/* timing test */
	int r, data_read;
	time_t t0, t1;
	double dt;

	mark_point();


/* Set options and start server */
#if !defined(NO_FILES)
	OPTIONS[opt_cnt++] = "document_root";
	OPTIONS[opt_cnt++] = ".";
#endif
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8080";
	OPTIONS[opt_cnt++] = "throttle";
	OPTIONS[opt_cnt++] = "*=1k";
	OPTIONS[opt_cnt] = NULL;

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.begin_request = test_throttle_begin_request;
	callbacks.end_request = test_throttle_end_request;

	ctx = test_mg_start(&callbacks, 0, OPTIONS, __LINE__);
	ck_assert(ctx != NULL);

	/* connect client */
	memset(client_err_buf, 0, sizeof(client_err_buf));
	memset(client_data_buf, 0, sizeof(client_data_buf));

	strcpy(client_err_buf, "reset-content");
	client = mg_download("127.0.0.1",
		8080,
		0,
		client_err_buf,
		sizeof(client_err_buf),
		"GET /throttle?q HTTP/1.0\r\n\r\n");

	ck_assert(ctx != NULL);
	ck_assert_str_eq(client_err_buf, "");

	client_ri = mg_get_response_info(client);

	ck_assert(client_ri != NULL);
	ck_assert_int_eq(client_ri->status_code, 200);

	ck_assert_int_eq(client_ri->content_length, 1024 * 10);

	data_read = 0;
	t0 = time(NULL);
	while (data_read < client_ri->content_length) {
		r = mg_read(client, client_data_buf, sizeof(client_data_buf));
		ck_assert_int_ge(r, 0);
		data_read += r;
	}
	t1 = time(NULL);
	dt = difftime(t1, t0) * 1000.0; /* Elapsed time in ms - in most systems
									 * only with second resolution */

	/* Time estimation: Data size is 10 kB, with 1 kB/s speed limit.
	 * The first block (1st kB) is transferred immediately, the second
	 * block (2nd kB) one second later, the third block (3rd kB) two
	 * seconds later, .. the last block (10th kB) nine seconds later.
	 * The resolution of time measurement using the "time" C library
	 * function is 1 second, so we should add +/- one second tolerance.
	 * Thus, download of 10 kB with 1 kB/s should not be faster than
	 * 8 seconds. */

	/* Check if there are at least 8 seconds */
	ck_assert_int_ge((int)dt, 8 * 1000);

	/* Nothing left to read */
	r = mg_read(client, client_data_buf, sizeof(client_data_buf));
	ck_assert_int_eq(r, 0);

	/* Close the client connection */
	mg_close_connection(client);

	/* Stop the server */
	test_mg_stop(ctx, __LINE__);

	mark_point();
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s throttle test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(throttle);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
