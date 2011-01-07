#!/usr/bin/env python

###################################
# python Digital Multimeter App
# (C) Andrew Tridgell 2011
# Released under GNU GPLv3 or later

import usb

class DMMException(Exception):
    '''an exception class for DMM errors'''
    def __init__(self, message):
        Exception.__init__(self, message)

class DMMDeviceError(DMMException):
    def __init__(self, message):
        DMMException.__init__(self, message)

class DMMTimeout(DMMDeviceError):
    def __init__(self, message):
        DMMDeviceError.__init__(self, "timeout")

class DMMDataError(DMMException):
    def __init__(self, message):
        DMMException.__init__(self, message)

class DMMData(object):
    '''a piece of data from a DMM'''
    def __init__(self):
        self.rawData = None
        self.model = "Unknown"
        self.fields = {}
        self.lcd = None
        self.units = None
        self.mode = None
        pass

class DMM(object):
    '''generic USB multimeter class'''
    def __init__(self, idVendor, idProduct, idEndpoint,
                 idInterface=0, model="Unknown", readSize=32):
        for bus in usb.busses():
            for device in bus.devices:
                if device.idVendor == idVendor and device.idProduct == idProduct:
                    self.device = device
                    self.handle = device.open()
                    try:
                        self.handle.detachKernelDriver(idInterface)
                    except usb.USBError:
                        pass
                    self.handle.claimInterface(idInterface)
                    self.model = model
                    self.readSize = readSize
                    self.idEndpoint = idEndpoint
                    self.debug = False
                    return
        raise DMMDeviceError("Unable to find USB device 0x%04x:0x%04x" % (
            idVendor, idProduct, idInterface))

    def read(self):
        '''read a set of data'''
        ret = DMMData()
        try:
            data = self.handle.interruptRead(self.idEndpoint, self.readSize)
            ret.rawData = data;
            if self.debug:
                for d in ret.rawData:
                    print("%02X " % d),
                print
                for i in range(0,len(ret.rawData)):
                    print("%2d " % i),
                print
        except usb.USBError, e:
            if str(e).find("No data available"):
                raise DMMTimeout(e)
            raise DMMDeviceError(e)
        return ret


class DMM_Victor86B(DMM):
    '''DMM interface for a Victor 86B'''
    def __init__(self):
        DMM.__init__(self, idVendor=0x1244, idProduct=0xd237,
                     idEndpoint=0x81, idInterface=0,
                     model="Victor86B")
    def read(self):
        tries = 4
        while tries > 0:
            tries -= 1
            ret = DMM.read(self)
            if len(ret.rawData) != 14:
                continue
            if ret.rawData[11:14] == ( 0, 0, 0):
                continue
            break
        if tries == 0:
            raise DMMDataError("Bad DMM data: %s" % ret.rawData)
        digmap = { 0x42: '0', 0x61: '1', 0x04: '2', 0xE6: '3', 0xA5: '4', 0x2E: '5',
                   0x4E: '6', 0xE1: '7', 0x46: '8', 0x26: '9', 0x67: ' ', 0xC8: 'L' }
        units = { (0x8F, 0x6E, 0xAc) : "Hz",
                  (0xBF, 0x6E, 0x6C) : "C",
                  (0x8F, 0x6E, 0x8C) : "V",
                  (0x8F, 0x6E, 0x6C) : "Ohm",
                  (0x8F, 0xAE, 0x6C) : "kOhm",
                  (0x8F, 0x7E, 0x7C) : "uA",
                  (0x8F, 0x6E, 0x7C) : "A",
                  }
        v = [(ret.rawData[10] & 0xF0) | (ret.rawData[3]>>4),
             (ret.rawData[9]  & 0xF0) | (ret.rawData[6]>>4),
             (ret.rawData[7]  & 0xF0) | (ret.rawData[5]>>4),
             (ret.rawData[0]  & 0xF0) | (ret.rawData[2]>>4)]
        offsets = [ 0, 0, 0xEF, 0x01 ]
        ret.lcd = ""
        for i in range(0,4):
            val = v[i]
            val += offsets[i]
            val &= 0xFF
            if val & 0x10:
                if i == 0:
                    ret.lcd += "-"
                else:
                    ret.lcd += "."
            val &= 0xEF
            if not val in digmap:
                raise DMMDataError("Digit %d invalid value 0x%02X" % (i+1, val))
            else:
                ret.lcd += digmap[val]
        ret.mode = ""
        u = (ret.rawData[11], ret.rawData[12], ret.rawData[13])
        if u in units:
            ret.units = units[u]
        if ret.rawData[1] == 0x47 and ret.rawData[4] == 0x71:
            ret.mode = "AC"
        if ret.rawData[1] == 0x57 and ret.rawData[4] == 0x71:
            ret.mode = "DC"
        return ret


############################################
# main program
if __name__ == "__main__":
    import sys

    dmm = DMM_Victor86B()
    dmm.debug = True
    while True:
        try:
            v = dmm.read()
            print("%s %s" % (v.lcd, v.units))
        except DMMTimeout:
            pass
        except DMMDataError, msg:
            print("DMM Data Error: %s" % msg)
        except KeyboardInterrupt:
            sys.exit(1)

