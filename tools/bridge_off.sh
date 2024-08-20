#!/bin/sh

ip link set br0 down
brctl delif br0 eth0
brctl delif br0 eth1

#ip link set eth1 up
#ip link set eth2 up
brctl delbr br0

get_interface_settings() {
    local iface=$1
    local file="/etc/network/interfaces.d/$iface"

    if [[ -f $file ]]; then
	local address=$(grep -w 'address' $file | awk '{print $2}')
	local netmask=$(grep -w 'netmask' $file | awk '{print $2}')
	local gateway=$(grep -w 'gateway' $file | awk '{print $2}')
	local dns=$(grep -w 'dns-nameservers' $file | awk '{print $2" "$3}')

	echo "$address $netmask $gateway $dns"
    else
	echo "File $file does not exist."
	exit 1
    fi
}

#eth0_settings=$(get_interface_settings "eth0")
#eth0_ip=$(echo $eth0_settings | awk '{print $1}')
#eth0_netmask=$(echo $eth0_settings | awk '{print $2}')
#eth0_gateway=$(echo $eth0_settings | awk '{print $3}')
#eth0_dns=$(echo $eth0_settings | awk '{print $4" "$5}')

#ifconfig eth0 $eth0_ip netmask $eth0_netmask up

#echo "nameserver $(echo $eth0_dns | awk '{print $1}')" > /etc/resolv.conf
#echo "nameserver $(echo $eth0_dns | awk '{print $2}')" > /etc/resolv.conf

#ip route add 10.13.80.254/32 dev eth0
#route add default gw $eth0_gateway eth0

#eth1_settings=$(get_interface_settings "eth1")
#eth1_ip=$(echo $eth1_settings | awk '{print $1}')
#eth1_netmask=$(echo $eth1_settings | awk '{print $2}')

#ifconfig eth1 $eth1_ip netmask $eth1_netmask up

#ip route add 10.13.80.85/32 dev eth1

#echo "post-up ip route add 10.13.80.254/32 dev eth0" >> /etc/network/interfaces.d/eth0
#echo "pre-down ip route del 10.13.80.254/32 dev eth0" >> /etc/network/interfaces.d/eth0
#echo "post-up ip route add 10.13.80.85/32 dev eth1" >> /etc/network/interfaces.d/eth1
#echo "pre-down ip route add 10.13.80.85/32 dev eth1" >> /etc/network/interfaces.d/eth1

reboot
