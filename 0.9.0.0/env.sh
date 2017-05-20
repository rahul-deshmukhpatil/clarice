#!/bin/bash

#
# Contains the directories and OS specs definitions.
##############################################################

export CD_HOME=`pwd`
export HOSTNAME=`hostname`
export HOSTOS=`uname`
if [ "$HOSTTYPE" == "x86_64" ]; then 
    export  HOSTBITS=64
else 
    export  HOSTBITS=32
fi
export LD_LIBRARY_PATH=/usr/local/lib:$CD_HOME/libs/$HOSTBITS

