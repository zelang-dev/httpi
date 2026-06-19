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
static int log_msg_func(http_t *conn, const char *message) {
	http_ini_t *ctx;
	char *ud;

	ASSERT(conn != NULL);
	ctx = httpi_context(conn);
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
	/* Client var */
	http_t *client;
	string client_err_buf;
	char client_data_buf[256];
	const httpi_t *client_ri;
	int len;

	/* Remove log files (they may exist from previous incomplete runs of
	 * this test) */
	(void)fs_unlink("error.log");
	(void)fs_unlink("access.log");

	/* connect client */
	client_err_buf = task_erred_str();
	memset(client_data_buf, 0, sizeof(client_data_buf));

	client = http_download("127.0.0.1",
		8080,
		0,
		"GET /not_existing_file.ext HTTP/1.0\r\n\r\n");

	ASSERT((ctx != NULL));
	ASSERT(str_is(client_err_buf, ""));

	client_ri = http_request_info(client);

	/* Check status - should be 404 Not Found */
	ASSERT((client_ri != NULL));
	ASSERT((http_get_code(client) == 404));

	/* Get body data (could exist, but does not have to) */
	len = http_read(client, client_data_buf, sizeof(client_data_buf));
	ASSERT((len >= 0));

	/* Close the client connection */
	http_close_connection(client);

	/* Stop the server */
	http_stop(ctx);
}

TEST(error_log_file) {
	/* Server var */
	http_ini_t *ctx;
	const char *OPTIONS[32];
	int opt_cnt = 0, result = 0;
	char err_buf[256];

	/* Set options and start server */
	OPTIONS[opt_cnt++] = "listening_ports";
	OPTIONS[opt_cnt++] = "8080";
	OPTIONS[opt_cnt++] = "error_log_file";
	OPTIONS[opt_cnt++] = "error.log";
	OPTIONS[opt_cnt++] = "access_log_file";
	OPTIONS[opt_cnt++] = "access.log";
	OPTIONS[opt_cnt++] = "document_root";
	OPTIONS[opt_cnt++] = ".";
	OPTIONS[opt_cnt] = NULL;

	ASSERT_TRUE(is_type(ctx = httpi_setup(0, NULL, NULL, server_opts(OPTIONS), err_buf, sizeof(err_buf)), (data_types)DATA_HTTP_SERVER));
	httpi_start(ctx, main_main);

	/* File content check var */
	FILE *f;
	char buf[1024];
	int len, ok;

	/* Check access.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("access.log", "r");
	ASSERT_MSG(f != NULL, "Cannot open access log file");
	ok = (NULL != fgets(buf, sizeof(buf) - 1, f));
	(void)fclose(f);
	ASSERT_MSG(ok, "Cannot read access log file");
	len = (int)strlen(buf);
	ASSERT(len > 0);
	ok = (NULL != strstr(buf, "not_existing_file.ext"));
	ASSERT_MSG(ok, "Did not find uri in access log file");
	ok = (NULL != strstr(buf, "404"));
	ASSERT_MSG(ok, "Did not find HTTP status code in access log file");

	/* Check error.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("error.log", "r");
	if (f) {
		(void)fgets(buf, sizeof(buf) - 1, f);
		fclose(f);
	}

	ASSERT_MSG(f == NULL,
		"Should not create error log file on 404, but got [%s]",
		buf);

/* Remove log files */
	(void)remove("error.log");
	(void)remove("access.log");

	/* Start server with bad options */
	ASSERT(str_is(OPTIONS[0], "listening_ports"));
	OPTIONS[1] = "bad !"; /* no r or s in string */

	ctx = httpi_setup(0, NULL, 0, server_opts(OPTIONS), buf, sizeof(buf));
	ASSERT_MSG(
		ctx == NULL,
		"Should not be able to start server with bad port configuration");

	/* Check access.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("access.log", "r");
	if (f) {
		(void)fgets(buf, sizeof(buf) - 1, f);
		fclose(f);
	}

	ASSERT_MSG(
		f == NULL,
		"Should not create access log file if start fails [%s]",
		buf);

	/* Check error.log */
	memset(buf, 0, sizeof(buf));
	f = fopen("error.log", "r");
	ASSERT_MSG(f != NULL, "Cannot open error log file");
	ok = (NULL != fgets(buf, sizeof(buf) - 1, f));
	(void)fclose(f);
	ASSERT_MSG(ok, "Cannot read error log file");
	ASSERT_TRUE(((len = (int)strlen(buf)) > 0));
	ASSERT_TRUE((ok = (NULL != strstr(buf, "port"))));
	ASSERT_MSG(ok, "Did not find port as error reason in error log file");

	/* Remove log files */
	(void)remove("error.log");
	(void)remove("access.log");

	return 0;
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s error `access_log` file test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(error_log_file);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
