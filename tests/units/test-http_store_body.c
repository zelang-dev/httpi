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

static int test_http_store_body_con_len = 20000;


static int test_http_store_body_put_delete_handler(http_t *conn, void *ignored) {
	char path[4096] = {0};
	const httpi_t *info = http_request_info(conn);
	http_ini_t *ctx = httpi_context(conn);
	int64_t rc;

	(void)ignored;

	mark_point();

	sprintf(path, "./%s", http_get_uri(conn));
	rc = http_store_body(ctx, conn, path);

	ck_assert_int_eq(test_http_store_body_con_len, rc);

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
		printf("http_store_body(%s) failed (ret: %ld)\n", path, (long)rc);

		return 500;
	}

	http_printf(conn,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type:text/plain;charset=UTF-8\r\n"
		"Connection:close\r\n\r\n"
		"%s OK (%ld bytes saved)\n",
		path,
		(long)rc);
	http_close_connection(conn);

	/* Debug output for tests */
	printf("http_store_body(%s) OK (%ld bytes)\n", path, (long)rc);

	mark_point();

	return 200;
}


static int test_http_store_body_begin_request_callback(http_t *conn) {
	const httpi_t *info = http_request_info(conn);

	string request_method = http_get_method(conn);

	/* Debug output for tests */
	printf("test_http_store_body_begin_request_callback called (%s)\n",
		 request_method);

	if ((strcmp(request_method, "PUT") == 0)
		|| (strcmp(request_method, "DELETE") == 0)) {
		return test_http_store_body_put_delete_handler(conn, NULL);
	}

	mark_point();

	return 0;
}


TEST(http_store_body) {
	/* Client data */
	char client_err_buf[256];
	char client_data_buf[1024];
	http_t *client;
	const httpi_t *client_ri;
	int r;
	char check_data[256];
	char *check_ptr;
	char errmsg[256] = {0};

	/* Server context handle */
	http_ini_t *ctx;
	http_clb_t callbacks;
	const char *options[] = {
#if !defined(NO_FILES)
		"document_root",
		".",
#endif
#if !defined(NO_CACHING)
		"static_file_max_age",
		"0",
#endif
		"listening_ports",
		"127.0.0.1:8082",
		"num_threads",
		"1",
		NULL
	};

	mark_point();

	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.handler = test_http_store_body_begin_request_callback;
	callbacks.log_message = log_msg_func;

	/* Initialize the library */
	mg_init_library(0);

	/* Start the server */
	ctx = http_start(&callbacks, (void *)errmsg, options);
	ck_assert_str_eq(errmsg, "");
	ck_assert(ctx != NULL);

	/* Run the server for 15 seconds */
	test_sleep(15);

	/* Call a test client */
	client = http_connect(
		"127.0.0.1", 8082, 0, client_err_buf, sizeof(client_err_buf));

	ck_assert_str_eq(client_err_buf, "");
	ck_assert(client != NULL);

	http_printf(client,
		"PUT /%s HTTP/1.0\r\nContent-Length: %i\r\n\r\n",
		"test_file_name.txt",
		test_http_store_body_con_len);

	r = 0;
	while (r < test_http_store_body_con_len) {
		int l = http_write(client, "1234567890", 10);
		ck_assert_int_eq(l, 10);
		r += 10;
	}

	r = http_get_response(client, client_err_buf, sizeof(client_err_buf), 10000);
	ck_assert_int_ge(r, 0);
	ck_assert_str_eq(client_err_buf, "");

	client_ri = http_request_info(client);
	ck_assert(client_ri != NULL);

	/* Response must be 200 OK  */
	ck_assert_int_eq(http_get_code(client), 200);

	/* Read PUT response */
	r = http_read(client, client_data_buf, sizeof(client_data_buf) - 1);
	ck_assert_int_gt(r, 0);
	client_data_buf[r] = 0;

	sprintf(check_data, "(%i bytes saved)", test_http_store_body_con_len);
	check_ptr = strstr(client_data_buf, check_data);
	ck_assert_ptr_ne(check_ptr, NULL);

	http_close_connection(client);

	/* Run the server for 5 seconds */
	test_sleep(5);

	/* Stop the server */
	test_mg_stop(ctx, __LINE__);

	/* Un-initialize the library */
	mg_exit_library();

	mark_point();
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
