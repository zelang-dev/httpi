#include "../test_assert.h"

void check_func(int condition, string_t cond_txt, unsigned line) {
	if (!condition) {
		printf("Fail on line %d: [%s]"CLR_LN, line, cond_txt);
	}
}

#define ASSERT(expr)                                                           \
	do {                                                                       \
		check_func(expr, #expr, __LINE__);                                     \
	} while (0)

const char *lastMessage;
static int log_msg_func(const http_t *conn, const char *message) {
	http_ini_t *ctx;
	char *ud;

	ASSERT(conn != NULL);
	ctx = httpi_context((http_t *)conn);
	ASSERT(ctx != NULL);
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

static int test_throttle_begin_request(http_t *conn) {
	const httpi_t *ri;
	long unsigned len = 1024 * 10;
	const char *block = "0123456789";
	unsigned long i, blocklen;

	ASSERT(conn != NULL);
	ri = http_request_info(conn);
	ASSERT(ri != NULL);

	ASSERT(str_is(http_get_method(conn), "GET"));
	ASSERT(str_is(http_get_path(conn), "/throttle"));
	ASSERT(str_is(http_get_uri(conn), "/throttle"));
	ASSERT(str_is(http_version(conn), "1.0"));
	ASSERT(str_is(http_get_query(conn), "q"));
	ASSERT(str_is(http_remote_addr(ri), "127.0.0.1"));

	http_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: %lu\r\n"
		"Connection: close\r\n\r\n",
		len);

	blocklen = (unsigned long)strlen(block);
	for (i = 0; i < len; i += blocklen) {
		http_write(conn, block, blocklen);
	}

	return 987; /* Not a valid HTTP response code,
				 * but it should be written to the log and passed to
				 * end_request. */
}

static void test_throttle_end_request(http_t *conn, int reply_status_code) {
	const httpi_t *ri;

	ASSERT(conn != NULL);
	ri = http_request_info(conn);
	ASSERT(ri != NULL);

	ASSERT(str_is(http_get_method(conn), "GET"));
	ASSERT(str_is(http_get_path(conn), "/throttle"));
	ASSERT(str_is(http_get_uri(conn), "/throttle"));
	ASSERT(str_is(http_version(conn), "1.0"));
	ASSERT(str_is(http_get_query(conn), "q"));
	ASSERT(str_is(http_remote_addr(ri), "127.0.0.1"));

	ASSERT(reply_status_code == 987);
}

void main_main(http_ini_t *ctx) {
	/* deferring stop the test server */
	defer(http_stop, ctx);

	/* Client var */
	http_t *client;
	char *client_err_buf = task_erred_str();
	char client_data_buf[256];
	const httpi_t *client_ri;

	/* timing test */
	int r, data_read;
	time_t t0, t1;
	double dt;

	/* connect client */
	memset(client_err_buf, 0, ERR_BUF);
	memset(client_data_buf, 0, sizeof(client_data_buf));

	strcpy(client_err_buf, "reset-content");
	client = http_download("127.0.0.1",
		8080,
		0,
		"GET /throttle?q HTTP/1.0\r\n\r\n");

	ASSERT(ctx != NULL);
	ASSERT(str_is(client_err_buf, ""));

	client_ri = http_request_info(client);

	ASSERT(client_ri != NULL);
	ASSERT_EQ_ABORT(http_get_code(client), 200);
	ASSERT_EQ_ABORT(http_get_length(client), (1024 * 10));

	data_read = 0;
	t0 = time(NULL);
	while (data_read < http_get_length(client)) {
		r = http_read(client, client_data_buf, sizeof(client_data_buf));
		ASSERT(r >= 0);
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
	ASSERT((int)dt >= (8 * 1000));

	/* Nothing left to read */
	r = http_read(client, client_data_buf, sizeof(client_data_buf));
	ASSERT(r == 0);

	/* Close the client connection */
	http_close_connection(client);
}

TEST(throttle) {
	/* Server var */
	http_ini_t *ctx;
	user_callbacks_t callbacks;
	const char *OPTIONS[32];
	int opt_cnt = 0;

	/* Set options and start server */
	OPTIONS[opt_cnt++] = "document_root";
	OPTIONS[opt_cnt++] = ".";
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8080";
	OPTIONS[opt_cnt++] = "throttle";
	OPTIONS[opt_cnt++] = "*=1k";
	OPTIONS[opt_cnt] = NULL;

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.handler = test_throttle_begin_request;
	callbacks.handler_done = test_throttle_end_request;
	ASSERT_TRUE(is_type((ctx = httpi_setup(0, &callbacks, null, server_opts(OPTIONS),
		null, 0)), (data_types)DATA_HTTP_SERVER));
	httpi_start(ctx, main_main);

	return 0;
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
