/*
  Bed tracking project - src.
  Created by JIRAYUT R., Dec 26, 2023.
  Released into the public domain.
*/
//* FOR RESET EEPROM
// if (true) {
//   for (int i = 0; i < 512; i++) {
//     EEPROM.write(i, 0);
//   }
//   EEPROM.commit();
//   delay(500);
// }
#include "ESP8266WiFi.h"
#include "FirebaseESP8266.h"
#include "ESP8266HTTPClient.h"
#include "SoftwareSerial.h"
#include "EEPROM.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#define EEPROM_SIZE 512
#define pingPin     D15
#define inPin       D14

FirebaseData firebaseDataReader;
FirebaseData firebaseSettingReader;
FirebaseData firebaseEnvConfig;
FirebaseJson firebaseJsonData;
FirebaseJsonArray firebaseArrayData;
SoftwareSerial R200Serial(D8, D9);

//********** Env Parameter **********//
byte rssi              =    0x00;
byte pc[2]             =   {0x00,0x00};
byte epc[11]           =   {0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00};
//*Inventory
byte single_read[9]    =   {0XAA,0XAA,0X0E,0X00,0X02,0X10,0X08,0X28,0XDD};
byte multi_read[10]    =   {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
byte stop_read[7]      =   {0XAA,0x00,0X28,0X00,0X00,0X28,0XDD};
//*Mode
byte dense_mode[8]     =   {0XAA,0X00,0XF5,0X00,0X01,0X01,0XF7,0XDD};
//*RFCH
byte rfch_90225_mhz[8] =   {0XAA,0X00,0XAB,0X00,0X01,0X00,0XAC,0XDD};
//*Region
byte us_region[8]      =   {0XAA,0X00,0X07,0X00,0X01,0X02,0X0A,0XDD};
//*PA Power
byte PA185DBM[9]       =   {0XAA,0X00,0XB6,0X00,0X02,0X07,0XA3,0XF9,0XDD};
unsigned int timeSec     = 0;
unsigned int timemin     = 0;
unsigned int firebaseSec = 0;
unsigned int firebaseMin = 0;
int address              = 0;
int lastEpcNumber        = 1;
int currentFloor         = 1;
int bedNumber            = 1;
int hourNow, minuteNow, secondNow;
String path              = "records/";
String mechineNumber     = "";
String WIFI_SSID         = "2.4F";
String WIFI_PASSWORD     = "abcdefgh";
String FIREBASE_HOST     = "https://bed-tracking-90a24-default-rtdb.asia-southeast1.firebasedatabase.app/";
String FIREBASE_AUTH     = "zzggUuynwOOo9LnLkEXFqP3fVnqZb0PGXE7GfqOD";
bool firebaseError       = false;
bool display             = true;
String state             = "";
const long offsetTime    = 0;
int distanceReading      = 0;
int EXECUTE_COMMAND_TIMER= 0;
int WAITING_COMMAND_TIMER= 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", offsetTime);
//***********************************//

//* STATE MANAGE MENT **//
#define INIT       0
#define READ_STATE 1
#define DB_STATE   2
#define TEST_STATE 3

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D8, INPUT);
  pinMode(D9, OUTPUT);
  Serial.begin(115200);
  R200Serial.begin(115200);
  delay(5000);
  Serial.println("R200 with Firebase - Bed tracking.");
  Serial.println("************* WIFI Sys Init **************");
  // Serial.print("Scaning..");
  // wifiScan();
  // Serial.println("Select SSID");
  // while(1){    
  //   if(Serial.available() > 0){
  //     WIFI_SSID = Serial.readString();
  //     WIFI_SSID.trim();
  //     Serial.println("SSID : " + WIFI_SSID);
  //     break;
  //   }
  // }
  // Serial.println("Enter password : ");
  // ESP.wdtEnable(10000);
  // while(1){    
  //   if(Serial.available() > 0){
  //     WIFI_PASSWORD = Serial.readString();
  //     WIFI_PASSWORD.trim();
  //     break;
  //   }
  // }
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
  timeClient.begin();
  // Serial.println("Enter Firebase HOST : ");
  // while(1){    
  //   if(Serial.available() > 0){
  //     FIREBASE_HOST = Serial.readString();
  //     FIREBASE_HOST.trim();
  //     Serial.println("FIREBASE_HOST: " + FIREBASE_HOST);
  //     break;
  //   }
  // }
  // Serial.println("Enter Firebase AUTH : ");
  // while(1){    
  //   if(Serial.available() > 0){
  //     FIREBASE_AUTH = Serial.readString();
  //     FIREBASE_AUTH.trim();
  //     Serial.println("FIREBASE_AUTH: " + FIREBASE_AUTH);
  //     break;
  //   }
  // }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  if(EEPROM.read(address)>0){
    setup_mechine(false);
    while(1){
      mechineNumber = mechineNumber + EEPROM_read(address, 1);
      address++;
      if(EEPROM_read(address, 1) == 0){
        Serial.println("Mechine Installed");
        Serial.println("Mechine Number : " + mechineNumber);
        break;
      }
    }
  }else{
    setup_mechine(true);
  }
  delay(5000);
  Serial.println("***************  Default setting  ***************");
  Serial.println("---------------Controller setting----------------");
  Serial.println("Buad Rate : '115200'");
  Serial.println("-------------------R200 setting------------------");
	Serial.println("MODE    : DENSE MODE");
	Serial.println("REGION  : US");
	Serial.println("POWER   : 185 dDm");
	Serial.println("RFCH    : 902.25 MHz");
  Serial.println("Command : ReadMulti");
  Serial.print("'");
  for(int x = 0 ; x < 10 ; x++){
    Serial.print(multi_read[x], HEX);
    Serial.print(" ");
    if(x == 9){
      Serial.println("'");
    }
  }
	R200Serial.write(dense_mode,8);
	R200Serial.write(us_region,8);
	R200Serial.write(PA185DBM,8);
	R200Serial.write(rfch_90225_mhz,8);
  ESP.wdtDisable();
  ESP.wdtEnable(WDTO_2S);
  Serial.println("Setup Finish");
}

int int_state = INIT;
void loop() { 
  change_state(); 
  req_time(false);
  ping_firebase();
  switch(int_state){
    case INIT :
      break;
    case READ_STATE :
      read_state();
      break;
    case DB_STATE :
      database_state();
      break;
    case TEST_STATE :
      test_state();
      break;
  }
}

/***********************************************   NTP    ************************************************/

unsigned long req_time_timer = millis();
unsigned long epochTime      = 0;
void req_time(bool display){
  if(millis() - req_time_timer > 1000){
    timeClient.update();    
    secondNow = timeClient.getSeconds();
    minuteNow = timeClient.getMinutes();
    hourNow   = timeClient.getHours();
    epochTime = timeClient.getEpochTime();
    // print_epc_char();
    // print_epc();
    if(display){
      Serial.print(hourNow);
      Serial.print(":");
      Serial.print(minuteNow);
      Serial.print(":");
      Serial.println(secondNow);     
      Serial.print("Epoch Time : ");
      Serial.println(timeClient.getEpochTime()); 
    }
    req_time_timer = millis();
  }
}


/***********************************************   NTP    ************************************************/

/*********************************************** FIREBASE ************************************************/

unsigned long pingTimer = 0;
void ping_firebase(){
  if(millis() - pingTimer > 10000){
    timeClient.update();
    long ping = timeClient.getEpochTime();
    Firebase.setInt(firebaseSettingReader, "/setting/" + mechineNumber + "/LastTimePing", ping);
    pingTimer = millis();
  }
}

void setup_mechine(bool check){
  timeClient.update();
  long ping = timeClient.getEpochTime();
  FirebaseJson firebaseSettingJsonData;
  firebaseSettingJsonData.set("Floor" ,1);
  firebaseSettingJsonData.set("BuadRate", 115200);
  firebaseSettingJsonData.set("Region", "US");
  firebaseSettingJsonData.set("PA", "18.5dBM");
  firebaseSettingJsonData.set("RFCH", "902.25MHz");
  firebaseSettingJsonData.set("EEPROM address", address);
  firebaseSettingJsonData.set("LastTimePing", ping);
  if(check){
    if(Firebase.pushJSON(firebaseSettingReader, "/setting", firebaseSettingJsonData)){
      mechineNumber = firebaseSettingReader.pushName();
      int lenMsg = EEPROM_write(address, mechineNumber);
      Serial.println("Create new Mechine");
      Serial.println("Mechine Number : " + EEPROM_read(address, lenMsg));
    }else{
      while(1){
        Serial.println("Firebase Error: " + firebaseSettingReader.errorReason() + " | please reset broad");
        delay(1000);
      }
    }
  }
  if (!Firebase.beginStream(firebaseSettingReader, "/setting")){
    Serial.println("Could not begin stream on setting path");
    Serial.println("REASON: " + firebaseSettingReader.errorReason());
    Serial.println();
  }else{
    Serial.println("Firebase Streaming on setting path");
    Serial.println();
  }
  if (!Firebase.beginStream(firebaseEnvConfig, "/envConfig")){
    Serial.println("Could not begin stream on envConfig path");
    Serial.println("REASON: " + firebaseSettingReader.errorReason());
    Serial.println();
  }else{
    Serial.println("Firebase Streaming on envConfig path");
    Serial.println();
    if(Firebase.getInt(firebaseEnvConfig, "/envConfig/READING_DISTANCE")){
      if(firebaseEnvConfig.dataType() == "int"){
        distanceReading = firebaseEnvConfig.intData();
      }
    }
    if(Firebase.getInt(firebaseEnvConfig, "/envConfig/WAITING_COMMAND_TIMER")){
      if(firebaseEnvConfig.dataType() == "int"){
        WAITING_COMMAND_TIMER = firebaseEnvConfig.intData();
      }
    }
    if(Firebase.getInt(firebaseEnvConfig, "/envConfig/EXECUTE_COMMAND_TIMER")){
      if(firebaseEnvConfig.dataType() == "int"){
        EXECUTE_COMMAND_TIMER = firebaseEnvConfig.intData();
      }
    }
  }
}

void insert_firebase(String dirpath, String data){
  FirebaseJson epcJsonData;
  epcJsonData.set("tag number", lastEpcNumber);
  epcJsonData.set("current floor", 1);
  epcJsonData.set("status", false);
  epcJsonData.set("bed number", bedNumber);
  if(Firebase.setJSON(firebaseDataReader, dirpath + data , epcJsonData)){
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

void update_firebase(String data){
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

void create_array(){
  for(int x ; x != 11 ; x++){
    firebaseArrayData.add(epc[x]);    
  }  
}

/*********************************************** FIREBASE ************************************************/

/********************************************** ULTRASONIC ***********************************************/
long duration, cm;
unsigned long ultra_timer = millis();
void read_ultra(){
  if(millis() - ultra_timer > 100){
    pinMode(pingPin, OUTPUT);
    digitalWrite(pingPin, LOW);
    delayMicroseconds(2);
    digitalWrite(pingPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(pingPin, LOW);
    pinMode(inPin, INPUT);
    duration = pulseIn(inPin, HIGH);
    cm = microsecondsToCentimeters(duration);
    ultra_timer = millis();
  }
}

long microsecondsToCentimeters(long microseconds){
  return microseconds / 29 / 2;
}

/********************************************** ULTRASONIC ***********************************************/

/******************************************* STATE MANAGEMENT ********************************************/
bool executed = false;

void change_state(){
  if(state == "i" && display == true){
    Serial.println("Current State : init");
    Serial.println();
    int_state = INIT;
    display = false;
  }else if(state == "r" && display == true){
    Serial.println("Current State :  read");
    Serial.println();
    int_state = READ_STATE;
    display = false;
  }else if(state == "d" && display == true){
    Serial.println("Current State :  insert data to database (db)");
    Serial.println();
    int_state = DB_STATE;
    display = false;
  }else if(state != "r" && state != "d" && state != "i" && display == true){
    Serial.print("Back to init state");
    Serial.println();
    state = "i";
  }
  if(Serial.available() > 0){
      state = Serial.readString();
      state.trim();
      Serial.print("STATE SELECTED : ");
      Serial.println(state);
      // state.trim();   
      display = true;  
  }
}

bool finish_read = true;
unsigned long insert_timer = 0;
void database_state(){
  read_ultra();
  execute_command_and_recive(false, (cm < 10));
  if(millis() - insert_timer > 1000){
    Serial.println(string_epc());
    insert_timer = millis();
  }
}

void test_state(){
  // read_ultra();
  // execute_command_and_recive(true, (cm < 200));
}

void read_state(){
  read_ultra();
  execute_command_and_recive(false, (cm < distanceReading));
}

/******************************************* STATE MANAGEMENT ********************************************/

/************************************************* R200 **************************************************/
unsigned long dataAdd        = 0;
unsigned long incomedate     = 0;
unsigned long parState       = 0;
unsigned long codeState      = 0;
unsigned long command_timmer = 0;
bool finisher               = false;

#define INIT_COMMAND 0
#define EXECUTE_COMMAND 1
#define WAITING_COMMAND 2
#define FINISH_COMMAND  3
unsigned int timer1 = millis();
int sub_state = INIT_COMMAND;

void execute_command_and_recive(bool display , bool cm_checker){
  read_response();
  switch (sub_state){
    case INIT_COMMAND :
      sub_state = EXECUTE_COMMAND;
      break;
    case EXECUTE_COMMAND :
      if(millis() - timer1 < EXECUTE_COMMAND_TIMER && cm_checker){
        R200Serial.write(multi_read,10);
      }
      if(millis() - timer1 > EXECUTE_COMMAND_TIMER && cm_checker){
        timer1 = millis();
        sub_state = WAITING_COMMAND;
      }
      break;
    case WAITING_COMMAND :
      if(millis() - timer1 < WAITING_COMMAND_TIMER && !cm_checker){
        R200Serial.write(stop_read,7);
      }
      if(millis() - timer1 > WAITING_COMMAND_TIMER && cm_checker){
        timer1 = millis();
        sub_state = FINISH_COMMAND;
      }
      break;
    case FINISH_COMMAND :
      timer1 = millis();
      if(display){
        print_epc();
      }
      sub_state = EXECUTE_COMMAND;
      break;      
  }
}

void execute_single_read_command(){
  if(millis() - command_timmer > 500){
    R200Serial.write(single_read,9);
    command_timmer = millis();
  }
}
void execute_multi_read_command(){
  if(millis() - command_timmer > 1000){
    R200Serial.write(multi_read,10);
    command_timmer = millis();
  }
}
void execute_stop_command(){
  if(millis() - command_timmer > 500){
    R200Serial.write(stop_read,7);
    command_timmer = millis();
  }
}

void clear_epc(){
  for(int x = 0 ; x<11 ; x++){
    epc[x] = 0x00;        
  }
}

void print_epc(){
  Serial.print("EPC: ");
  for(int x = 0 ; x != 11 ; x+=1){
    if(epc[x] < 16){
      Serial.print("0");
      Serial.print(epc[x], HEX);
      Serial.print(" ");
    }else{
      Serial.print(epc[x], HEX);
      Serial.print(" ");
    }
  }
  Serial.print("| Distance: ");
  Serial.println(cm);  
}

void print_epc_char(){
  Serial.print("EPC: ");
  for(int x = 0 ; x != 11 ; x+=1){
    Serial.print(int(epc[x]));
    Serial.print(" ");
  } 
  Serial.println();
}

String string_epc(){
  String text = "";
  for(int x = 0 ; x != 11 ; x+=1){
    if(epc[x] < 16){
      // Serial.print("0");
      // Serial.print(epc[x], HEX);
      // Serial.print(" ");
      text = text + "0" + String(epc[x], HEX);
    }else{
      text = text + String(epc[x], HEX);
      // Serial.print(epc[x], HEX);
      // Serial.print(" ");
    }
    if(x != 10){
      text = text + " ";
    }
  }  
  return text;
}

void read_response(){
  if(R200Serial.available() > 0){
    incomedate = R200Serial.read();
    if((incomedate == 0x02)&(parState == 0)){
        parState = 1;
    }else if((parState == 1)&(incomedate == 0x22)&(codeState == 0)){  
        codeState = 1;
        dataAdd = 3;
    }else if(codeState == 1){
        dataAdd ++;
        if(dataAdd == 6){
          rssi = incomedate;           
        }else if((dataAdd == 7)|(dataAdd == 8)){
          if(dataAdd == 7){
            pc[0] = incomedate;
          }else {
            pc[1] = incomedate;
          }
        }else if((dataAdd >= 9)&(dataAdd <= 20)){
          epc[dataAdd - 9] = incomedate;
        }else if(dataAdd >= 21){
          dataAdd= 0;
          parState = 0;
          codeState = 0;
        }
      }else{
        dataAdd= 0;
        parState = 0;
        codeState = 0;
    }
  }   
}

/************************************************* R200 **************************************************/

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
