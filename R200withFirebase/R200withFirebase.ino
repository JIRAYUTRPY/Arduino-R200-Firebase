#include "ESP8266WiFi.h"
#include "FirebaseESP8266.h"
#include "ESP8266HTTPClient.h"
#include "EEPROM.h"
#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
FirebaseData firebaseDataReader;
FirebaseData firebaseSettingReader;
FirebaseJson firebaseJsonData;
///****** R200 Commamnd *******///
//*Inventory
unsigned char ReadMulti[10] =   {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
//*Mode
unsigned char DenseMode[8] =     {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};
//*RFCH
unsigned char RFCH90225MHz[8] = {0XAA,0X00,0XAB,0X00,0X01,0X00,0XAC,0XDD};
//*Region
unsigned char USRegion[8] =     {0XAA,0X00,0X07,0X00,0X01,0X02,0X0A,0XDD};
//*PA Power
unsigned char PA185DBM[9] =     {0XAA,0X00,0XB6,0X00,0X02,0X07,0XA3,0XF9,0XDD};
///****************************///

//********** Env Parameter **********//
unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;
String path = "records/";
String lastEpc = "";
String state = "init";
String mechineNumber = "";
bool firebaseError = false;
bool display = false;
int address = 0;
int lastEpcNumber = 1;
int currentFloor = 1;
int bedNumber = 1;
//***********************************//

void setup() {
  EEPROM.begin(512);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(112500);
  while(1){
    if(Serial.available() > 0){
        break;
    }else{
      Serial.println("Please Enter any character for next step");
    }
  }
  Serial.println("R200 with Firebase - Bed tracking.");
  Serial.println("************* WIFI Sys Init **************");
  Serial.print("Scaning..");
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  Serial.println("************* Firebase Interface Init **************");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  String readMechine = EEPROM_read(address, 10);
  bool readEEPROMFinish = true;
  if(readMechine.length() > 1){
      while(readEEPROMFinish){
          if(EEPROM_read(address, 1) != 0){
            mechineNumber = mechineNumber + EEPROM_read(address, 1);
            address++;
          }else{
            readEEPROMFinish = false;
          }
      }
  }else{
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
      Serial.println("Mechine Number on EEPROM : " + EEPROM_read(address, lenMsg));
    }else{
      firebaseError = true;
      while(firebaseError){
        Serial.println("Firebase Error: " + firebaseSettingReader.errorReason() + " | please reset broad");
        delay(1000);
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
  Serial.println("Controller : Buad Rate '115200'");  
  Serial.println("R200       : Mode 'Dense'");
  Serial.write(DenseMode,8);
  Serial.println("           : Region 'US'");
  Serial.write(USRegion,8);
  Serial.println("           : PA Power '18.5dBm'");
  Serial.write(PA185DBM,8);
  Serial.println("           : RFCH '902.25MHz'");
  Serial.write(RFCH90225MHz,8);
  Serial.print("             : Command -> ReadMulti '");
  for(int x = 0 ; x < 10 ; x++){
    Serial.print(ReadMulti[x], HEX);
    Serial.print(" ");
    if(x == 9){
      Serial.println("'");
    }
  }
  Serial.println();
}

void loop() {
  if(state == "init"){
    changeState();
  }else if(state == "d"){
    lastEpc = read();
    if(lastEpc != "wait" && lastEpc != ""){
      if(Firebase.ready()){
        putEpc(lastEpc);
      }else{
        Serial.println("Firebase not ready");
        Serial.println();
        state = "init";
      }          
    }
    changeState();
  }else if(state == "r"){    
    lastEpc = read();
    if(lastEpc != "wait" && lastEpc != ""){
      if(Firebase.ready()){
        updateEpc(lastEpc);
      }else{
        Serial.println("Firebase not ready");
        Serial.println();
        state = "init";
      }          
    }
  }
}

int wifiScan() {
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

    // Print unsorted scan results
    for (int8_t i = 0; i < scanResult; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);

      // get extra info
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

  // Wait a bit before scanning again
  delay(5000);
  return scanResult;
}

String EEPROM_read(int index, int length){
  String text = "";
  char ch = 1;
  for(int i = index ; (i< (index + length)) && ch; ++i ){
    text.concat(ch);
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

void changeState(){
  if(state == "init" && display == true){
    Serial.println("Current State : init | Select State working: db(d) or read(r)");
    Serial.println();
    display = false;
  }else if(state == "d" && display == true){
    Serial.println("Current State :  db  | Select State working: read(r)");
    Serial.println();
    display = false;
  }else{
    Serial.print("Please enter (d) or (r)");
    Serial.println();
    state = "init";
  }
  if(Serial.available() > 0){
      state = Serial.read();   
      display = true;       
  }
}


String read(){
  timeSec ++ ;
    if(timeSec >= 50000){
      timemin ++;
      timeSec = 0;
      if(timemin >= 20){
        timemin = 0;
        Serial.write(ReadMulti,10);
      }
    }
  return multipleReading();
}

String multipleReading(){
  String epc = "";
  if(Serial.available() > 0){
    incomedate = Serial.read();
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
           Serial.println(incomedate, HEX);
        }
       }
       else if((dataAdd >= 9)&(dataAdd <= 20)){
        if(dataAdd == 9){
          Serial.print("EPC:");
          epc = String(incomedate, HEX);
        }else{
          Serial.print(incomedate, HEX);
          epc = epc + " " + String(incomedate, HEX);
        }
       }
       else if(dataAdd >= 21){
        Serial.println(" "); 
        dataAdd= 0;
        parState = 0;
        codeState = 0;
        return epc;
        }
    }
     else{
      dataAdd= 0;
      parState = 0;
      codeState = 0;
      return "wait";
    }
  }
  return "wait";
}
