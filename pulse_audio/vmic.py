#!/usr/bin/env python3
'''
switch virtual audio devices (VMic/VSpeaker) between real devices, linked to
function keys on keyboard

all apps and the defaults point at VMic/VSpeaker, and those are remapped onto
the selected real devices
'''

import argparse
import json
import subprocess
import sys
import time

VSINK = "VSpeaker"
VSOURCE = "VMic"

# name: (description, source prefix, sink prefix)
DEVICES = {
    "DESKTOP": ("DESKTOP",
                "alsa_input.usb-046d_HD_Pro_Webcam_C920",
                "alsa_output.pci-0000_80_1f.3-platform-sof_sdw.HiFi__HDMI1__sink"),
    "HEADSET": ("HEADSET",
                "alsa_input.usb-GN_Netcom_A_S_Jabra",
                "alsa_output.usb-GN_Netcom_A_S_Jabra"),
    "MS": ("MS HEADSET",
           "alsa_input.usb-Microsoft_Microsoft_USB_Link",
           "alsa_output.usb-Microsoft_Microsoft_USB_Link"),
    "MSBT": ("MS HEADSET on bluetooth",
             "bluez_input.A0:4A:5E:F7:86:2A",
             "bluez_output.A0:4A:5E:F7:86:2A"),
    "LAPTOP": ("LAPTOP",
               "alsa_input.pci-0000_80_1f.3-platform-sof_sdw.HiFi__Mic__source",
               "alsa_output.pci-0000_80_1f.3-platform-sof_sdw.HiFi__Speaker__sink"),
    "NOX": ("NOX39G",
            "bluez_source.00_02_5B_D5_F6_ED.handsfree_head_unit",
            "bluez_sink.00_02_5B_D5_F6_ED.handsfree_head_unit"),
    "M3": ("M3",
           "bluez_input.E9:E6:56:88:9A:51",
           "bluez_output.E9_E6_56_88_9A_51.1"),
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


def find_device(kind, prefix):
    '''find a real sink/source by name prefix, returns full name'''
    matches = [d["name"] for d in plist(kind)
               if norm(prefix) in norm(d["name"])
               and not d["name"].endswith(".monitor")
               and d["name"] not in (VSINK, VSOURCE)]
    if not matches:
        return None
    if len(matches) > 1:
        print("WARNING: multiple %s match %s, using %s" % (kind, prefix, matches[-1]))
    return matches[-1]


def bluez_mac(prefix):
    if not prefix.startswith("bluez_"):
        return None
    return norm(prefix.split('.')[1])


def try_bt_headset_profile(prefix):
    '''bluetooth mics only exist with a headset profile, try to enable one'''
    mac = bluez_mac(prefix)
    if mac is None:
        return False
    for card in plist("cards"):
        if mac not in norm(card["name"]):
            continue
        profiles = [p for p, v in card.get("profiles", {}).items()
                    if p.startswith("headset") and v.get("available", True)]
        if not profiles:
            print("WARNING: no headset profile on %s" % card["name"])
            return False
        # prefer mSBC/higher quality profiles, which sort later
        profile = sorted(profiles)[-1]
        print("Setting %s profile %s" % (card["name"], profile))
        pactl("set-card-profile", card["name"], profile)
        return True
    print("WARNING: no bluetooth card for %s, is it connected?" % prefix)
    return False


def wait_for(kind, prefix, timeout):
    t0 = time.time()
    while True:
        name = find_device(kind, prefix)
        if name is not None or time.time() - t0 > timeout:
            return name
        time.sleep(0.2)


def wait_for_exact(kind, name, timeout):
    t0 = time.time()
    while time.time() - t0 < timeout:
        devs = {d["name"]: d for d in plist(kind)}
        if name in devs:
            return devs[name]
        time.sleep(0.1)
    raise AudioError("%s did not appear after %.1fs" % (name, timeout))


def wait_gone(kind, name, timeout):
    t0 = time.time()
    while time.time() - t0 < timeout:
        if name not in [d["name"] for d in plist(kind)]:
            return
        time.sleep(0.1)
    raise AudioError("%s still present after unload" % name)


def unload_virtual():
    # json module list lacks the index, so use the short form
    for line in pactl("list", "short", "modules").stdout.splitlines():
        f = line.split('\t')
        if len(f) < 3:
            continue
        idx, name, arg = f[0], f[1], f[2]
        if ((name == "module-remap-sink" and ("sink_name=%s" % VSINK) in arg) or
                (name == "module-remap-source" and ("source_name=%s" % VSOURCE) in arg)):
            pactl("unload-module", idx)
    wait_gone("sinks", VSINK, 3)
    wait_gone("sources", VSOURCE, 3)


def load_virtual(source, sink):
    # use names not indexes for master so it survives device index changes
    pactl("load-module", "module-remap-source", "source_name=%s" % VSOURCE,
          "source_properties=device.description=%s" % VSOURCE, "master=%s" % source)
    pactl("load-module", "module-remap-sink", "sink_name=%s" % VSINK,
          "sink_properties=device.description=%s" % VSINK, "master=%s" % sink)
    vsink = wait_for_exact("sinks", VSINK, 5)
    vsource = wait_for_exact("sources", VSOURCE, 5)
    return vsink["index"], vsource["index"]


def stream_name(s):
    p = s.get("properties", {})
    return "%s (%s) #%s" % (p.get("application.name") or p.get("node.name", "?"),
                            p.get("media.name", "?"), s["index"])


def app_streams(kind):
    '''streams belonging to apps, skipping the loopback streams of modules'''
    return [s for s in plist(kind)
            if s.get("owner_module") is None
            and "pulse.module.id" not in s.get("properties", {})]


def monitor_indexes():
    return set(d["index"] for d in plist("sources") if d["name"].endswith(".monitor"))


UNLINKED = 0xFFFFFFFF


def move_streams(vsink_idx, vsource_idx, timeout, verbose):
    '''move all app streams, retrying as apps may recreate streams on device changes'''
    failed = {}
    moved = set()
    t0 = time.time()
    rnd = 0
    while True:
        wrong = []
        unsettled = 0
        for s in app_streams("sink-inputs"):
            if s["sink"] != vsink_idx:
                wrong.append(("playback", "move-sink-input", VSINK, s, s["sink"]))
        monitors = monitor_indexes()
        for s in app_streams("source-outputs"):
            # leave apps recording a speaker monitor alone
            if s["source"] != vsource_idx and s["source"] not in monitors:
                wrong.append(("capture", "move-source-output", VSOURCE, s, s["source"]))

        for kind, cmd, target, s, cur in wrong:
            key = (kind, s["index"])
            # streams are unlinked for a while after the remap reload, don't re-move those
            if cur == UNLINKED and key in moved:
                unsettled += 1
                continue
            r = pactl(cmd, s["index"], target, check=False)
            moved.add(key)
            if r.returncode != 0:
                failed[key] = "%s: %s" % (stream_name(s), r.stderr.strip())
            elif verbose:
                print("  [%u] moved %s %s" % (rnd, kind, stream_name(s)))

        if not wrong or time.time() - t0 > timeout:
            break
        time.sleep(0.2)
        rnd += 1

    stragglers = ["%s %s" % (kind, stream_name(s)) for kind, cmd, target, s, cur in wrong]
    return stragglers, failed


def main():
    parser = argparse.ArgumentParser(description="switch VMic/VSpeaker between audio devices")
    parser.add_argument("device", help="|".join(DEVICES.keys()))
    parser.add_argument("--timeout", type=float, default=4, help="seconds to keep retrying stream moves")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    dev = args.device.upper()
    if dev not in DEVICES:
        print("Bad audio type %s, use one of %s" % (dev, "|".join(DEVICES.keys())))
        return 1
    desc, source_prefix, sink_prefix = DEVICES[dev]
    print("Using %s" % desc)

    source = find_device("sources", source_prefix)
    profile_changed = False
    if source is None and try_bt_headset_profile(source_prefix):
        profile_changed = True
        source = wait_for("sources", source_prefix, 5)
    sink = wait_for("sinks", sink_prefix, 3 if profile_changed else 0)
    if source is None or sink is None:
        if source is None:
            print("ERROR: no source matching %s" % source_prefix)
        if sink is None:
            print("ERROR: no sink matching %s" % sink_prefix)
        return 1
    print("SOURCE: %s  SINK: %s" % (source, sink))

    # can't change the master while playing, so reload the remap modules
    unload_virtual()
    vsink_idx, vsource_idx = load_virtual(source, sink)

    # new streams, and streams apps re-create, follow the defaults
    pactl("set-default-sink", VSINK)
    pactl("set-default-source", VSOURCE)

    stragglers, failed = move_streams(vsink_idx, vsource_idx, args.timeout, args.verbose)

    pactl("set-source-volume", source, "100%")
    pactl("set-source-volume", VSOURCE, "100%")

    if stragglers:
        print("WARNING: streams not switched:")
        for s in stragglers:
            print("  " + s)
        for v in failed.values():
            print("  error: " + v)
        return 2
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (AudioError, subprocess.TimeoutExpired, json.JSONDecodeError) as e:
        print("ERROR: %s" % e)
        sys.exit(1)
