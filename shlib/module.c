


const char *libversion(void);
const char *libversion2(void);

const char *mod_libversion(void) {
	return libversion();
}

#ifdef ENABLE_V2
const char *mod_libversion2(void) {
	return libversion2();
}
#endif
