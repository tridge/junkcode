#!/usr/bin/env python3
'''
switch virtual audio devices (VMic/VSpeaker) between real devices, linked to
function keys on keyboard

all apps and the defaults point at VMic/VSpeaker, and those are remapped onto
the selected real devices. With no arguments shows the current mapping
'''

import argparse
import fcntl
import json
import math
import os
import re
import shlex
import struct
import subprocess
import sys
import time

VSINK = "VSpeaker"
VSOURCE = "VMic"
UNLINKED = 0xFFFFFFFF

# alsa devices are matched by name prefix, bluetooth by MAC. hid is a USB
# telephony device set off-hook while selected, to keep a wireless headset on
DEVICES = {
    "DESKTOP": dict(desc="DESKTOP",
                    source="alsa_input.usb-046d_HD_Pro_Webcam_C920",
                    # any of HDMI1-3: the monitor can come back on a different one after sleep
                    sink="alsa_output.pci-0000_80_1f.3-platform-sof_sdw.HiFi__HDMI"),
    "HEADSET": dict(desc="HEADSET",
                    source="alsa_input.usb-GN_Netcom_A_S_Jabra",
                    sink="alsa_output.usb-GN_Netcom_A_S_Jabra",
                    hid="0B0E:1016"),
    "MS": dict(desc="MS HEADSET",
               source="alsa_input.usb-Microsoft_Microsoft_USB_Link",
               sink="alsa_output.usb-Microsoft_Microsoft_USB_Link"),
    "MSBT": dict(desc="MS HEADSET on bluetooth", bt="A0:4A:5E:F7:86:2A"),
    "LAPTOP": dict(desc="LAPTOP",
                   source="alsa_input.pci-0000_80_1f.3-platform-sof_sdw.HiFi__Mic__source",
                   sink="alsa_output.pci-0000_80_1f.3-platform-sof_sdw.HiFi__Speaker__sink"),
    "NOX": dict(desc="NOX39G", bt="00:02:5B:D5:F6:ED"),
    "M3": dict(desc="M3", bt="E9:E6:56:88:9A:51"),
}

# per virtual device: device list kind, stream list kind, backend node name, move command, stream field
VIRT = {
    VSINK: ("sinks", "sink-inputs", "output." + VSINK, "move-sink-input", "sink"),
    VSOURCE: ("sources", "source-outputs", "input." + VSOURCE, "move-source-output", "source"),
}


class AudioError(Exception):
    pass


ERRORS = (AudioError, subprocess.TimeoutExpired, json.JSONDecodeError, OSError)


# pactl lists, valid until something changes or we wait for something to change
cache = {}


def pactl(*args, check=True):
    if args[0] not in ("-f", "list"):
        cache.clear()
    r = subprocess.run(["pactl"] + [str(a) for a in args],
                       capture_output=True, text=True, timeout=10)
    if check and r.returncode != 0:
        raise AudioError("pactl %s failed: %s" % (" ".join(map(str, args)), r.stderr.strip()))
    return r


def plist(kind):
    if kind not in cache:
        cache[kind] = json.loads(pactl("-f", "json", "list", kind).stdout or "[]")
    return cache[kind]


def defaults():
    '''(default sink, default source)'''
    if "info" not in cache:
        cache["info"] = json.loads(pactl("-f", "json", "info").stdout)
    return cache["info"].get("default_sink_name"), cache["info"].get("default_source_name")


def sleep(t):
    cache.clear()
    time.sleep(t)


def norm(name):
    # bluez names vary between ':' and '_' in the MAC
    return name.replace(':', '_').lower()


def is_real(d):
    return not d["name"].endswith(".monitor") and d["name"] not in VIRT


def spec_matches(spec, kind, d):
    '''does real device d match an alias spec'''
    if "bt" in spec:
        mac = norm(spec["bt"])
        props = d.get("properties", {})
        return (d["name"].startswith("bluez_") and mac in norm(d["name"])) or \
            norm(props.get("api.bluez5.address", "")) == mac
    prefix = spec["source" if kind == "sources" else "sink"]
    return norm(prefix) in norm(d["name"])


def plugged(d):
    '''false only if every port reports its jack unplugged'''
    ports = d.get("ports", [])
    return not ports or any(p.get("availability") != "not available" for p in ports)


def find_device(kind, spec):
    devs = [d for d in plist(kind) if is_real(d) and spec_matches(spec, kind, d)]
    if not devs:
        return None
    # prefer plugged in ports, then the stable bluetooth proxy name, which has no profile suffix
    devs.sort(key=lambda d: (not plugged(d), len(d["name"])))
    if not plugged(devs[0]):
        print("WARNING: no %s matching %s is plugged in, using %s" % (kind, spec["desc"], devs[0]["name"]))
    elif len(devs) > 1 and plugged(devs[1]):
        print("NOTE: multiple %s match, using %s" % (kind, devs[0]["name"]))
    return devs[0]["name"]


def aliases_for(kind, name):
    devs = [d for d in plist(kind) if d["name"] == name]
    if not devs:
        return []
    return [a for a, spec in DEVICES.items() if spec_matches(spec, kind, devs[0])]


def bt_card(mac):
    for card in plist("cards"):
        if norm(mac) in norm(card["name"]):
            return card
    return None


def duplex(profile):
    return profile.get("sinks", 0) > 0 and profile.get("sources", 0) > 0


def set_profile(mac, card_name, profile, timeout=5):
    '''set a card profile and wait until it is active'''
    if pactl("set-card-profile", card_name, profile, check=False).returncode != 0:
        return False
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        card = bt_card(mac)
        if card is not None and card.get("active_profile") == profile:
            return True
        sleep(0.2)
    return False


def resolve(spec):
    '''find real source and sink for an alias, switching bluetooth to a mic profile if needed.
    Returns (source, sink, restore) where restore undoes any profile change'''
    restore = lambda: None
    if "bt" not in spec:
        return find_device("sources", spec), find_device("sinks", spec), restore

    card = bt_card(spec["bt"])
    if card is None:
        raise AudioError("no bluetooth card for %s, is it connected?" % spec["bt"])
    profiles = card.get("profiles", {})
    active = card.get("active_profile")
    # don't rely on wireplumber autoswitch: it ignores VMic's loopback stream
    if duplex(profiles.get(active, {})):
        return find_device("sources", spec), find_device("sinks", spec), restore

    candidates = sorted([p for p, v in profiles.items() if duplex(v) and v.get("available", True)],
                        key=lambda p: profiles[p].get("priority", 0), reverse=True)
    if not candidates:
        raise AudioError("no available headset profile on %s" % card["name"])

    def restore():
        print("Restoring %s profile %s" % (card["name"], active))
        if not set_profile(spec["bt"], card["name"], active):
            print("WARNING: failed to restore %s profile %s" % (card["name"], active))

    try:
        for profile in candidates:
            print("Setting %s profile %s" % (card["name"], profile))
            # the device proxies exist even without a mic profile, so check the card
            if not set_profile(spec["bt"], card["name"], profile):
                print("WARNING: profile %s did not activate" % profile)
                continue
            source, sink = find_device("sources", spec), find_device("sinks", spec)
            if source and sink:
                return source, sink, restore
            print("WARNING: profile %s gave no mic/speaker" % profile)
    except Exception:
        restore()
        raise
    restore()
    raise AudioError("no working headset profile on %s" % card["name"])


def device_index(kind, name):
    for d in plist(kind):
        if d["name"] == name:
            return d["index"]
    return None


def backend(vname):
    '''the remap module's stream that feeds the real device'''
    _, skind, node, _, _ = VIRT[vname]
    for s in plist(skind):
        if s.get("properties", {}).get("node.name") == node:
            return s
    return None


def backend_target(vname):
    '''name of the real device a virtual device is currently connected to'''
    lkind, _, _, _, field = VIRT[vname]
    b = backend(vname)
    if b is None:
        return None
    if b[field] == UNLINKED:
        return "(unlinked)"
    names = {d["index"]: d["name"] for d in plist(lkind)}
    return names.get(b[field], "(unknown #%s)" % b[field])


def module_args(arg):
    try:
        return dict(a.split('=', 1) for a in shlex.split(arg) if '=' in a)
    except ValueError:
        return {}


def our_modules(vname):
    key = "sink_name" if vname == VSINK else "source_name"
    mod = "module-remap-sink" if vname == VSINK else "module-remap-source"
    ret = []
    # json module list lacks the index, so use the short form
    for line in pactl("list", "short", "modules").stdout.splitlines():
        f = line.split('\t')
        if len(f) >= 3 and f[1] == mod and module_args(f[2]).get(key) == vname:
            ret.append(f[0])
    return ret


def load_virtual(vname, master):
    if vname == VSINK:
        args = ["module-remap-sink", "sink_name=" + VSINK,
                "sink_properties=device.description=" + VSINK]
    else:
        args = ["module-remap-source", "source_name=" + VSOURCE,
                "source_properties=device.description=" + VSOURCE]
    idx = pactl("load-module", *args, "master=" + master).stdout.strip()
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if backend(vname) is not None and device_index(VIRT[vname][0], vname) is not None:
            return idx
        sleep(0.1)
    pactl("unload-module", idx, check=False)
    raise AudioError("%s did not appear after loading" % vname)


def monitor_recordings():
    '''source-outputs recording a monitor, as index -> monitor name'''
    names = {d["index"]: d["name"] for d in plist("sources")}
    return {s["index"]: names[s["source"]] for s in plist("source-outputs")
            if names.get(s["source"], "").endswith(".monitor")}


# recordings of monitors that were unlinked by recreating VSpeaker
saved_monitors = {}


def recreate_virtual(vname, master):
    '''slow path, used when a virtual device or its backend is missing'''
    if vname == VSINK:
        saved_monitors.update({i: m for i, m in monitor_recordings().items()
                               if m == VSINK + ".monitor"})
    for idx in our_modules(vname):
        pactl("unload-module", idx)
    load_virtual(vname, master)
    # put the recordings back on the new VSpeaker.monitor
    deadline = time.monotonic() + 3
    while saved_monitors and time.monotonic() < deadline:
        current = monitor_recordings()
        pending = [i for i, m in saved_monitors.items() if current.get(i) != m]
        if not pending:
            break
        for idx in pending:
            pactl("move-source-output", idx, saved_monitors[idx], check=False)
        sleep(0.3)


def set_target(stream, master, force=False):
    '''target a stream by device name, unlike pactl move which stores a serial that
    changes when a device reconnects'''
    cache.clear()
    node = stream.get("properties", {}).get("object.id")
    if node is None:
        raise AudioError("no node id for %s" % stream_name(stream))
    subprocess.run(["pw-metadata", "-d", node, "target.node"], capture_output=True, timeout=10)
    if force:
        # setting an unchanged value doesn't trigger a relink
        subprocess.run(["pw-metadata", "-d", node, "target.object"], capture_output=True, timeout=10)
    r = subprocess.run(["pw-metadata", node, "target.object", master, "Spa:String"],
                       capture_output=True, text=True, timeout=10)
    if r.returncode != 0:
        raise AudioError("pw-metadata failed: %s" % r.stderr.strip())


def node_metadata(node):
    '''default metadata of a node, as {key: value}'''
    r = subprocess.run(["pw-metadata", str(node)], capture_output=True, text=True, timeout=10)
    return dict(re.findall(r"key:'([^']*)' value:'([^']*)'", r.stdout))


def in_place(vname, master):
    '''virtual device already linked to master and targeting it by name'''
    lkind, _, _, _, field = VIRT[vname]
    b = backend(vname)
    if b is None or device_index(lkind, vname) is None or b[field] != device_index(lkind, master):
        return False
    meta = node_metadata(b.get("properties", {}).get("object.id"))
    return meta.get("target.object") == master and "target.node" not in meta


def retarget(vname, master, timeout=3):
    '''point a virtual device at a real device, verifying the move took effect.
    Returns False if it was already there'''
    lkind, _, _, _, field = VIRT[vname]
    if in_place(vname, master):
        return False
    if backend(vname) is None or device_index(lkind, vname) is None:
        print("%s missing, (re)loading it" % vname)
        recreate_virtual(vname, master)
    # always set the target, to replace any serial based target from pactl move
    set_target(backend(vname), master)
    deadline = time.monotonic() + timeout
    last_move = time.monotonic()
    while time.monotonic() < deadline:
        b = backend(vname)
        want = device_index(lkind, master)
        if b is None or want is None:
            raise AudioError("%s or %s disappeared" % (vname, master))
        if b[field] == want:
            return True
        # re-issue periodically until it takes effect
        if time.monotonic() - last_move > 0.5:
            set_target(b, master, force=True)
            last_move = time.monotonic()
        sleep(0.1)
    raise AudioError("%s did not move to %s" % (vname, master))


def stream_name(s):
    p = s.get("properties", {})
    return "%s (%s) #%s" % (p.get("application.name") or p.get("node.name", "?"),
                            p.get("media.name", "?"), s["index"])


def app_streams(skind):
    '''streams belonging to apps, skipping the loopback streams of modules'''
    return [s for s in plist(skind)
            if s.get("owner_module") is None
            and "pulse.module.id" not in s.get("properties", {})]


def monitor_recorder(s):
    '''a recording of a monitor that is currently unlinked'''
    p = s.get("properties", {})
    return s["index"] in saved_monitors or p.get("stream.capture.sink") == "true" or \
        str(p.get("target.object", "")).endswith(".monitor")


def wrong_streams():
    '''app streams not on the virtual devices, as (vname, stream)'''
    sinks = {d["name"]: d["index"] for d in plist("sinks")}
    sources = {d["index"]: d["name"] for d in plist("sources")}
    ret = [(VSINK, s) for s in app_streams("sink-inputs") if s["sink"] != sinks.get(VSINK)]
    for s in app_streams("source-outputs"):
        name = sources.get(s["source"], "")
        # leave apps recording a speaker monitor alone
        if name != VSOURCE and not name.endswith(".monitor") and not monitor_recorder(s):
            ret.append((VSOURCE, s))
    return ret


def peak_meter(s):
    # e.g. pavucontrol level meters
    return s.get("properties", {}).get("stream.monitor") == "true"


def pinned(s):
    '''streams that ask not to be moved, PA DONT_MOVE maps to node.dont-reconnect'''
    p = s.get("properties", {})
    return peak_meter(s) or p.get("node.dont-move") == "true" or p.get("node.dont-reconnect") == "true"


def move_streams(timeout, verbose, settle):
    '''move app streams until nothing has needed moving for settle seconds, as apps
    may recreate streams after a device change'''
    cache.clear()
    last_ok = {}
    reported = set()
    start = time.monotonic()
    quiet_since = start
    rnd = 0
    while True:
        now = time.monotonic()
        wrong = [(v, s) for v, s in wrong_streams() if not pinned(s)]
        if wrong:
            quiet_since = now
        elif now - quiet_since >= settle:
            return True
        if now - start > timeout:
            return False
        for vname, s in wrong:
            key = (vname, s["index"])
            # give a successful move time to take effect before re-issuing it
            if now - last_ok.get(key, -10) < 0.5:
                continue
            r = pactl(VIRT[vname][3], s["index"], vname, check=False)
            if r.returncode == 0:
                last_ok[key] = now
                # the app may recreate its stream after the move
                settle = max(settle, 0.3)
                if verbose:
                    print("  [%u] moved %s to %s" % (rnd, stream_name(s), vname))
            elif verbose and key not in reported:
                reported.add(key)
                print("  [%u] move %s failed: %s" % (rnd, stream_name(s), r.stderr.strip()))
        sleep(0.2)
        rnd += 1


def device_desc(lkind, name):
    aliases = aliases_for(lkind, name)
    return "%s (%s)" % (name, ",".join(aliases) if aliases else "no alias")


def stream_device(vname, s):
    '''description of the device a stream is currently on'''
    lkind, _, _, _, field = VIRT[vname]
    if s[field] == UNLINKED:
        return "(unlinked)"
    names = {d["index"]: d["name"] for d in plist(lkind)}
    if s[field] not in names:
        return "(unknown #%s)" % s[field]
    return device_desc(lkind, names[s[field]])


def show_status():
    for vname in (VSOURCE, VSINK):
        lkind = VIRT[vname][0]
        target = backend_target(vname)
        if target is None:
            print("%-9s not loaded" % (vname + ":"))
            continue
        print("%-9s %s" % (vname + ":", device_desc(lkind, target)))
    dsink, dsource = defaults()
    if dsink != VSINK or dsource != VSOURCE:
        print("WARNING: defaults are sink=%s source=%s" % (dsink, dsource))
    wrong = [(v, s) for v, s in wrong_streams() if not peak_meter(s)]
    if wrong:
        print("Streams not on virtual devices:")
        for vname, s in wrong:
            print("  %s%s" % (stream_name(s), " (pinned)" if pinned(s) else ""))
            print("    on %s" % stream_device(vname, s))


def lock():
    path = os.path.join(os.environ.get("XDG_RUNTIME_DIR", "/tmp"), "vmic.lock")
    f = open(path, "w")
    deadline = time.monotonic() + 15
    while True:
        try:
            fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
            return f
        except BlockingIOError:
            if time.monotonic() > deadline:
                raise AudioError("another vmic is running")
            time.sleep(0.1)


def ensure_volume(kind, name, minimum, force=False):
    '''unmute and raise volume to at least minimum percent, or exactly if force.
    Failure only warns, as the switch itself has worked'''
    try:
        devs = [d for d in plist(kind + "s") if d["name"] == name]
        if not devs:
            print("WARNING: no %s %s to set volume on" % (kind, name))
            return
        d = devs[0]
        if d["mute"]:
            print("Unmuting %s" % name)
            pactl("set-%s-mute" % kind, name, "0")
        vols = [c["value"] for c in d["volume"].values()]
        target = minimum * 65536 // 100
        if (force and any(v != target for v in vols)) or min(vols) < target:
            print("Setting %s volume to %u%%" % (name, minimum))
            pactl("set-%s-volume" % kind, name, "%u%%" % minimum)
    except ERRORS as e:
        print("WARNING: volume of %s: %s" % (name, e))


def beep(freq=2000, duration=0.12, rate=48000, level=0.15):
    '''short tone on VSpeaker to confirm the switch, without waiting for it to play'''
    n = int(rate * duration)
    ramp = rate // 200
    samples = (int(32767 * level * min(1, i / ramp, (n - i) / ramp) *
                   math.sin(2 * math.pi * freq * i / rate)) for i in range(n))
    data = struct.pack("<%uh" % n, *samples)
    try:
        p = subprocess.Popen(["pacat", "--raw", "--device", VSINK, "--format=s16le",
                              "--rate=%u" % rate, "--channels=1"], stdin=subprocess.PIPE)
        p.stdin.write(data)
        p.stdin.close()
    except ERRORS as e:
        print("WARNING: beep failed: %s" % e)


def set_hook(hid, off_hook):
    '''off-hook opens the radio link to the headset and holds it open, even in
    its cradle, until set back on-hook'''
    vid, pid = hid.upper().split(":")
    want = "HID_ID=0003:0000%s:0000%s" % (vid, pid)
    for h in sorted(os.listdir("/sys/class/hidraw")):
        try:
            with open("/sys/class/hidraw/%s/device/uevent" % h) as f:
                if want not in f.read().split():
                    continue
            # output report 2: bit 0 is off-hook
            fd = os.open("/dev/" + h, os.O_WRONLY | os.O_NONBLOCK)
            try:
                os.write(fd, bytes([2, int(off_hook), 0]))
            finally:
                os.close(fd)
            print("Set %s %s" % (h, "off-hook" if off_hook else "on-hook"))
            return
        except OSError as e:
            print("WARNING: hook on %s failed: %s" % (h, e))
            return
    # not plugged in only matters if we wanted it
    if off_hook:
        print("WARNING: no hidraw device for %s" % hid)


def switch(dev, timeout, verbose):
    spec = DEVICES[dev]
    print("Using %s" % spec["desc"])
    prev = {v: backend_target(v) for v in VIRT}
    source, sink, restore_profile = resolve(spec)
    if source is None or sink is None:
        restore_profile()
        if source is None:
            print("ERROR: no source for %s" % dev)
        if sink is None:
            print("ERROR: no sink for %s" % dev)
        return 1
    print("SOURCE: %s  SINK: %s" % (source, sink))
    if "hid" in spec:
        set_hook(spec["hid"], True)

    # retarget the existing virtual devices, so app streams stay connected
    try:
        changed = retarget(VSOURCE, source)
        changed = retarget(VSINK, sink) or changed
        # new streams, and streams that follow the default, go to the virtual devices
        if defaults() != (VSINK, VSOURCE):
            pactl("set-default-sink", VSINK)
            pactl("set-default-source", VSOURCE)
    except ERRORS as e:
        print("ERROR: %s, rolling back" % e)
        # profile first, as it may recreate the previous devices
        restore_profile()
        for vname, master in prev.items():
            if master and not master.startswith("("):
                try:
                    retarget(vname, master)
                except ERRORS as e2:
                    print("ERROR: rollback of %s failed: %s" % (vname, e2))
        return 1

    # release headsets we switched away from, so their cradle turns them off
    for hid in set(d["hid"] for d in DEVICES.values() if "hid" in d) - {spec.get("hid")}:
        set_hook(hid, False)

    # a hotkey switch should never leave us silent
    ensure_volume("source", source, 100, force=True)
    ensure_volume("sink", sink, 100, force=True)
    ensure_volume("source", VSOURCE, 100)
    ensure_volume("sink", VSINK, 50)
    beep()

    # apps may recreate streams after a device change, otherwise one pass is enough
    settled = move_streams(timeout, verbose, settle=0.3 if changed else 0)

    show_status()
    if not settled:
        print("WARNING: streams did not settle within %.1fs" % timeout)
        return 2
    if backend_target(VSOURCE) != source or backend_target(VSINK) != sink:
        print("WARNING: virtual devices not on the requested devices")
        return 2
    return 0


def main():
    parser = argparse.ArgumentParser(description="switch VMic/VSpeaker between audio devices, "
                                     "or show the current mapping with no device")
    parser.add_argument("device", nargs="?", help="|".join(DEVICES.keys()))
    parser.add_argument("--timeout", type=float, default=5, help="seconds to keep moving app streams")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    if args.device is None:
        show_status()
        return 0

    dev = args.device.upper()
    if dev not in DEVICES:
        print("Bad audio type %s, use one of %s" % (dev, "|".join(DEVICES.keys())))
        return 1

    with lock():
        return switch(dev, args.timeout, args.verbose)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except ERRORS as e:
        print("ERROR: %s" % e)
        sys.exit(1)
