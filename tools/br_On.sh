#!/bin/bash
# Crete bridge
brctl addbr br0

# Add existing interfaces to bridge
brctl addif br0 eth1 eth2

# Or assign IP address to bridge interface (Don't recommend)
#ip addr add 192.168.11.10/24 brd + dev br0 // ip route add default via 192.168.2.1 dev br0