#!/bin/bash

apt-get update
apt upgrade
apt install net-tools
apt install build-essential
apt-get install manpages-dev
apt install nano
apt install htop
apt install bridge-utils
apt install cron
apt install ntpdate
apt-get install libmodbus-dev
sudo apt-get install libncurses5-dev libncursesw5-dev

# Install dependencies from requirements.txt
pip install -r requirements.txt

# Run gcc on wo_req_crctable.c
gcc ./sources/RVD_V1.0.0b0.c -o ./sources/rvd-v1.0.0b0 -lpthread -lmodbus
chmod +x run.sh
chmod +x ./tools/br_On.sh
chmod 774 ./tools/br_Off.sh