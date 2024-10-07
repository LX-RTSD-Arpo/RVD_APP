/*
  ModbusRTUSlaveExample

  This example demonstrates how to setup and use the ModbusRTUSlave library.
  It is intended to be used with a second board running ModbusRTUMasterExample from the ModbusRTUMaster library.  
  
  Circuit:
  - A pushbutton switch from pin 2 to GND
  - A pushbutton switch from pin 3 to GND
  - A LED from pin 5 to GND with a 330 ohm series resistor
  - A LED from pin 6 to GND with a 330 ohm series resistor
  - A LED from pin 7 to GND with a 330 ohm series resistor
  - A LED from pin 8 to GND with a 330 ohm series resistor
  - The center pin of a potentiometer to pin A0, and the outside pins of the potentiometer to 5V and GND
  - The center pin of a potentiometer to pin A0, and the outside pins of the potentiometer to 5V and GND
  
  !!! If your board's logic voltage is 3.3V, use 3.3V instead of 5V; if in doubt use the IOREF pin !!!
  
  - Pin 10 to pin 11 of the master/client board
  - Pin 11 to pin 10 of the master/client board
  - GND to GND of the master/client board
  
  A schematic and illustration of the circuit is in the extras folder of the ModbusRTUSlave library.

  - Pin 13 is set up as the driver enable pin. This pin will be HIGH whenever the board is transmitting.
  
  Created: 2023-07-22
  By: C. M. Bulliner
  Modified: 2023-07-29
  By: C. M. Bulliner
  
*/

#include <ModbusRTUSlave.h>

const int ledPin = 13;
const byte coilsPins[4] = {8, 7, 3, 2};
const byte discreteInputsPins[2] = {6, 5};
const byte inputRegistersPins[2] = {A0, A1};
const uint8_t dePin = 13;

//SoftwareSerial mySerial(rxPin, txPin);
ModbusRTUSlave modbus(Serial, dePin); // serial port, driver enable pin for rs-485 (optional)

bool coils[3];
bool precoils[2];
bool discreteInputs[2];
uint16_t holdingRegisters[2];
uint16_t inputRegisters[2];
uint16_t volt[2];

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(coilsPins[2], OUTPUT);
  pinMode(coilsPins[3], OUTPUT);
  pinMode(discreteInputsPins[0], INPUT_PULLUP);
  pinMode(discreteInputsPins[1], INPUT_PULLUP);
  pinMode(inputRegistersPins[0], INPUT);
  pinMode(inputRegistersPins[1], INPUT);

  modbus.configureCoils(coils, 3);                       // bool array of coil values, number of coils
  modbus.configureDiscreteInputs(discreteInputs, 2);     // bool array of discrete input values, number of discrete inputs
  modbus.configureHoldingRegisters(holdingRegisters, 2); // unsigned 16 bit integer array of holding register values, number of holding registers
  modbus.configureInputRegisters(inputRegisters, 2);     // unsigned 16 bit integer array of input register values, number of input registers

  holdingRegisters[0] = 500;
  modbus.begin(1, 9600);                                // slave id, baud rate, config (optional)
}

void loop() {
  precoils[0] = coils[0];
  precoils[1] = coils[1];
  // coils[0] = false;
  // coils[1] = false;
  //discreteInputs[0] = 1;//!digitalRead(discreteInputsPins[0]);
  //discreteInputs[1] = 0;//!digitalRead(discreteInputsPins[1]);
  //volt[0] = analogRead(holdingRegistersPins[0]);
  //volt[1] = analogRead(holdingRegistersPins[1]);
  //holdingRegisters[0] = analogRead(holdingRegistersPins[0]);//((volt[0]/4) / 1024) * 61;
  //holdingRegisters[1] = analogRead(holdingRegistersPins[1]);//((volt[1]/4) / 1024) * 61;

  inputRegisters[0] = analogRead(inputRegistersPins[0]);
  inputRegisters[1] = analogRead(inputRegistersPins[1]);

  modbus.poll();

  if ((precoils[0] != coils[0]) || (precoils[1] != coils[1])) {
    digitalWrite(coilsPins[2], coils[0]);
    digitalWrite(coilsPins[3], coils[1]);
  }

  // Serial.println(inputRegisters[0], DEC);

  if (coils[2] == 1) {
    delay(holdingRegisters[0]);
    coils[0] = false;
    coils[1] = false;
  }
}
