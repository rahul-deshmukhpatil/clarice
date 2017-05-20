#!/bin/bash

sudo ../scripts/turnoff.sh on 4 16
sudo ../scripts/irqbind.sh 0-4 0 45

if [ $# -ne 1 ]; then
	echo "Usage $0 <no-of-runs>"
	echo "		$0 5"
	exit -1
fi

logDir="app-logs"
mkdir -p $logDir
rm $logDir/*

runs=$1
i=0
while [ $i -lt $runs ];
do
	export MAINTAIN_ORDER=false
	../bin/64/mdapp-release -c sample-no-order.xml -s ".*" #-p 10 -f output-$i
	#./count_chars.py output-$i > charCount-$i
	mv MarketDataUS-clarice.log MarketDataUS-clarice.log-$i
	mv output-$i charCount-$i MarketDataUS-clarice.log-$i $logDir/

	export MAINTAIN_ORDER=true
	../bin/64/mdapp-release -c sample.xml -s ".*" #-o -p 10 -f output-$i-maintain
	#./count_chars.py output-$i-maintain > charCount-$i-maintain
	mv MarketDataUS-clarice.log MarketDataUS-clarice.log-$i-maintain
	mv output-$i-maintain charCount-$i-maintain MarketDataUS-clarice.log-$i-maintain $logDir/

	i=`expr $i + 1`
done
