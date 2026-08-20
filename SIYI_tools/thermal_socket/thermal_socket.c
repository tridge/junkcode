/*
  Thermal capture daemon for the SIYI MT11.

  A TCP client receives the most recent 640x512 raw thermal frame from:
    /mnt/DCIM/capture/YYYY-MM-DD/YYYY-MM-DD_HH-MM-SS_milliseconds_I.bin
 */
#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <zlib.h>

#define LISTEN_PORT 7345
#define THERMAL_DIR "/mnt/DCIM/capture"
#define EXPECTED_SIZE (640 * 512 * 2)
#define TARGET_SEND_TIME_S 0.5

#define PACKED __attribute__((__packed__))

struct PACKED header {
    char fname[128];
    uint32_t compressed_size;
    double timestamp;
};

struct watch {
    int wd;
    char *path;
};

struct thermal_state {
    const char *root;
    int inotify_fd;
    struct watch *watches;
    size_t watch_count;
    size_t watch_capacity;
    bool have_latest;
    char latest[PATH_MAX];
    struct timespec latest_mtime;
    bool latest_key_valid;
    int latest_key[7];
};

static double ts_to_double(const struct timespec *ts)
{
    return (double)ts->tv_sec + (double)ts->tv_nsec * 1.0e-9;
}

static bool has_suffix(const char *s, const char *suffix)
{
    size_t slen = strlen(s);
    size_t suffix_len = strlen(suffix);
    return slen >= suffix_len && strcmp(s + slen - suffix_len, suffix) == 0;
}

/*
  MT11 milliseconds are not zero-padded. Parse the name numerically instead
  of assuming lexical order (for example, 609 ms is later than 8 ms).
 */
static bool capture_key(const char *path, int key[7])
{
    const char *base = strrchr(path, '/');
    int consumed = 0;

    base = base == NULL ? path : base + 1;
    if (sscanf(base, "%d-%d-%d_%d-%d-%d_%d_I.bin%n",
               &key[0], &key[1], &key[2], &key[3], &key[4], &key[5],
               &key[6], &consumed) != 7) {
        return false;
    }
    return base[consumed] == '\0';
}

static int compare_key(const int a[7], const int b[7])
{
    unsigned i;

    for (i = 0; i < 7; i++) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

static bool newer_mtime(const struct timespec *a, const struct timespec *b)
{
    if (a->tv_sec != b->tv_sec) {
        return a->tv_sec > b->tv_sec;
    }
    return a->tv_nsec > b->tv_nsec;
}

static void consider_file(struct thermal_state *state, const char *path)
{
    struct stat st;
    int key[7];
    bool key_valid;
    bool newer;

    if (!has_suffix(path, "_I.bin") || lstat(path, &st) != 0 ||
        !S_ISREG(st.st_mode) || st.st_size != EXPECTED_SIZE) {
        return;
    }

    key_valid = capture_key(path, key);
    if (!state->have_latest) {
        newer = true;
    } else if (key_valid && state->latest_key_valid) {
        newer = compare_key(key, state->latest_key) > 0;
    } else {
        newer = newer_mtime(&st.st_mtim, &state->latest_mtime);
    }
    if (!newer) {
        return;
    }

    snprintf(state->latest, sizeof(state->latest), "%s", path);
    state->latest_mtime = st.st_mtim;
    state->latest_key_valid = key_valid;
    if (key_valid) {
        memcpy(state->latest_key, key, sizeof(key));
    }
    state->have_latest = true;
}

static const char *watch_path(const struct thermal_state *state, int wd)
{
    size_t i;

    for (i = 0; i < state->watch_count; i++) {
        if (state->watches[i].wd == wd) {
            return state->watches[i].path;
        }
    }
    return NULL;
}

static bool path_is_watched(const struct thermal_state *state, const char *path)
{
    size_t i;

    for (i = 0; i < state->watch_count; i++) {
        if (strcmp(state->watches[i].path, path) == 0) {
            return true;
        }
    }
    return false;
}

static void remove_watch_record(struct thermal_state *state, int wd)
{
    size_t i;

    for (i = 0; i < state->watch_count; i++) {
        if (state->watches[i].wd == wd) {
            free(state->watches[i].path);
            state->watches[i] = state->watches[state->watch_count - 1];
            state->watch_count--;
            return;
        }
    }
}

static int add_watch(struct thermal_state *state, const char *path)
{
    static const uint32_t mask = IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE |
                                 IN_DELETE | IN_MOVED_FROM | IN_DELETE_SELF |
                                 IN_MOVE_SELF;
    struct watch *new_watches;
    int wd;

    if (state->inotify_fd < 0 || path_is_watched(state, path)) {
        return 0;
    }
    wd = inotify_add_watch(state->inotify_fd, path, mask);
    if (wd < 0) {
        return -1;
    }
    if (state->watch_count == state->watch_capacity) {
        size_t capacity = state->watch_capacity == 0 ? 8 : state->watch_capacity * 2;
        new_watches = realloc(state->watches, capacity * sizeof(*new_watches));
        if (new_watches == NULL) {
            inotify_rm_watch(state->inotify_fd, wd);
            return -1;
        }
        state->watches = new_watches;
        state->watch_capacity = capacity;
    }
    state->watches[state->watch_count].wd = wd;
    state->watches[state->watch_count].path = strdup(path);
    if (state->watches[state->watch_count].path == NULL) {
        inotify_rm_watch(state->inotify_fd, wd);
        return -1;
    }
    state->watch_count++;
    return 0;
}

/* Install each watch before scanning, closing the create-before-watch race. */
static void scan_tree(struct thermal_state *state, const char *path,
                      bool add_watches)
{
    DIR *dir;
    struct dirent *entry;

    if (add_watches) {
        add_watch(state, path);
    }
    dir = opendir(path);
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        char child[PATH_MAX];
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >=
            (int)sizeof(child) ||
            lstat(child, &st) != 0) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            scan_tree(state, child, add_watches);
        } else if (S_ISREG(st.st_mode)) {
            consider_file(state, child);
        }
    }
    closedir(dir);
}

static void process_inotify(struct thermal_state *state)
{
    char buffer[16 * 1024]
        __attribute__((aligned(__alignof__(struct inotify_event))));
    bool rescan = false;
    ssize_t length;

    while ((length = read(state->inotify_fd, buffer, sizeof(buffer))) > 0) {
        char *position = buffer;

        while (position < buffer + length) {
            const struct inotify_event *event =
                (const struct inotify_event *)position;
            const char *directory = watch_path(state, event->wd);
            char path[PATH_MAX];
            bool have_path = false;

            if (event->mask & IN_Q_OVERFLOW) {
                rescan = true;
            }
            if (directory != NULL && event->len != 0 &&
                snprintf(path, sizeof(path), "%s/%s", directory, event->name) <
                    (int)sizeof(path)) {
                have_path = true;
            }

            if (have_path && (event->mask & IN_ISDIR) &&
                (event->mask & (IN_CREATE | IN_MOVED_TO))) {
                /* Scan once in case files appeared before this watch was added. */
                scan_tree(state, path, true);
            } else if (have_path && !(event->mask & IN_ISDIR) &&
                       (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO))) {
                consider_file(state, path);
            }

            if (have_path && state->have_latest &&
                (event->mask & (IN_DELETE | IN_MOVED_FROM)) &&
                strcmp(path, state->latest) == 0) {
                state->have_latest = false;
                rescan = true;
            }
            if (event->mask & IN_IGNORED) {
                remove_watch_record(state, event->wd);
            }
            position += sizeof(*event) + event->len;
        }
    }
    if (length < 0 && errno != EAGAIN && errno != EINTR) {
        perror("inotify read");
    }
    if (rescan) {
        scan_tree(state, state->root, true);
    }
}

static int open_socket_in(unsigned port)
{
    struct sockaddr_in address;
    int one = 1;
    int fd;

    memset(&address, 0, sizeof(address));
    address.sin_port = htons(port);
    address.sin_family = AF_INET;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static bool write_all(int fd, const uint8_t *buffer, size_t length)
{
    while (length > 0) {
        ssize_t written = write(fd, buffer, length);

        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
        buffer += written;
        length -= (size_t)written;
    }
    return true;
}

static bool rate_limited_send(int fd, const uint8_t *buffer, uint32_t size)
{
    const uint32_t chunk_size = 1024;
    const useconds_t delay =
        (useconds_t)(1.0e6 * TARGET_SEND_TIME_S * chunk_size / size) + 1;

    while (size > 0) {
        uint32_t chunk = size > chunk_size ? chunk_size : size;

        if (!write_all(fd, buffer, chunk)) {
            return false;
        }
        buffer += chunk;
        size -= chunk;
        usleep(delay);
    }
    return true;
}

static void log_sent(const char *filename, uint32_t compressed_size)
{
    struct timespec now;
    struct tm tm;
    char timestamp[32] = "unknown-time";

    if (clock_gettime(CLOCK_REALTIME, &now) == 0 &&
        gmtime_r(&now.tv_sec, &tm) != NULL) {
        size_t length = strftime(timestamp, sizeof(timestamp),
                                 "%Y-%m-%dT%H:%M:%S", &tm);

        if (length > 0) {
            snprintf(timestamp + length, sizeof(timestamp) - length,
                     ".%03ldZ", now.tv_nsec / 1000000L);
        }
    }
    printf("%s Sent %s compressed_size=%u\n", timestamp, filename,
           (unsigned)compressed_size);
}


static void serve_connection(int fd, const char *filename)
{
    struct header header;
    struct stat st;
    uint8_t *input = NULL;
    uint8_t *compressed = NULL;
    uLongf compressed_size = compressBound(EXPECTED_SIZE);
    size_t offset = 0;
    int file_fd = -1;

    file_fd = open(filename, O_RDONLY);
    if (file_fd < 0 || fstat(file_fd, &st) != 0 ||
        st.st_size != EXPECTED_SIZE) {
        goto cleanup;
    }
    input = malloc(EXPECTED_SIZE);
    compressed = malloc(compressed_size);
    if (input == NULL || compressed == NULL) {
        goto cleanup;
    }
    while (offset < EXPECTED_SIZE) {
        ssize_t count = read(file_fd, input + offset, EXPECTED_SIZE - offset);

        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            goto cleanup;
        }
        offset += (size_t)count;
    }
    if (compress2(compressed, &compressed_size, input, EXPECTED_SIZE, 1) != Z_OK ||
        compressed_size > UINT32_MAX) {
        goto cleanup;
    }

    memset(&header, 0, sizeof(header));
    memcpy(header.fname, filename, strnlen(filename, sizeof(header.fname) - 1));
    header.compressed_size = (uint32_t)compressed_size;
    header.timestamp = ts_to_double(&st.st_mtim);
    if (!write_all(fd, (const uint8_t *)&header, sizeof(header))) {
        goto cleanup;
    }
    if (!rate_limited_send(fd, compressed, header.compressed_size)) {
        goto cleanup;
    }
    log_sent(header.fname, header.compressed_size);

cleanup:
    if (file_fd >= 0) {
        close(file_fd);
    }
    free(input);
    free(compressed);
}

static void listener(const char *root, unsigned port)
{
    struct thermal_state state;
    int listen_fd;

    memset(&state, 0, sizeof(state));
    state.root = root;
    state.inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (state.inotify_fd < 0) {
        fprintf(stderr,
                "inotify unavailable (%s); using connection-time scans\n",
                strerror(errno));
    } else {
        scan_tree(&state, root, true);
        printf("Watching %s with inotify (%zu directories)\n",
               root, state.watch_count);
    }

    listen_fd = open_socket_in(port);
    if (listen_fd < 0 || listen(listen_fd, 20) < 0) {
        perror("listen");
        exit(1);
    }
    printf("Waiting for connections on port %u\n", port);

    for (;;) {
        struct pollfd fds[2];
        nfds_t count = 1;
        int result;

        memset(fds, 0, sizeof(fds));
        fds[0].fd = listen_fd;
        fds[0].events = POLLIN;
        if (state.inotify_fd >= 0) {
            fds[1].fd = state.inotify_fd;
            fds[1].events = POLLIN;
            count = 2;
        }
        result = poll(fds, count, 1000);
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result < 0) {
            perror("poll");
            exit(1);
        }

        if (state.inotify_fd >= 0 && (fds[1].revents & POLLIN)) {
            process_inotify(&state);
        }
        /* Handle startup before the SD mount, or a later SD remount. */
        if (state.inotify_fd >= 0 && !path_is_watched(&state, root)) {
            scan_tree(&state, root, true);
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr address;
            socklen_t address_length = sizeof(address);
            int fd = accept(listen_fd, &address, &address_length);

            if (fd < 0) {
                continue;
            }
            if (state.inotify_fd < 0) {
                scan_tree(&state, root, false);
            }
            if (state.have_latest) {
                fflush(NULL);
                pid_t child = fork();

                if (child == 0) {
                    close(listen_fd);
                    if (state.inotify_fd >= 0) {
                        close(state.inotify_fd);
                    }
                    serve_connection(fd, state.latest);
                    close(fd);
                    fflush(stdout);
                    _exit(0);
                } else if (child < 0) {
                    perror("fork");
                }
            }
            close(fd);
        }
    }
}

int main(int argc, char **argv)
{
    const char *root = THERMAL_DIR;
    unsigned port = LISTEN_PORT;

    if (argc > 3) {
        fprintf(stderr, "Usage: %s [capture-directory [port]]\n", argv[0]);
        return 1;
    }
    if (argc >= 2) {
        root = argv[1];
    }
    if (argc == 3) {
        char *end = NULL;
        unsigned long value = strtoul(argv[2], &end, 10);

        if (argv[2][0] == '\0' || *end != '\0' ||
            value == 0 || value > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[2]);
            return 1;
        }
        port = (unsigned)value;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    listener(root, port);
    return 0;
}
