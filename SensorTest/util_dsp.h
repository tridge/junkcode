void HAP_printf(const char *file, int line, const char *format, ...);

#define HAP_DEBUG(msg) HAP_debug(msg, 0, __FILE__, __LINE__)
#define HAP_PRINTF(...) HAP_printf(__FILE__, __LINE__, __VA_ARGS__)
