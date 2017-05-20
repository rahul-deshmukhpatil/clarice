#!/bin/bash 


if [ $# -ne 1 ]; then
	>&2 echo "Usage : $0 process-name
			: $0 mdapp-debug" 
	exit -1;
fi

app_name=$1
id=`pgrep $app_name`
logs_dir=logs-$id
mkdir  -p $logs_dir
rm $logs_dir/*

	top -b -n 1 -H -p `pgrep $app_name` | awk '{ if (NR > 7) { print } }' | 
	while read line
	do
		echo $line
		t_id=`echo $line | awk '{print $1}'`
		p_name=`echo $line | awk '{print $13}'`
		echo $pname.$t_id 
		strace -c -tt -p $t_id 2> $logs_dir/$p_name.$t_id & 
	done
wait
