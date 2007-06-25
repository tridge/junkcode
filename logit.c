static void logit(const char *format, ...)
{
	va_list ap;
	FILE *f = fopen("/tmp/mylog", "a");
	if (!f) return;
	va_start(ap, format);
	vfprintf(f, format, ap);
	va_end(ap);
	fclose(f);
}
