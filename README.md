# Setup Programs
## Instruction : Setup
```shell
apt-get update
apt upgrade
apt install net-tools
apt install build-essential
apt-get install manpages-dev
apt install nano
apt install gpiod
apt install htop
apt install bridge-utils
apt install cron
apt install ntpdate
apt-get install libmodbus-dev
sudo apt-get install libncurses5-dev libncursesw5-dev
```

# System Setting
## Set Network Properties Path
- Run command in commandline below,
```shell
nano /etc/network/interfaces.d/<interface>
```
### Interface: eth0
- Description: Set WAN to LAN and assign ip to eth0
```sh
# eth0.txt

auto eth0
#iface eth0 inet dhcp
iface eth0 inet static
    address 10.13.80.87/24
    netmask 255.255.255.0
    gateway 10.13.80.254
    dns-nameservers 8.8.8.8 8.8.4.4
    post-up ip route add 10.13.80.254/32 dev eth0
    pre-down ip route del 10.13.80.254/32 dev eth0
```
### Interface: eth1
- Description : Assign ip to eth1
```sh
# eth1.txt

auto eth1
iface eth1 inet static
	address 10.13.80.86/24
	netmask 255.255.255.0
	broadcast 10.13.80.255
	post-up ip route add 10.13.80.85/32 dev eth1
	pre-down ip route del 10.13.80.85/32 dev eth1
```
## Set Network Bridge
- Run command in commandline below,
```shell
nano /etc/sysctl.conf
```
Uncomment line
```sh
--> # net.ipv4.ip_forward=1
```
- Assign IP to bridge br0
```shell
nano /etc/network/interfaces.d/br0
```
```sh
# br0.txt

auto eth0
iface eth0 inet manual

auto eth1
iface eth1 inet manual

auto br0
iface br0 inet static
	address 10.13.80.87
	netmask 255.255.255.0
	gateway 10.13.80.254
	#post-up ip route add 10.13.80.254/32 dev br0
    #pre-down ip route del 10.13.80.254/32 dev br0
	bridge_ports eth0 eth1
	bridge_stp off
	bridge_fd 0
```
## Apply setting
```shell
sudo service networking restart
```

# NTP Service Setup
## Set timezone
```shell
timedatectl list-timezones
sudo timedatectl set-timezone Asia/Bangkok
```

## Synchronize time with NTP server
```shell
ntpdate <NTP Server>
```

## etc.
- Systemd-timesyncd
    1. apt install systemd-timesyncd
    2. nano /etc/systemd/timesyncd.conf
    3. systemctl restart systemd-timesyncd

- CHRONY
    1. apt install chrony
    2. nano /etc/chrony/chrony.conf
    3. pool <YOUR NTP SERVER> iburst

# Remote Configuration (TMConfiguration)
## Turn on bridge network for configuration
- Run command below,
```shell
nano bridge_on.sh
```
bridge_on.sh

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

## Turn off bridge network when finish configuring
- Run command below,
```shell
nano bridge_off.sh
```
bridge_off.sh

    #!/bin/sh

    ip link set br0 down
    brctl delif br0 eth0
    brctl delif br0 eth1

    brctl delbr br0

    reboot

Run command below to activate
```shell
sed -i -e 's/\r$//' bridge_on.sh
sed -i -e 's/\r$//' bridge_off.sh
chmod +x bridge_on.sh
chmod +x bridge_off.sh
```

# Kill the RVD program when its perform background runs
- Open system monitoring tool
```shell
htop
```
- Find ".rvd" process
- Press "F9" -> SIGKILL
- Press "F10" to quit

# Executing the RVD Program
Run command below
```shell
sed -i -e 's/\r$//' run.sh
chmod +x run.sh
./run.sh
```

# Startup
- Go to rc.local
```shell
nano /etc/rc.local
```

rc.local

    #!/bin/sh -e
    # rc.local

    # List of programs to run at startup
    /root/br_Off.sh &
    /root/RVD/run.sh &

# Task scheduler
Run 
```shell
crontab -e
```

    SHELL=/bin/sh
    HOME=/root
    PATH=/usr/local/sbin:usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin

    # * * * * * your_command
    # | | | | |
    # | | | | +-- Day of the week (0 - 7) (Sunday is 0 or 7)
    # | | | +---- Month (1 - 12)
    # | | +------ Day of the month (1 - 31)
    # | +-------- Hour (0 - 23)
    # +---------- Minute (0 - 59)

    * * * * * /root/RVD/run.sh # Check the RVD program every 1 minute 
</p>

    SHELL=/bin/bash
    HOME=/home/apo
    PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

    * * * * * /bin/bash /home/apo/restart_script.sh
    service cron restart or systemctl restart cron

Instruction "usbipd wsl attach --busid <id>"

# NTP Server Setup (on Windows OS)
run command below
```shell
Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\w32time\TimeProviders\NtpServer" -Name "Enabled" -Value 1

Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\services\W32Time\Config" -Name "AnnounceFlags" -Value 5

Restart-Service w32Time

New-NetFirewallRule `  -Name "NTP Server Port" `  -DisplayName "NTP Server Port" `  -Description 'Allow NTP Server Port' `  -Profile Any `  -Direction Inbound `  -Action Allow `  -Protocol UDP `  -Program Any `  -LocalAddress Any `  -LocalPort 123
