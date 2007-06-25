
char* ipstr_list_add(char **ipstr_list, const struct in_addr *ip)
{
	char *new_str = NULL;

	if (*ipstr_list) {
		asprintf(&new_str, "%s:%s", *ipstr_list, inet_ntoa(*ip));
		free(*ipstr_list);
	} else {
		asprintf(&new_str, "%s", inet_ntoa(*ip));
	}
	
	*ipstr_list = new_str;
	return new_str;
}
