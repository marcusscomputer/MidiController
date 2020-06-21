#!/bin/bash

MIDIMIX=`amidi -l | grep "MIDI Mix" | grep -oP '(?<=hw:)[\d.]+'`
amidi -p hw:$MIDIMIX,0,0 -d > /tmp/akaiMidiMixData &
~/Developer/Projects/MidiController/midimix &

while :
do
	echo "" > /tmp/akaiMidiMixData
	sleep 5
done
