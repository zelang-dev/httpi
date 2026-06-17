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
	http_t *client_conn;
	httpi_t *client_ri;
	char client_err[256];
	int client_res, i;
	FILE *f;

	/* Server is running now */
	test_sleep(1);

	/* Remove error files (in case they exist) */
	(void)remove("error.htm");
	(void)remove("error4xx.htm");
	(void)remove("error404.htm");


	/* Ask for something not existing - should get default 404 */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect_client("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ck_assert_str_eq(client_err, "");
	ck_assert(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ck_assert_int_ge(client_res, 0);
	ck_assert_str_eq(client_err, "");
	client_ri = http_request_info(client_conn);
	ck_assert(client_ri != NULL);

	ck_assert_int_eq(http_get_code(client_conn), 404);
	http_close_connection(client_conn);
	test_sleep(1);


	/* Try DELETE when put_delete_auth_file is not configured */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ck_assert_str_eq(client_err, "");
	ck_assert(client_conn != NULL);

	http_printf(client_conn, "DELETE /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ck_assert_int_ge(client_res, 0);
	ck_assert_str_eq(client_err, "");
	client_ri = http_request_info(client_conn);
	ck_assert(client_ri != NULL);

	ck_assert_int_eq(http_get_code(client_conn), 405);
	http_close_connection(client_conn);
	test_sleep(1);


	/* Create an error.htm file */
	f = fopen("error.htm", "wt");
	ck_assert(f != NULL);
	(void)fprintf(f, "err-all");
	(void)fclose(f);


	/* Ask for something not existing - should get error.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ck_assert_str_eq(client_err, "");
	ck_assert(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ck_assert_int_ge(client_res, 0);
	ck_assert_str_eq(client_err, "");
	client_ri = http_request_info(client_conn);
	ck_assert(client_ri != NULL);

	ck_assert_int_eq(http_get_code(client_conn), 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ck_assert_int_eq(client_res, 7);
	client_err[8] = 0;
	ck_assert_str_eq(client_err, "err-all");
	test_sleep(1);

	/* Create an error4xx.htm file */
	f = fopen("error4xx.htm", "wt");
	ck_assert(f != NULL);
	(void)fprintf(f, "err-4xx");
	(void)fclose(f);


	/* Ask for something not existing - should get error4xx.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ck_assert_str_eq(client_err, "");
	ck_assert(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ck_assert_int_ge(client_res, 0);
	ck_assert_str_eq(client_err, "");
	client_ri = http_request_info(client_conn);
	ck_assert(client_ri != NULL);

	ck_assert_int_eq(http_get_code(client_conn), 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ck_assert_int_eq(client_res, 7);
	client_err[8] = 0;
	ck_assert_str_eq(client_err, "err-4xx");
	test_sleep(1);

	/* Create an error404.htm file */
	f = fopen("error404.htm", "wt");
	ck_assert(f != NULL);
	(void)fprintf(f, "err-404");
	(void)fclose(f);


	/* Ask for something not existing - should get error404.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ck_assert_str_eq(client_err, "");
	ck_assert(client_conn != NULL);

	http_printf(client_conn, "GET /something/not/existing HTTP/1.0\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ck_assert_int_ge(client_res, 0);
	ck_assert_str_eq(client_err, "");
	client_ri = http_request_info(client_conn);
	ck_assert(client_ri != NULL);

	ck_assert_int_eq(http_get_code(client_conn), 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ck_assert_int_eq(client_res, 7);
	client_err[8] = 0;
	ck_assert_str_eq(client_err, "err-404");
	test_sleep(1);


	/* Ask in a malformed way - should get error4xx.htm */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ck_assert_str_eq(client_err, "");
	ck_assert(client_conn != NULL);

	http_printf(client_conn, "Gimme some file!\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ck_assert_int_ge(client_res, 0);
	ck_assert_str_eq(client_err, "");
	client_ri = http_request_info(client_conn);
	ck_assert(client_ri != NULL);

	ck_assert_int_eq(http_get_code(client_conn), 200);

	client_res = (int)http_read(client_conn, client_err, sizeof(client_err));
	http_close_connection(client_conn);
	ck_assert_int_eq(client_res, 7);
	client_err[8] = 0;
	ck_assert_str_eq(client_err, "err-4xx");
	test_sleep(1);


	/* Remove all error files created by this test */
	(void)remove("error.htm");
	(void)remove("error4xx.htm");
	(void)remove("error404.htm");


	/* Stop the server */
	test_mg_stop(ctx, __LINE__);


	/* HTTP 1.1 GET request - must not work, since server is already stopped  */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));

	ck_assert(client_conn == NULL);
	ck_assert_str_ne(client_err, "");

	test_sleep(1);
}

TEST(error_log) {
	http_ini_t *ctx;
	FILE *f;

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
	ctx = test_mg_start(&callbacks, (void *)errmsg, OPTIONS, 0);

	/* Details of errmsg may vary, but it may not be empty */
	ck_assert_str_ne(errmsg, "");
	ck_assert(ctx == NULL);
	ck_assert_str_eq(errmsg, "Invalid option: unknown_option");

	/* Remove invalid option */
	for (i = 0; OPTIONS[i]; i++) {
		if (strstr(OPTIONS[i], "unknown_option")) {
			OPTIONS[i] = 0;
		}
	}

	/* Test with bad num_thread option */
	memset(errmsg, 0, sizeof(errmsg));
	ctx = test_mg_start(&callbacks, (void *)errmsg, OPTIONS, 0);

	/* Details of errmsg may vary, but it may not be empty */
	ck_assert_str_ne(errmsg, "");
	ck_assert(ctx == NULL);
	ck_assert_str_eq(errmsg, "Invalid number of worker threads");

/* Set to a number - but use a number above the limit */
#if defined(MAX_WORKER_THREADS)
	sprintf(bad_thread_num, "%u", MAX_WORKER_THREADS + 1);
#else
	sprintf(bad_thread_num, "%lu", 1000000000lu);
#endif

	/* Test with bad num_thread option */
	memset(errmsg, 0, sizeof(errmsg));
	ctx = test_mg_start(&callbacks, (void *)errmsg, OPTIONS, 0);

	/* Details of errmsg may vary, but it may not be empty */
	ck_assert_str_ne(errmsg, "");
	ck_assert(ctx == NULL);
	ck_assert_str_eq(errmsg, "Too many worker threads");


	/* HTTP 1.0 GET request - server is not running */
	memset(client_err, 0, sizeof(client_err));
	http_t *client_conn =
		http_connect("127.0.0.1", 8080, 0, client_err, sizeof(client_err));
	ck_assert(client_conn == NULL);

	/* Error message detail may vary - it may not be empty and should contain
	 * some information "connect" failed */
	ck_assert_str_ne(client_err, "");
	ck_assert(strstr(client_err, "connect"));

	/* This time start the server with a valid configuration */
	sprintf(bad_thread_num, "%i", 1);
	memset(errmsg, 0, sizeof(errmsg));
	ctx = test_mg_start(&callbacks, (void *)errmsg, OPTIONS, __LINE__);

	ck_assert_str_eq(errmsg, "");
	ck_assert(ctx != NULL);
	return 0;
}

TEST(test_error_log_file) {
	/* Server var */
	http_ini_t *ctx;
	const char *OPTIONS[32];
	int opt_cnt = 0;

	/* Client var */
	http_t *client;
	char client_err_buf[256];
	char client_data_buf[256];
	const httpi_t *client_ri;

	/* File content check var */
	FILE *f;
	char buf[1024];
	int len, ok;

	mark_point();

	/* Set options and start server */
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8080";
	OPTIONS[opt_cnt++] = "error_log_file";
	OPTIONS[opt_cnt++] = "error.log";
	OPTIONS[opt_cnt++] = "access_log_file";
	OPTIONS[opt_cnt++] = "access.log";
#if !defined(NO_FILES)
	OPTIONS[opt_cnt++] = "document_root";
	OPTIONS[opt_cnt++] = ".";
#endif
	OPTIONS[opt_cnt] = NULL;

	ctx = test_mg_start(NULL, 0, OPTIONS, __LINE__);
	ck_assert(ctx != NULL);

	/* Remove log files (they may exist from previous incomplete runs of
	 * this test) */
	(void)remove("error.log");
	(void)remove("access.log");

	/* connect client */
	memset(client_err_buf, 0, sizeof(client_err_buf));
	memset(client_data_buf, 0, sizeof(client_data_buf));

	client = http_download("127.0.0.1",
		8080,
		0,
		client_err_buf,
		sizeof(client_err_buf),
		"GET /not_existing_file.ext HTTP/1.0\r\n\r\n");

	ck_assert(ctx != NULL);
	ck_assert_str_eq(client_err_buf, "");

	client_ri = http_request_info(client);

	/* Check status - should be 404 Not Found */
	ck_assert(client_ri != NULL);
	ck_assert_int_eq(http_get_code(client), 404);

	/* Get body data (could exist, but does not have to) */
	len = http_read(client, client_data_buf, sizeof(client_data_buf));
	ck_assert_int_ge(len, 0);

	/* Close the client connection */
	http_close_connection(client);

	/* Stop the server */
	test_mg_stop(ctx, __LINE__);


	/* Check access.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("access.log", "r");
	ck_assert_msg(f != NULL, "Cannot open access log file");
	ok = (NULL != fgets(buf, sizeof(buf) - 1, f));
	(void)fclose(f);
	ck_assert_msg(ok, "Cannot read access log file");
	len = (int)strlen(buf);
	ck_assert_int_gt(len, 0);
	ok = (NULL != strstr(buf, "not_existing_file.ext"));
	ck_assert_msg(ok, "Did not find uri in access log file");
	ok = (NULL != strstr(buf, "404"));
	ck_assert_msg(ok, "Did not find HTTP status code in access log file");

	/* Check error.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("error.log", "r");
	if (f) {
		(void)fgets(buf, sizeof(buf) - 1, f);
		fclose(f);
	}
	ck_assert_msg(f == NULL,
		"Should not create error log file on 404, but got [%s]",
		buf);

/* Remove log files */
	(void)remove("error.log");
	(void)remove("access.log");

	/* Start server with bad options */
	ck_assert_str_eq(OPTIONS[0], "listening_ports");
	OPTIONS[1] = "bad !"; /* no r or s in string */

	ctx = test_mg_start(NULL, 0, OPTIONS, 0);
	ck_assert_msg(
		ctx == NULL,
		"Should not be able to start server with bad port configuration");

	/* Check access.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("access.log", "r");
	if (f) {
		(void)fgets(buf, sizeof(buf) - 1, f);
		fclose(f);
	}
	ck_assert_msg(
		f == NULL,
		"Should not create access log file if start fails, but got [%s]",
		buf);

	/* Check error.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("error.log", "r");
	ck_assert_msg(f != NULL, "Cannot open access log file");
	ok = (NULL != fgets(buf, sizeof(buf) - 1, f));
	(void)fclose(f);
	ck_assert_msg(ok, "Cannot read access log file");
	len = (int)strlen(buf);
	ck_assert_int_gt(len, 0);
	ok = (NULL != strstr(buf, "port"));
	ck_assert_msg(ok, "Did not find port as error reason in error log file");


	/* Remove log files */
	(void)remove("error.log");
	(void)remove("access.log");

	mark_point();
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s `error_log` handling test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(error_log);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
