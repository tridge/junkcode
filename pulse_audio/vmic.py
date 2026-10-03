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
                    sink="alsa_output.pci-0000_80_1f.3-platform-sof_sdw.HiFi__HDMI1__sink"),
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


def find_device(kind, spec):
    matches = [d["name"] for d in plist(kind) if is_real(d) and spec_matches(spec, kind, d)]
    if not matches:
        return None
    # prefer the stable bluetooth proxy name, which has no profile suffix
    matches.sort(key=len)
    if len(matches) > 1:
        print("NOTE: multiple %s match, using %s" % (kind, matches[0]))
    return matches[0]


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
    if duplex(profiles.get(active, {})):
        return find_device("sources", spec), find_device("sinks", spec), restore

    candidates = sorted([p for p, v in profiles.items() if duplex(v) and v.get("available", True)],
                        key=lambda p: profiles[p].get("priority", 0), reverse=True)
    if not candidates:
        raise AudioError("no available headset profile on %s" % card["name"])

    def restore():
        print("Restoring %s profile %s" % (card["name"], active))
        pactl("set-card-profile", card["name"], active, check=False)

    for profile in candidates:
        print("Setting %s profile %s" % (card["name"], profile))
        if pactl("set-card-profile", card["name"], profile, check=False).returncode != 0:
            continue
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            source, sink = find_device("sources", spec), find_device("sinks", spec)
            if source and sink:
                return source, sink, restore
            time.sleep(0.2)
        print("WARNING: profile %s gave no mic/speaker" % profile)
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


def recreate_virtual(vname, master):
    '''slow path, used when a virtual device or its backend is missing'''
    saved = monitor_recordings() if vname == VSINK else {}
    for idx in our_modules(vname):
        pactl("unload-module", idx)
    load_virtual(vname, master)
    # recordings of the old VSpeaker.monitor get unlinked, put them back
    for idx, mon in saved.items():
        if mon == VSINK + ".monitor":
            pactl("move-source-output", idx, mon, check=False)


def retarget(vname, master, timeout=3):
    '''point a virtual device at a real device, verifying the move took effect'''
    lkind, _, _, cmd, field = VIRT[vname]
    if backend(vname) is None or device_index(lkind, vname) is None:
        print("%s missing, (re)loading it" % vname)
        recreate_virtual(vname, master)
    deadline = time.monotonic() + timeout
    last_move = 0
    while time.monotonic() < deadline:
        b = backend(vname)
        want = device_index(lkind, master)
        if b is None or want is None:
            raise AudioError("%s or %s disappeared" % (vname, master))
        if b[field] == want:
            return
        # moves can report success but not happen, so re-issue periodically
        if time.monotonic() - last_move > 0.5:
            pactl(cmd, b["index"], master, check=False)
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


def wrong_streams():
    '''app streams not on the virtual devices, as (vname, stream)'''
    sinks = {d["name"]: d["index"] for d in plist("sinks")}
    sources = {d["index"]: d["name"] for d in plist("sources")}
    ret = [(VSINK, s) for s in app_streams("sink-inputs") if s["sink"] != sinks.get(VSINK)]
    for s in app_streams("source-outputs"):
        name = sources.get(s["source"], "")
        # leave apps recording a speaker monitor alone
        if name != VSOURCE and not name.endswith(".monitor"):
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
            break
        if now - start > timeout:
            break
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
    prev = {v: backend_target(v) for v in VIRT}
    try:
        retarget(VSOURCE, source)
        retarget(VSINK, sink)
    except AudioError as e:
        print("ERROR: %s, rolling back" % e)
        for vname, master in prev.items():
            if master and not master.startswith("("):
                try:
                    retarget(vname, master)
                except AudioError as e2:
                    print("ERROR: rollback of %s failed: %s" % (vname, e2))
        restore_profile()
        return 1

    # new streams, and streams that follow the default, go to the virtual devices
    pactl("set-default-sink", VSINK)
    pactl("set-default-source", VSOURCE)

    move_streams(timeout, verbose)

    pactl("set-source-volume", source, "100%")
    pactl("set-source-volume", VSOURCE, "100%")

    show_status()
    if [s for v, s in wrong_streams() if not pinned(s)] or backend_target(VSOURCE) != source or backend_target(VSINK) != sink:
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
    except (AudioError, subprocess.TimeoutExpired, json.JSONDecodeError) as e:
        print("ERROR: %s" % e)
        sys.exit(1)
