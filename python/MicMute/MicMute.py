#!/usr/bin/env python3
'''
app to allow for global mute/unmute of apps. Written as discord has no tray icon mute indicator
Author: Andrew Tridgell, June 2020
'''

import os
import signal
import threading
import datetime

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('AppIndicator3', '0.1')
gi.require_version('Notify', '0.7')

from gi.repository import Gtk as gtk
from gi.repository import AppIndicator3 as appindicator
from gi.repository import Notify as notify

APPINDICATOR_ID = 'MicMute'

app_dir = os.path.dirname(os.path.realpath(__file__))
print(app_dir)

# run at 5Hz
TIMER_PERIOD = 0.2

last_muted = False
has_quit = False

import pulsectl
pulse = pulsectl.Pulse('pulse_mute')

def get_src_list():
    '''get app sources'''
    ret = []
    for src in pulse.source_output_list():
        if src.name == 'Peak detect':
            continue
        ret.append(src)
    return ret

def all_muted():
    '''return true if all sources muted'''
    ret = True
    for src in get_src_list():
        if src.mute != 1:
            ret = False
    return ret

def update(indicator):
    global has_quit
    if has_quit:
        return
    threading.Timer(TIMER_PERIOD, update, args=[indicator]).start()
    muted = all_muted()
    global last_muted
    if muted == last_muted:
        return
    if muted:
        indicator.set_icon_full(os.path.join(app_dir, 'mic-muted.jpg'), 'muted')
    else:
        indicator.set_icon_full(os.path.join(app_dir, 'mic-active.jpg'), 'active')
    last_muted = muted

def main():
    indicator = appindicator.Indicator.new(APPINDICATOR_ID, os.path.join(app_dir, 'mic-active.jpg'), appindicator.IndicatorCategory.SYSTEM_SERVICES)
    indicator.set_status(appindicator.IndicatorStatus.ACTIVE)
    indicator.set_menu(build_menu())
    notify.init(APPINDICATOR_ID)
    threading.Timer(TIMER_PERIOD, update, args=[indicator]).start()
    gtk.main()

def build_menu():
    menu = gtk.Menu()

    item = gtk.MenuItem(label='MuteAll')
    item.connect('activate', mute_all)
    menu.append(item)

    item = gtk.MenuItem(label='UnMuteAll')
    item.connect('activate', unmute_all)
    menu.append(item)
    
    item = gtk.MenuItem(label='Quit')
    item.connect('activate', quit)
    menu.append(item)

    menu.show_all()
    return menu

def quit(_):
    global has_quit
    has_quit = True
    notify.uninit()
    gtk.main_quit()


def mute_all(_):
    for src in get_src_list():
        pulse.mute(src, True)

def unmute_all(_):
    for src in get_src_list():
        pulse.mute(src, False)

def show_state():
    for src in get_src_list():
        print(src.name, src.mute)

def toggle_mute():
    if all_muted():
        unmute_all("")
    else:
        mute_all("")

from argparse import ArgumentParser
parser = ArgumentParser(description="pulseaudio mute control")
parser.add_argument("--mute-all", action='store_true', help="mute all application inputs")
parser.add_argument("--unmute-all", action='store_true', help="unmute all application inputs")
parser.add_argument("--toggle", action='store_true', help="toggle mute")
parser.add_argument("--tray", action='store_true', help="install in tray")
args = parser.parse_args()

if args.mute_all:
    mute_all("")
elif args.unmute_all:
    unmute_all("")
elif args.toggle:
    toggle_mute()
elif args.tray:
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    main()
else:
    show_state()
