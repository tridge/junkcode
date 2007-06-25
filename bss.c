static char in_bss2[16384];
char in_data = 1;

main()
{
	static char in_bss[16384];
	char on_stack;
	
	printf("in_bss=%p in_bss2=%p on_stack=%p in_data=%p in_code=%p\n",
	       in_bss, in_bss2, &on_stack, &in_data, main);
}
