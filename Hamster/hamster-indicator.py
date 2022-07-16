#!/usr/bin/env python3
'''
simple gtk indicator to make working with hamster more efficient
'''

import os
import signal
import threading
import datetime

import gi
gi.require_version('Gtk','3.0')
gi.require_version('AppIndicator3', '0.1')
gi.require_version('Notify', '0.7')
from gi.repository import Gtk as gtk
from gi.repository import AppIndicator3 as appindicator
from gi.repository import Notify as notify

from hamster import client

storage = client.Storage()
active = False
active_str = "inactive"
last_dt_str = ""

APPINDICATOR_ID = 'Hamster'

def update(indicator):
    global active, last_dt_str
    threading.Timer(0.5, update, args=[indicator]).start()
    facts = storage.get_todays_facts()
    if facts and not facts[-1].end_time:
        now = datetime.datetime.now()
        dt = now - facts[-1].start_time
        dt_hr = dt.seconds // 3600
        dt_min = (dt.seconds - dt_hr*3600) // 60
        dt_str = "%u:%02u" % (dt_hr, dt_min)
        if not active or dt_str != last_dt_str:
            active = True
            last_dt_str = dt_str
            indicator.set_menu(build_menu(facts[-1].activity + " " + dt_str))
            indicator.set_icon_full(os.path.abspath('hamster-applet-active.png'), "active")
    else:
        if active:
            active = False
            indicator.set_menu(build_menu("inactive"))
            indicator.set_icon_full(os.path.abspath('hamster-applet-inactive.png'), "inactive")

def main():
    indicator = appindicator.Indicator.new(APPINDICATOR_ID, os.path.abspath('hamster-applet-inactive.png'), appindicator.IndicatorCategory.SYSTEM_SERVICES)
    indicator.set_status(appindicator.IndicatorStatus.ACTIVE)
    indicator.set_menu(build_menu("inactive"))
    notify.init(APPINDICATOR_ID)
    threading.Timer(0.5, update, args=[indicator]).start()
    gtk.main()

def format_fact(f):
    ret = f.activity
    if f.category:
        ret += "@%s" % f.category
    if f.tags:
        ret += " [%s]" % ' '.join(f.tags)
    return ret

def recent_facts(days=365):
    now = datetime.datetime.now()
    facts = storage.get_facts(now - datetime.timedelta(days=days), now)
    facts.sort(key=lambda x: x.start_time, reverse=True)
    return facts

def previous_items(start_menu):

    select_list = []

    facts = recent_facts()
    for f in facts:
        fmt = format_fact(f)
        if not fmt in select_list:
            select_list.append(fmt)
            item = gtk.MenuItem(label=fmt)
            item.connect('activate', start_item)
            start_menu.append(item)

def build_menu(activity):
    menu = gtk.Menu()

    item_activity = gtk.MenuItem(label=activity)
    menu.append(item_activity)

    item_stop = gtk.MenuItem(label='Stop')
    item_stop.connect('activate', stop)
    menu.append(item_stop)
    
    item_start = gtk.MenuItem(label="Start")
    menu.append(item_start)
    start_menu = gtk.Menu()
    item_start.set_submenu(start_menu)

    previous_items(start_menu)

    menu.show_all()
    return menu

def start_item(item):
    fmt = item.get_label()
    facts = recent_facts()
    for f in facts:
        if fmt == format_fact(f):
            f.start_time = datetime.datetime.now()
            f.end_time = None
            storage.add_fact(f)
            break

def stop(_):
    storage.stop_tracking()
    
if __name__ == "__main__":
    print("hamster-indicator starting")
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    main()
