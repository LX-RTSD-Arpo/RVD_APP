# Update Repository (when online) 
1. connect to [RVD-IP] in ssh terminal, then run script below

```shell
dhclient -r eth0
dhclient eth0
dhclient -r eth0
dhclient eth0
cd RVD_APP
git pull
```

# Update Repository (when offline)
1. Stop cron from running program by run
```sh
kill -9 $(ps aux | grep run.sh | grep -v grep | awk '{print $2}' | head -n 1)
kill -9 $(ps aux | grep rvd | grep -v grep | awk '{print $2}' | head -n 1)
```
2. Copy & Upload files from RVD_APP into RVD_APP directory via MobaXterm, then run
```sh
sed -i -e 's/\r$//' /root/RVD_APP/run.sh
sed -i -e 's/\r$//' /root/RVD_APP/tools/bridge_on.sh
sed -i -e 's/\r$//' /root/RVD_APP/tools/bridge_off.sh
sed -i -e 's/\r$//' /root/RVD_APP/tools/br_Off.sh
sed -i -e 's/\r$//' /root/RVD_APP/config/arduino_install.sh
gcc /root/RVD_APP/sources/RVD_V1.0.0b1.c -o /root/RVD_APP/sources/rvd-v1.0.0b1 -lpthread -lmodbus
gcc /root/RVD_APP/tests/IO_Test.c -o /root/RVD_APP/tests/iotest -lmodbus
```

<!-- 2. Extract Python package
tar -xzvf flask_package.tar.gz
tar -xzvf pytz_package.tar.gz
python3 -m pip install /usr/local/lib/python3.8/dist-packages/flask
python3 -m pip install /usr/local/lib/python3.8/dist-packages/pytz -->

3. Install python packages offline
- Copy file pytz_package_offline and flask_package_offline from RVD Update package into root directory, then run
```sh
python3 -m pip install --no-index -f "/root/pytz_package_offline/" pytz
python3 -m pip install --no-index -f "/root/flask_package_offline/" flask
```

# Make an installation
- Run script below
```shell
sed -i -e 's/\r$//' /root/RVD_APP/config/install_script.sh
cd /root/RVD_APP/config
chmod +x install_script.sh
./install_script.sh
```


# Time-Synchronization Test
- Run script below
```shell
sudo timedatectl set-ntp false
sudo timedatectl set-time '2024-10-09 14:30:00'
sudo timedatectl set-ntp true
```

Not sure what it is
```shell
dhclient -r eth0
dhclient eth0
cd RVD_APP
git stash push tools/bridge_on.sh
git pull
reboot
```
