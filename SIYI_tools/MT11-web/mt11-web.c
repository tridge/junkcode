#define _GNU_SOURCE

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <ftw.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SERVER_NAME "mt11-web/1.1"
#define DEFAULT_PORT 8080
#define MAX_HEADER (16U * 1024U)
#define MAX_BODY (256U * 1024U)
#define MAX_CONFIG (64U * 1024U)
#ifndef APP_DIR
#define APP_DIR "/app"
#endif
#ifndef MEDIA_ROOT
#define MEDIA_ROOT "/mnt"
#endif
#ifndef CONFIG_PATH
#define CONFIG_PATH "/app/config.ini"
#endif
#ifndef CONFIG_BACKUP_PATH
#define CONFIG_BACKUP_PATH "/app/config.ini.web.bak"
#endif
#ifndef PASSWORD_PATH
#define PASSWORD_PATH "/app/web.pass"
#endif
#ifndef CAMERA_PATH
#define CAMERA_PATH "/app/siyi_camera_app"
#endif
#ifndef WEB_PATH
#define WEB_PATH "/app/bin/mt11-web"
#endif
#ifndef APP_LOG_PATH
#define APP_LOG_PATH "/run/siyi_camera_app.log"
#endif
#ifndef APP_LOG_OLD_PATH
#define APP_LOG_OLD_PATH "/run/siyi_camera_app.log.1"
#endif
#define APP_LOG_ROTATE_SIZE (512U * 1024U)
#define APP_LOG_DISPLAY_SIZE (256U * 1024U)

static int listen_fd = -1;
static char csrf_token[65];

struct string_buffer {
    char *data;
    size_t len;
    size_t cap;
};

struct request {
    char method[12];
    char path[4096];
    char query[4096];
    char *storage;
    size_t header_len;
    char *body;
    size_t body_len;
};

struct option {
    const char *value;
    const char *label;
};

enum parameter_kind {
    PARAM_BOOLEAN,
    PARAM_INTEGER,
    PARAM_FLOAT,
    PARAM_IPV4,
    PARAM_ENUM,
    PARAM_LUT
};

struct parameter {
    const char *form_name;
    const char *section;
    const char *key;
    const char *label;
    const char *help;
    enum parameter_kind kind;
    double minimum;
    double maximum;
    double step;
    const struct option *options;
    size_t option_count;
};

struct ini_update {
    const char *section;
    char key[48];
    char value[96];
    bool found;
};

static const struct option boolean_options[] = {
    {"n", "Disabled"}, {"y", "Enabled"}
};

static const struct option codec_options[] = {
    {"96", "H.264 (internal payload code 96)"},
    {"265", "H.265 / HEVC (internal payload code 265)"}
};

static const struct option resolution_options[] = {
    {"5", "1280 x 720 (code 5)"},
    {"6", "1920 x 1080 (code 6)"},
    {"26", "3840 x 2160 / 4K (code 26)"}
};

static const struct option iso_options[] = {
    {"0", "Auto"}, {"1", "ISO 100"}, {"2", "ISO 200"},
    {"3", "ISO 400"}, {"4", "ISO 800"}, {"5", "ISO 1600"},
    {"6", "ISO 3200"}
};

static const struct option shutter_options[] = {
    {"0", "Auto"}, {"1", "1/30 s"}, {"2", "1/50 s"},
    {"3", "1/100 s"}, {"4", "1/250 s"}, {"5", "1/500 s"},
    {"6", "1/750 s"}, {"7", "1/1000 s"}, {"8", "1/2000 s"}
};

static const struct option metering_options[] = {
    {"0", "Average"}, {"1", "Center-weighted"}, {"2", "Spot"}
};

static const struct option awb_options[] = {
    {"0", "Auto"}, {"1", "Daylight"}, {"2", "Cloudy"},
    {"3", "Fluorescent"}, {"4", "Incandescent"}
};

static const struct parameter core_parameters[] = {
    {"autorecord", "misc", "autorecord", "Automatic recording",
     "Start SD-card recording automatically after application startup.",
     PARAM_BOOLEAN, 0, 0, 0, boolean_options, 2},
    {"rtsp_resolution_0", "misc", "rtsp_resolution_0", "Main RTSP resolution",
     "Resolution of rtsp://CAMERA:8554/video1. These three sizes are explicitly handled by the firmware setter.",
     PARAM_ENUM, 0, 0, 0, resolution_options, 3},
    {"rtsp_encode_type_0", "misc", "rtsp_encode_type_0", "Main RTSP codec",
     "Codec of video1. The config uses the encoder payload IDs 96 and 265, not protocol values 1 and 2.",
     PARAM_ENUM, 0, 0, 0, codec_options, 2},
    {"rtsp_resolution_1", "misc", "rtsp_resolution_1", "Sub RTSP resolution",
     "Resolution of rtsp://CAMERA:8554/video2.",
     PARAM_ENUM, 0, 0, 0, resolution_options, 3},
    {"rtsp_encode_type_1", "misc", "rtsp_encode_type_1", "Sub RTSP codec",
     "Codec of video2.", PARAM_ENUM, 0, 0, 0, codec_options, 2},
    {"record_resolution", "misc", "record_resolution", "Recording resolution",
     "Resolution code used by the MP4 recording path.",
     PARAM_ENUM, 0, 0, 0, resolution_options, 3},

    {"net_ip", "net_config", "ip", "Camera IPv4 address",
     "Applied to eth0 during application startup. Changing it changes the address used to reconnect.",
     PARAM_IPV4, 0, 0, 0, NULL, 0},
    {"net_netmask", "net_config", "netmask", "IPv4 netmask",
     "Must be a contiguous IPv4 network mask.", PARAM_IPV4, 0, 0, 0, NULL, 0},
    {"net_gateway", "net_config", "gateway", "IPv4 gateway",
     "Default gateway installed by the application.", PARAM_IPV4, 0, 0, 0, NULL, 0},

    {"brightness_val", "isp", "brightness_val", "Brightness",
     "Visible-camera ISP CSC brightness; applied to both RGB sensors.",
     PARAM_INTEGER, 0, 100, 1, NULL, 0},
    {"saturation_val", "isp", "saturation_val", "Saturation",
     "Visible-camera ISP CSC saturation; applied to both RGB sensors.",
     PARAM_INTEGER, 0, 100, 1, NULL, 0},
    {"contrast_val", "isp", "contrast_val", "Contrast",
     "Visible-camera ISP CSC contrast; applied to both RGB sensors.",
     PARAM_INTEGER, 0, 100, 1, NULL, 0},
    {"ev_val", "isp", "ev_val", "Exposure compensation",
     "Integer tenths from -1.0 EV to +1.0 EV; for example -5 means -0.5 EV.",
     PARAM_INTEGER, -10, 10, 1, NULL, 0},
    {"iso_val", "isp", "iso_val", "ISO",
     "Automatic exposure gain or one of the firmware's six fixed gain steps.",
     PARAM_ENUM, 0, 0, 0, iso_options, 7},
    {"shutter_speed", "isp", "shutter_speed", "Shutter speed",
     "Automatic exposure time or a firmware-defined fixed exposure time.",
     PARAM_ENUM, 0, 0, 0, shutter_options, 9},
    {"metering_mode", "isp", "metering_mode", "Metering mode",
     "Average uses a mild center bias; center-weighted uses a stronger center weight. Config-file spot metering starts at coordinate 0,0.",
     PARAM_ENUM, 0, 0, 0, metering_options, 3},
    {"awb_mode", "isp", "awb_mode", "White balance",
     "Automatic white balance or one of four fixed RGB gain presets.",
     PARAM_ENUM, 0, 0, 0, awb_options, 5},

    {"temp_frame", "thermal", "temp_frame", "Raw temperature frame",
     "Use the stacked UVC image+radiometric mode. Thermal still captures then save a 640x512 little-endian Kelvin*64 .bin file; thermal frame rate changes from 30 to 25 fps.",
     PARAM_BOOLEAN, 0, 0, 0, boolean_options, 2},

    {"default_distance", "thermal_default_env", "target_distance", "Default target distance (m)",
     "Baseline environmental-correction target distance.", PARAM_FLOAT, 0.01, 10000, 0.01, NULL, 0},
    {"default_emissivity", "thermal_default_env", "target_emissivity", "Default emissivity",
     "Baseline target emissivity as a fraction.", PARAM_FLOAT, 0.01, 1, 0.01, NULL, 0},
    {"default_humidity", "thermal_default_env", "environment_humidity", "Default relative humidity",
     "Baseline relative humidity as a fraction from 0 to 1.", PARAM_FLOAT, 0, 1, 0.01, NULL, 0},
    {"default_atmospheric_temperature", "thermal_default_env", "atmospheric_temperature", "Default atmospheric temperature (C)",
     "Baseline atmospheric temperature used for radiometric correction.", PARAM_FLOAT, -273.15, 1000, 0.01, NULL, 0},
    {"default_reflection_temperature", "thermal_default_env", "reflection_temperature", "Default reflected temperature (C)",
     "Baseline reflected apparent temperature used for radiometric correction.", PARAM_FLOAT, -273.15, 1000, 0.01, NULL, 0},

    {"env_distance", "thermal_env", "target_distance", "Target distance (m)",
     "Active target distance for Linux-side environmental/distance correction.", PARAM_FLOAT, 0.01, 10000, 0.01, NULL, 0},
    {"env_emissivity", "thermal_env", "target_emissivity", "Target emissivity",
     "Active target emissivity as a fraction.", PARAM_FLOAT, 0.01, 1, 0.01, NULL, 0},
    {"env_humidity", "thermal_env", "environment_humidity", "Relative humidity",
     "Active relative humidity as a fraction from 0 to 1.", PARAM_FLOAT, 0, 1, 0.01, NULL, 0},
    {"env_atmospheric_temperature", "thermal_env", "atmospheric_temperature", "Atmospheric temperature (C)",
     "Active atmospheric temperature used for radiometric correction.", PARAM_FLOAT, -273.15, 1000, 0.01, NULL, 0},
    {"env_reflection_temperature", "thermal_env", "reflection_temperature", "Reflected temperature (C)",
     "Active reflected apparent temperature used for radiometric correction.", PARAM_FLOAT, -273.15, 1000, 0.01, NULL, 0},

    {"low_gain_low_point", "thermal_calib", "low_gain_low_point", "Low-gain low point (C)",
     "Factory/user two-point calibration boundary for thermal low-gain mode.", PARAM_FLOAT, -273.15, 2000, 0.01, NULL, 0},
    {"low_gain_high_point", "thermal_calib", "low_gain_high_point", "Low-gain high point (C)",
     "Factory/user two-point calibration boundary for thermal low-gain mode.", PARAM_FLOAT, -273.15, 2000, 0.01, NULL, 0},
    {"high_gain_low_point", "thermal_calib", "high_gain_low_point", "High-gain low point (C)",
     "Factory/user two-point calibration boundary for thermal high-gain mode.", PARAM_FLOAT, -273.15, 2000, 0.01, NULL, 0},
    {"high_gain_high_point", "thermal_calib", "high_gain_high_point", "High-gain high point (C)",
     "Factory/user two-point calibration boundary for thermal high-gain mode.", PARAM_FLOAT, -273.15, 2000, 0.01, NULL, 0}
};

static void log_message(const char *fmt, ...)
{
    char timestamp[64];
    time_t now = time(NULL);
    struct tm tm_now;
    va_list ap;

    localtime_r(&now, &tm_now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_now);
    fprintf(stderr, "[%s] ", timestamp);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
}

static void sb_init(struct string_buffer *sb)
{
    memset(sb, 0, sizeof(*sb));
}

static bool sb_reserve(struct string_buffer *sb, size_t extra)
{
    size_t needed;
    size_t new_cap;
    char *new_data;

    if (extra > SIZE_MAX - sb->len - 1) {
        return false;
    }
    needed = sb->len + extra + 1;
    if (needed <= sb->cap) {
        return true;
    }
    new_cap = sb->cap ? sb->cap : 4096;
    while (new_cap < needed) {
        if (new_cap > SIZE_MAX / 2) {
            new_cap = needed;
            break;
        }
        new_cap *= 2;
    }
    new_data = realloc(sb->data, new_cap);
    if (new_data == NULL) {
        return false;
    }
    sb->data = new_data;
    sb->cap = new_cap;
    return true;
}

static bool sb_append_n(struct string_buffer *sb, const char *text, size_t len)
{
    if (!sb_reserve(sb, len)) {
        return false;
    }
    memcpy(sb->data + sb->len, text, len);
    sb->len += len;
    sb->data[sb->len] = '\0';
    return true;
}

static bool sb_append(struct string_buffer *sb, const char *text)
{
    return sb_append_n(sb, text, strlen(text));
}

static bool sb_appendf(struct string_buffer *sb, const char *fmt, ...)
{
    va_list ap;
    va_list copy;
    int length;

    va_start(ap, fmt);
    va_copy(copy, ap);
    length = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (length < 0 || !sb_reserve(sb, (size_t)length)) {
        va_end(ap);
        return false;
    }
    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, ap);
    va_end(ap);
    sb->len += (size_t)length;
    return true;
}

static bool sb_append_html(struct string_buffer *sb, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;

    while (*p != '\0') {
        switch (*p) {
        case '&':
            if (!sb_append(sb, "&amp;")) return false;
            break;
        case '<':
            if (!sb_append(sb, "&lt;")) return false;
            break;
        case '>':
            if (!sb_append(sb, "&gt;")) return false;
            break;
        case '"':
            if (!sb_append(sb, "&quot;")) return false;
            break;
        case '\'':
            if (!sb_append(sb, "&#39;")) return false;
            break;
        default:
            if (*p >= 0x20 || *p == '\n' || *p == '\r' || *p == '\t') {
                if (!sb_append_n(sb, (const char *)p, 1)) return false;
            }
            break;
        }
        p++;
    }
    return true;
}

static bool sb_append_url(struct string_buffer *sb, const char *text)
{
    static const char hex[] = "0123456789ABCDEF";
    const unsigned char *p = (const unsigned char *)text;

    while (*p != '\0') {
        if (isalnum(*p) || *p == '-' || *p == '_' || *p == '.' || *p == '~') {
            if (!sb_append_n(sb, (const char *)p, 1)) return false;
        } else {
            char encoded[3] = {'%', hex[*p >> 4], hex[*p & 15]};
            if (!sb_append_n(sb, encoded, sizeof(encoded))) return false;
        }
        p++;
    }
    return true;
}

static bool sb_append_log_html(struct string_buffer *sb, const char *text, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        unsigned char value = (unsigned char)text[i];
        if (value == 0x1b && i + 1 < length && text[i + 1] == '[') {
            i += 2;
            while (i < length) {
                value = (unsigned char)text[i];
                if (value >= 0x40 && value <= 0x7e) break;
                i++;
            }
            continue;
        }
        if (value == '\r') continue;
        if (value == '&') {
            if (!sb_append(sb, "&amp;")) return false;
        } else if (value == '<') {
            if (!sb_append(sb, "&lt;")) return false;
        } else if (value == '>') {
            if (!sb_append(sb, "&gt;")) return false;
        } else if (value >= 0x20 || value == '\n' || value == '\t') {
            if (!sb_append_n(sb, (const char *)&text[i], 1)) return false;
        }
    }
    return true;
}

static bool send_all(int fd, const void *buffer, size_t length)
{
    const char *p = buffer;

    while (length > 0) {
        ssize_t sent = send(fd, p, length, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (sent == 0) return false;
        p += sent;
        length -= (size_t)sent;
    }
    return true;
}

static void send_response(int fd, int status, const char *reason,
                          const char *content_type, const char *body,
                          size_t body_len, const char *extra_headers)
{
    char header[2048];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Server: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "X-Frame-Options: DENY\r\n"
        "Content-Security-Policy: default-src 'none'; style-src 'unsafe-inline'; "
        "img-src 'self'; media-src 'self'; "
        "form-action 'self'; frame-ancestors 'none'\r\n"
        "%s\r\n",
        status, reason, SERVER_NAME, content_type, body_len,
        extra_headers ? extra_headers : "");

    if (header_len <= 0 || (size_t)header_len >= sizeof(header)) return;
    (void)send_all(fd, header, (size_t)header_len);
    if (body_len > 0) (void)send_all(fd, body, body_len);
}

static void send_text_error(int fd, int status, const char *reason,
                            const char *message, const char *extra_headers)
{
    send_response(fd, status, reason, "text/plain; charset=utf-8",
                  message, strlen(message), extra_headers);
}

static char *read_file(const char *path, size_t limit, size_t *length)
{
    int fd;
    struct stat st;
    char *buffer;
    size_t used = 0;

    *length = 0;
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return NULL;
    if (fstat(fd, &st) < 0 || st.st_size < 0 || (uint64_t)st.st_size > limit) {
        close(fd);
        errno = EFBIG;
        return NULL;
    }
    buffer = malloc((size_t)st.st_size + 1);
    if (buffer == NULL) {
        close(fd);
        return NULL;
    }
    while (used < (size_t)st.st_size) {
        ssize_t got = read(fd, buffer + used, (size_t)st.st_size - used);
        if (got < 0) {
            if (errno == EINTR) continue;
            free(buffer);
            close(fd);
            return NULL;
        }
        if (got == 0) break;
        used += (size_t)got;
    }
    close(fd);
    buffer[used] = '\0';
    *length = used;
    return buffer;
}

static char *read_file_tail(const char *path, size_t limit, size_t *length)
{
    int fd;
    struct stat st;
    off_t start = 0;
    size_t wanted;
    size_t used = 0;
    char *buffer;

    *length = 0;
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) return NULL;
    if (fstat(fd, &st) < 0 || st.st_size < 0) {
        close(fd);
        return NULL;
    }
    if ((uint64_t)st.st_size > limit) start = st.st_size - (off_t)limit;
    wanted = (size_t)(st.st_size - start);
    if (lseek(fd, start, SEEK_SET) < 0) {
        close(fd);
        return NULL;
    }
    buffer = malloc(wanted + 1);
    if (buffer == NULL) {
        close(fd);
        return NULL;
    }
    while (used < wanted) {
        ssize_t got = read(fd, buffer + used, wanted - used);
        if (got < 0) {
            if (errno == EINTR) continue;
            free(buffer);
            close(fd);
            return NULL;
        }
        if (got == 0) break;
        used += (size_t)got;
    }
    close(fd);
    buffer[used] = '\0';
    *length = used;
    return buffer;
}

static char *read_app_log(size_t *length)
{
    size_t current_len = 0;
    size_t old_len = 0;
    char *current = read_file_tail(APP_LOG_PATH, APP_LOG_DISPLAY_SIZE, &current_len);
    char *old = NULL;
    char *combined;

    if (current_len < APP_LOG_DISPLAY_SIZE) {
        old = read_file_tail(APP_LOG_OLD_PATH,
                             APP_LOG_DISPLAY_SIZE - current_len, &old_len);
    }
    if (current == NULL && old == NULL) {
        *length = 0;
        return strdup("");
    }
    combined = malloc(old_len + current_len + 1);
    if (combined == NULL) {
        free(old);
        free(current);
        *length = 0;
        return NULL;
    }
    if (old_len > 0) memcpy(combined, old, old_len);
    if (current_len > 0) memcpy(combined + old_len, current, current_len);
    combined[old_len + current_len] = '\0';
    *length = old_len + current_len;
    free(old);
    free(current);
    return combined;
}

static bool random_token(char output[65])
{
    unsigned char bytes[32];
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    size_t used = 0;

    if (fd < 0) return false;
    while (used < sizeof(bytes)) {
        ssize_t got = read(fd, bytes + used, sizeof(bytes) - used);
        if (got < 0) {
            if (errno == EINTR) continue;
            close(fd);
            return false;
        }
        if (got == 0) {
            close(fd);
            return false;
        }
        used += (size_t)got;
    }
    close(fd);
    for (size_t i = 0; i < sizeof(bytes); i++) {
        snprintf(output + i * 2, 3, "%02x", bytes[i]);
    }
    output[64] = '\0';
    return true;
}

static const char *find_header(const struct request *request, const char *name,
                               char *value, size_t value_size)
{
    const char *p = request->storage;
    const char *end = request->storage + request->header_len;
    size_t name_len = strlen(name);

    while (p < end) {
        const char *line_end = memchr(p, '\n', (size_t)(end - p));
        if (line_end == NULL) line_end = end;
        if ((size_t)(line_end - p) > name_len + 1 &&
            strncasecmp(p, name, name_len) == 0 && p[name_len] == ':') {
            const char *value_start = p + name_len + 1;
            const char *value_end;
            while (value_start < line_end && isspace((unsigned char)*value_start)) {
                value_start++;
            }
            value_end = line_end;
            while (value_end > value_start && isspace((unsigned char)value_end[-1])) {
                value_end--;
            }
            if ((size_t)(value_end - value_start) >= value_size) return NULL;
            memcpy(value, value_start, (size_t)(value_end - value_start));
            value[value_end - value_start] = '\0';
            return value;
        }
        p = line_end + (line_end < end ? 1 : 0);
    }
    return NULL;
}

static bool parse_content_length(const char *headers, size_t header_len,
                                 size_t *content_length)
{
    struct request temporary = {.storage = (char *)headers, .header_len = header_len};
    char value[64];
    char *end;
    unsigned long parsed;

    *content_length = 0;
    if (find_header(&temporary, "Content-Length", value, sizeof(value)) == NULL) {
        return true;
    }
    errno = 0;
    parsed = strtoul(value, &end, 10);
    if (errno != 0 || *value == '\0' || *end != '\0' || parsed > MAX_BODY) {
        return false;
    }
    *content_length = (size_t)parsed;
    return true;
}

static int receive_request(int fd, struct request *request)
{
    size_t capacity = MAX_HEADER + MAX_BODY + 1;
    size_t used = 0;
    size_t header_len = 0;
    size_t content_length = 0;
    char *storage = calloc(1, capacity);

    if (storage == NULL) return 500;
    while (used < MAX_HEADER) {
        ssize_t got = recv(fd, storage + used, capacity - used - 1, 0);
        if (got < 0) {
            if (errno == EINTR) continue;
            free(storage);
            return 400;
        }
        if (got == 0) break;
        used += (size_t)got;
        storage[used] = '\0';
        char *boundary = strstr(storage, "\r\n\r\n");
        if (boundary != NULL) {
            header_len = (size_t)(boundary + 4 - storage);
            if (header_len > MAX_HEADER) {
                free(storage);
                return 431;
            }
            break;
        }
    }
    if (header_len == 0) {
        free(storage);
        return used >= MAX_HEADER ? 431 : 400;
    }
    if (!parse_content_length(storage, header_len, &content_length)) {
        free(storage);
        return 413;
    }
    while (used < header_len + content_length) {
        ssize_t got = recv(fd, storage + used, capacity - used - 1, 0);
        if (got < 0) {
            if (errno == EINTR) continue;
            free(storage);
            return 400;
        }
        if (got == 0) {
            free(storage);
            return 400;
        }
        used += (size_t)got;
    }
    if (sscanf(storage, "%11s %4095s", request->method, request->path) != 2) {
        free(storage);
        return 400;
    }
    char *query = strchr(request->path, '?');
    if (query != NULL) {
        snprintf(request->query, sizeof(request->query), "%s", query + 1);
        *query = '\0';
    }
    request->storage = storage;
    request->header_len = header_len;
    request->body = storage + header_len;
    request->body_len = content_length;
    request->body[content_length] = '\0';
    return 0;
}

static int b64_value(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

static bool decode_base64(const char *input, unsigned char *output,
                          size_t output_size, size_t *output_len)
{
    unsigned accumulator = 0;
    unsigned bits = 0;
    size_t used = 0;

    for (const unsigned char *p = (const unsigned char *)input; *p != '\0'; p++) {
        int value;
        if (*p == '=') break;
        if (isspace(*p)) continue;
        value = b64_value(*p);
        if (value < 0) return false;
        accumulator = (accumulator << 6) | (unsigned)value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (used + 1 >= output_size) return false;
            output[used++] = (unsigned char)((accumulator >> bits) & 0xff);
        }
    }
    output[used] = '\0';
    *output_len = used;
    return true;
}

static bool constant_time_equal(const char *a, size_t a_len,
                                const char *b, size_t b_len)
{
    size_t max_len = a_len > b_len ? a_len : b_len;
    unsigned difference = (unsigned)(a_len ^ b_len);

    for (size_t i = 0; i < max_len; i++) {
        unsigned char av = i < a_len ? (unsigned char)a[i] : 0;
        unsigned char bv = i < b_len ? (unsigned char)b[i] : 0;
        difference |= av ^ bv;
    }
    return difference == 0;
}

static int authenticate(const struct request *request)
{
    char authorization[1024];
    unsigned char decoded[768];
    size_t decoded_len;
    size_t password_len;
    char *password = read_file(PASSWORD_PATH, 512, &password_len);
    char *colon;
    bool valid;

    if (password == NULL) return -1;
    while (password_len > 0 &&
           (password[password_len - 1] == '\n' || password[password_len - 1] == '\r')) {
        password[--password_len] = '\0';
    }
    if (password_len == 0) {
        free(password);
        return -1;
    }
    if (find_header(request, "Authorization", authorization,
                    sizeof(authorization)) == NULL ||
        strncasecmp(authorization, "Basic ", 6) != 0 ||
        !decode_base64(authorization + 6, decoded, sizeof(decoded), &decoded_len)) {
        free(password);
        return 0;
    }
    colon = memchr(decoded, ':', decoded_len);
    if (colon == NULL) {
        free(password);
        return 0;
    }
    valid = (size_t)(colon - (char *)decoded) == 5 &&
            memcmp(decoded, "admin", 5) == 0 &&
            constant_time_equal(colon + 1,
                                decoded_len - (size_t)(colon + 1 - (char *)decoded),
                                password, password_len);
    free(password);
    return valid ? 1 : 0;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *url_decode(const char *input, size_t length, size_t *decoded_len)
{
    char *output = malloc(length + 1);
    size_t used = 0;

    if (output == NULL) return NULL;
    for (size_t i = 0; i < length; i++) {
        unsigned char value;
        if (input[i] == '+') {
            value = ' ';
        } else if (input[i] == '%' && i + 2 < length) {
            int high = hex_value(input[i + 1]);
            int low = hex_value(input[i + 2]);
            if (high < 0 || low < 0) {
                free(output);
                return NULL;
            }
            value = (unsigned char)((high << 4) | low);
            i += 2;
        } else {
            value = (unsigned char)input[i];
        }
        if (value == '\0') {
            free(output);
            return NULL;
        }
        output[used++] = (char)value;
    }
    output[used] = '\0';
    *decoded_len = used;
    return output;
}

static char *encoded_value(const char *source, size_t source_length,
                           const char *key, size_t *value_len)
{
    const char *p = source;
    const char *end = source + source_length;
    size_t key_len = strlen(key);

    while (p < end) {
        const char *pair_end = memchr(p, '&', (size_t)(end - p));
        const char *equals;
        if (pair_end == NULL) pair_end = end;
        equals = memchr(p, '=', (size_t)(pair_end - p));
        if (equals != NULL && (size_t)(equals - p) == key_len &&
            memcmp(p, key, key_len) == 0) {
            return url_decode(equals + 1, (size_t)(pair_end - equals - 1), value_len);
        }
        p = pair_end + (pair_end < end ? 1 : 0);
    }
    return NULL;
}

static char *form_value(const struct request *request, const char *key,
                        size_t *value_len)
{
    return encoded_value(request->body, request->body_len, key, value_len);
}

static char *query_value(const struct request *request, const char *key,
                         size_t *value_len)
{
    return encoded_value(request->query, strlen(request->query), key, value_len);
}

static bool valid_csrf(const struct request *request)
{
    size_t length;
    char *token = form_value(request, "csrf", &length);
    bool valid = token != NULL &&
                 constant_time_equal(token, length, csrf_token, strlen(csrf_token));
    free(token);
    return valid;
}

static void normalize_newlines(char *text, size_t *length)
{
    size_t source = 0;
    size_t dest = 0;

    while (source < *length) {
        if (text[source] == '\r') {
            if (source + 1 < *length && text[source + 1] == '\n') source++;
            text[dest++] = '\n';
        } else {
            text[dest++] = text[source];
        }
        source++;
    }
    text[dest] = '\0';
    *length = dest;
}

static bool validate_ini(const char *config, size_t length,
                         char *error, size_t error_size)
{
    size_t offset = 0;
    unsigned line_number = 0;
    unsigned sections = 0;
    unsigned assignments = 0;

    if (length == 0 || length > MAX_CONFIG) {
        snprintf(error, error_size, "Config size must be between 1 and %u bytes",
                 MAX_CONFIG);
        return false;
    }
    while (offset < length) {
        size_t line_start = offset;
        size_t line_end;
        size_t start;
        size_t end;
        line_number++;
        while (offset < length && config[offset] != '\n') offset++;
        line_end = offset;
        if (offset < length) offset++;
        start = line_start;
        end = line_end;
        while (start < end && isspace((unsigned char)config[start])) start++;
        while (end > start && isspace((unsigned char)config[end - 1])) end--;
        if (start == end || config[start] == '#' || config[start] == ';') continue;
        if (config[start] == '[') {
            const char *close = memchr(config + start + 1, ']', end - start - 1);
            if (close == NULL) {
                snprintf(error, error_size, "Line %u has an unterminated section", line_number);
                return false;
            }
            for (const char *p = close + 1; p < config + end; p++) {
                if (!isspace((unsigned char)*p)) {
                    snprintf(error, error_size, "Line %u has text after its section", line_number);
                    return false;
                }
            }
            sections++;
            continue;
        }
        const char *equals = memchr(config + start, '=', end - start);
        if (equals == NULL) {
            snprintf(error, error_size, "Line %u is neither a section nor key=value", line_number);
            return false;
        }
        const char *key_end = equals;
        while (key_end > config + start && isspace((unsigned char)key_end[-1])) key_end--;
        if (key_end == config + start) {
            snprintf(error, error_size, "Line %u has an empty key", line_number);
            return false;
        }
        assignments++;
    }
    if (sections == 0 || assignments == 0) {
        snprintf(error, error_size, "Config must contain a section and at least one assignment");
        return false;
    }
    return true;
}

static void trim_span(const char **start, const char **end)
{
    while (*start < *end && isspace((unsigned char)**start)) (*start)++;
    while (*end > *start && isspace((unsigned char)(*end)[-1])) (*end)--;
}

static bool span_equal(const char *start, const char *end, const char *text)
{
    size_t length = (size_t)(end - start);
    return strlen(text) == length && memcmp(start, text, length) == 0;
}

static bool ini_get_value(const char *config, const char *wanted_section,
                          const char *wanted_key, char *output, size_t output_size)
{
    const char *p = config;
    char section[80] = "";

    while (*p != '\0') {
        const char *line_end = strchr(p, '\n');
        const char *start = p;
        const char *end;
        if (line_end == NULL) line_end = p + strlen(p);
        end = line_end;
        trim_span(&start, &end);
        if (start < end && *start == '[') {
            const char *close = memchr(start + 1, ']', (size_t)(end - start - 1));
            if (close != NULL) {
                const char *name_start = start + 1;
                const char *name_end = close;
                size_t length;
                trim_span(&name_start, &name_end);
                length = (size_t)(name_end - name_start);
                if (length >= sizeof(section)) length = sizeof(section) - 1;
                memcpy(section, name_start, length);
                section[length] = '\0';
            }
        } else if (start < end && *start != '#' && *start != ';' &&
                   strcmp(section, wanted_section) == 0) {
            const char *equals = memchr(start, '=', (size_t)(end - start));
            if (equals != NULL) {
                const char *key_start = start;
                const char *key_end = equals;
                trim_span(&key_start, &key_end);
                if (span_equal(key_start, key_end, wanted_key)) {
                    const char *value_start = equals + 1;
                    const char *value_end = end;
                    size_t length;
                    trim_span(&value_start, &value_end);
                    if (value_end > value_start + 1 && *value_start == '"' &&
                        value_end[-1] == '"') {
                        value_start++;
                        value_end--;
                        trim_span(&value_start, &value_end);
                    }
                    length = (size_t)(value_end - value_start);
                    if (length >= output_size) return false;
                    memcpy(output, value_start, length);
                    output[length] = '\0';
                    return true;
                }
            }
        }
        p = *line_end == '\n' ? line_end + 1 : line_end;
    }
    return false;
}

static struct ini_update *find_update(struct ini_update *updates, size_t count,
                                      const char *section,
                                      const char *key_start, const char *key_end)
{
    for (size_t i = 0; i < count; i++) {
        if (strcmp(updates[i].section, section) == 0 &&
            span_equal(key_start, key_end, updates[i].key)) return &updates[i];
    }
    return NULL;
}

static char *ini_apply_updates(const char *config, struct ini_update *updates,
                               size_t count, size_t *new_length,
                               char *error, size_t error_size)
{
    struct string_buffer output;
    const char *p = config;
    char section[80] = "";

    for (size_t i = 0; i < count; i++) updates[i].found = false;
    sb_init(&output);
    while (*p != '\0') {
        const char *line_end = strchr(p, '\n');
        const char *start = p;
        const char *end;
        struct ini_update *update = NULL;
        if (line_end == NULL) line_end = p + strlen(p);
        end = line_end;
        trim_span(&start, &end);
        if (start < end && *start == '[') {
            const char *close = memchr(start + 1, ']', (size_t)(end - start - 1));
            if (close != NULL) {
                const char *name_start = start + 1;
                const char *name_end = close;
                size_t length;
                trim_span(&name_start, &name_end);
                length = (size_t)(name_end - name_start);
                if (length >= sizeof(section)) length = sizeof(section) - 1;
                memcpy(section, name_start, length);
                section[length] = '\0';
            }
        } else if (start < end && *start != '#' && *start != ';') {
            const char *equals = memchr(start, '=', (size_t)(end - start));
            if (equals != NULL) {
                const char *key_start = start;
                const char *key_end = equals;
                trim_span(&key_start, &key_end);
                update = find_update(updates, count, section, key_start, key_end);
                if (update != NULL && update->found) {
                    snprintf(error, error_size, "Duplicate config key [%s] %s",
                             section, update->key);
                    free(output.data);
                    return NULL;
                }
                if (update != NULL) {
                    const char *value_start = equals + 1;
                    const char *value_end = end;
                    trim_span(&value_start, &value_end);
                    if (value_end > value_start + 1 && *value_start == '"' &&
                        value_end[-1] == '"') {
                        value_start++;
                        value_end--;
                        trim_span(&value_start, &value_end);
                    }
                    if (span_equal(value_start, value_end, update->value)) {
                        update->found = true;
                        update = NULL;
                    } else {
                        if (!sb_append_n(&output, p, (size_t)(equals + 1 - p)) ||
                            !sb_appendf(&output, " \"%s\"", update->value)) {
                            free(output.data);
                            return NULL;
                        }
                        update->found = true;
                    }
                }
            }
        }
        if (update == NULL && !sb_append_n(&output, p, (size_t)(line_end - p))) {
            free(output.data);
            return NULL;
        }
        if (*line_end == '\n' && !sb_append_n(&output, "\n", 1)) {
            free(output.data);
            return NULL;
        }
        p = *line_end == '\n' ? line_end + 1 : line_end;
    }
    for (size_t i = 0; i < count; i++) {
        if (!updates[i].found) {
            snprintf(error, error_size, "Config key [%s] %s is missing",
                     updates[i].section, updates[i].key);
            free(output.data);
            return NULL;
        }
    }
    if (output.len > MAX_CONFIG) {
        snprintf(error, error_size, "Updated config is too large");
        free(output.data);
        return NULL;
    }
    *new_length = output.len;
    return output.data;
}

static bool parse_long_strict(const char *text, long minimum, long maximum,
                              long *result)
{
    char *end;
    long value;
    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || *text == '\0' || *end != '\0' ||
        value < minimum || value > maximum) return false;
    *result = value;
    return true;
}

static bool parse_double_strict(const char *text, double minimum, double maximum,
                                double *result)
{
    char *end;
    double value;
    errno = 0;
    value = strtod(text, &end);
    if (errno != 0 || *text == '\0' || *end != '\0' || !isfinite(value) ||
        value < minimum || value > maximum) return false;
    *result = value;
    return true;
}

static bool valid_netmask(const char *text)
{
    struct in_addr address;
    uint32_t mask;
    uint32_t inverse;
    if (inet_pton(AF_INET, text, &address) != 1) return false;
    mask = ntohl(address.s_addr);
    inverse = ~mask;
    return (inverse & (inverse + 1U)) == 0;
}

static bool option_contains(const struct parameter *parameter, const char *value)
{
    for (size_t i = 0; i < parameter->option_count; i++) {
        if (strcmp(parameter->options[i].value, value) == 0) return true;
    }
    return false;
}

static bool valid_lut_value(const char *text)
{
    const char *p = text;
    for (unsigned item = 0; item < 3; item++) {
        char *end;
        long value;
        errno = 0;
        value = strtol(p, &end, 10);
        if (errno != 0 || end == p || value < INT32_MIN || value > INT32_MAX) return false;
        if (item < 2) {
            if (*end != ',') return false;
            p = end + 1;
        } else if (*end != '\0') {
            return false;
        }
    }
    return true;
}

static bool append_update(struct ini_update *updates, size_t capacity, size_t *count,
                          const char *section, const char *key, const char *value,
                          char *error, size_t error_size)
{
    if (*count >= capacity || strlen(value) >= sizeof(updates[0].value)) {
        snprintf(error, error_size, "Too many parameters or parameter value too long");
        return false;
    }
    updates[*count].section = section;
    snprintf(updates[*count].key, sizeof(updates[*count].key), "%s", key);
    snprintf(updates[*count].value, sizeof(updates[*count].value), "%s", value);
    updates[*count].found = false;
    (*count)++;
    return true;
}

static bool collect_parameter_updates(const struct request *request,
                                      struct ini_update *updates, size_t capacity,
                                      size_t *update_count,
                                      char *error, size_t error_size)
{
    size_t count = 0;

    for (size_t i = 0; i < sizeof(core_parameters) / sizeof(core_parameters[0]); i++) {
        const struct parameter *parameter = &core_parameters[i];
        size_t value_length = 0;
        char *value = form_value(request, parameter->form_name, &value_length);
        bool valid = value != NULL && value_length < sizeof(updates[0].value);
        char normalized[96] = "";
        long integer_value;
        double float_value;
        struct in_addr ipv4;

        if (valid) {
            switch (parameter->kind) {
            case PARAM_BOOLEAN:
            case PARAM_ENUM:
                valid = option_contains(parameter, value);
                if (valid) snprintf(normalized, sizeof(normalized), "%s", value);
                break;
            case PARAM_INTEGER:
                valid = parse_long_strict(value, (long)parameter->minimum,
                                          (long)parameter->maximum, &integer_value);
                if (valid) snprintf(normalized, sizeof(normalized), "%ld", integer_value);
                break;
            case PARAM_FLOAT:
                valid = parse_double_strict(value, parameter->minimum,
                                            parameter->maximum, &float_value);
                if (valid) snprintf(normalized, sizeof(normalized), "%s", value);
                break;
            case PARAM_IPV4:
                valid = inet_pton(AF_INET, value, &ipv4) == 1;
                if (valid && strcmp(parameter->key, "netmask") == 0) valid = valid_netmask(value);
                if (valid) snprintf(normalized, sizeof(normalized), "%s", value);
                break;
            case PARAM_LUT:
                valid = valid_lut_value(value);
                if (valid) snprintf(normalized, sizeof(normalized), "%s", value);
                break;
            }
        }
        if (!valid) {
            snprintf(error, error_size, "Invalid value for %s", parameter->label);
            free(value);
            return false;
        }
        if (!append_update(updates, capacity, &count, parameter->section,
                           parameter->key, normalized, error, error_size)) {
            free(value);
            return false;
        }
        free(value);
    }

    for (unsigned which = 0; which < 2; which++) {
        static const char *prefixes[] = {"snap", "record"};
        static const char *keys[] = {"snap_screen", "record_screen"};
        unsigned mask = 0;
        const unsigned bits[] = {0, 1, 3};
        const char *suffixes[] = {"wide", "zoom", "thermal"};
        for (size_t item = 0; item < 3; item++) {
            char name[40];
            size_t length = 0;
            char *value;
            snprintf(name, sizeof(name), "%s_%s", prefixes[which], suffixes[item]);
            value = form_value(request, name, &length);
            if (value != NULL && length == 1 && value[0] == '1') mask |= 1U << bits[item];
            free(value);
        }
        char value[16];
        snprintf(value, sizeof(value), "%u", mask);
        if (!append_update(updates, capacity, &count, "misc", keys[which], value,
                           error, error_size)) return false;
    }

    for (unsigned index = 0; index < 30; index++) {
        char form_name[32];
        char *value;
        size_t length = 0;
        snprintf(form_name, sizeof(form_name), "lut_%u", index);
        value = form_value(request, form_name, &length);
        if (value == NULL || length >= sizeof(updates[0].value) || !valid_lut_value(value)) {
            snprintf(error, error_size, "lut_%u must contain exactly three comma-separated signed integers", index);
            free(value);
            return false;
        }
        char key[16];
        snprintf(key, 16, "lut_%u", index);
        if (!append_update(updates, capacity, &count, "temperature_drift", key,
                           value, error, error_size)) {
            free(value);
            return false;
        }
        free(value);
    }

    if (strtod(updates[28].value, NULL) >= strtod(updates[29].value, NULL)) {
        snprintf(error, error_size, "Low-gain calibration low point must be below its high point");
        return false;
    }
    if (strtod(updates[30].value, NULL) >= strtod(updates[31].value, NULL)) {
        snprintf(error, error_size, "High-gain calibration low point must be below its high point");
        return false;
    }

    *update_count = count;
    return true;
}

static void free_parameter_updates(struct ini_update *updates, size_t count)
{
    (void)updates;
    (void)count;
}

static bool write_all(int fd, const void *buffer, size_t length)
{
    const char *p = buffer;
    while (length > 0) {
        ssize_t written = write(fd, p, length);
        if (written < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (written == 0) return false;
        p += written;
        length -= (size_t)written;
    }
    return true;
}

static int capture_app_log(void)
{
    char buffer[8192];
    int output = -1;
    off_t output_size = 0;

    for (;;) {
        ssize_t got = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (got < 0) {
            if (errno == EINTR) continue;
            return 1;
        }
        if (got == 0) break;
        if (output < 0) {
            struct stat st;
            output = open(APP_LOG_PATH, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0640);
            if (output < 0) return 1;
            if (fstat(output, &st) == 0) output_size = st.st_size;
            if ((uint64_t)output_size > APP_LOG_ROTATE_SIZE) {
                close(output);
                (void)unlink(APP_LOG_OLD_PATH);
                output = open(APP_LOG_PATH,
                              O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0640);
                if (output < 0) return 1;
                output_size = 0;
            }
        }
        if ((uint64_t)output_size + (size_t)got > APP_LOG_ROTATE_SIZE) {
            close(output);
            (void)rename(APP_LOG_PATH, APP_LOG_OLD_PATH);
            output = open(APP_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0640);
            if (output < 0) return 1;
            output_size = 0;
        }
        if (!write_all(output, buffer, (size_t)got)) {
            close(output);
            return 1;
        }
        output_size += got;
    }
    if (output >= 0) close(output);
    return 0;
}

static bool copy_to_temp(const char *source, const char *temp_path,
                         mode_t mode, uid_t uid, gid_t gid)
{
    int source_fd = -1;
    int dest_fd = -1;
    char buffer[8192];
    bool ok = false;

    source_fd = open(source, O_RDONLY | O_CLOEXEC);
    if (source_fd < 0) goto done;
    dest_fd = open(temp_path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, mode);
    if (dest_fd < 0) goto done;
    if (fchown(dest_fd, uid, gid) < 0) goto done;
    for (;;) {
        ssize_t got = read(source_fd, buffer, sizeof(buffer));
        if (got < 0) {
            if (errno == EINTR) continue;
            goto done;
        }
        if (got == 0) break;
        if (!write_all(dest_fd, buffer, (size_t)got)) goto done;
    }
    if (fsync(dest_fd) < 0) goto done;
    ok = true;
done:
    if (source_fd >= 0) close(source_fd);
    if (dest_fd >= 0) close(dest_fd);
    if (!ok) unlink(temp_path);
    return ok;
}

static bool save_config(const char *config, size_t length,
                        char *error, size_t error_size)
{
    struct stat st;
    char config_temp[PATH_MAX];
    char backup_temp[PATH_MAX];
    int fd = -1;
    int dir_fd = -1;
    bool ok = false;

    if (!validate_ini(config, length, error, error_size)) return false;
    if (stat(CONFIG_PATH, &st) < 0) {
        snprintf(error, error_size, "Cannot stat %s: %s", CONFIG_PATH, strerror(errno));
        return false;
    }
    snprintf(config_temp, sizeof(config_temp), "%s/.config.ini.web.new.%ld", APP_DIR, (long)getpid());
    snprintf(backup_temp, sizeof(backup_temp), "%s/.config.ini.web.bak.%ld", APP_DIR, (long)getpid());
    unlink(config_temp);
    unlink(backup_temp);

    if (!copy_to_temp(CONFIG_PATH, backup_temp, st.st_mode & 0777, st.st_uid, st.st_gid) ||
        rename(backup_temp, CONFIG_BACKUP_PATH) < 0) {
        snprintf(error, error_size, "Cannot create config backup: %s", strerror(errno));
        goto done;
    }
    fd = open(config_temp, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, st.st_mode & 0777);
    if (fd < 0 || fchown(fd, st.st_uid, st.st_gid) < 0 ||
        !write_all(fd, config, length) || fsync(fd) < 0) {
        snprintf(error, error_size, "Cannot write new config: %s", strerror(errno));
        goto done;
    }
    close(fd);
    fd = -1;
    if (rename(config_temp, CONFIG_PATH) < 0) {
        snprintf(error, error_size, "Cannot install new config: %s", strerror(errno));
        goto done;
    }
    dir_fd = open(APP_DIR, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dir_fd >= 0) (void)fsync(dir_fd);
    ok = true;
done:
    if (fd >= 0) close(fd);
    if (dir_fd >= 0) close(dir_fd);
    unlink(config_temp);
    unlink(backup_temp);
    return ok;
}

static bool pid_is_camera(pid_t pid)
{
    char proc_path[64];
    char executable[PATH_MAX];
    ssize_t length;

    snprintf(proc_path, sizeof(proc_path), "/proc/%ld/exe", (long)pid);
    length = readlink(proc_path, executable, sizeof(executable) - 1);
    if (length < 0) return false;
    executable[length] = '\0';
    return strcmp(executable, CAMERA_PATH) == 0;
}

static size_t camera_pids(pid_t *pids, size_t max_pids)
{
    DIR *proc = opendir("/proc");
    const struct dirent *entry;
    size_t count = 0;

    if (proc == NULL) return 0;
    while ((entry = readdir(proc)) != NULL) {
        char *end;
        long value;
        if (!isdigit((unsigned char)entry->d_name[0])) continue;
        errno = 0;
        value = strtol(entry->d_name, &end, 10);
        if (errno != 0 || *end != '\0' || value <= 1 || value > INT_MAX) continue;
        if (pid_is_camera((pid_t)value)) {
            if (count < max_pids) pids[count] = (pid_t)value;
            count++;
        }
    }
    closedir(proc);
    return count;
}

static pid_t start_camera(void)
{
    int output_pipe[2];
    pid_t logger;
    pid_t camera;

    if (pipe2(output_pipe, O_CLOEXEC) < 0) return -1;
    logger = fork();
    if (logger < 0) {
        close(output_pipe[0]);
        close(output_pipe[1]);
        return -1;
    }
    if (logger == 0) {
        int console;
        if (listen_fd >= 0) close(listen_fd);
        close(output_pipe[1]);
        if (dup2(output_pipe[0], STDIN_FILENO) < 0) _exit(127);
        if (output_pipe[0] != STDIN_FILENO) close(output_pipe[0]);
        console = open("/dev/console", O_WRONLY | O_NOCTTY);
        if (console >= 0) {
            (void)dup2(console, STDERR_FILENO);
            if (console > STDERR_FILENO) close(console);
        }
        signal(SIGCHLD, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        execl(WEB_PATH, WEB_PATH, "--capture-app-log", (char *)NULL);
        _exit(127);
    }

    camera = fork();
    if (camera < 0) {
        close(output_pipe[0]);
        close(output_pipe[1]);
        (void)kill(logger, SIGTERM);
        return -1;
    }
    if (camera == 0) {
        int null_fd;
        if (listen_fd >= 0) close(listen_fd);
        close(output_pipe[0]);
        if (dup2(output_pipe[1], STDOUT_FILENO) < 0 ||
            dup2(output_pipe[1], STDERR_FILENO) < 0) _exit(127);
        if (output_pipe[1] > STDERR_FILENO) close(output_pipe[1]);
        null_fd = open("/dev/null", O_RDONLY);
        if (null_fd >= 0) {
            (void)dup2(null_fd, STDIN_FILENO);
            if (null_fd > STDERR_FILENO) close(null_fd);
        }
        signal(SIGCHLD, SIG_DFL);
        signal(SIGPIPE, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        (void)setsid();
        if (chdir("/app") < 0) _exit(127);
        if (setenv("LD_LIBRARY_PATH", "/app/libs", 1) < 0 ||
            setenv("TMP", "/run", 1) < 0) _exit(127);
        execl(CAMERA_PATH, CAMERA_PATH, (char *)NULL);
        _exit(127);
    }
    close(output_pipe[0]);
    close(output_pipe[1]);
    return camera;
}

static bool restart_camera(char *error, size_t error_size)
{
    pid_t pids[16];
    size_t count = camera_pids(pids, sizeof(pids) / sizeof(pids[0]));

    if (count > sizeof(pids) / sizeof(pids[0])) count = sizeof(pids) / sizeof(pids[0]);
    for (size_t i = 0; i < count; i++) (void)kill(pids[i], SIGTERM);
    for (unsigned attempt = 0; attempt < 30 && camera_pids(pids, 1) > 0; attempt++) {
        usleep(100000);
    }
    count = camera_pids(pids, sizeof(pids) / sizeof(pids[0]));
    if (count > sizeof(pids) / sizeof(pids[0])) count = sizeof(pids) / sizeof(pids[0]);
    for (size_t i = 0; i < count; i++) (void)kill(pids[i], SIGKILL);
    if (count > 0) usleep(200000);

    pid_t child = start_camera();
    if (child < 0) {
        snprintf(error, error_size, "Cannot start %s: %s", CAMERA_PATH, strerror(errno));
        return false;
    }
    usleep(300000);
    if (kill(child, 0) < 0 && errno == ESRCH) {
        snprintf(error, error_size, "%s exited immediately", CAMERA_PATH);
        return false;
    }
    return true;
}

static long process_rss_kib(pid_t pid)
{
    char path[64];
    FILE *file;
    char line[256];
    long result = -1;

    snprintf(path, sizeof(path), "/proc/%ld/status", (long)pid);
    file = fopen(path, "r");
    if (file == NULL) return -1;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "VmRSS: %ld kB", &result) == 1) break;
    }
    fclose(file);
    return result;
}

static bool read_memory(long *total_kib, long *available_kib)
{
    FILE *file = fopen("/proc/meminfo", "r");
    char line[256];

    *total_kib = -1;
    *available_kib = -1;
    if (file == NULL) return false;
    while (fgets(line, sizeof(line), file) != NULL) {
        (void)sscanf(line, "MemTotal: %ld kB", total_kib);
        (void)sscanf(line, "MemAvailable: %ld kB", available_kib);
    }
    fclose(file);
    return *total_kib >= 0 && *available_kib >= 0;
}

static bool is_mounted(const char *mountpoint)
{
    FILE *file = fopen("/proc/mounts", "r");
    char device[256];
    char path[256];
    char type[64];
    bool mounted = false;

    if (file == NULL) return false;
    while (fscanf(file, "%255s %255s %63s %*s %*d %*d", device, path, type) == 3) {
        if (strcmp(path, mountpoint) == 0) {
            mounted = true;
            break;
        }
    }
    fclose(file);
    return mounted;
}

static void format_bytes(uint64_t bytes, char output[32])
{
    static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = (double)bytes;
    size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < sizeof(units) / sizeof(units[0])) {
        value /= 1024.0;
        unit++;
    }
    snprintf(output, 32, value >= 10.0 || unit == 0 ? "%.0f %s" : "%.1f %s",
             value, units[unit]);
}

static void append_filesystem_status(struct string_buffer *page,
                                     const char *label, const char *path)
{
    struct statvfs fs;
    char used[32] = "unavailable";
    char total[32] = "unavailable";
    unsigned percent = 0;
    bool mounted = strcmp(path, "/mnt") != 0 || is_mounted(path);

    if (mounted && statvfs(path, &fs) == 0 && fs.f_blocks > 0) {
        uint64_t total_bytes = (uint64_t)fs.f_blocks * fs.f_frsize;
        uint64_t free_bytes = (uint64_t)fs.f_bavail * fs.f_frsize;
        uint64_t used_bytes = total_bytes - free_bytes;
        format_bytes(used_bytes, used);
        format_bytes(total_bytes, total);
        percent = (unsigned)((used_bytes * 100U) / total_bytes);
    }
    sb_appendf(page, "<tr><th>%s</th><td>%s%s / %s (%u%%)</td></tr>",
               label, mounted ? "" : "not mounted; ", used, total, percent);
}

static void ipv4_addresses(char *output, size_t output_size)
{
    struct ifaddrs *addresses;
    struct ifaddrs *item;
    size_t used = 0;

    output[0] = '\0';
    if (getifaddrs(&addresses) != 0) return;
    for (item = addresses; item != NULL; item = item->ifa_next) {
        char address[INET_ADDRSTRLEN];
        int length;
        if (item->ifa_addr == NULL || item->ifa_addr->sa_family != AF_INET ||
            strcmp(item->ifa_name, "lo") == 0) continue;
        if (inet_ntop(AF_INET, &((struct sockaddr_in *)item->ifa_addr)->sin_addr,
                      address, sizeof(address)) == NULL) continue;
        length = snprintf(output + used, output_size - used, "%s%s=%s",
                          used ? ", " : "", item->ifa_name, address);
        if (length < 0 || (size_t)length >= output_size - used) break;
        used += (size_t)length;
    }
    freeifaddrs(addresses);
}

static const char *file_extension(const char *path)
{
    const char *base = strrchr(path, '/');
    const char *dot;
    if (base == NULL) base = path; else base++;
    dot = strrchr(base, '.');
    return dot != NULL ? dot + 1 : "";
}

static const char *file_mime_type(const char *path)
{
    const char *ext = file_extension(path);
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "webp") == 0) return "image/webp";
    if (strcasecmp(ext, "bmp") == 0) return "image/bmp";
    if (strcasecmp(ext, "mp4") == 0 || strcasecmp(ext, "m4v") == 0) return "video/mp4";
    if (strcasecmp(ext, "webm") == 0) return "video/webm";
    if (strcasecmp(ext, "mov") == 0) return "video/quicktime";
    if (strcasecmp(ext, "mkv") == 0) return "video/x-matroska";
    if (strcasecmp(ext, "avi") == 0) return "video/x-msvideo";
    if (strcasecmp(ext, "txt") == 0 || strcasecmp(ext, "log") == 0 ||
        strcasecmp(ext, "ini") == 0 || strcasecmp(ext, "md") == 0) {
        return "text/plain; charset=utf-8";
    }
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    return "application/octet-stream";
}

static bool mime_is_image(const char *mime)
{
    return strncmp(mime, "image/", 6) == 0;
}

static bool mime_is_video(const char *mime)
{
    return strncmp(mime, "video/", 6) == 0;
}

static bool path_is_below(const char *root, const char *path)
{
    size_t length = strlen(root);
    return strncmp(root, path, length) == 0 && path[length] == '/';
}

static void path_parent(const char *path, char output[PATH_MAX])
{
    char *slash;
    snprintf(output, PATH_MAX, "%s", path);
    while (strlen(output) > 1 && output[strlen(output) - 1] == '/') {
        output[strlen(output) - 1] = '\0';
    }
    slash = strrchr(output, '/');
    if (slash == output) output[1] = '\0';
    else if (slash != NULL) *slash = '\0';
    else snprintf(output, PATH_MAX, "/");
}

static bool canonical_existing_path(const char *path, char resolved[PATH_MAX],
                                    char *error, size_t error_size)
{
    if (path == NULL || path[0] != '/') {
        snprintf(error, error_size, "Path must be absolute");
        return false;
    }
    if (realpath(path, resolved) == NULL) {
        snprintf(error, error_size, "Cannot resolve path: %s", strerror(errno));
        return false;
    }
    return true;
}

static bool path_can_be_deleted(const char *path)
{
    char root[PATH_MAX];
    char resolved[PATH_MAX];
    char parent[PATH_MAX];
    struct stat st;

    if (path == NULL || path[0] != '/' || realpath(MEDIA_ROOT, root) == NULL ||
        lstat(path, &st) < 0) return false;
    if (S_ISLNK(st.st_mode)) {
        path_parent(path, parent);
        if (realpath(parent, resolved) == NULL) return false;
        return strcmp(resolved, root) == 0 || path_is_below(root, resolved);
    }
    if (realpath(path, resolved) == NULL) return false;
    return path_is_below(root, resolved);
}

static int remove_tree_item(const char *path, const struct stat *st,
                            int type, struct FTW *ftw)
{
    (void)st;
    (void)type;
    (void)ftw;
    return remove(path);
}

static bool delete_media_path(const char *path, char *deleted_path,
                              size_t deleted_size, char *error, size_t error_size)
{
    char root[PATH_MAX];
    char resolved[PATH_MAX];
    char parent[PATH_MAX];
    char safe_target[PATH_MAX];
    const char *base;
    struct stat st;
    int result;

    if (path == NULL || path[0] != '/' || realpath(MEDIA_ROOT, root) == NULL ||
        lstat(path, &st) < 0) {
        snprintf(error, error_size, "Target does not exist");
        return false;
    }
    if (S_ISLNK(st.st_mode)) {
        path_parent(path, parent);
        base = strrchr(path, '/');
        if (base == NULL || base[1] == '\0' || realpath(parent, resolved) == NULL ||
            !(strcmp(resolved, root) == 0 || path_is_below(root, resolved))) {
            snprintf(error, error_size, "Deletion is permitted only beneath %s", MEDIA_ROOT);
            return false;
        }
        if (snprintf(safe_target, sizeof(safe_target), "%s/%s", resolved, base + 1) >=
            (int)sizeof(safe_target)) {
            snprintf(error, error_size, "Path is too long");
            return false;
        }
        if (lstat(safe_target, &st) < 0 || !S_ISLNK(st.st_mode)) {
            snprintf(error, error_size, "Target changed while preparing deletion");
            return false;
        }
        result = unlink(safe_target);
        snprintf(deleted_path, deleted_size, "%s", safe_target);
    } else {
        if (realpath(path, resolved) == NULL || !path_is_below(root, resolved)) {
            snprintf(error, error_size, "Deletion is permitted only beneath %s", MEDIA_ROOT);
            return false;
        }
        if (lstat(resolved, &st) < 0) {
            snprintf(error, error_size, "Target changed while preparing deletion");
            return false;
        }
        if (S_ISDIR(st.st_mode)) {
            result = nftw(resolved, remove_tree_item, 32,
                          FTW_DEPTH | FTW_PHYS | FTW_MOUNT);
        } else {
            result = unlink(resolved);
        }
        snprintf(deleted_path, deleted_size, "%s", resolved);
    }
    if (result != 0) {
        snprintf(error, error_size, "Deletion failed: %s", strerror(errno));
        return false;
    }
    return true;
}

static void append_path_query(struct string_buffer *page, const char *route,
                              const char *path)
{
    sb_append(page, route);
    sb_append_url(page, path);
}

static void format_file_time(time_t value, char output[64])
{
    struct tm local;
    localtime_r(&value, &local);
    strftime(output, 64, "%Y-%m-%d %H:%M:%S", &local);
}

static char file_type_char(mode_t mode)
{
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISREG(mode)) return 'f';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    return '?';
}

static const char *page_style =
        ":root{color-scheme:light dark;--bg:#f4f7fa;--card:#fff;--text:#17212b;"
        "--muted:#607080;--line:#d7e0e8;--accent:#1769aa;--danger:#b42318}"
        "@media(prefers-color-scheme:dark){:root{--bg:#10161d;--card:#18222d;"
        "--text:#e8edf2;--muted:#a7b3bf;--line:#344352;--accent:#77bdf2;--danger:#ff8a80}}"
        "*{box-sizing:border-box}body{max-width:1050px;margin:auto;padding:24px;"
        "font:15px/1.5 system-ui,sans-serif;color:var(--text);background:var(--bg)}"
        "h1{margin-bottom:4px}h2{margin-top:28px}.muted{color:var(--muted)}"
        "nav{display:flex;gap:8px;margin:18px 0}nav a{padding:8px 12px;border:1px solid var(--line);"
        "border-radius:6px;color:var(--accent);text-decoration:none;background:var(--card)}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px}"
        ".card{padding:18px;border:1px solid var(--line);border-radius:10px;background:var(--card)}"
        "table{width:100%;border-collapse:collapse}th,td{padding:7px;border-bottom:1px solid var(--line);text-align:left}"
        "textarea{width:100%;min-height:520px;padding:12px;border:1px solid var(--line);border-radius:7px;"
        "background:var(--card);color:var(--text);font:13px/1.45 ui-monospace,monospace;tab-size:4}"
        "input[type=text],input[type=number],select{width:100%;padding:8px;border:1px solid var(--line);"
        "border-radius:6px;background:var(--card);color:var(--text)}"
        ".fields{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:16px}"
        ".field label{display:block;font-weight:650;margin-bottom:5px}.help{color:var(--muted);font-size:13px;margin-top:5px}"
        ".checks{display:flex;flex-wrap:wrap;gap:16px}.checks label{white-space:nowrap}"
        "details.card{margin-top:16px}summary{font-size:1.25em;font-weight:650;cursor:pointer}"
        ".actions{position:sticky;bottom:0;padding:10px 0;background:var(--bg)}"
        ".preview{display:block;max-width:100%;max-height:75vh;margin:16px auto;background:#000}"
        ".pathbox{display:flex;gap:8px}.pathbox input{flex:1}.file-actions{white-space:nowrap}"
        ".delete-link{color:var(--danger)}"
        "button{margin:8px 8px 0 0;padding:9px 14px;border:0;border-radius:6px;color:white;"
        "background:var(--accent);font-weight:650;cursor:pointer}.danger{background:var(--danger)}"
        ".notice{padding:12px;border-left:5px solid var(--accent);background:var(--card)}"
        ".error{border-color:var(--danger)}code{background:var(--bg);padding:2px 4px;border-radius:3px}"
        "@media(max-width:600px){body{padding:12px}textarea{min-height:420px}}";

static void append_nav(struct string_buffer *page)
{
    sb_append(page, "<nav><a href=\"/\">Status</a><a href=\"/parameters\">Parameters</a>"
                    "<a href=\"/raw\">Raw config</a><a href=\"/files?path=");
    sb_append_url(page, MEDIA_ROOT);
    sb_append(page, "\">Files</a><a href=\"/log\">Camera output</a></nav>");
}

static void append_notice(struct string_buffer *page, const char *message, bool is_error)
{
    if (message == NULL) return;
    sb_appendf(page, "<p class=\"notice%s\">", is_error ? " error" : "");
    sb_append_html(page, message);
    sb_append(page, "</p>");
}

static void append_parameter_field(struct string_buffer *page, const char *config,
                                   const struct parameter *parameter)
{
    char value[96] = "";
    bool present = ini_get_value(config, parameter->section, parameter->key,
                                 value, sizeof(value));

    sb_append(page, "<div class=field><label for=\"");
    sb_append_html(page, parameter->form_name);
    sb_append(page, "\">");
    sb_append_html(page, parameter->label);
    sb_append(page, "</label>");
    if (parameter->kind == PARAM_ENUM || parameter->kind == PARAM_BOOLEAN) {
        sb_append(page, "<select required name=\"");
        sb_append_html(page, parameter->form_name);
        sb_append(page, "\" id=\"");
        sb_append_html(page, parameter->form_name);
        sb_append(page, "\">");
        if (!present || !option_contains(parameter, value)) {
            sb_append(page, "<option selected disabled value=\"\">Unknown/missing current value</option>");
        }
        for (size_t i = 0; i < parameter->option_count; i++) {
            sb_append(page, "<option value=\"");
            sb_append_html(page, parameter->options[i].value);
            sb_appendf(page, "\"%s>", present && strcmp(value, parameter->options[i].value) == 0 ? " selected" : "");
            sb_append_html(page, parameter->options[i].label);
            sb_append(page, "</option>");
        }
        sb_append(page, "</select>");
    } else {
        sb_append(page, "<input required name=\"");
        sb_append_html(page, parameter->form_name);
        sb_append(page, "\" id=\"");
        sb_append_html(page, parameter->form_name);
        if (parameter->kind == PARAM_IPV4) {
            sb_append(page, "\" type=text inputmode=numeric value=\"");
        } else {
            sb_appendf(page, "\" type=number min=\"%.8g\" max=\"%.8g\" step=\"%.8g\" value=\"",
                       parameter->minimum, parameter->maximum, parameter->step);
        }
        if (present) sb_append_html(page, value);
        sb_append(page, "\">");
    }
    sb_append(page, "<div class=help>");
    sb_append_html(page, parameter->help);
    sb_appendf(page, " <code>[%s] %s</code></div></div>", parameter->section, parameter->key);
}

static void append_parameter_range(struct string_buffer *page, const char *config,
                                   size_t first, size_t end)
{
    sb_append(page, "<div class=fields>");
    for (size_t i = first; i < end; i++) append_parameter_field(page, config, &core_parameters[i]);
    sb_append(page, "</div>");
}

static void append_screen_mask(struct string_buffer *page, const char *config,
                               const char *prefix, const char *key, const char *label)
{
    char value[32] = "0";
    unsigned long mask = 0;
    char *end;
    (void)ini_get_value(config, "misc", key, value, sizeof(value));
    errno = 0;
    mask = strtoul(value, &end, 10);
    if (errno != 0 || *end != '\0') mask = 0;
    sb_append(page, "<div class=field><label>");
    sb_append_html(page, label);
    sb_append(page, "</label><div class=checks>");
    const char *names[] = {"wide", "zoom", "thermal"};
    const char *labels[] = {"Wide/composited RGB", "Zoom RGB", "Thermal"};
    const unsigned bits[] = {0, 1, 3};
    for (size_t i = 0; i < 3; i++) {
        sb_append(page, "<label><input type=checkbox value=1 name=\"");
        sb_append_html(page, prefix);
        sb_append(page, "_");
        sb_append_html(page, names[i]);
        sb_appendf(page, "\"%s> ", (mask & (1UL << bits[i])) ? " checked" : "");
        sb_append_html(page, labels[i]);
        sb_append(page, "</label>");
    }
    sb_appendf(page, "</div><div class=help>Bit mask <code>[%s] %s</code>: bit 0 wide/composited, bit 1 zoom, bit 3 thermal.</div></div>",
               "misc", key);
}

static char *render_page(const char *message, bool message_is_error, size_t *page_len)
{
    struct string_buffer page;
    char now_text[64];
    char uptime_text[64] = "unavailable";
    char load_text[128] = "unavailable";
    char addresses[512];
    time_t now = time(NULL);
    struct tm tm_now;
    double uptime = 0.0;
    FILE *file;
    long mem_total = -1;
    long mem_available = -1;
    pid_t pids[8];
    size_t pid_count = camera_pids(pids, sizeof(pids) / sizeof(pids[0]));
    long camera_rss = pid_count > 0 ? process_rss_kib(pids[0]) : -1;
    long web_rss = process_rss_kib(getpid());

    localtime_r(&now, &tm_now);
    strftime(now_text, sizeof(now_text), "%Y-%m-%d %H:%M:%S %Z", &tm_now);
    file = fopen("/proc/uptime", "r");
    if (file != NULL) {
        if (fscanf(file, "%lf", &uptime) == 1) {
            snprintf(uptime_text, sizeof(uptime_text), "%u d %02u:%02u:%02u",
                     (unsigned)(uptime / 86400), (unsigned)(uptime / 3600) % 24,
                     (unsigned)(uptime / 60) % 60, (unsigned)uptime % 60);
        }
        fclose(file);
    }
    file = fopen("/proc/loadavg", "r");
    if (file != NULL) {
        if (fgets(load_text, sizeof(load_text), file) != NULL) {
            load_text[strcspn(load_text, "\r\n")] = '\0';
        }
        fclose(file);
    }
    (void)read_memory(&mem_total, &mem_available);
    ipv4_addresses(addresses, sizeof(addresses));

    sb_init(&page);
    sb_append(&page, "<!doctype html><html lang=en><head><meta charset=utf-8>"
                    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                    "<title>MT11 control</title><style>");
    sb_append(&page, page_style);
    sb_append(&page, "</style></head><body><header><h1>MT11 control</h1>"
                    "<div class=muted>Authenticated administrative interface</div></header>");
    append_nav(&page);
    append_notice(&page, message, message_is_error);
    sb_append(&page, "<div class=grid><section class=card><h2>Status</h2><table>");
    sb_appendf(&page, "<tr><th>Camera app</th><td>%s", pid_count ? "running" : "stopped");
    if (pid_count) sb_appendf(&page, " (PID %ld, RSS %ld KiB)", (long)pids[0], camera_rss);
    sb_append(&page, "</td></tr>");
    sb_appendf(&page, "<tr><th>Web service</th><td>PID %ld, RSS %ld KiB</td></tr>",
               (long)getpid(), web_rss);
    sb_appendf(&page, "<tr><th>Time</th><td>%s</td></tr>", now_text);
    sb_appendf(&page, "<tr><th>Uptime</th><td>%s</td></tr>", uptime_text);
    sb_appendf(&page, "<tr><th>Load</th><td>%s</td></tr>", load_text);
    sb_appendf(&page, "<tr><th>Memory</th><td>%ld MiB available / %ld MiB</td></tr>",
               mem_available / 1024, mem_total / 1024);
    sb_appendf(&page, "<tr><th>IPv4</th><td>%s</td></tr>", addresses[0] ? addresses : "unavailable");
    append_filesystem_status(&page, "Rootfs", "/");
    append_filesystem_status(&page, "Application", "/app");
    append_filesystem_status(&page, "microSD", "/mnt");
    sb_append(&page, "</table><p><a href=\"/\">Refresh status</a></p></section>"
                    "<section class=card><h2>Actions</h2>"
                    "<form method=post action=/restart><input type=hidden name=csrf value=\"");
    sb_append(&page, csrf_token);
    sb_append(&page, "\"><button type=submit>Restart siyi_camera_app</button></form>"
                    "<form method=post action=/reboot><input type=hidden name=csrf value=\"");
    sb_append(&page, csrf_token);
    sb_append(&page, "\"><label><input type=checkbox name=confirm value=yes required> "
                    "I confirm this camera should reboot</label><br>"
                    "<button class=danger type=submit>Reboot camera</button></form>"
                    "<p class=muted>Authentication user: <code>admin</code>. The password is read "
                    "from <code>/app/web.pass</code> for every request. This service is HTTP, not HTTPS; "
                    "keep it on the isolated camera network.</p></section></div></body></html>");
    *page_len = page.len;
    return page.data;
}

static char *render_parameter_page(const char *message, bool message_is_error,
                                   size_t *page_len)
{
    struct string_buffer page;
    size_t config_len = 0;
    char *config = read_file(CONFIG_PATH, MAX_CONFIG, &config_len);
    (void)config_len;
    if (config == NULL) config = strdup("");
    if (config == NULL) return NULL;

    sb_init(&page);
    sb_append(&page, "<!doctype html><html lang=en><head><meta charset=utf-8>"
                    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                    "<title>MT11 parameters</title><style>");
    sb_append(&page, page_style);
    sb_append(&page, "</style></head><body><header><h1>MT11 parameters</h1>"
                    "<div class=muted>Validated controls reconstructed from siyi_camera_app v1.0.12</div></header>");
    append_nav(&page);
    append_notice(&page, message, message_is_error);
    sb_append(&page, "<p class=notice>Changes take effect when <code>siyi_camera_app</code> restarts. "
                    "Use <strong>Save and restart</strong> unless you are batching changes. Factory thermal "
                    "calibration and drift values are shown under Advanced and should normally be left unchanged.</p>"
                    "<form method=post action=/parameters><input type=hidden name=csrf value=\"");
    sb_append(&page, csrf_token);
    sb_append(&page, "\"><section class=card><h2>Streaming and storage</h2>");
    append_parameter_range(&page, config, 0, 6);
    sb_append(&page, "<div class=fields>");
    append_screen_mask(&page, config, "snap", "snap_screen", "Still-capture paths");
    append_screen_mask(&page, config, "record", "record_screen", "MP4 recording paths");
    sb_append(&page, "</div></section><section class=card><h2>Network</h2>");
    append_parameter_range(&page, config, 6, 9);
    sb_append(&page, "<p class=help>Changing the camera address may make this page disconnect after restart. "
                    "The base root filesystem also initially assigns 192.168.144.25 before the application applies these values.</p>"
                    "</section><section class=card><h2>Visible-camera image controls</h2>");
    append_parameter_range(&page, config, 9, 17);
    sb_append(&page, "</section><section class=card><h2>Thermal acquisition</h2>");
    append_parameter_range(&page, config, 17, 18);
    sb_append(&page, "</section><section class=card><h2>Default thermal environment</h2>");
    append_parameter_range(&page, config, 18, 23);
    sb_append(&page, "</section><section class=card><h2>Active thermal environment</h2>");
    append_parameter_range(&page, config, 23, 28);
    sb_append(&page, "</section><details class=card><summary>Advanced thermal calibration</summary>"
                    "<p class=notice>These values feed vendor calibration routines. Valid file syntax is known, "
                    "but the exact meaning of each drift-LUT column is not yet identified.</p>");
    append_parameter_range(&page, config, 28, 32);
    sb_append(&page, "<h3>Temperature-drift LUT</h3><div class=fields>");
    for (unsigned i = 0; i < 30; i++) {
        char key[24];
        char value[96] = "";
        snprintf(key, sizeof(key), "lut_%u", i);
        (void)ini_get_value(config, "temperature_drift", key, value, sizeof(value));
        sb_appendf(&page, "<div class=field><label for=\"lut_%u\">lut_%u</label>"
                          "<input required type=text pattern=\"-?[0-9]+,-?[0-9]+,-?[0-9]+\" "
                          "name=\"lut_%u\" id=\"lut_%u\" value=\"", i, i, i, i);
        sb_append_html(&page, value);
        sb_append(&page, "\"><div class=help>Exactly three comma-separated signed integers.</div></div>");
    }
    sb_append(&page, "</div></details><div class=actions>"
                    "<button type=submit name=action value=save>Save parameters</button>"
                    "<button type=submit name=action value=save_restart>Save and restart camera app</button>"
                    "</div></form></body></html>");
    free(config);
    *page_len = page.len;
    return page.data;
}

static char *render_raw_page(const char *message, bool message_is_error, size_t *page_len)
{
    struct string_buffer page;
    size_t config_len = 0;
    char *config = read_file(CONFIG_PATH, MAX_CONFIG, &config_len);
    (void)config_len;
    if (config == NULL) config = strdup("");
    if (config == NULL) return NULL;
    sb_init(&page);
    sb_append(&page, "<!doctype html><html lang=en><head><meta charset=utf-8>"
                    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                    "<title>MT11 raw config</title><style>");
    sb_append(&page, page_style);
    sb_append(&page, "</style></head><body><header><h1>Raw config.ini</h1>"
                    "<div class=muted>Advanced editor for vendor or newly discovered settings</div></header>");
    append_nav(&page);
    append_notice(&page, message, message_is_error);
    sb_append(&page, "<section class=card><form method=post action=/config>"
                    "<input type=hidden name=csrf value=\"");
    sb_append(&page, csrf_token);
    sb_append(&page, "\"><textarea name=config spellcheck=false>");
    sb_append_html(&page, config);
    sb_append(&page, "</textarea><br><button type=submit name=action value=save>Save config</button>"
                    "<button type=submit name=action value=save_restart>Save and restart camera app</button>"
                    "</form><p class=muted>Saves are syntax-checked and atomic. The previous file is kept at "
                    "<code>/app/config.ini.web.bak</code>. The Parameters page performs stronger per-value validation.</p>"
                    "</section></body></html>");
    free(config);
    *page_len = page.len;
    return page.data;
}

static char *render_files_page(const char *requested_path, const char *message,
                               bool message_is_error, size_t *page_len,
                               char *error, size_t error_size)
{
    struct string_buffer page;
    char resolved[PATH_MAX];
    char parent[PATH_MAX];
    DIR *directory;
    const struct dirent *entry;
    unsigned shown = 0;

    if (!canonical_existing_path(requested_path != NULL ? requested_path : MEDIA_ROOT,
                                 resolved, error, error_size)) return NULL;
    directory = opendir(resolved);
    if (directory == NULL) {
        snprintf(error, error_size, "Cannot open directory: %s", strerror(errno));
        return NULL;
    }
    path_parent(resolved, parent);
    sb_init(&page);
    sb_append(&page, "<!doctype html><html lang=en><head><meta charset=utf-8>"
                    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                    "<title>MT11 files</title><style>");
    sb_append(&page, page_style);
    sb_append(&page, "</style></head><body><header><h1>Filesystem</h1>"
                    "<div class=muted>Authenticated browsing and downloads; deletion is restricted to /mnt</div></header>");
    append_nav(&page);
    append_notice(&page, message, message_is_error);
    sb_append(&page, "<section class=card><form class=pathbox method=get action=/files>"
                    "<input aria-label=Path required type=text name=path value=\"");
    sb_append_html(&page, resolved);
    sb_append(&page, "\"><button type=submit>Open path</button></form>"
                    "<p><a href=\"/files?path=%2F\">/</a> &middot; "
                    "<a href=\"/files?path=%2Fapp\">/app</a> &middot; "
                    "<a href=\"/files?path=%2Fmnt\">/mnt</a></p></section>"
                    "<section class=card><h2>");
    sb_append_html(&page, resolved);
    sb_append(&page, "</h2><table><thead><tr><th>Name</th><th>Type</th><th>Size</th>"
                    "<th>Modified</th><th>Mode</th><th>Actions</th></tr></thead><tbody>");
    if (strcmp(resolved, "/") != 0) {
        sb_append(&page, "<tr><td><a href=\"");
        append_path_query(&page, "/files?path=", parent);
        sb_append(&page, "\">../</a></td><td>directory</td><td>-</td><td>-</td><td>-</td><td></td></tr>");
    }
    while ((entry = readdir(directory)) != NULL && shown < 5000) {
        char child[PATH_MAX];
        char time_text[64];
        char size_text[32] = "-";
        struct stat lst;
        struct stat target;
        bool target_is_dir;
        const char *route;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        int child_length = strcmp(resolved, "/") == 0 ?
            snprintf(child, sizeof(child), "/%s", entry->d_name) :
            snprintf(child, sizeof(child), "%s/%s", resolved, entry->d_name);
        if (child_length < 0 || child_length >= (int)sizeof(child) || lstat(child, &lst) < 0) continue;
        target = lst;
        if (S_ISLNK(lst.st_mode)) (void)stat(child, &target);
        target_is_dir = S_ISDIR(target.st_mode);
        route = target_is_dir ? "/files?path=" : "/view?path=";
        format_file_time(lst.st_mtime, time_text);
        if (S_ISREG(lst.st_mode)) format_bytes((uint64_t)lst.st_size, size_text);
        sb_append(&page, "<tr><td><a href=\"");
        append_path_query(&page, route, child);
        sb_append(&page, "\">");
        sb_append_html(&page, entry->d_name);
        if (target_is_dir) sb_append(&page, "/");
        sb_appendf(&page, "</a></td><td>%c%s</td><td>%s</td><td>%s</td><td>%04o</td><td class=file-actions>",
                   file_type_char(lst.st_mode), S_ISLNK(lst.st_mode) ? " (link)" : "",
                   size_text, time_text, (unsigned)(lst.st_mode & 07777));
        if (S_ISREG(target.st_mode)) {
            sb_append(&page, "<a href=\"");
            append_path_query(&page, "/file?path=", child);
            sb_append(&page, "&amp;download=1\">Download</a>");
        }
        if (path_can_be_deleted(child)) {
            sb_append(&page, " &middot; <a class=delete-link href=\"");
            append_path_query(&page, "/delete-confirm?path=", child);
            sb_append(&page, "\">Delete</a>");
        }
        sb_append(&page, "</td></tr>");
        shown++;
    }
    closedir(directory);
    sb_append(&page, "</tbody></table>");
    if (shown == 5000) sb_append(&page, "<p class=notice>Only the first 5000 entries are shown.</p>");
    sb_append(&page, "<p class=muted>Type letters: d directory, f regular file, l symbolic link, "
                    "c character device, b block device, p FIFO, s socket. Files outside /mnt can be "
                    "viewed or downloaded but never deleted through this service.</p></section></body></html>");
    *page_len = page.len;
    return page.data;
}

static char *render_view_page(const char *requested_path, size_t *page_len,
                              char *error, size_t error_size)
{
    struct string_buffer page;
    char resolved[PATH_MAX];
    char parent[PATH_MAX];
    char size_text[32];
    struct stat st;
    const char *mime;

    if (!canonical_existing_path(requested_path, resolved, error, error_size) ||
        stat(resolved, &st) < 0 || !S_ISREG(st.st_mode)) {
        snprintf(error, error_size, "View target must be a regular file");
        return NULL;
    }
    mime = file_mime_type(resolved);
    path_parent(resolved, parent);
    format_bytes((uint64_t)st.st_size, size_text);
    sb_init(&page);
    sb_append(&page, "<!doctype html><html lang=en><head><meta charset=utf-8>"
                    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                    "<title>MT11 file viewer</title><style>");
    sb_append(&page, page_style);
    sb_append(&page, "</style></head><body><header><h1>File viewer</h1><div class=muted>");
    sb_append_html(&page, resolved);
    sb_append(&page, "</div></header>");
    append_nav(&page);
    sb_append(&page, "<section class=card><p><a href=\"");
    append_path_query(&page, "/files?path=", parent);
    sb_append(&page, "\">Back to directory</a> &middot; <a href=\"");
    append_path_query(&page, "/file?path=", resolved);
    sb_append(&page, "&amp;download=1\">Download</a>");
    if (path_can_be_deleted(resolved)) {
        sb_append(&page, " &middot; <a class=delete-link href=\"");
        append_path_query(&page, "/delete-confirm?path=", resolved);
        sb_append(&page, "\">Delete</a>");
    }
    sb_appendf(&page, "</p><p class=muted>%s; %s</p>", mime, size_text);
    if (mime_is_image(mime)) {
        sb_append(&page, "<img class=preview alt=\"Image preview\" src=\"");
        append_path_query(&page, "/file?path=", resolved);
        sb_append(&page, "\">");
    } else if (mime_is_video(mime)) {
        sb_append(&page, "<video class=preview controls preload=metadata src=\"");
        append_path_query(&page, "/file?path=", resolved);
        sb_append(&page, "\">This browser cannot play the video; use Download.</video>");
    } else {
        sb_append(&page, "<p>No inline preview is available for this file type. Use Download.</p>");
    }
    sb_append(&page, "</section></body></html>");
    *page_len = page.len;
    return page.data;
}

static char *render_delete_page(const char *requested_path, size_t *page_len,
                                char *error, size_t error_size)
{
    struct string_buffer page;
    char resolved[PATH_MAX];
    struct stat st;

    if (requested_path == NULL || lstat(requested_path, &st) < 0 ||
        !path_can_be_deleted(requested_path)) {
        snprintf(error, error_size, "Deletion is permitted only for existing targets beneath %s", MEDIA_ROOT);
        return NULL;
    }
    if (S_ISLNK(st.st_mode)) snprintf(resolved, sizeof(resolved), "%s", requested_path);
    else if (realpath(requested_path, resolved) == NULL) {
        snprintf(error, error_size, "Cannot resolve delete target");
        return NULL;
    }
    sb_init(&page);
    sb_append(&page, "<!doctype html><html lang=en><head><meta charset=utf-8>"
                    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                    "<title>Confirm deletion</title><style>");
    sb_append(&page, page_style);
    sb_append(&page, "</style></head><body><header><h1>Confirm deletion</h1></header>");
    append_nav(&page);
    sb_append(&page, "<section class=card><p class=\"notice error\">This permanently deletes ");
    sb_append_html(&page, resolved);
    if (S_ISDIR(st.st_mode)) sb_append(&page, " and all files and directories beneath it");
    sb_append(&page, ". This operation cannot be undone.</p><form method=post action=/delete>"
                    "<input type=hidden name=csrf value=\"");
    sb_append(&page, csrf_token);
    sb_append(&page, "\"><input type=hidden name=path value=\"");
    sb_append_html(&page, resolved);
    sb_append(&page, "\"><label><input required type=checkbox name=confirm value=yes> "
                    "I understand this deletion is permanent</label><br>"
                    "<button class=danger type=submit>Delete permanently</button></form></section></body></html>");
    *page_len = page.len;
    return page.data;
}

static char *render_log_page(size_t *page_len)
{
    struct string_buffer page;
    size_t log_len = 0;
    char *log = read_app_log(&log_len);

    if (log == NULL) return NULL;
    sb_init(&page);
    sb_append(&page, "<!doctype html><html lang=en><head><meta charset=utf-8>"
                    "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
                    "<meta http-equiv=refresh content=3><title>MT11 camera output</title>"
                    "<style>:root{color-scheme:light dark;--bg:#f4f7fa;--card:#fff;--text:#17212b;"
                    "--muted:#607080;--line:#d7e0e8;--accent:#1769aa}"
                    "@media(prefers-color-scheme:dark){:root{--bg:#10161d;--card:#18222d;"
                    "--text:#e8edf2;--muted:#a7b3bf;--line:#344352;--accent:#77bdf2}}"
                    "*{box-sizing:border-box}body{margin:0;padding:20px;font:15px/1.5 system-ui,sans-serif;"
                    "color:var(--text);background:var(--bg)}h1{margin-bottom:4px}.muted{color:var(--muted)}"
                    "nav{display:flex;gap:8px;margin:18px 0}nav a{padding:8px 12px;border:1px solid var(--line);"
                    "border-radius:6px;color:var(--accent);text-decoration:none;background:var(--card)}"
                    "pre{margin:0;padding:14px;border:1px solid var(--line);border-radius:8px;"
                    "background:var(--card);white-space:pre-wrap;overflow-wrap:anywhere;"
                    "font:12px/1.4 ui-monospace,monospace;min-height:70vh}</style></head><body>"
                    "<header><h1>MT11 control</h1><div class=muted>Camera application output; "
                    "refreshes every 3 seconds</div></header><nav>"
                    "<a href=\"/\">Status</a><a href=\"/parameters\">Parameters</a>"
                    "<a href=\"/raw\">Raw config</a><a href=\"/files?path=%2Fmnt\">Files</a>"
                    "<a href=\"/log\">Camera output</a>"
                    "<a href=\"/log.txt\">Plain text</a></nav><pre>");
    if (log_len == 0) {
        sb_append(&page, "No captured output yet. Restart siyi_camera_app to begin capture.");
    } else {
        sb_append_log_html(&page, log, log_len);
    }
    sb_appendf(&page, "</pre><p class=muted>Showing the newest %zu bytes. The RAM log rotates "
                      "at %u KiB and retains one previous segment.</p></body></html>",
               log_len, APP_LOG_ROTATE_SIZE / 1024U);
    free(log);
    *page_len = page.len;
    return page.data;
}

static void send_page(int fd, const char *message, bool is_error)
{
    size_t length;
    char *page = render_page(message, is_error, &length);
    if (page == NULL) {
        send_text_error(fd, 500, "Internal Server Error", "Out of memory\n", NULL);
        return;
    }
    send_response(fd, is_error ? 400 : 200, is_error ? "Bad Request" : "OK",
                  "text/html; charset=utf-8", page, length, NULL);
    free(page);
}

static void send_parameter_page(int fd, const char *message, bool is_error)
{
    size_t length;
    char *page = render_parameter_page(message, is_error, &length);
    if (page == NULL) {
        send_text_error(fd, 500, "Internal Server Error", "Unable to read config or out of memory\n", NULL);
        return;
    }
    send_response(fd, is_error ? 400 : 200, is_error ? "Bad Request" : "OK",
                  "text/html; charset=utf-8", page, length, NULL);
    free(page);
}

static void send_raw_page(int fd, const char *message, bool is_error)
{
    size_t length;
    char *page = render_raw_page(message, is_error, &length);
    if (page == NULL) {
        send_text_error(fd, 500, "Internal Server Error", "Unable to read config or out of memory\n", NULL);
        return;
    }
    send_response(fd, is_error ? 400 : 200, is_error ? "Bad Request" : "OK",
                  "text/html; charset=utf-8", page, length, NULL);
    free(page);
}

static void send_log_page(int fd)
{
    size_t length;
    char *page = render_log_page(&length);
    if (page == NULL) {
        send_text_error(fd, 500, "Internal Server Error", "Out of memory\n", NULL);
        return;
    }
    send_response(fd, 200, "OK", "text/html; charset=utf-8", page, length, NULL);
    free(page);
}

static void send_log_text(int fd)
{
    size_t length = 0;
    char *log = read_app_log(&length);
    if (log == NULL) {
        send_text_error(fd, 500, "Internal Server Error", "Unable to read log\n", NULL);
        return;
    }
    send_response(fd, 200, "OK", "text/plain; charset=utf-8", log, length, NULL);
    free(log);
}

static void send_files_page(int fd, const char *path, const char *message, bool is_error)
{
    char error[256];
    size_t length;
    char *page = render_files_page(path, message, is_error, &length, error, sizeof(error));
    if (page == NULL) {
        send_text_error(fd, 400, "Bad Request", error, NULL);
        return;
    }
    send_response(fd, is_error ? 400 : 200, is_error ? "Bad Request" : "OK",
                  "text/html; charset=utf-8", page, length, NULL);
    free(page);
}

static void send_view_page(int fd, const char *path)
{
    char error[256];
    size_t length;
    char *page = render_view_page(path, &length, error, sizeof(error));
    if (page == NULL) {
        send_text_error(fd, 400, "Bad Request", error, NULL);
        return;
    }
    send_response(fd, 200, "OK", "text/html; charset=utf-8", page, length, NULL);
    free(page);
}

static void send_delete_page(int fd, const char *path)
{
    char error[256];
    size_t length;
    char *page = render_delete_page(path, &length, error, sizeof(error));
    if (page == NULL) {
        send_text_error(fd, 403, "Forbidden", error, NULL);
        return;
    }
    send_response(fd, 200, "OK", "text/html; charset=utf-8", page, length, NULL);
    free(page);
}

enum range_result {
    RANGE_NONE,
    RANGE_VALID,
    RANGE_INVALID
};

static enum range_result parse_byte_range(const struct request *request, uint64_t size,
                                          uint64_t *start, uint64_t *end)
{
    char value[256];
    char *dash;
    char *parse_end;
    unsigned long long first;
    unsigned long long last;

    if (find_header(request, "Range", value, sizeof(value)) == NULL) return RANGE_NONE;
    if (strncmp(value, "bytes=", 6) != 0 || strchr(value + 6, ',') != NULL || size == 0) {
        return RANGE_INVALID;
    }
    dash = strchr(value + 6, '-');
    if (dash == NULL) return RANGE_INVALID;
    *dash = '\0';
    if (value[6] == '\0') {
        errno = 0;
        last = strtoull(dash + 1, &parse_end, 10);
        if (errno != 0 || dash[1] == '\0' || *parse_end != '\0' || last == 0) return RANGE_INVALID;
        if (last > size) last = size;
        *start = size - last;
        *end = size - 1;
        return RANGE_VALID;
    }
    errno = 0;
    first = strtoull(value + 6, &parse_end, 10);
    if (errno != 0 || *parse_end != '\0' || first >= size) return RANGE_INVALID;
    if (dash[1] == '\0') {
        last = size - 1;
    } else {
        errno = 0;
        last = strtoull(dash + 1, &parse_end, 10);
        if (errno != 0 || *parse_end != '\0' || last < first) return RANGE_INVALID;
        if (last >= size) last = size - 1;
    }
    *start = first;
    *end = last;
    return RANGE_VALID;
}

static void safe_download_name(const char *path, char output[256])
{
    const char *base = strrchr(path, '/');
    size_t used = 0;
    if (base == NULL) base = path; else base++;
    while (*base != '\0' && used + 1 < 256) {
        unsigned char value = (unsigned char)*base++;
        output[used++] = value >= 0x20 && value < 0x7f && value != '"' && value != '\\' ?
                         (char)value : '_';
    }
    if (used == 0) output[used++] = '_';
    output[used] = '\0';
}

static void send_file_response(int client, const struct request *request,
                               const char *requested_path, bool download)
{
    char resolved[PATH_MAX];
    char error[256];
    char filename[256];
    char header[4096];
    struct stat st;
    int file = -1;
    uint64_t start = 0;
    uint64_t end;
    uint64_t remaining;
    enum range_result range;
    bool head = strcmp(request->method, "HEAD") == 0;
    const char *mime;
    int header_length;

    if (!canonical_existing_path(requested_path, resolved, error, sizeof(error))) {
        send_text_error(client, 404, "Not Found", error, NULL);
        return;
    }
    file = open(resolved, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (file < 0 || fstat(file, &st) < 0 || !S_ISREG(st.st_mode) || st.st_size < 0) {
        if (file >= 0) close(file);
        send_text_error(client, 400, "Bad Request", "Download target must be a regular file\n", NULL);
        return;
    }
    end = (uint64_t)st.st_size == 0 ? 0 : (uint64_t)st.st_size - 1;
    range = parse_byte_range(request, (uint64_t)st.st_size, &start, &end);
    if (range == RANGE_INVALID) {
        header_length = snprintf(header, sizeof(header),
            "HTTP/1.1 416 Range Not Satisfiable\r\nServer: %s\r\n"
            "Content-Range: bytes */%" PRIu64 "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
            SERVER_NAME, (uint64_t)st.st_size);
        if (header_length > 0 && (size_t)header_length < sizeof(header))
            (void)send_all(client, header, (size_t)header_length);
        close(file);
        return;
    }
    remaining = (uint64_t)st.st_size == 0 ? 0 : end - start + 1;
    mime = file_mime_type(resolved);
    safe_download_name(resolved, filename);
    if (range == RANGE_VALID) {
        header_length = snprintf(header, sizeof(header),
            "HTTP/1.1 206 Partial Content\r\nServer: %s\r\nContent-Type: %s\r\n"
            "Content-Length: %" PRIu64 "\r\nContent-Range: bytes %" PRIu64 "-%" PRIu64 "/%" PRIu64 "\r\n"
            "Accept-Ranges: bytes\r\nContent-Disposition: %s; filename=\"%s\"\r\n"
            "Cache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n",
            SERVER_NAME, mime, remaining, start, end, (uint64_t)st.st_size,
            download ? "attachment" : "inline", filename);
    } else {
        header_length = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\nServer: %s\r\nContent-Type: %s\r\n"
            "Content-Length: %" PRIu64 "\r\nAccept-Ranges: bytes\r\n"
            "Content-Disposition: %s; filename=\"%s\"\r\nCache-Control: no-store\r\n"
            "X-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n",
            SERVER_NAME, mime, remaining, download ? "attachment" : "inline", filename);
    }
    if (header_length <= 0 || (size_t)header_length >= sizeof(header) ||
        !send_all(client, header, (size_t)header_length) || head) {
        close(file);
        return;
    }
    while (remaining > 0) {
        char buffer[64 * 1024];
        size_t wanted = remaining < sizeof(buffer) ? (size_t)remaining : sizeof(buffer);
        ssize_t got = pread(file, buffer, wanted, (off_t)start);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0 || !send_all(client, buffer, (size_t)got)) break;
        start += (uint64_t)got;
        remaining -= (uint64_t)got;
    }
    close(file);
}

static void send_file_response_async(int client, const struct request *request,
                                     const char *path, bool download)
{
    pid_t worker = fork();
    if (worker < 0) {
        send_file_response(client, request, path, download);
        return;
    }
    if (worker == 0) {
        if (listen_fd >= 0) close(listen_fd);
        send_file_response(client, request, path, download);
        close(client);
        _exit(0);
    }
}

static void schedule_reboot(void)
{
    pid_t child = fork();
    if (child != 0) return;
    if (listen_fd >= 0) close(listen_fd);
    sleep(2);
    sync();
    reboot(RB_AUTOBOOT);
    _exit(1);
}

static void handle_request(int fd, const char *peer)
{
    struct request request = {0};
    int receive_status = receive_request(fd, &request);
    int auth;

    if (receive_status != 0) {
        if (receive_status == 413) {
            send_text_error(fd, 413, "Payload Too Large", "Request body too large\n", NULL);
        } else if (receive_status == 431) {
            send_text_error(fd, 431, "Request Header Fields Too Large", "Headers too large\n", NULL);
        } else {
            send_text_error(fd, 400, "Bad Request", "Malformed HTTP request\n", NULL);
        }
        return;
    }
    auth = authenticate(&request);
    if (auth < 0) {
        send_text_error(fd, 503, "Service Unavailable",
                        "Missing or empty /app/web.pass\n", NULL);
        goto done;
    }
    if (auth == 0) {
        log_message("authentication failed from %s", peer);
        send_text_error(fd, 401, "Unauthorized", "Authentication required\n",
                        "WWW-Authenticate: Basic realm=\"MT11 camera\", charset=\"UTF-8\"\r\n");
        goto done;
    }
    if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/") == 0) {
        send_page(fd, NULL, false);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/parameters") == 0) {
        send_parameter_page(fd, NULL, false);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/raw") == 0) {
        send_raw_page(fd, NULL, false);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/files") == 0) {
        size_t path_length = 0;
        char *path = query_value(&request, "path", &path_length);
        if (path == NULL) path = strdup(MEDIA_ROOT);
        if (path == NULL || path_length >= PATH_MAX) {
            send_text_error(fd, 400, "Bad Request", "Invalid or overly long path\n", NULL);
        } else {
            send_files_page(fd, path, NULL, false);
        }
        free(path);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/view") == 0) {
        size_t path_length = 0;
        char *path = query_value(&request, "path", &path_length);
        if (path == NULL || path_length >= PATH_MAX) {
            send_text_error(fd, 400, "Bad Request", "Invalid or missing path\n", NULL);
        } else {
            send_view_page(fd, path);
        }
        free(path);
    } else if ((strcmp(request.method, "GET") == 0 || strcmp(request.method, "HEAD") == 0) &&
               strcmp(request.path, "/file") == 0) {
        size_t path_length = 0;
        size_t download_length = 0;
        char *path = query_value(&request, "path", &path_length);
        char *download = query_value(&request, "download", &download_length);
        bool as_download = download != NULL && download_length == 1 && download[0] == '1';
        if (path == NULL || path_length >= PATH_MAX) {
            send_text_error(fd, 400, "Bad Request", "Invalid or missing path\n", NULL);
        } else {
            send_file_response_async(fd, &request, path, as_download);
        }
        free(download);
        free(path);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/delete-confirm") == 0) {
        size_t path_length = 0;
        char *path = query_value(&request, "path", &path_length);
        if (path == NULL || path_length >= PATH_MAX) {
            send_text_error(fd, 400, "Bad Request", "Invalid or missing path\n", NULL);
        } else {
            send_delete_page(fd, path);
        }
        free(path);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/log") == 0) {
        send_log_page(fd);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/log.txt") == 0) {
        send_log_text(fd);
    } else if (strcmp(request.method, "GET") == 0 && strcmp(request.path, "/healthz") == 0) {
        const char *health = camera_pids(NULL, 0) > 0 ? "camera_app=running\n" : "camera_app=stopped\n";
        send_response(fd, 200, "OK", "text/plain; charset=utf-8",
                      health, strlen(health), NULL);
    } else if (strcmp(request.method, "POST") == 0) {
        char error[256];
        if (!valid_csrf(&request)) {
            log_message("CSRF check failed from %s for %s", peer, request.path);
            send_page(fd, "Invalid or expired form token; reload the page", true);
        } else if (strcmp(request.path, "/delete") == 0) {
            size_t path_length = 0;
            size_t confirmation_length = 0;
            char *path = form_value(&request, "path", &path_length);
            char *confirmation = form_value(&request, "confirm", &confirmation_length);
            bool confirmed = confirmation != NULL && confirmation_length == 3 &&
                             memcmp(confirmation, "yes", 3) == 0;
            char deleted[PATH_MAX];
            char parent[PATH_MAX];
            if (!confirmed) {
                send_text_error(fd, 400, "Bad Request", "Deletion confirmation was not checked\n", NULL);
            } else if (path == NULL || path_length >= PATH_MAX) {
                send_text_error(fd, 400, "Bad Request", "Invalid or missing delete path\n", NULL);
            } else if (!delete_media_path(path, deleted, sizeof(deleted), error, sizeof(error))) {
                log_message("filesystem deletion rejected/failed for %s: %s", peer, error);
                send_text_error(fd, 403, "Forbidden", error, NULL);
            } else {
                path_parent(deleted, parent);
                log_message("filesystem path deleted by %s: %s", peer, deleted);
                send_files_page(fd, parent, "Path deleted permanently", false);
            }
            free(confirmation);
            free(path);
        } else if (strcmp(request.path, "/restart") == 0) {
            if (restart_camera(error, sizeof(error))) {
                log_message("camera application restarted by %s", peer);
                send_page(fd, "siyi_camera_app restarted", false);
            } else {
                log_message("camera restart failed for %s: %s", peer, error);
                send_page(fd, error, true);
            }
        } else if (strcmp(request.path, "/parameters") == 0) {
            struct ini_update updates[80] = {0};
            size_t update_count = 0;
            size_t old_length = 0;
            size_t new_length = 0;
            size_t action_len = 0;
            char *old_config = read_file(CONFIG_PATH, MAX_CONFIG, &old_length);
            char *new_config = NULL;
            char *action = form_value(&request, "action", &action_len);
            bool restart = action != NULL && action_len == strlen("save_restart") &&
                           memcmp(action, "save_restart", action_len) == 0;
            (void)old_length;
            if (old_config == NULL) {
                snprintf(error, sizeof(error), "Cannot read %s: %s", CONFIG_PATH, strerror(errno));
                send_parameter_page(fd, error, true);
            } else if (!collect_parameter_updates(&request, updates,
                                                  sizeof(updates) / sizeof(updates[0]),
                                                  &update_count, error, sizeof(error))) {
                send_parameter_page(fd, error, true);
            } else if ((new_config = ini_apply_updates(old_config, updates, update_count,
                                                       &new_length, error, sizeof(error))) == NULL) {
                send_parameter_page(fd, error, true);
            } else if (!save_config(new_config, new_length, error, sizeof(error))) {
                log_message("parameter save failed for %s: %s", peer, error);
                send_parameter_page(fd, error, true);
            } else if (restart && !restart_camera(error, sizeof(error))) {
                log_message("parameters saved but restart failed for %s: %s", peer, error);
                send_parameter_page(fd, error, true);
            } else {
                log_message("parameters saved%s by %s", restart ? " and camera restarted" : "", peer);
                send_parameter_page(fd, restart ? "Parameters saved and siyi_camera_app restarted" :
                                                   "Parameters saved; restart the camera app to apply them",
                                    false);
            }
            free(action);
            free(new_config);
            free(old_config);
            free_parameter_updates(updates, update_count);
        } else if (strcmp(request.path, "/config") == 0) {
            size_t config_len = 0;
            size_t action_len = 0;
            char *config = form_value(&request, "config", &config_len);
            char *action = form_value(&request, "action", &action_len);
            bool restart = action != NULL && action_len == strlen("save_restart") &&
                           memcmp(action, "save_restart", action_len) == 0;
            if (config == NULL) {
                send_raw_page(fd, "Form did not contain config data", true);
            } else {
                normalize_newlines(config, &config_len);
                if (!save_config(config, config_len, error, sizeof(error))) {
                    log_message("config save failed for %s: %s", peer, error);
                    send_raw_page(fd, error, true);
                } else if (restart && !restart_camera(error, sizeof(error))) {
                    log_message("config saved but restart failed for %s: %s", peer, error);
                    send_raw_page(fd, error, true);
                } else {
                    log_message("config saved%s by %s", restart ? " and camera restarted" : "", peer);
                    send_raw_page(fd, restart ? "Config saved and siyi_camera_app restarted" : "Config saved", false);
                }
            }
            free(config);
            free(action);
        } else if (strcmp(request.path, "/reboot") == 0) {
            size_t confirmation_len = 0;
            char *confirmation = form_value(&request, "confirm", &confirmation_len);
            bool confirmed = confirmation != NULL && confirmation_len == 3 &&
                             memcmp(confirmation, "yes", 3) == 0;
            free(confirmation);
            if (!confirmed) {
                send_page(fd, "Reboot confirmation was not checked", true);
            } else {
                log_message("camera reboot requested by %s", peer);
                const char *body = "<!doctype html><meta charset=utf-8><title>MT11 rebooting</title>"
                                   "<h1>Camera rebooting</h1><p>Reconnect in about one minute.</p>";
                send_response(fd, 200, "OK", "text/html; charset=utf-8",
                              body, strlen(body), NULL);
                schedule_reboot();
            }
        } else {
            send_text_error(fd, 404, "Not Found", "Unknown action\n", NULL);
        }
    } else {
        send_text_error(fd, 404, "Not Found", "Not found\n", NULL);
    }
done:
    free(request.storage);
}

static void handle_signal(int signal_number)
{
    (void)signal_number;
    if (listen_fd >= 0) close(listen_fd);
    listen_fd = -1;
}

static int create_listener(unsigned port)
{
    struct sockaddr_in address = {0};
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    int one = 1;

    if (fd < 0) return -1;
    (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(fd, 16) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int main(int argc, char **argv)
{
    unsigned port = DEFAULT_PORT;

    if (argc == 2 && strcmp(argv[1], "--capture-app-log") == 0) {
        return capture_app_log();
    }
    if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        char *end;
        unsigned long value = strtoul(argv[2], &end, 10);
        if (*argv[2] == '\0' || *end != '\0' || value < 1 || value > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[2]);
            return 2;
        }
        port = (unsigned)value;
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [-p port] | --capture-app-log\n", argv[0]);
        return 2;
    }
    if (!random_token(csrf_token)) {
        perror("Cannot create CSRF token");
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    listen_fd = create_listener(port);
    if (listen_fd < 0) {
        perror("Cannot listen");
        return 1;
    }
    log_message("%s listening on 0.0.0.0:%u", SERVER_NAME, port);
    while (listen_fd >= 0) {
        struct sockaddr_in peer_address;
        socklen_t peer_length = sizeof(peer_address);
        char peer[INET_ADDRSTRLEN] = "unknown";
        int client = accept4(listen_fd, (struct sockaddr *)&peer_address,
                             &peer_length, SOCK_CLOEXEC);
        if (client < 0) {
            if (errno == EINTR) continue;
            if (listen_fd < 0 || errno == EBADF) break;
            log_message("accept failed: %s", strerror(errno));
            continue;
        }
        (void)inet_ntop(AF_INET, &peer_address.sin_addr, peer, sizeof(peer));
        struct timeval timeout = {.tv_sec = 10, .tv_usec = 0};
        (void)setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        (void)setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        handle_request(client, peer);
        close(client);
    }
    log_message("server stopped");
    return 0;
}
