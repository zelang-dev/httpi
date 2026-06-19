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

void main_main(http_ini_t *ctx) {
	http_t *client_conn;
	httpi_t *client_ri;
	char client_err[256];
	int client_res, i;
	FILE *f;

	/* Server is running now */
	delay(5);

	/* Remove error files (in case they exist) */
	(void)fs_unlink("error.htm");
	(void)fs_unlink("error4xx.htm");
	(void)fs_unlink("error404.htm");

	/* Ask for something not existing - should get default 404 */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ASSERT(str_is(client_err, ""));
	ASSERT(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ASSERT_EQ_ABORT(-1, client_res);
	ASSERT(str_is(client_err, ""));
	client_ri = http_request_info(client_conn);
	ASSERT(client_ri != NULL);

	ASSERT(http_get_code(client_conn) == 404);
	http_close_connection(client_conn);
	delay(1);


	/* Try DELETE when put_delete_auth_file is not configured */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ASSERT(str_is(client_err, ""));
	ASSERT(client_conn != NULL);

	http_printf(client_conn, "DELETE /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ASSERT_EQ_ABORT(-1, client_res);
	ASSERT(str_is(client_err, ""));
	client_ri = http_request_info(client_conn);
	ASSERT(client_ri != NULL);

	ASSERT(http_get_code(client_conn) == 405);
	http_close_connection(client_conn);
	delay(1);

	/* Create an error.htm file */
	ASSERT((async_fprintf("error.htm", "wt", "err-all") > 0));

	/* Ask for something not existing - should get error.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ASSERT(str_is(client_err, ""));
	ASSERT(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ASSERT(client_res >= 0);
	ASSERT(str_is(client_err, ""));
	client_ri = http_request_info(client_conn);
	ASSERT(client_ri != NULL);

	ASSERT(http_get_code(client_conn) == 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ASSERT(client_res == 7);
	client_err[8] = 0;
	ASSERT(str_is(client_err, "err-all"));
	delay(1);

	/* Create an error4xx.htm file */
	ASSERT((async_fprintf("error4xx.htm", "wt", "err-4xx") > 0));

	/* Ask for something not existing - should get error4xx.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ASSERT(str_is(client_err, ""));
	ASSERT(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ASSERT(client_res >= 0);
	ASSERT(str_is(client_err, ""));
	client_ri = http_request_info(client_conn);
	ASSERT(client_ri != NULL);

	ASSERT(http_get_code(client_conn) == 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ASSERT(client_res == 7);
	client_err[8] = 0;
	ASSERT(str_is(client_err, "err-4xx"));
	delay(1);

	/* Create an error404.htm file */
	ASSERT((async_fprintf("error404.htm", "wt", "err-404") > 0));

	/* Ask for something not existing - should get error404.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ASSERT(str_is(client_err, ""));
	ASSERT(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ASSERT(client_res >= 0);
	ASSERT(str_is(client_err, ""));
	client_ri = http_request_info(client_conn);
	ASSERT(client_ri != NULL);

	ASSERT(http_get_code(client_conn) == 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ASSERT(client_res == 7);
	client_err[8] = 0;
	ASSERT(str_is(client_err, "err-404"));
	delay(1);

	/* Ask in a malformed way - should get error4xx.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ASSERT(str_is(client_err, ""));
	ASSERT(client_conn != NULL);

	http_printf(client_conn, "Gimme some file!\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ASSERT(client_res >= 0);
	ASSERT(str_is(client_err, ""));
	client_ri = http_request_info(client_conn);
	ASSERT(client_ri != NULL);

	ASSERT(http_get_code(client_conn) == 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ASSERT(client_res == 7);
	client_err[8] = 0;
	ASSERT(str_is(client_err, "err-4xx"));
	delay(1);

	/* Remove all error files created by this test */
	(void)fs_unlink("error.htm");
	(void)fs_unlink("error4xx.htm");
	(void)fs_unlink("error404.htm");

	/* Stop the server */
	http_stop(ctx);
}

TEST(http_error) {
	http_ini_t *ctx;

	char bad_thread_num[32] = "badnumber";
	user_callbacks_t callbacks;
	char errmsg[256];
	char client_err[256];
	const httpi_t *client_ri;
	int client_res, i;

	const char *OPTIONS[32];
	int opt_cnt = 0;

	OPTIONS[opt_cnt++] = "document_root";
	OPTIONS[opt_cnt++] = ".";
	OPTIONS[opt_cnt++] = "error_pages";
	OPTIONS[opt_cnt++] = "./";
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8080";
	OPTIONS[opt_cnt++] = "num_threads";
	OPTIONS[opt_cnt++] = bad_thread_num;
	OPTIONS[opt_cnt++] = "unknown_option";
	OPTIONS[opt_cnt++] = "unknown_option_value";
	OPTIONS[opt_cnt] = NULL;

	memset(&callbacks, 0, sizeof(callbacks));

	callbacks.log_message = log_msg_func;

	/* test with unknown option */
	memset(errmsg, 0, sizeof(errmsg));
	ctx = httpi_setup(0, &callbacks, (void *)errmsg, server_opts(OPTIONS), errmsg, sizeof(errmsg));

	/* Details of errmsg may vary, but it may not be empty */
	ASSERT(!str_is(errmsg, ""));
	ASSERT(ctx == NULL);
	ASSERT_STR("Invalid configuration option: unknown_option", errmsg);

	/* Remove invalid option */
	for (i = 0; OPTIONS[i]; i++) {
		if (strstr(OPTIONS[i], "unknown_option")) {
			OPTIONS[i] = 0;
		}
	}

	/* HTTP 1.0 GET request - server is not running */
	memset(client_err, 0, sizeof(client_err));
	http_t *client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));
	ASSERT(client_conn == NULL);

	/* Error message detail may vary - it may not be empty and should contain
	 * some information "connect" failed */
	ASSERT(!str_is(client_err, ""));
	ASSERT((strstr(client_err, "connect") != NULL));

	memset(errmsg, 0, sizeof(errmsg));
	ASSERT_TRUE(is_type((ctx = httpi_setup(0, &callbacks, (void *)errmsg, server_opts(OPTIONS),
		errmsg, sizeof(errmsg))), (data_types)DATA_HTTP_SERVER));
	ASSERT(str_is(errmsg, ""));
	httpi_start(ctx, main_main);

	/* HTTP 1.1 GET request - must not work, since server is already stopped  */
	memset(client_err, 0, sizeof(client_err));
	ASSERT_TRUE(((client_conn = http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err))) == NULL));
	ASSERT_FALSE(str_is(client_err, ""));

	return 0;
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s `http_error` page handling test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(http_error);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
