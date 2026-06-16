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
	/* Client var */
	http_t *client;
	char client_err_buf[256];
	char client_data_buf[256];
	const char *OPTIONS[32];

	const httpi_t *client_ri;
	int opt_cnt = 0;

	/* File content check var */
	FILE *f;
	char buf[1024];
	int len, ok;

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

TEST(error_log_file) {
	/* Server var */
	http_ini_t *ctx;
	const char *OPTIONS[32];
	int opt_cnt = 0;

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
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s error `access_log` file test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(httpi_https_start);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
