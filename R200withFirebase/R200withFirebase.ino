#include "ESP8266WiFi.h"
#include "FirebaseESP8266.h"
#include "ESP8266HTTPClient.h"
#include "EEPROM.h"
#include "SoftwareSerial.h"

FirebaseData firebaseDataReader;
FirebaseData firebaseSettingReader;
FirebaseJson firebaseJsonData;
SoftwareSerial R200Serial(D3, D4);
#define EEPROM_SIZE 512
///****** R200 Commamnd *******///
//*Inventory
unsigned char ReadMulti[10]   =   {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
unsigned char stopRead[7]     =   {0XAA,0x00,0X28,0X00,0X00,0X28,0XDD};
//*Mode
unsigned char DenseMode[8]    =   {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};
//*RFCH
unsigned char RFCH90225MHz[8] =   {0XAA,0X00,0XAB,0X00,0X01,0X00,0XAC,0XDD};
//*Region
unsigned char USRegion[8]     =   {0XAA,0X00,0X07,0X00,0X01,0X02,0X0A,0XDD};
//*PA Power
unsigned char PA185DBM[9]     =   {0XAA,0X00,0XB6,0X00,0X02,0X07,0XA3,0XF9,0XDD};
///****************************///

//********** Env Parameter **********//
unsigned char lastEpcArr[10] =   {0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00};
unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int firebaseSec = 0;
unsigned int firebaseMin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;
int stackArrayCheck = 10;
String path = "records/";
String lastEpc = "";
String state = "i";
String mechineNumber = "";
bool firebaseError = false;
bool display = true;
int address = 0;
int lastEpcNumber = 1;
int currentFloor = 1;
int bedNumber = 1;
String WIFI_SSID = "";
String WIFI_PASSWORD = "";
String FIREBASE_HOST = "";
String FIREBASE_AUTH = "";
bool sameEpc = false;
//***********************************//

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D3, INPUT);
  pinMode(D4, OUTPUT);
  Serial.begin(115200);
  R200Serial.begin(115200);
  Serial.println("R200 with Firebase - Bed tracking.");
  Serial.println("************* WIFI Sys Init **************");
  Serial.print("Scaning..");
  wifiScan();
  Serial.println("Select SSID");
  while(1){    
    if(Serial.available() > 0){
      WIFI_SSID = Serial.readString();
      WIFI_SSID.trim();
      Serial.println("SSID : " + WIFI_SSID);
      break;
    }
  }
  Serial.println("Enter password : ");
  while(1){    
    if(Serial.available() > 0){
      WIFI_PASSWORD = Serial.readString();
      WIFI_PASSWORD.trim();
      break;
    }
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { 
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("Enter Firebase HOST : ");
  while(1){    
    if(Serial.available() > 0){
      FIREBASE_HOST = Serial.readString();
      FIREBASE_HOST.trim();
      Serial.println("FIREBASE_HOST: " + FIREBASE_HOST);
      break;
    }
  }
  Serial.println("Enter Firebase AUTH : ");
  while(1){    
    if(Serial.available() > 0){
      FIREBASE_AUTH = Serial.readString();
      FIREBASE_AUTH.trim();
      Serial.println("FIREBASE_AUTH: " + FIREBASE_AUTH);
      break;
    }
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  if(EEPROM.read(address)==0){
    FirebaseJson firebaseSettingJsonData;
    firebaseSettingJsonData.set("buad rate", 115200);
    firebaseSettingJsonData.set("region", "US");
    firebaseSettingJsonData.set("pa pwer", "18.5dBM");
    firebaseSettingJsonData.set("RFCH", "902.25MHz");
    firebaseSettingJsonData.set("EEPROM address", address);
    firebaseSettingJsonData.set("floor" ,1);
    if(Firebase.pushJSON(firebaseSettingReader, "/setting", firebaseSettingJsonData)){
      mechineNumber = firebaseSettingReader.pushName();
      int lenMsg = EEPROM_write(address, mechineNumber);
      Serial.println("Create new Mechine");
      Serial.println("Mechine Number : " + EEPROM_read(address, lenMsg));
    }else{
      firebaseError = true;
      while(firebaseError){
        Serial.println("Firebase Error: " + firebaseSettingReader.errorReason() + " | please reset broad");
        delay(1000);
      }
    }
  }else{
    bool readEEPROMFinish = true;
    while(readEEPROMFinish){
      mechineNumber = mechineNumber + EEPROM_read(address, 1);
      address++;
      if(EEPROM_read(address, 1) == 0){
        Serial.println("Mechine Installed");
        Serial.println("Mechine Number : " + mechineNumber);
        break;
      }
    }
  }  
  if (!Firebase.beginStream(firebaseSettingReader, "/setting")){
    Serial.println("Could not begin stream");
    Serial.println("REASON: " + firebaseSettingReader.errorReason());
    Serial.println();
  }else{
    Serial.println("Firebase Streaming on setting path");
    Serial.println();
  }
  Serial.println("");
  Serial.println("***************  Default setting  ***************");
  Serial.println("Controller setting");
  Serial.println("Buad Rate : '115200'");
  Serial.println("R200 setting");
  Serial.println("Mode : 'Dense'");
  R200Serial.println("Region : 'US'");
  R200Serial.println("PA Power : '18.5dBm'");
  Serial.println("RFCH : '902.25MHz'"); 
  Serial.print("Command : ReadMulti '");
  for(int x = 0 ; x < 10 ; x++){
    Serial.print(ReadMulti[x], HEX);
    Serial.print(" ");
    if(x == 9){
      Serial.println("'");
    }
  }
  R200Serial.write(DenseMode,8);
  R200Serial.write(USRegion,8);
  R200Serial.write(PA185DBM,8);
  R200Serial.write(RFCH90225MHz,8);
  Serial.println("Setup Finish");
}

void loop() {  
  changeState(); 
  databaseState();
  readState();
}

/*********************************************** FIREBASE ************************************************/

void putEpc(String data){
  FirebaseJson epcJsonData;
  epcJsonData.set("tag number", lastEpcNumber);
  epcJsonData.set("current floor", 1);
  epcJsonData.set("status", false);
  epcJsonData.set("bed number", bedNumber);
  if(Firebase.setJSON(firebaseDataReader, path + data , epcJsonData)){
    Serial.println("Firebase Created");
    Serial.println();
    lastEpcNumber ++;
    if(lastEpcNumber % 2 == 0){
      bedNumber++;
    }
  }else{
    Serial.println("Cannot Created");
    Serial.println("Firebase Reason: " + firebaseDataReader.errorReason());
    Serial.println();
  }
}

void updateEpc(String data){
  FirebaseJson epcJsonData;
  epcJsonData.set("current floor", currentFloor);
  epcJsonData.set("status", !Firebase.getBool(firebaseDataReader, path + data));
  if(Firebase.updateNode(firebaseDataReader, path + data, epcJsonData)){
    Serial.println("Firebase Updated");
    Serial.println();
  }else{
    Serial.println("Cannot Updated");
    Serial.println("Firebase Reason: " + firebaseDataReader.errorReason());
    Serial.println();
  }  
}

/*********************************************** FIREBASE ************************************************/

/******************************************* STATE MANAGEMENT ********************************************/

void changeState(){
  if(state == "i" && display == true){
    Serial.println("Current State : init");
    Serial.println();
    display = false;
  }else if(state == "d" && display == true){
    Serial.println("Current State :  insert data to database (db)");
    Serial.println();
    display = false;
  }else if(state == "r" && display == true){
    Serial.println("Current State :  read");
    Serial.println();
    display = false;
  }else if(state != "d" && state != "i" && state != "r" && display == true){
    Serial.print("Back to init state");
    Serial.println();
    state = "i";
  }
  if(Serial.available() > 0){
      state = Serial.readString();
      state.trim();   
      display = true;   
  }
}

void databaseState(){
  readMultiCommand();
  if(state == "d"){
    read();
  }
}

void readState(){
  readMultiCommand();
  if(state == "r"){
    read();
  }
}

/******************************************* STATE MANAGEMENT ********************************************/

/************************************************* WIFI **************************************************/

void wifiScan() {
  String ssid;
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t *bssid;
  int32_t channel;
  bool hidden;
  int scanResult;

  Serial.println(F("Starting WiFi scan..."));
  scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  if (scanResult == 0) {
    Serial.println(F("No networks found"));
  } else if (scanResult > 0) {
    Serial.printf(PSTR("%d networks found:\n"), scanResult);
    for (int8_t i = 0; i < scanResult; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);
      const bss_info *bssInfo = WiFi.getScanInfoByIndex(i);
      String phyMode;
      const char *wps = "";
      if (bssInfo) {
        phyMode.reserve(12);
        phyMode = F("802.11");
        String slash;
        if (bssInfo->phy_11b) {
          phyMode += 'b';
          slash = '/';
        }
        if (bssInfo->phy_11g) {
          phyMode += slash + 'g';
          slash = '/';
        }
        if (bssInfo->phy_11n) {
          phyMode += slash + 'n';
        }
        if (bssInfo->wps) {
          wps = PSTR("WPS");
        }
      }
      Serial.printf(PSTR("  %02d: [CH %02d] [%02X:%02X:%02X:%02X:%02X:%02X] %ddBm %c %c %-11s %3S %s\n"), i, channel, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], rssi, (encryptionType == ENC_TYPE_NONE) ? ' ' : '*', hidden ? 'H' : 'V', phyMode.c_str(), wps, ssid.c_str());
      yield();
    }
  } else {
    Serial.printf(PSTR("WiFi scan error %d"), scanResult);
  }
}

/************************************************* WIFI **************************************************/

/************************************************ EEPROM *************************************************/

String EEPROM_read(int index, int length){
  String text = "";
  char ch = 1;
  for(int i = index ; (i< (index + length)) && ch; ++i ){
    if (ch = EEPROM.read(i)) {
      text.concat(ch);
    }
  }
  return text;
}

int EEPROM_write(int index, String text){
  for(int i = index ; i < text.length() + index ; i++){
    EEPROM.write(i, text[i - index]);    
  }
  EEPROM.write(index + text.length(), 0);
  EEPROM.commit();
  return text.length() + 1; 
}

/************************************************ EEPROM *************************************************/

/************************************************* R200 **************************************************/

void readMultiCommand(){
  if(millis() - timeSec > 1000){
    R200Serial.write(ReadMulti,10);
    timeSec = millis();
  }
}

void read(){
  timeSec ++ ;
  if(timeSec >= 50000){
    timemin ++;
    timeSec = 0;
    if(timemin >= 20){
      timemin = 0;
      R200Serial.write(ReadMulti,10);
    }
  }
  multipleReading();
}

void multipleReading(){
  String epc = "";
  if(R200Serial.available() > 0){
    incomedate = R200Serial.read();
    if((incomedate == 0x02)&(parState == 0))
    {
      parState = 1;
    }
    else if((parState == 1)&(incomedate == 0x22)&(codeState == 0)){  
        codeState = 1;
        dataAdd = 3;
    }
    else if(codeState == 1){
      dataAdd ++;
      if(dataAdd == 6)
      {
        // Serial.print("RSSI:"); 
        // Serial.println(incomedate, HEX); 
        }
       else if((dataAdd == 7)|(dataAdd == 8)){
        if(dataAdd == 7){
          // Serial.print("PC:"); 
          // Serial.print(incomedate, HEX);
        }
        else {
          //  Serial.println(incomedate, HEX);
        }
       }
       else if((dataAdd >= 9)&(dataAdd <= 20)){
         if(dataAdd > 10){
           if(lastEpcArr[dataAdd - 9] == incomedate){
            // lastEpcArr[dataAdd - 9] = incomedate;
            stackArrayCheck--;
            // Serial.printf("DEBUG STACK ARRAY : %d \n", stackArrayCheck);
          }else{
            lastEpcArr[dataAdd - 9] = incomedate;
          }
        }
       }
       else if(dataAdd >= 21){
        if(stackArrayCheck == 0){
          sameEpc = true;
        }else{
          Serial.print("EPC: ");
          for(int x = 12 ; x != 0 ; x--){
            Serial.print(lastEpcArr[x], HEX);
            Serial.print(" ");
          }
          Serial.println(" ");
          sameEpc = false;
        }
        stackArrayCheck = 12;
        dataAdd= 0;
        parState = 0;
        codeState = 0;
        }
    }
     else{
      dataAdd= 0;
      parState = 0;
      codeState = 0;
    }
  }
}

/************************************************* R200 **************************************************/
