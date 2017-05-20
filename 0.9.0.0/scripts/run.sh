#!/bin/bash

sudo ../scripts/turnoff.sh on 4 16
sudo ../scripts/irqbind.sh  0-3 0 100

./regression.sh 5

