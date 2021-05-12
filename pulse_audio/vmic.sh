#!/bin/bash

# script to switch virtual audio between desktop and headset, linked to
# function keys on keyboard

[ $# -eq 1 ] || {
    echo "vmic.sh DESKTOP|HEADSET"
    exit 1
}

# the devices I use
speakers="alsa_output.pci-0000_00_1f.3.analog-stereo"
webcam="alsa_input.usb-046d_HD_Pro_Webcam_C920_39959BBF-02.analog-stereo"
jabrainput="alsa_input.usb-GN_Netcom_A_S_Jabra_PRO_930_01914FBBE002-00.mono-fallback"
jabraoutput="alsa_output.usb-GN_Netcom_A_S_Jabra_PRO_930_01914FBBE002-00.mono-fallback"


if [ $1 = "HEADSET" ]; then
    echo "Using HEADSET"
    source="$jabrainput"
    dest="$jabraoutput"
elif [ $1 = "DESKTOP" ]; then
    echo "Using DESKTOP"
    source="$webcam"
    dest="$speakers"
else
    echo "Bad audio type $1"
    exit 1
fi

# we have to reload as you can't change the master while playing
pactl unload-module module-remap-source
pactl load-module module-remap-source source_name=VMic source_properties="device.description=VMic" master="$source"
pactl unload-module module-remap-sink
pactl load-module module-remap-sink sink_name=VSpeaker sink_properties="device.description=VSpeaker" master="$dest"

# get the long name of the virtual devices
sink=$(pactl list short sinks | grep VSpeaker | tail -1 | cut -f1)
source=$(pactl list short sources | grep VMic | tail -1 | cut -f1)

# change all apps
sinkinputs=$(pactl list short sink-inputs | cut -f1)
for si in $sinkinputs; do
    pactl move-sink-input $si $sink 2> /dev/null
done

sourceoutputs=$(pactl list short source-outputs | cut -f1)
for so in $sourceoutputs; do
    pactl move-source-output $so $source 2> /dev/null
done
