#!/bin/bash

# script to switch virtual audio between desktop and headset, linked to
# function keys on keyboard

[ $# -eq 1 ] || {
    echo "vmic.sh DESKTOP|HEADSET|MSBT|LAPTOP"
    exit 1
}

# the devices I use
speakers="alsa_output.pci-0000_01_00.1.hdmi-stereo"
webcam="alsa_input.usb-046d_HD_Pro_Webcam_C920"
jabrainput="alsa_input.usb-GN_Netcom_A_S_Jabra"
jabraoutput="alsa_output.usb-GN_Netcom_A_S_Jabra"
msout="alsa_output.usb-Microsoft_Microsoft_USB_Link"
msin="alsa_input.usb-Microsoft_Microsoft_USB_Link"
lapin="alsa_output.pci-0000_01_00.1.hdmi-stereo.monitor"
msbtout="bluez_sink.A0_4A_5E_F7_86_2A"
msbtin="bluez_source.A0_4A_5E_F7_86_2A"
noxout="bluez_sink.00_02_5B_D5_F6_ED.handsfree_head_unit"
noxin="bluez_source.00_02_5B_D5_F6_ED.handsfree_head_unit"

DEVICE="${1^^}"

if [ $DEVICE = "HEADSET" ]; then
    echo "Using HEADSET"
    source="$jabrainput"
    sink="$jabraoutput"
elif [ $DEVICE = "MS" ]; then
    echo "Using MS HEADSET"
    source="$msin"
    sink="$msout"
elif [ $DEVICE = "MSBT" ]; then
    echo "Using MS HEADSET on bluetooth"
    source="$msbtin"
    sink="$msbtout"
elif [ $DEVICE = "DESKTOP" ]; then
    echo "Using DESKTOP"
    source="$webcam"
    sink="$speakers"
elif [ $DEVICE = "LAPTOP" ]; then
    echo "Using LAPTOP"
    source="$lapin"
    sink="$speakers"
elif [ $DEVICE = "NOX" ]; then
    echo "Using NOX39G"
    source="$noxin"
    sink="$noxout"
else
    echo "Bad audio type $DEVICE"
    exit 1
fi

echo "SOURCE: $source  SINK: $sink"

# normalise to get the long names of the devices
sink=$(pactl list short sinks | grep $sink | tail -1 | cut -f1)
source=$(pactl list short sources | grep $source | tail -1 | cut -f1)

# we have to reload as you can't change the master while playing
pactl unload-module module-remap-source
pactl unload-module module-remap-sink

sourceoutputs=$(pactl list short source-outputs | cut -f1)
sinkinputs=$(pactl list short sink-inputs | cut -f1)

pactl load-module module-remap-source source_name=VMic source_properties="device.description=VMic" master="$source"
pactl load-module module-remap-sink sink_name=VSpeaker sink_properties="device.description=VSpeaker" master="$sink"

# get the long name of the virtual devices
vsink=$(pactl list short sinks | grep VSpeaker | tail -1 | cut -f1)
vsource=$(pactl list short sources | grep VMic | tail -1 | cut -f1)

# change all apps
for si in $sinkinputs; do
    pactl move-sink-input $si $vsink 2> /dev/null
done

for so in $sourceoutputs; do
    pactl move-source-output $so $vsource 2> /dev/null
done

# set input volumes
pactl set-source-volume $source "100%"
pactl set-source-volume $vsource "100%"
