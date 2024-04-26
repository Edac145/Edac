
#include <ModbusRtu.h>
#include <SoftwareSerial.h>

#define TXEN 2
#define slave_id 5

#define RT_OHMS 10000   // Ω
#define B 3977      // K
#define VCC 5    //Supply voltage
#define R 10000  //R=10KΩ

const float OffSet = 0.5 ;

float RT_0, VR_0, ln_0, TX_0, T_0, VRT_0;
float RT_1, VR_1, ln_1, TX_1, T_1, VRT_1;

uint32_t temperature = 0;
int pressure= 0;
uint16_t CheckDelay = 0,printdelay;

uint16_t tempADC,pressADC;

uint16_t au16data[16] = {
  3, 1415, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};


SoftwareSerial mySerial(0, 1);
Modbus slave(slave_id, mySerial, TXEN);

void setup() {
  mySerial.begin(9600);
  // Serial.begin(9600);

  T_0 = 25 + 273.15;
  T_1 = 25 + 273.15;
  slave.start();
}
union Floarconv{
float FlaotData;
uint16_t convData[2];
} converFloat;

void loop() {
  // Analog 0 (Pressure)
  pressADC = analogRead(A0);
  float aData = pressADC * 5.00 / 1024; 
  
  VRT_0 = (aData - OffSet) * 340; 
  // VRT_0= 2.3;
 // VRT_0 = (5.00 / 1023.00) * VRT_0;
  // VR_0 = VCC - VRT_0;
  // RT_0 = VRT_0 / (VR_0 / R);
  // ln_0 = log(RT_0 / RT_OHMS);
  // TX_0 = (1 / ((ln_0 / B) + (1 / T_0)));
  // TX_0 = TX_0 - 273.15;
  
  pressure = (uint16_t)TX_0;
if(printdelay>5000)
{
  Serial.print("\nPressure:\t");
  // Serial.println("\t");
  Serial.print(VRT_0);
  // Serial.println("C\t\t");
  // Serial.print(TX_0 + 273.15);        
  // Serial.println("K\t\t");
  // Serial.print((TX_0 * 1.8) + 32);   
  // Serial.println("F\t");
  // Serial.print(pressADC);
  Serial.print("\t");
  Serial.print(pressADC);
}


  // Analog 1 (Temperature)
  VRT_1 = analogRead(A1);
  tempADC =  (uint16_t) VRT_1;
  VRT_1 = (5.00 / 1023.00) * VRT_1;
  VR_1 = VCC - VRT_1;
  RT_1 = VRT_1 / (VR_1 / R);
  ln_1 = log(RT_1 / RT_OHMS);
  TX_1 = (1 / ((ln_1 / B) + (1 / T_1)));
  TX_1 = TX_1 - 273.15;

uint32_t temperature = TX_1 * 100; 

if(printdelay>5000)
{
  // Serial.print("\tTemperature:");
  // Serial.print("\t");
  // Serial.print(TX_1);
  // Serial.println("C\t\t");
  // Serial.print(TX_1 + 273.15);        
  // Serial.println("K\t\t");
  // Serial.print((TX_1 * 1.8) + 32);   
  // Serial.println("F\t");
  // Serial.print(tempADC);
  printdelay=0;
}
  converFloat.FlaotData = TX_1;
  au16data[0] = converFloat.convData[0];
  au16data[1] = converFloat.convData[1];
  converFloat.FlaotData = VRT_0 ;
  au16data[2] = converFloat.convData[0];
  au16data[3] = converFloat.convData[1];
  au16data[4] = tempADC;
  au16data[5] = pressADC;

  slave.poll(au16data, 6);
  delay(1);
  printdelay++;
}
