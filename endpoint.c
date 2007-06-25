struct dcerpc_endpoint_list {
	int count;
	char *names[];
};


struct dcerpc_endpoint_list xx = {
	2,
	{ "foo", "bar" }
};
