#!/bin/bash

sudo ip link set br0 down
sudo ip link set eth0 down
sudo ip link set eth1 down

sudo ip addr flush dev br0
sudo ip addr flush dev eth0
sudo ip addr flush dev eth1

sudo brctl addbr br0
sudo brctl addif br0 eth0
sudo brctl addif br0 eth1

sudo ip link set br0 up
sudo ip link set eth0 up
sudo ip link set eth1 up

#brctl addbr br0
#brctl addif br0 eth1 eth2
