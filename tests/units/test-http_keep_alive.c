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

void main_main(http_ini_t *ctx) {
	/* deferring stop the server and clean up */
	defer(http_stop, ctx);

	/* Client var */
	http_t *client_conn;
	const httpi_t *client_ri;
	char client_err[256];
	int client_res, i;
	const char *connection_header;

	/* HTTP 1.1 GET request */
	memset(client_err, 0, sizeof(client_err));
	client_conn =
		http_connect("127.0.0.1", 8081, 0, client_err, sizeof(client_err));

	ASSERT(str_is(client_err, ""));
	ASSERT(client_conn != NULL);

	http_printf(client_conn,
		"GET / HTTP/1.1\r\nHost: "
		"localhost:8081\r\nConnection: keep-alive\r\n\r\n");
	client_res =
		http_get_response(client_conn, client_err, sizeof(client_err), 10000);
	ASSERT(client_res == -1);
	ASSERT(str_is(client_err, ""));
	client_ri = http_request_info(client_conn);
	ASSERT(client_ri != NULL);

	ASSERT(http_get_code(client_conn) == 403);

	connection_header = http_get_header(client_conn, "Connection");

	/* Error replies will close the connection, even if keep-alive is set. */
	ASSERT(connection_header != NULL);
	ASSERT_STR_ABORT("close", connection_header);
	http_close_connection(client_conn);
}

TEST(keep_alive) {
	http_ini_t *ctx;
	const char *OPTIONS[] = {
		"listening_ports",
		"8081",
		"request_timeout_ms",
		"10000",
		"enable_keep_alive",
		"yes",
		"document_root",
		".",
		"enable_directory_listing",
		"no",
		NULL
	};

	ASSERT_TRUE(is_type((ctx = httpi_setup(0, null, null, server_opts(OPTIONS),
		null, 0)), (data_types)DATA_HTTP_SERVER));
	httpi_start(ctx, main_main);

	return 0;
}

TEST(list) {
	int result = 0;

	/* print headline */
	cout("HttPi %s `Keep Alive` test\n\n", httpi_version());

	/* start stop server */
	EXEC_TEST(keep_alive);

	return result;
}

int main(int argc, char **argv) {
	TEST_FUNC(list());
}
