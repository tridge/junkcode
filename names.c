
static const char *rpcecho_endpoint_strings[] = {
	"rpcecho"
};

struct dcerpc_endpoint_list {
	int count;
	const char **names;
};

static const struct dcerpc_endpoint_list rpcecho_endpoints = {
	1,
	rpcecho_endpoint_strings
};
