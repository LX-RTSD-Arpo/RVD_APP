#!/bin/bash

if ping -q -c 1 -W 1 8.8.8.8 >/dev/null; then
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
    sudo mv bin/arduino-cli /usr/local/bin/
    chmod +x /usr/local/bin/arduino-cli
    arduino-cli version
    arduino-cli config init
    arduino-cli core update-index
    arduino-cli core search arduino
    arduino-cli core install arduino:avr
    arduino-cli lib install "ModbusRTUSlave"
else
    mv arduino-cli /usr/local/bin/
    chmod +x /usr/local/bin/arduino-cli
    arduino-cli config init
    arduino-cli core update-index
    arduino-cli core search arduino
    arduino-cli core download arduino:avr
    arduino-cli lib search "ModbusRTUSlave"
    arduino-cli lib download "ModbusRTUSlave"
fi

arduino-cli compile --fqbn arduino:avr:nano /root/RVD_APP/include/RVD_Modbus_Slave/RVD_Modbus_Slave.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:nano /root/RVD_APP/include/RVD_Modbus_Slave/RVD_Modbus_Slave.ino