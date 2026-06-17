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

#define LARGE_FILE_SIZE (1024 * 1024 * 10)

static int test_large_file_begin_request(http_t *conn) {
	const httpi_t*ri;
	long unsigned len = LARGE_FILE_SIZE;
	const char *block = "0123456789";
	uint64_t i;
	size_t blocklen;

	ck_assert(conn != NULL);
	ri = http_request_info(conn);
	ck_assert(ri != NULL);

	ck_assert_str_eq(http_get_method(conn), "GET");
	ck_assert_str_eq(http_get_version(conn), "1.1");
	//ck_assert_str_eq(ri->remote_addr, "127.0.0.1");
	ck_assert_ptr_eq(http_get_query(conn), NULL);
	ck_assert_ptr_ne(http_get_uri(conn), NULL);

	http_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: %lu\r\n"
		"Connection: close\r\n\r\n",
		len);

	blocklen = strlen(block);

	for (i = 0; i < len; i += blocklen) {
		http_write(conn, block, blocklen);
	}

	mark_point();

	return 200;
}

TEST(large_file) {
	/* Server var */
	http_ini_t *ctx;
	user_callbacks_t callbacks;
	const char *OPTIONS[32];
	int opt_cnt = 0;
#if !defined(NO_SSL)
	const char *ssl_cert = locate_ssl_cert();
#endif
	char errmsg[256] = {0};

	/* Client var */
	http_t *client;
	char client_err_buf[256];
	char client_data_buf[256];
	const httpi_t *client_ri;
	int64_t data_read;
	int r;
	int retry, retry_ok_cnt, retry_fail_cnt;

/* Set options and start server */
#if !defined(NO_FILES)
	OPTIONS[opt_cnt++] = "document_root";
	OPTIONS[opt_cnt++] = ".";
#endif
#if defined(NO_SSL)
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8080";
#else
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8443s";
	OPTIONS[opt_cnt++] = "ssl_certificate";
	OPTIONS[opt_cnt++] = ssl_cert;
#if defined(__MACH__) && defined(__APPLE__)
	/* The Apple builds on Travis CI seem to have problems with TLS1.x
	 * Allow SSLv3 and TLS */
	OPTIONS[opt_cnt++] = "ssl_protocol_version";
	OPTIONS[opt_cnt++] = "2";
#else
	/* The Linux builds on Travis CI work fine with TLS1.2 */
	OPTIONS[opt_cnt++] = "ssl_protocol_version";
	OPTIONS[opt_cnt++] = "4";
#endif
	ck_assert(ssl_cert != NULL);
#endif
	OPTIONS[opt_cnt] = NULL;


	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.handler = test_large_file_begin_request;
	callbacks.log_message = log_msg_func;

	ctx = test_mg_start(&callbacks, (void *)errmsg, OPTIONS, __LINE__);
	ck_assert_str_eq(errmsg, "");
	ck_assert(ctx != NULL);

	/* Try downloading several times */
	retry_ok_cnt = 0;
	retry_fail_cnt = 0;
	for (retry = 0; retry < 3; retry++) {
		int fail = 0;
		/* connect client */
		memset(client_err_buf, 0, sizeof(client_err_buf));
		memset(client_data_buf, 0, sizeof(client_data_buf));

		client =
			http_download("127.0.0.1",
#if defined(NO_SSL)
				8080,
				0,
#else
				8443,
				1,
#endif
				client_err_buf,
				sizeof(client_err_buf),
				"GET /large.file HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

		ck_assert(client != NULL);
		ck_assert_str_eq(client_err_buf, "");

		client_ri = http_request_info(client);

		ck_assert(client_ri != NULL);
		ck_assert_int_eq(http_get_code(client), 200);
		ck_assert_int_eq(http_get_length(client), LARGE_FILE_SIZE);

		data_read = 0;
		long long content_length = http_get_length(client);
		while (data_read < content_length) {
			r = http_read(client, client_data_buf, sizeof(client_data_buf));
			if (r < 0) {
				fail = 1;
				break;
			};
			data_read += r;
		}

		/* Nothing left to read */
		r = http_read(client, client_data_buf, sizeof(client_data_buf));
		if (fail) {
			ck_assert_int_eq(r, -1);
			retry_fail_cnt++;
		} else {
			ck_assert_int_eq(r, 0);
			retry_ok_cnt++;
		}

		/* Close the client connection */
		http_close_connection(client);
	}

#if defined(_WIN32)
// TODO: Check this problem on AppVeyor
// ck_assert_int_le(retry_fail_cnt, 2);
// ck_assert_int_ge(retry_ok_cnt, 1);
#else
	ck_assert_int_eq(retry_fail_cnt, 0);
	ck_assert_int_eq(retry_ok_cnt, 3);
#endif

	/* Stop the server */
	test_mg_stop(ctx, __LINE__);

	mark_point();
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s `large file` request test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(large_file);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
