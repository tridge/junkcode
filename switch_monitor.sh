#!/bin/sh

export PATH=$HOME/bin:$PATH

SWITCH=$HOME/.switch_monitor
STATE=0

if [ -r $SWITCH ]; then
    STATE=`cat $SWITCH`
fi

case "$STATE" in
    BOTH)
	STATE=LCD
	radeontool dac off
	radeontool light on
	;;
    LCD)
	STATE=CRT
	radeontool dac on
	radeontool light off
	;;
    CRT)
	STATE=BOTH
	radeontool dac on
	radeontool light on
	;;
esac

echo $STATE > $SWITCH
echo $STATE
export DISPLAY=:0
echo $STATE | osd_cat -f '-*-*-*-*-*-*-*-400-*-*-*-*-*-*' -d2 -pmiddle -Acenter
