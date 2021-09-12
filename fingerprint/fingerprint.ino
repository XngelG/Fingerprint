#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>
#include <Adafruit_Fingerprint.h>

#define SerialDebug true

SoftwareSerial mySerial(3, 2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 177);

IPAddress server(192, 168, 0, 10);

EthernetClient client;

typedef struct{
  uint8_t ID;
  char Name[20];
}Register;
Register regDB[20];

bool ReadReqAck;        //Offset: 0
bool ReadyStatus;       //Offset: 1
bool ReadComplete;      //Offset: 2
bool DeleteAck;         //Offset: 3
bool DeleteComplete;    //Offset: 4
bool RegisterAck;       //Offset: 5
bool RegisterComplete;  //Offset: 6
bool ReadReq;           //Offset: 10
bool ReadCmpltClr;      //Offset: 11
bool DeleteReq;         //Offset: 12
bool DeleteCmpltClr;    //Offset: 13
bool RegisterReq;       //Offset: 14
bool RegCmpltClr;       //Offset: 15
bool ReadReqAckClr;     //Offset: 16
bool RegAckClr;         //Offset: 17

void setup() {
  delay(100);
  Ethernet.init(10); 
  Ethernet.begin(mac, ip);
  delay(100);
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Begin");

  while (Ethernet.linkStatus() == LinkOFF) {
    if(SerialDebug)Serial.println("Ethernet cable is not connected.");
    delay(500);
  }

  delay(1000);

  for(int i=0; i<20; i++){
    EEPROM.get(21*i, regDB[i]);
    if(regDB[i].ID == 0){
      regDB[i].ID = 0;
      strcpy(regDB[i].Name,"");
      break;
      }
    Serial.print(regDB[i].ID);
    Serial.print('\t');
    Serial.println(regDB[i].Name);
  }

  finger.begin(57600);
  delay(5);
  while (!finger.verifyPassword()) delay(100);

  Serial.println("Ready");

  ReadyStatus = true;
  writeBit(1,ReadyStatus);
}

//bool ReadReqAck;        //Offset: 0
//bool ReadyStatus;       //Offset: 1
//bool ReadComplete;      //Offset: 2
//bool DeleteAck;         //Offset: 3
//bool DeleteComplete;    //Offset: 4
//bool RegisterAck;       //Offset: 5
//bool RegisterComplete;  //Offset: 6
//bool ReadReq;           //Offset: 10
//bool ReadCmpltClr;      //Offset: 11
//bool DeleteReq;         //Offset: 12
//bool DeleteCmpltClr;    //Offset: 13
//bool RegisterReq;       //Offset: 14
//bool RegCmpltClr;       //Offset: 15
//bool ReadReqAckClr;     //Offset: 16
//bool RegAckClr;         //Offset: 17

void loop() {
  //Updating reading routine
  ReadReq = readBit(10);
  ReadCmpltClr = readBit(11);
  DeleteReq = readBit(12);
  DeleteCmpltClr = readBit(13);
  RegisterReq = readBit(14);
  RegCmpltClr = readBit(15);
  ReadReqAckClr = readBit(16);
  RegAckClr = readBit(17);

  if(ReadReq){
    ReadyStatus = false;
    writeBit(1,ReadyStatus);
    ReadReqAck = true;
    writeBit(0,ReadReqAck);
    int FingerprintResult;
    for (int i = 0; i < 20; i++)
    {
      FingerprintResult = getFingerprintIDez(); 
      if(FingerprintResult != -1) break;
      else delay(100);
    }
    writeWord(0,(unsigned int)FingerprintResult);
    if (FingerprintResult != -1)
    {
      for (int i = 0; i < 20; i++)
      {
        if (regDB[i].ID == FingerprintResult)
        {
          writeString(5,String(regDB[i].Name));
          break;
        }
      }
    }
    
    ReadComplete = true;
    writeBit(2,ReadComplete);
    ReadyStatus = true;
    writeBit(1,ReadyStatus);
  }
  if (ReadReqAckClr)
  {
    ReadReqAck = false;
    writeBit(0,ReadReqAck);
  }
  if (ReadCmpltClr)
  {
    ReadComplete = false;
    writeBit(2,ReadComplete);
  }

  if (RegisterReq)
  {
    ReadyStatus = false;
    writeBit(1,ReadyStatus);
    RegisterAck = true;
    writeBit(5,RegisterAck);
    unsigned int FingerprintNoReq = readWord(20);
    String FingerprintNameReq = readStr(25);
    while (!getFingerprintEnroll(FingerprintNoReq,FingerprintNameReq));
    RegisterComplete = true;
    writeBit(6,RegisterComplete);
    ReadyStatus = true;
    writeBit(1,ReadyStatus);
  }
  if (RegAckClr)
  {
    RegisterAck = false;
    writeBit(5,RegisterAck);
  }
  if (RegCmpltClr)
  {
    RegisterComplete = false;
    writeBit(6,RegisterComplete);
  }    
  
  if (DeleteReq)
  {
    ReadyStatus = false;
    writeBit(1,ReadyStatus);
    unsigned int DeleteNoReq = readWord(21);
    deleteFingerprint(DeleteNoReq);
    DeleteComplete = true;
    writeBit(4,DeleteComplete);
    ReadyStatus = true;
    writeBit(1,ReadyStatus);
  }
  if (DeleteCmpltClr)
  {
    DeleteComplete = false;
    writeBit(4,DeleteComplete);
  }
  
  

}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
  }

  for (int i = 0; i < 20; i++)
    {
      if (regDB[i].ID == id)
      {
        for (int j = i; j < 19; j++)
        {
          regDB[j].ID = regDB[j+1].ID;
          strcpy(regDB[j].Name,regDB[j+1].Name);
        }
      }
    }
    for (int i = 0; i < 20; i++)
    {
      EEPROM.put(21*i, regDB[i]);
    }

  return p;
}

uint8_t getFingerprintEnroll(uint8_t id, String FingerprintNameReq) {

  int p = -1;
  unsigned int FingerprintRegStatus;
   
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      FingerprintRegStatus = 1;
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      FingerprintRegStatus = 2;
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      FingerprintRegStatus = 3;
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      FingerprintRegStatus = 4;
      Serial.println("Imaging error");
      break;
    default:
      FingerprintRegStatus = 5;
      Serial.println("Unknown error");
      break;
    }
    writeWord(1,FingerprintRegStatus);
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      FingerprintRegStatus = 6;
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      FingerprintRegStatus = 7;
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      FingerprintRegStatus = 3;
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      FingerprintRegStatus = 8;
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      FingerprintRegStatus = 8;
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      FingerprintRegStatus = 5;
      Serial.println("Unknown error");
      return p;
  }
  writeWord(1,FingerprintRegStatus);

  FingerprintRegStatus = 9;
  writeWord(1,FingerprintRegStatus);
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      FingerprintRegStatus = 1;
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      FingerprintRegStatus = 2;
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      FingerprintRegStatus = 3;
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      FingerprintRegStatus = 4;
      Serial.println("Imaging error");
      break;
    default:
      FingerprintRegStatus = 5;
      Serial.println("Unknown error");
      break;
    }
    writeWord(1,FingerprintRegStatus);
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
    FingerprintRegStatus = 6;
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      FingerprintRegStatus = 7;
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      FingerprintRegStatus = 3;
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      FingerprintRegStatus = 8;
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      FingerprintRegStatus = 8;
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      FingerprintRegStatus = 5;
      Serial.println("Unknown error");
      return p;
  }
  writeWord(1,FingerprintRegStatus);

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    FingerprintRegStatus = 10;
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    FingerprintRegStatus = 3;
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    FingerprintRegStatus = 11;
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    FingerprintRegStatus = 5;
    Serial.println("Unknown error");
    return p;
  }
  writeWord(1,FingerprintRegStatus);

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    FingerprintRegStatus = 12;
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    FingerprintRegStatus = 3;
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    FingerprintRegStatus = 13;
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    FingerprintRegStatus = 14;
    Serial.println("Error writing to flash");
    return p;
  } else {
    FingerprintRegStatus = 5;
    Serial.println("Unknown error");
    return p;
  }
  writeWord(1,FingerprintRegStatus);
  for(int i=0; i<20; i++){
      if(regDB[i].ID == id) 
      { 
        FingerprintNameReq.toCharArray(regDB[i].Name,20);
        EEPROM.put(21*i, regDB[i]);
        break;
      }
      else if(regDB[i].ID == 0)
      {
        regDB[i].ID = id;
        FingerprintNameReq.toCharArray(regDB[i].Name,20);
        EEPROM.put(21*i, regDB[i]);
        break;
      }
  }
  
  return true;
}

int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}

void writeBit(int offset, bool value){
  while(!client.connect(server, 8501)) delay(1);
  client.print("WR B");
  client.print(String(offset));
  client.print(" ");
  if(value)client.println("1");
  else client.println("0");
  while(!client.available())delay(1);
  while(client.available())client.read();
  client.stop();  
}

void writeWord(int offset, unsigned int value){
  while(!client.connect(server, 8501)) delay(1);
  client.print("WR W");
  client.print(String(offset));
  client.print(" ");
  client.println(String(value));
  while(!client.available())delay(1);
  while(client.available())client.read();
  client.stop();  
}

unsigned int readWord(int offset){
  String tmp;
  while(!client.connect(server, 8501)) delay(1);
  client.print("RD W");
  client.println(String(offset));
  while(!client.available())delay(1);
  while(client.available()){
    tmp.concat((char)client.read());
  }
  client.stop();
  return (unsigned int) tmp.toInt();
}

bool readBit(int offset){
  while(!client.connect(server, 8501)) delay(1);
  client.print("RD B");
  client.println(String(offset));
  while(!client.available())delay(1);
  while(client.available()){
    char c = client.read();
    if(c == '1'){
      client.stop();
      return true;
    }
    else if(c == '0'){
      client.stop();
      return false;
    }
  }
}

void writeString(int offset, String msg){
  while(!client.connect(server, 8501)) delay(1);
  client.print("WRS W");
  client.print(String(offset));
  client.print(" ");
  client.print(String(msg.length()));
  client.print(" ");
  for(int i=0; i<msg.length(); i++){
    unsigned int c = (unsigned int)msg.charAt(i);
    client.print(String(c));
    if(i<msg.length()-1){
      client.print(" ");
    }
    else{
      client.println();
    }
  }
  while(!client.available())delay(1);
  while(client.available())client.read();
  client.stop();
}

String readStr(int offset){
  String result = "";
  String tmp = "";
  while(!client.connect(server, 8501)) delay(1);
  client.print("RDS W");
  client.print(String(offset));
  client.println(" 20");
  while(!client.available())delay(1);
  while(client.available()){
    tmp.concat((char)client.read());
  }
  for (int i=0; i<20; i++){
    result.concat((char)tmp.substring(i*6,i*6+5).toInt());
  }
  client.stop();
  return result;
}
