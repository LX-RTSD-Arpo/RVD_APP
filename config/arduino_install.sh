#!/bin/bash

curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/instal
sudo mv bin/arduino-cli /usr/local/bin/
arduino-cli version
arduino-cli config init
arduino-cli core update-index
arduino-cli core search arduino
arduino-cli core install arduino:avr
arduino-cli lib install "ModbusRTUSlave"
cd RVD_APP/
git pull
arduino-cli compile --fqbn arduino:avr:nano /root/RVD_APP/include/RVD_Modbus_Slave/RVD_Modbus_Slave.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano /root/RVD_APP/include/RVD_Modbus_Slave/RVD_Modbus_Slave.ino

