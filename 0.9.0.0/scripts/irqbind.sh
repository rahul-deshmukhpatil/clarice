#!/bin/bash

if [ $# -ne 3 ]; then
	echo "Usage : sudo $0 [cpu-core-range] [from interrupt] [to interrupt]"
	echo "Usage : sudo ./irqbind.sh 0-4 0 45"
	exit -1
fi

cpulist=$1
cpuFrom=$2
cputTo=$3
echo "Binding interrupts $cpuFrom-$cputTo to cores $cpulist"

i=$cpuFrom

while [ $i -le $cputTo ];
do
	if [ -f /proc/irq/$i/smp_affinity_list ]; then
		echo "Setting irq $i affinity to core $cpulist"
		echo "echo $cpulisti > /proc/irq/$i/smp_affinity_list"
		echo $cpulist > /proc/irq/$i/smp_affinity_list
	fi
	i=$((i + 1))
done

if  [ -f /proc/irq/default_smp_affinity ]; then
	echo 3 > /proc/irq/default_smp_affinity
fi
