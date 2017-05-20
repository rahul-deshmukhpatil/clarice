#!/bin/bash
if [ $# -ne 3 ]; then
	echo "Usage : sudo $0 [on/off] [from core] [to core]"
	echo "Usage : sudo ./turnoff.sh on 4 16"
	echo "Usage : sudo ./turnoff.sh off 4 16"
	exit 0
fi

if [ "$1" == "on" ]; then
	turnOnOff=1;
	onoff="on"
else
	turnOnOff=0;
	onoff="off"
fi

if [ $# -eq 3 ]; then
	cpu=$2
	cpu2=$3
	while [ $cpu -le $cpu2 ];
	do 
		echo "Turning $onoff cpu $cpu"
		sudo echo $turnOnOff > /sys/devices/system/cpu/cpu$cpu/online
		cpu=$((cpu + 1))
	done
	exit 0
fi

echo "Not a valid usage"
echo "$0 [on/off] cpu_no"
echo "$0 [on/off] cpu1 cpu2"
