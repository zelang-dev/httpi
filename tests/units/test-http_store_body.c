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

void check_func_msg(int condition, string_t cond_txt, unsigned line, string_t msg) {
	if (!condition) {
		printf("Fail on line %d: [%s] with %s"CLR_LN, line, cond_txt, msg);
	}
}

#define ASSERT_MSG(expr, msg, ...)												\
	do {                                                                       	\
		check_func_msg(expr, #expr, __LINE__, msg);							\
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

static int test_http_store_body_con_len = 20000;

static int test_http_store_body_put_delete_handler(http_t *conn, void *ignored) {
	char path[4096] = {0};
	const httpi_t *info = http_request_info(conn);
	http_ini_t *ctx = httpi_context(conn);
	int64_t rc;

	(void)ignored;
	sprintf(path, ".%s", http_get_uri(conn));
	rc = http_store_body(ctx, conn, path);

	ASSERT_EQ(test_http_store_body_con_len, rc);

	if (rc < 0) {
		http_printf(conn,
			"HTTP/1.1 500 Internal Server Error\r\n"
			"Content-Type:text/plain;charset=UTF-8\r\n"
			"Connection:close\r\n\r\n"
			"%s (ret: %ld)\n",
			path,
			(long)rc);
		http_close_connection(conn);

		/* Debug output for tests */
		printf("http_store_body(%s) failed (ret: %ld)"CLR_LN, path, (long)rc);

		return 500;
	}

	http_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type:text/plain;charset=UTF-8\r\n"
		"Connection:close\r\n\r\n"
		"%s OK (%ld bytes saved)\n",
		path,
		(long)rc);

	//http_close_connection(conn);

	/* Debug output for tests */
	printf("http_store_body(%s) OK (%ld bytes)"CLR_LN, path, (long)rc);

	return 200;
}

static int test_http_store_body_begin_request_callback(http_t *conn) {
	const httpi_t *info = http_request_info(conn);

	string request_method = http_get_method(conn);

	/* Debug output for tests */
	printf("test_http_store_body_begin_request_callback called (%s)"CLR_LN,
		 request_method);

	if ((strcmp(request_method, "PUT") == 0)
		|| (strcmp(request_method, "DELETE") == 0)) {
		return test_http_store_body_put_delete_handler(conn, NULL);
	}

	return 0;
}

void main_main(http_ini_t *ctx) {
	/* Client data */
	char client_err_buf[256];
	char client_data_buf[1024];
	http_t *client;
	const httpi_t *client_ri;
	int r;
	char check_data[256];
	char *check_ptr;

	/* deferring stop the test server */
	/* Un-initialize the library */
	defer(http_stop, ctx);

	use_ca_certificate("cert.pem");
	tls_selfserver_set();

	/* Run the server for 15 seconds */
	delay(15);

	/* Call a test client */
	client = http_connect(
		"127.0.0.1", 8082, 0, client_err_buf, sizeof(client_err_buf));

	ASSERT(str_is(client_err_buf, ""));
	ASSERT(client != NULL);

	http_printf(client,
		"PUT /%s HTTP/1.0\r\nContent-Length: %i\r\n\r\n",
		"test_file_name.txt",
		test_http_store_body_con_len);

	r = 0;
	while (r < test_http_store_body_con_len) {
		int l = http_write(client, "1234567890", 10);
		ASSERT(l == 10);
		r += 10;
	}

	r = http_get_response(client, client_err_buf, sizeof(client_err_buf), 10000);
	ASSERT(r >= 0);
	ASSERT(str_is(client_err_buf, ""));

	client_ri = http_request_info(client);
	ASSERT(client_ri != NULL);

	/* Response must be 200 OK  */
	ASSERT(http_get_code(client) == 200);

	/* Read PUT response */
	r = http_read(client, client_data_buf, sizeof(client_data_buf) - 1);
	ASSERT(r > 0);
	client_data_buf[r] = 0;

	sprintf(check_data, "(%i bytes saved)", test_http_store_body_con_len);
	check_ptr = strstr(client_data_buf, check_data);
	ASSERT(check_ptr != NULL);

	http_close_connection(client);
}

TEST(http_store_body) {
	char errmsg[256] = {0};
	/* Server context handle */
	http_ini_t *ctx;
	user_callbacks_t callbacks;
	const char *options[] = {
		"document_root",
		".",
		"static_file_max_age",
		"0",
		"listening_ports",
		"127.0.0.1:8082",
		"num_threads",
		"1",
		NULL
	};

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.handler = test_http_store_body_begin_request_callback;
	callbacks.log_message = log_msg_func;

	/* Initialize the library */
	ASSERT_TRUE(is_type((ctx = httpi_setup(0, &callbacks, (void *)errmsg, server_opts(options),
		errmsg, sizeof(errmsg))), (data_types)DATA_HTTP_SERVER));
	ASSERT_TRUE(str_is(errmsg, ""));

	/* Start the server */
	httpi_start(ctx, main_main);

	return 0;
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s incoming body store test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(http_store_body);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
