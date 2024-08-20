#!/bin/bash

# Install neccesory packages
apt-get update && sudo apt-get upgrade -y
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

# Set timezone & sync-time
sudo timedatectl set-timezone Asia/Bangkok
ntpdate 10.13.80.254

# Network configuration
SOURCE_DIR="/root/RVD/config"
DEST_DIR="/etc/network/interfaces.d"

if [[ ! -f "$SOURCE_DIR/custom_eth0" || ! -f "$SOURCE_DIR/custom_eth1" || ! -f "$SOURCE_DIR/custom_br0" ]]; then
  echo "Custom configuration files not found in $SOURCE_DIR"
  exit 1
fi

sudo cp "$SOURCE_DIR/custom_eth0" "$DEST_DIR/eth0"
sudo cp "$SOURCE_DIR/custom_eth1" "$DEST_DIR/eth1"
sudo cp "$SOURCE_DIR/custom_br0" "$DEST_DIR/br0"

FILE="/etc/sysctl.conf"

# Check if the line is commented out and uncomment it
sed -i 's/^#\(net.ipv4.ip_forward=1\)/\1/' "$FILE"

# Reload the sysctl settings
sysctl -p

echo "The line 'net.ipv4.ip_forward=1' has been uncommented and sysctl settings reloaded."

sed -i -e 's/\r$//' /root/RVD/run.sh
sed -i -e 's/\r$//' /root/RVD/tools/bridge_on.sh
sed -i -e 's/\r$//' /root/RVD/tools/bridge_off.sh
sed -i -e 's/\r$//' /root/RVD/tools/br_Off.sh
chmod +x /root/RVD/run.sh
chmod +x /root/RVD/tools/bridge_on.sh
chmod +x /root/RVD/tools/bridge_off.sh
chmod +x /root/RVD/tools/br_Off.sh

# Cron configuration
CRON_JOB_CONTENT=$(cat <<EOF
SHELL=/bin/sh
HOME=/root
PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin

# * * * * * your_command
# | | | | |
# | | | | +-- Day of the week (0 - 7) (Sunday is 0 or 7)
# | | | +---- Month (1 - 12)
# | | +------ Day of the month (1 - 31)
# | +-------- Hour (0 - 23)
# +---------- Minute (0 - 59)

* * * * * /root/RVD/run.sh # Check the RVD program every 1 minute 
EOF
)

# Add the cron jobs
echo "$CRON_JOB_CONTENT" | sudo crontab -

echo "Crontab has been updated with the specified jobs."


# Backup existing rc.local
cp /etc/rc.local /etc/rc.local.bak

# Write the new content to rc.local
cat <<EOL > /etc/rc.local
#!/bin/sh -e
# rc.local

# List of programs to run at startup
/root/RVD/tools/br_Off.sh &
/root/RVD/run.sh &

exit 0
EOL

# Ensure rc.local is executable
chmod +x /etc/rc.local

echo "rc.local has been updated and made executable."

service networking restart
systemctl restart networking