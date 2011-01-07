#!/usr/bin/env python

###################################
# python Digital Multimeter App
# (C) Andrew Tridgell 2011
# Released under GNU GPLv3 or later

from dmmui import *
from dmmread import *
import sys

def tick():
    '''update display'''
    global device
    global tickcount
    try:
        v = device.read()
        ui.LCD.display(v.lcd)
        if v.units:
            ui.units.setText(v.units)
        else:
            ui.units.setText("-")
        if v.mode:
            ui.mode.setText(v.mode)
        else:
            ui.mode.setText("")
        if tickcount & 1:
            ui.tick.setText('*')
        else:
            ui.tick.setText(' ')
        tickcount += 1
    except DMMTimeout:
        pass
    except DMMDataError, msg:
        print("DMM Data Error: %s" % msg)
    except KeyboardInterrupt:
        sys.exit(1)


############################################
# main program
if __name__ == "__main__":
    global device, tickcount

    device = DMM_Victor86B()
    tickcount = 0

    app = QtGui.QApplication(sys.argv)
    dmm = QtGui.QMainWindow()
    ui = Ui_DMMUI()
    ui.setupUi(dmm)

    ctimer = QtCore.QTimer()
    QtCore.QObject.connect(ctimer, QtCore.SIGNAL("timeout()"), tick)
    ctimer.start(1)

    dmm.show()
    sys.exit(app.exec_())
