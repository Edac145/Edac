#include <ModbusRtu.h>
#include <SoftwareSerial.h>

#define TXEN 2
#define slave_id 1

#define RT_OHMS 10000   // Ω
#define B 3977      // K
#define VCC 5    //Supply voltage
#define R 10000  //R=10KΩ

const float OffSet = 0.5;

float RT_0, VR_0, ln_0, TX_0, T_0, VRT_0;
float RT_1, VR_1, ln_1, TX_1, T_1, VRT_1;

uint32_t temperature = 0;
int pressure = 0;
uint16_t CheckDelay = 0, printdelay;

uint16_t tempADC, pressADC;

// Moving median parameters
const int numReadings = 10;  // Adjust this value based on the desired filter strength
int tempReadings[numReadings];
int pressReadings[numReadings];

uint16_t au16data[16] = {
  3, 1415, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};

SoftwareSerial mySerial(0, 1);
Modbus slave(slave_id, mySerial, TXEN);

union Floarconv {
  float FlaotData;
  uint16_t convData[2];
} converFloat;

void insertIntoSortedArray(int *array, int newValue, int size) {
  int i;
  for (i = size - 1; (i >= 0 && array[i] > newValue); i--) {
    array[i + 1] = array[i];
  }
  array[i + 1] = newValue;
}

int getMedian(int *array, int size) {
  if (size % 2 == 0) {
    return (array[size / 2 - 1] + array[size / 2]) / 2;
  } else {
    return array[size / 2];
  }
}

void setup() {
  mySerial.begin(9600);
  T_0 = 25 + 273.15;
  T_1 = 25 + 273.15;
  slave.start();
}

void loop() {
  // Analog 0 (Pressure)
  pressADC = analogRead(A0);
  float aData = pressADC * 5.00 / 1024;

  VRT_0 = (aData - OffSet) * 340;

  // Apply moving median filter to pressure
  insertIntoSortedArray(pressReadings, pressADC, numReadings);
  pressure = getMedian(pressReadings, numReadings);

  if (printdelay > 5000) {
    Serial.print("\nPressure:\t");
    Serial.print(VRT_0);
    Serial.print("\t");
    Serial.print(pressure);
  }

  // Analog 1 (Temperature)
  VRT_1 = analogRead(A1);
  tempADC = (uint16_t)VRT_1;
  VRT_1 = (5.00 / 1023.00) * VRT_1;
  VR_1 = VCC - VRT_1;
  RT_1 = VRT_1 / (VR_1 / R);
  ln_1 = log(RT_1 / RT_OHMS);
  TX_1 = (1 / ((ln_1 / B) + (1 / T_1)));
  TX_1 = TX_1 - 273.15;

  // Apply moving median filter to temperature
  insertIntoSortedArray(tempReadings, tempADC, numReadings);
  temperature = getMedian(tempReadings, numReadings);

  if (printdelay > 5000) {
    printdelay = 0;
    Serial.print("\tTemperature:");
    Serial.print("\t");
    Serial.print(TX_1);
    Serial.print("C\t\t");
    Serial.print(TX_1 + 273.15);
    Serial.print("K\t\t");
    Serial.print((TX_1 * 1.8) + 32);
    Serial.println("F\t");
    Serial.print(tempADC);
  }

  converFloat.FlaotData = TX_1;
  au16data[0] = converFloat.convData[0];
  au16data[1] = converFloat.convData[1];
  converFloat.FlaotData = VRT_0;
  au16data[2] = converFloat.convData[0];
  au16data[3] = converFloat.convData[1];
  au16data[4] = temperature;
  au16data[5] = pressure;

  slave.poll(au16data, 6);
  delay(1);
  printdelay++;
}
