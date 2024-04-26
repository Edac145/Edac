

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "SD.h"
 

#include "SPI.h"
#include "USBMSC.h"
#include <ModbusMaster.h>
#include <SoftwareSerial.h>
// #include <espsoftwareserial.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <Arduino.h>

/***************************
 * Definitions
 **************************/
#define SD_MISO 37
#define SD_MOSI 35
#define SD_SCK 36
#define SD_CS 38
#define RX_PIN 18
#define TX_PIN 17

#define DE_RE_PIN 12
#define NUM_SLAVES 25
#define PrintCustomeLOG   1// #define DATA_LOG_FREQUENCY 1  // in minutes

#define USER_LOG         1
#define CALIBRATION_LOG  2

#define DATA_LOG_FILE_NAME "/userlog.csv"
#define CALLIBRATION_FILE_NAME "/callibration.csv"
#define BOOT_BUTTON 0
#define MinutesInDay        1440

/***************************
 * Variables
 **************************/
 
const uint32_t NVM_Offset = 0x290000;
const char* filePath = DATA_LOG_FILE_NAME;
const char* dataLogFilePath = CALLIBRATION_FILE_NAME;
const int SD_CS_PIN = 38;

uint8_t FLASH_Address = 0;
const int MIN_DATA_LOG_FREQUENCY = 0;
const int MAX_DATA_LOG_FREQUENCY = 9999;
ModbusMaster node[NUM_SLAVES];
char receivedData[100];
File dataFile;
unsigned long int DataLogMinute = 0, DataLogMinuteFrequency = 0;
int DATA_LOG_FREQUENCY;
USBHIDKeyboard keyboard;
USBMSC msc;

const int MAX_NUMBERS = 10; // Change this to your desired maximum number of integers
int numbers[MAX_NUMBERS];
int numCount = 0;


// int slaveIdsToReadTP[] = { 1,7,13,19,25};
int slaveIdsToReadTP[] = { 1,2,3};
ThreeWire myWire(6, 5, 9);  // IO, SCLK, CE
SoftwareSerial rs485Serial(RX_PIN, TX_PIN);
RtcDS1302<ThreeWire> Rtc(myWire);
struct date {
int day, month, year,Hour,Minutes;
}strCurrentRtc;
unsigned long int DaysForRTC ;
unsigned long int currentMinute;
char retrycounts;

int slaveToRead;
unsigned long lastLoopMillis = 0;  // Added declaration
static int j;


/***************************
 * Prototypes
 **************************/
short readSerialData(void);
unsigned long int convertDateToDays(struct date date);
/***************************
 * Code
 **************************/

template<typename T>
void FlashWrite(uint32_t address, const T& value) {
  ESP.flashEraseSector((NVM_Offset+address)/4096);
  ESP.flashWrite(NVM_Offset+address, (uint32_t*)&value, sizeof(value));
}
 
template<typename T>
void FlashRead(uint32_t address, T& value) {
  ESP.flashRead(NVM_Offset+address, (uint32_t*)&value, sizeof(value));
}
uint16_t checksumCalculator(uint8_t* data, uint16_t length)
{


  uint16_t curr_crc = 0x0000;
  uint8_t sum1 = (uint8_t)curr_crc;
  uint8_t sum2 = (uint8_t)(curr_crc >> 8);
  int index;

  for (index = 0; index < length; index = index + 1) {

    sum1 = (sum1 + data[index]) % 255;
    sum2 = (sum2 + sum1) % 255;
  }

  return (sum2 << 8) | sum1;
 }

int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {

  // log("write");

  // SD.writeRAW((uint8_t*)buffer, lba);

  // return bufsize;

  uint8_t NoOfSectorsToWrite;
  // log("write");
  uint8_t* data = (uint8_t*)buffer;
  NoOfSectorsToWrite = bufsize / 512;
  for (int i = 0; i < NoOfSectorsToWrite; i++) {
    SD.writeRAW((uint8_t*)data, lba);
    lba++;
    data += 512;
  }

  return bufsize;
  }

int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  uint8_t NoOfSectorsToRead;
  // log("read");
  uint8_t* data = (uint8_t*)buffer;
  NoOfSectorsToRead = bufsize / 512;
  for (int i = 0; i < NoOfSectorsToRead; i++) {
    SD.readRAW((uint8_t*)data, lba);
    lba++;
    data += 512;
  }

  return bufsize;
}

bool onStartStop(uint8_t power_condition, bool start, bool load_eject)

{
  return true;
}

void log(const char* str) {
  Serial.println(str);
}

bool isBootButtonClicked() {
  return digitalRead(BOOT_BUTTON) == LOW;
}

void saveDataToSD(const char* data, unsigned char bLogType) {


  if(bLogType == USER_LOG)
  {
      if (!SD.exists(DATA_LOG_FILE_NAME))
      {
        File dataFile = SD.open(DATA_LOG_FILE_NAME, FILE_WRITE);
        if (dataFile) {
          dataFile.print("Slave_Id,Date,Time,Temperature,Pressure");
          dataFile.close();
        } else {
          log("Failed to create file");
        }
      }

      dataFile = SD.open(DATA_LOG_FILE_NAME, FILE_APPEND);
      if (dataFile) {
        dataFile.print(data);
        dataFile.close();
      }
  }
  else if(bLogType == CALIBRATION_LOG)
  {
      if (!SD.exists(CALLIBRATION_FILE_NAME))
      {
        File dataFile = SD.open(CALLIBRATION_FILE_NAME, FILE_WRITE);
        if (dataFile) {
          dataFile.print("Slave_Id,Date,Time,Temperature,temperature_ADC,Pressure,pressure_ADC");
          dataFile.close();
        } else {
          log("Failed to create file");
        }
      }

      dataFile = SD.open(CALLIBRATION_FILE_NAME, FILE_APPEND);
      if (dataFile) {
        dataFile.print(data);
        dataFile.close();
      }

  }

  // else
  //  {
  //   log("Failed to open file for writing");
  //  }
}

void preTransmission() {
  digitalWrite(DE_RE_PIN, 1);
}

void postTransmission() {
  digitalWrite(DE_RE_PIN, 0);
}

void setup() {
        pinMode(11,OUTPUT);
        Serial.begin(9600);
       
  //     Serial.println("Enter the slave id:");
   
  //  // Read the incoming byte
  //  while (!Serial.available()) {
  //    Serial.println("Waiting for data...");
  //    delay(1000);
  //  }
   
  //  slaveToRead = Serial.read();
   
  //  // Print the received byte to the Serial Monitor
  //  Serial.print("Received: ");
  //  Serial.println(slaveToRead);
  // slaveToRead = 2;
        
  //  #ifdef PrintCustomeLOG
    static SPIClass* spi = NULL;
    uint8_t buf[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
  uint16_t crc = checksumCalculator(buf, 10);
      Serial.begin(9600);
      Serial.println();

      Serial.print("Calculated CRC is: ");
      Serial.println(crc, HEX);
      // #endif

            
  //     Serial.println("Enter the slave id:");
   
  //  // Read the incoming byte
  //  while (!Serial.available()) {
  //    Serial.println("Waiting for data...");
  //    delay(1000);
  //  }
   
   
  //  slaveToRead = Serial.parseInt();  // Convert ASCII to numerical value
  
   
  //  // Print the received byte to the Serial Monitor
  //  Serial.print("Received: ");
  //  Serial.println(slaveToRead);
 
  spi = new SPIClass(FSPI);
  spi->begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (SD.begin(SD_CS_PIN, *spi, 40000000)) {

    log("SD begin");
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
     #ifdef PrintCustomeLOG
    Serial.println("No SD card attached");
    #endif
    return;
  }


   Serial.println("Enter the slave id(s) (comma-separated): ");

   
   // Read the incoming byte
   while (!Serial.available()) {
     Serial.println("Waiting for data...");
     delay(1000);
   }

   if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n'); // Read a line until newline character '\n'
    
    // Parse and store the integers
    numCount = parseAndStoreIntegers(inputString);

    // Display the stored numbers
    displayStoredNumbers();
  }


  msc.vendorID("EDAC");
  msc.productID("OCEAN");
  msc.productRevision("1.0");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.onStartStop(onStartStop);
  msc.mediaPresent(true);
  msc.begin(SD.cardSize() / 512, 512);
  USB.begin();
  rs485Serial.begin(9600, SWSERIAL_8N1);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  Rtc.Begin();
  //  while (!Serial)
  // {
  //    ;
  // }

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();
  if (!Rtc.IsDateTimeValid()) {
    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  DataLogMinute = now.Minute();

  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    // Rtc.SetDateTime(compiled);
  }

  else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }

  else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  if (!SD.exists(DATA_LOG_FILE_NAME)) {
    File dataFile = SD.open(DATA_LOG_FILE_NAME, FILE_WRITE);
    if (dataFile) {
      dataFile.println("Slave_Id,Date,Time,Temperature,Pressure");
      dataFile.close();
    }

    else
    {
      log("Failed to create file");
    }
  }

  else {
    log("File already exists");
  }
    if (!SD.exists(CALLIBRATION_FILE_NAME)) {
    File dataFile = SD.open(CALLIBRATION_FILE_NAME, FILE_WRITE);
    if (dataFile) {
      dataFile.println("Slave_Id,Date,Time,Temperature,temperature_ADC,Pressure,pressure_ADC");
      dataFile.close();
    }

    else
    {
      log("Failed to create file");
    }
  }

  else {
    log("File already exists");
  }

 FlashRead<int>(0, DATA_LOG_FREQUENCY);

if(DATA_LOG_FREQUENCY > 9999)
{
 DATA_LOG_FREQUENCY = 1;
}
else if(DATA_LOG_FREQUENCY <= 0)
{
 DATA_LOG_FREQUENCY = 1;
}
else
{
// TO DO : Code to be added to read Data logging frequency
  DataLogMinuteFrequency = DATA_LOG_FREQUENCY;
}
  for (int i = 0; i < NUM_SLAVES; i++) {
    node[i].begin(i + 1, rs485Serial);
    node[i].preTransmission(preTransmission);
    node[i].postTransmission(postTransmission);
    
  }
 

}

void loop1() {
  if (isBootButtonClicked()) {
    keyboard.write('a');
    log("klik!");
  }
}

void loop()
{
  
  // if (isBootButtonClicked()) {
  //   keyboard.write('a');
  //   log("klik!");
  // }
  readSerialData();

  // Get the current minute

RtcDateTime now = Rtc.GetDateTime();
delay(500);

strCurrentRtc.day = now.Day();
strCurrentRtc.month = now.Month();
strCurrentRtc.year = now.Year();
strCurrentRtc.Hour = now.Hour();
strCurrentRtc.Minutes =  now.Minute();

  DaysForRTC = convertDateToDays(strCurrentRtc);
  currentMinute = ((DaysForRTC  * MinutesInDay) + (strCurrentRtc.Hour * 60) + strCurrentRtc.Minutes);
  
  // if (currentMinute >=  DataLogMinute) {

  //   if (readSerialData() == 0) {
  //     printDateTime(now);
  //     Serial.print("  DATA_LOG_FREQUENCY: ");  Serial.print(DATA_LOG_FREQUENCY);
  //     Serial.print("  DataLogMinuteFrequency : ");  Serial.print(  DataLogMinuteFrequency );
  //     Serial.print("  currentMinute: ");  Serial.print(  currentMinute );   Serial.print(  DataLogMinute );
   
  //   }

    // Update DataLogMinute so that next data log can be done on next minute as per logging frequency
    
      DataLogMinute = currentMinute +  DataLogMinuteFrequency;

       // Get the current minute
 

  // Check if 0.2 minutes have passed
  if (millis() - lastLoopMillis >= 5000) {  // 12000 milliseconds = 12 seconds
    lastLoopMillis = millis();  // Update the last loop time
    for(; ;)
    {
    // Loop through all slaves and read data once per minute
      digitalWrite(11,HIGH);
    for (int i = 0; i < NUM_SLAVES; i++)
     {
      uint8_t result;
      float temp;
      uint16_t temperatureADC;

      float press;
      uint16_t pressureADC;

      uint16_t us_press;

       union Floarconv{
       float FlaotData;
       uint16_t convData[2]; 

        } converFloat;
                 
 
        
      readSerialData();

      if (i + 1 == numbers[0] || i + 1 == numbers[1] || i + 1 == numbers[2] || i + 1 == numbers[3] || i + 1 == numbers[4] || numbers[5] )

      {
        result = 1;
        retrycounts = 3;
        do{
           result = node[i].readHoldingRegisters(0x000, 6);

        }while((result != node[i].ku8MBSuccess) && (retrycounts--));
       
        if (result == node[i].ku8MBSuccess) {
        // temp = node[i].getResponseBuffer(0);
        // press = node[i].getResponseBuffer(1);
        converFloat.convData[0] =node[i].getResponseBuffer(0);
        converFloat.convData[1] = node[i].getResponseBuffer(1);
        temp=converFloat.FlaotData;
        converFloat.convData[0] =node[i].getResponseBuffer(2);
        converFloat.convData[1] = node[i].getResponseBuffer(3);        
        press = converFloat.FlaotData;
      

        // us_press =  (int)press;

        temperatureADC = node[i].getResponseBuffer(4);
        pressureADC = node[i].getResponseBuffer(5);
        
        RtcDateTime now = Rtc.GetDateTime();

        #ifdef PrintCustomeLOG
          Serial.print("Slave ");
          Serial.print(i + 1);
          Serial.print(" Temperature: ");
          Serial.print(temp);
          Serial.print(" TemperatureADC: ");
          Serial.print(temperatureADC);
          Serial.print("  Pressure: ");
          Serial.print(press);
          Serial.print("  PressureADC: ");
          Serial.print(pressureADC);

          Serial.print(" ");
          printDateTime(now);
          Serial.println();
        #endif

          char modbusData[100];
          sprintf(modbusData, "%d,%02u/%02u/%04u,%02u:%02u:%02u, %.2f, %.2f \n", (i + 1), now.Month(), now.Day(), now.Year(), now.Hour(), now.Minute(), now.Second(), temp, press);
          saveDataToSD(modbusData, USER_LOG);

          sprintf(modbusData, "%d,%02u/%02u/%04u,%02u:%02u:%02u, %.2f, %d, %.2f, %d \n", (i + 1), now.Month(), now.Day(), now.Year(), now.Hour(), now.Minute(), now.Second(), temp,temperatureADC,press,pressureADC);
          saveDataToSD(modbusData, CALIBRATION_LOG);

        }

        else
        {
             RtcDateTime now = Rtc.GetDateTime();

            #ifdef PrintCustomeLOG
              Serial.print("Slave ");
              Serial.print(i + 1);
              Serial.print(" - Modbus error:  ");
              printDateTime(now);
              Serial.println();
            #endif

            char modbusData[100];
            sprintf(modbusData, "%d,%02u/%02u/%04u,%02u:%02u:%02u,Error \n", (i + 1), now.Month(), now.Day(), now.Year(), now.Hour(), now.Minute(), now.Second());

            saveDataToSD(modbusData, USER_LOG);
            saveDataToSD(modbusData, CALIBRATION_LOG);

        }

      }
      // else
      // {
      //   result = 1;
      //   retrycounts = 3;
      //   do{
      //   result = node[i].readHoldingRegisters(0x000, 6);

      //   }while((result != node[i].ku8MBSuccess) && (retrycounts--));
       
      //   if (result == node[i].ku8MBSuccess)
      //   {
      //       // temp = node[i].getResponseBuffer(0);
      //     converFloat.convData[0] = node[i].getResponseBuffer(0);
      //     converFloat.convData[1] = node[i].getResponseBuffer(1);
      //     temperatureADC = node[i].getResponseBuffer(4);

      //     temp=converFloat.FlaotData;
        
      //     RtcDateTime now = Rtc.GetDateTime();

      //     #ifdef PrintCustomeLOG
      //       Serial.print("Slave ");
      //       Serial.print(i + 1);
      //       Serial.print(" Temperature: ");
      //       Serial.print(temp);
      //       Serial.print(" TemperatureADC : ");
      //       Serial.print(temperatureADC);

      //       Serial.print(" ");
      //       printDateTime(now);
      //       Serial.println();
      //     #endif

      //     char modbusData[100];
      //     sprintf(modbusData, "%d,%02u/%02u/%04u,%02u:%02u:%02u, %.2f \n", (i + 1), now.Month(), now.Day(), now.Year(), now.Hour(), now.Minute(), now.Second(), temp);
      //     saveDataToSD(modbusData, USER_LOG);

      //     sprintf(modbusData, "%d,%02u/%02u/%04u,%02u:%02u:%02u, %.2f, %d \n", (i + 1), now.Month(), now.Day(), now.Year(), now.Hour(), now.Minute(), now.Second(), temp,temperatureADC);
      //     saveDataToSD(modbusData, CALIBRATION_LOG);
      //   }
      //   else
      //   {
      //       RtcDateTime now = Rtc.GetDateTime();
      //       #ifdef PrintCustomeLOG
      //         Serial.print("Slave ");
      //         Serial.print(i + 1);
      //         Serial.print(" - Modbus error:  ");
      //         printDateTime(now);
      //         Serial.println();
      //       #endif

      //       char modbusData[100];
      //       sprintf(modbusData, "%d,%02u/%02u/%04u,%02u:%02u:%02u,Error \n", (i + 1), now.Month(), now.Day(), now.Year(), now.Hour(), now.Minute(), now.Second());

      //       saveDataToSD(modbusData, USER_LOG);
      //       saveDataToSD(modbusData, CALIBRATION_LOG);
      //   }
      // }
       DataLogMinute = currentMinute + DataLogMinuteFrequency;

    //    if ((j + 1) % 3 == 0) {
    //     delay(5000);
    // }
    
    }
    digitalWrite(11,LOW);
    
    delay(5000);
    }
      // delay(120000);

  }
}

int parseAndStoreIntegers(String inputString) {
  int numCount = 0;
  char* strtokBuffer;
  char* token = strtok_r(const_cast<char*>(inputString.c_str()), ",", &strtokBuffer);

  while (token != NULL && numCount < MAX_NUMBERS) {
    numbers[numCount++] = atoi(token);
    token = strtok_r(NULL, ",", &strtokBuffer);
  }

  return numCount;
}

void displayStoredNumbers() {
  Serial.print("Stored Numbers: ");
  for (int i = 0; i < numCount; i++) {
    Serial.print(numbers[i]);
    if (i < numCount - 1) {
      Serial.print(", ");
    }
  }
  Serial.println();
}



short readSerialData(void)
{
  short rxDatacount;
  rxDatacount = Serial.readBytes(receivedData, 10);  // Use readBytes to ensure correct type

  if (rxDatacount >= 10) {
    uint16_t CRC = checksumCalculator((uint8_t*)receivedData, 8);  // Convert to uint8_t* here
    uint8_t SUM1 = CRC;
    uint8_t SUM2 = (CRC >> 8);

    if ((SUM1 == receivedData[8]) && (SUM2 == receivedData[9])) {

      int year, month, day, hour, minute, second;

      hour = receivedData[0];
      minute = receivedData[1];
      second = receivedData[2];
      day = receivedData[3];
      month = receivedData[4];
      year = receivedData[5];
      RtcDateTime receivedDateTime(year, month, day, hour, minute, second);
      Rtc.SetDateTime(receivedDateTime);
      int LogFreq =  (receivedData[6] | (receivedData[7] << 8));
      FlashWrite<int>(FLASH_Address, LogFreq);
      Serial.print("OK");
    } 

    else {
      Serial.print("FAIL");
    }
    Serial.flush();
    return rxDatacount;
  }
  return 0;
}

void printDateTime(const RtcDateTime& dt)
{
  char datestring[20];
  snprintf_P(datestring, sizeof(datestring),
  PSTR("%02u/%02u/%04u,%02u:%02u:%02u"),dt.Month(), dt.Day(), dt.Year(),dt.Hour(), dt.Minute(), dt.Second());
  Serial.print(datestring);
}


unsigned long int convertDateToDays(struct date date){
    unsigned long int totalDays;
    int numLeap = 0;
    int monthsAddFromYearStart[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int i;

    // First, calculate the number of leap year since year one (not including date's year).
      numLeap = date.year / 4;
      if(( date.year % 4) == 0)
      {
          if(date.month > 2)
          {
             numLeap++;
          }
      }
      else
      {
        numLeap++;
      }

    // If it is a leap year, as of March there has been an extra day.

    // (Year - 1) * 356 + a day per leap year + days totaling the previous months + days of this month
    if(date.year !=0)
    {
        totalDays = (date.year - 1) * 365 + numLeap + monthsAddFromYearStart[date.month - 1] + date.day;
    }
    else
    {
      totalDays =  numLeap + monthsAddFromYearStart[date.month - 1] + date.day;
    }
    return totalDays;
}

