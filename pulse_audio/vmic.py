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
import os
import shlex
import subprocess
import sys
import time

VSINK = "VSpeaker"
VSOURCE = "VMic"
UNLINKED = 0xFFFFFFFF

# alsa devices are matched by name prefix, bluetooth by MAC
DEVICES = {
    "DESKTOP": dict(desc="DESKTOP",
                    source="alsa_input.usb-046d_HD_Pro_Webcam_C920",
                    # any of HDMI1-3: the monitor can come back on a different one after sleep
                    sink="alsa_output.pci-0000_80_1f.3-platform-sof_sdw.HiFi__HDMI"),
    "HEADSET": dict(desc="HEADSET",
                    source="alsa_input.usb-GN_Netcom_A_S_Jabra",
                    sink="alsa_output.usb-GN_Netcom_A_S_Jabra"),
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


def pactl(*args, check=True):
    r = subprocess.run(["pactl"] + [str(a) for a in args],
                       capture_output=True, text=True, timeout=10)
    if check and r.returncode != 0:
        raise AudioError("pactl %s failed: %s" % (" ".join(map(str, args)), r.stderr.strip()))
    return r


def plist(kind):
    return json.loads(pactl("-f", "json", "list", kind).stdout or "[]")


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
        time.sleep(0.2)
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
        time.sleep(0.1)
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
        time.sleep(0.3)


def set_target(stream, master, force=False):
    '''target a stream by device name, unlike pactl move which stores a serial that
    changes when a device reconnects'''
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


def retarget(vname, master, timeout=3):
    '''point a virtual device at a real device, verifying the move took effect'''
    lkind, _, _, _, field = VIRT[vname]
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
            return
        # re-issue periodically until it takes effect
        if time.monotonic() - last_move > 0.5:
            set_target(b, master, force=True)
            last_move = time.monotonic()
        time.sleep(0.1)
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


def move_streams(timeout, verbose, settle=1.0):
    '''move app streams until nothing has needed moving for settle seconds, as apps
    may recreate streams after a device change'''
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
                if verbose:
                    print("  [%u] moved %s to %s" % (rnd, stream_name(s), vname))
            elif verbose and key not in reported:
                reported.add(key)
                print("  [%u] move %s failed: %s" % (rnd, stream_name(s), r.stderr.strip()))
        time.sleep(0.2)
        rnd += 1


def show_status():
    for vname in (VSOURCE, VSINK):
        lkind = VIRT[vname][0]
        target = backend_target(vname)
        if target is None:
            print("%-9s not loaded" % (vname + ":"))
            continue
        aliases = aliases_for(lkind, target)
        print("%-9s %s (%s)" % (vname + ":", target, ",".join(aliases) if aliases else "no alias"))
    dsink = pactl("get-default-sink").stdout.strip()
    dsource = pactl("get-default-source").stdout.strip()
    if dsink != VSINK or dsource != VSOURCE:
        print("WARNING: defaults are sink=%s source=%s" % (dsink, dsource))
    wrong = [(v, s) for v, s in wrong_streams() if not peak_meter(s)]
    if wrong:
        print("Streams not on virtual devices:")
        for vname, s in wrong:
            print("  %s%s" % (stream_name(s), " (pinned)" if pinned(s) else ""))


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

    # retarget the existing virtual devices, so app streams stay connected
    try:
        retarget(VSOURCE, source)
        retarget(VSINK, sink)
        # new streams, and streams that follow the default, go to the virtual devices
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

    settled = move_streams(timeout, verbose)

    pactl("set-source-volume", source, "100%")
    pactl("set-source-volume", VSOURCE, "100%")

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
