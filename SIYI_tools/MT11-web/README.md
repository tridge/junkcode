# MT11 administrative web service

`mt11-web` is a small static AArch64 HTTP server for the MT11 camera. It does
not require Python or additional target libraries.

Features:

- live process, memory, storage, network and uptime status;
- a camera-output tab that refreshes every three seconds;
- an authenticated filesystem browser with inline image/video viewing,
  range-capable file downloads and media-directory navigation;
- permanent file and recursive-directory deletion restricted to canonical
  targets strictly beneath `/mnt`, with a separate confirmation page;
- a typed parameter page with codec/resolution menus, decoded imaging-path
  masks, ISP controls, network checks, thermal controls and calibration guards;
- raw editing of `/app/config.ini` for unknown and future vendor settings;
- syntax validation, atomic saves and `/app/config.ini.web.bak`;
- restart of `/app/siyi_camera_app` without restarting the web service; and
- camera reboot with explicit confirmation.

All routes require HTTP Basic authentication. The username is `admin`; the
password is the trimmed contents of `/app/web.pass`, reread for every request.
The password file should be mode 0600. A per-process random CSRF token protects
all state-changing requests.

The service deliberately implements HTTP rather than TLS. It is intended only
for the MT11's isolated management network.

The Files tab starts at `/mnt`, but can browse and download regular files from
other absolute paths for diagnosis. It serves common image and video types with
their correct MIME types and supports HTTP byte ranges so browser video players
can seek. File transfers run in short-lived worker processes so a large video
download does not block status and control requests.

Deletion has a deliberately narrower boundary than reading: `/mnt` itself and
everything outside it are rejected. The server resolves the target again at
deletion time, does not follow symlinks while recursively walking directories,
and removes an in-tree symlink itself rather than its destination. Deletion is
permanent and has no trash/recovery layer.

The reverse-engineered parameter definitions, value tables and confidence notes
are maintained in [`../MT11_config.md`](../MT11_config.md). Typed saves update
only known keys and preserve the rest of the file, including comments and
unknown settings.

Browse to `http://192.168.144.25:8080/` and log in as `admin`. To inspect or
change the password from a trusted host:

```sh
ssh MT11 'cat /app/web.pass'
ssh MT11 'printf "%s\n" "new-password" >/app/web.pass && chmod 0600 /app/web.pass'
```

The password change takes effect on the next request. No service restart is
needed.

Camera stdout/stderr is piped through the same binary's
`--capture-app-log` mode. It writes only to `/run/siyi_camera_app.log`, rotates
at 512 KiB and retains one previous segment, so verbose output cannot consume
the application flash or grow without bound. The web page displays the newest
256 KiB.

## Build

```sh
make -C web
```

The normal build is statically linked for AArch64.

## Deploy an update

```sh
rsync -av -e 'ssh -F /home/tridge/.ssh/config' \
    web/mt11-web MT11:/app/bin/mt11-web.new
ssh -F /home/tridge/.ssh/config MT11 \
    'chmod 0755 /app/bin/mt11-web.new && mv /app/bin/mt11-web.new /app/bin/mt11-web'
```

Restart only the web service after replacing the executable. The persistent
startup entry in `/app/app_init.sh` is:

```sh
/app/siyi_camera_app 2>&1 | /app/bin/mt11-web --capture-app-log &
/app/bin/mt11-web -p 8080 >/run/mt11-web.log 2>&1 &
```

Runtime output goes to `/run/mt11-web.log` to avoid writing routine logs to
flash.
