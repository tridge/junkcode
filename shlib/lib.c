static const char version[] = LIBVERSION;

const char *libversion(void) {
	return version;
}

#ifdef ENABLE_V2
const char *libversion2(void) {
	return version+1;
}
#endif
