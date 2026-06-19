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

#define LARGE_FILE_SIZE (1024 * 1024 * 10)
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

static int test_large_file_begin_request(http_t *conn) {
	const httpi_t *ri;
	long unsigned len = LARGE_FILE_SIZE;
	const char *block = "0123456789";
	uint64_t i;
	size_t blocklen;

	ASSERT(conn != NULL);
	ri = http_request_info(conn);
	ASSERT(ri != NULL);

	ASSERT(str_is(http_get_method(conn), "GET"));
	ASSERT(str_is(http_version(conn), "1.1"));
	ASSERT(str_is(http_remote_addr(ri), "127.0.0.1"));
	ASSERT(http_get_query(conn) == NULL);
	ASSERT(http_get_uri(conn) != NULL);

	http_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: %lu\r\n"
		"Connection: close\r\n\r\n",
		len);

	blocklen = strlen(block);

	for (i = 0; i < len; i += blocklen) {
		http_write(conn, block, blocklen);
	}

	return 200;
}

void main_main(http_ini_t *ctx) {
	/* deferring stop the test server */
	defer(http_stop, ctx);

	use_ca_certificate("cert.pem");
	tls_selfserver_set();
	/* Client var */
	http_t *client;
	string client_err_buf = task_erred_str();
	char client_data_buf[256];
	const httpi_t *client_ri;
	int64_t data_read;
	int r;
	int retry, retry_ok_cnt, retry_fail_cnt;

	/* Try downloading several times */
	retry_ok_cnt = 0;
	retry_fail_cnt = 0;
	for (retry = 0; retry < 3; retry++) {
		int fail = 0;
		/* connect client */
		memset(client_err_buf, 0, sizeof(ERR_BUF));
		memset(client_data_buf, 0, sizeof(client_data_buf));

		client = http_download("127.0.0.1", 8443, 1,
				"GET /large.file HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

		ASSERT(client != NULL);
		ASSERT(str_is(client_err_buf, ""));

		client_ri = http_request_info(client);

		ASSERT(client_ri != NULL);
		ASSERT(http_get_code(client) == 200);
		ASSERT(http_get_length(client) == LARGE_FILE_SIZE);

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
			ASSERT(r == -1);
			retry_fail_cnt++;
		} else {
			ASSERT(r == 0);
			retry_ok_cnt++;
		}

		/* Close the client connection */
		http_close_connection(client);
	}

	ASSERT_EQ_ABORT(retry_fail_cnt, 0);
	ASSERT_EQ_ABORT(retry_ok_cnt, 3);
}

TEST(large_file) {
	/* Server var */
	http_ini_t *ctx;
	user_callbacks_t callbacks;
	const char *OPTIONS[32];
	int opt_cnt = 0;
	char errmsg[256] = {0};

	/* Set options and start server */
	OPTIONS[opt_cnt++] = "document_root";
	OPTIONS[opt_cnt++] = ".";
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8443s";
	//OPTIONS[opt_cnt++] = "ssl_certificate";
	//OPTIONS[opt_cnt++] = ssl_cert;
	OPTIONS[opt_cnt] = NULL;

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.handler = test_large_file_begin_request;
	callbacks.log_message = log_msg_func;

	ASSERT_TRUE(is_type((ctx = httpi_setup(0, &callbacks, (void *)errmsg, server_opts(OPTIONS),
		errmsg, sizeof(errmsg))), (data_types)DATA_HTTP_SERVER));
	ASSERT(str_is(errmsg, ""));
	httpi_start(ctx, main_main);

	return 0;

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
