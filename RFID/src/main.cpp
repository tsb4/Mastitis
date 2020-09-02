#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <WiFi.h> 
#include "MLX90641_API.h"
#include "MLX9064X_I2C_Driver.h"

#define debug  Serial

const byte MLX90641_address = 0x33; //Default 7-bit unshifted address of the MLX90641
#define TA_SHIFT 8 //Default shift for MLX90641 in open air
uint16_t eeMLX90641[832];
float MLX90641To[192];
uint16_t MLX90641Frame[242];
paramsMLX90641 MLX90641;
int errorno = 0;

const char* ssid = "Heim";
const char* password = "acgabensb";
WiFiServer server(80);

#define RST_PIN    15   
#define SS_PIN     2   

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

int estado=0;
int id;
float temp[192];
String packet;

boolean validateID();
void setupRFID();
void printRequestRFID();
void setupWIFI();
boolean messageWIFI();
void generatePacket();
boolean isConnected();
void setupMLX();
void takeImage();

void setup() 
{
  Serial.begin(115200);   // Inicia a serial
  setupRFID();
  setupWIFI();
  setupMLX();
}

void loop() 
{
  if(estado==0){
    printRequestRFID();
    estado =1;
  }
  if(estado==1){
	  if (validateID()){
      estado = 2;
    }
  }
  if(estado == 2){
    Serial.println("Esperando envio de dados...");
    delay(1000);
    takeImage();
    for(int i=0;i<192;i++){
      temp[i] = MLX90641To[i];
    }
    estado =3;
    generatePacket();
  }
  if(estado==3){
    if(messageWIFI()) estado = 0;
  }
  
}

void printRequestRFID(){
  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
}

void setupRFID(){
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522
}

boolean validateID(){
  //Serial.println("Aproxime o seu cartao do leitor...");
  //Serial.println();
  // Procura por cartao RFID
  if( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return false;
  }
  // Seleciona o cartao RFID
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return false;
  }
  //Mostra UID na serial
  Serial.print("UID da tag :");
  String conteudo= "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     //Serial.print(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     //Serial.print(String(mfrc522.uid.uidByte[i], HEX));
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();
  Serial.println(conteudo);
  

  if(conteudo.substring(1) == "47 9E 7F 5F") //UID 1 - Cartao
  {
    id =1;
    Serial.println("vaca Identificada! - Vaca 1");
    Serial.println();
	  return true;
  }
  else{
    Serial.println("Vaca não válida!!!");
    estado=0;
	  return false;
  }
}

void setupWIFI(){
  Serial.println();
  Serial.print("Conectando-se a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi conectada.");
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());
 
  server.begin();
}

boolean messageWIFI(){
  bool ret = false;
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/plain");
            client.println();
            client.print(packet);
            client.println();
            ret = true;
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
  return ret;
}

void generatePacket(){
  packet = "";
  packet.concat(String(id));
  packet.concat(";");
  for(int i=0;i<192;i++){
    packet.concat(String(temp[i]));
    packet.concat(",");
  }
  Serial.println(packet);
}

void setupMLX(){
  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz
 
  while (!debug);

  if (isConnected() == false) {
    debug.println("MLX90641 not detected at default I2C address. Please check wiring. Freezing.");
    while (1);
  }
   //Get device parameters - We only have to do this once
  int status;
  status = MLX90641_DumpEE(MLX90641_address, eeMLX90641);
  errorno = status;//MLX90641_CheckEEPROMValid(eeMLX90641);//eeMLX90641[10] & 0x0040;//

  if (status != 0) {
    debug.println("Failed to load system parameters");
    while(1);
  }

  status = MLX90641_ExtractParameters(eeMLX90641, &MLX90641);
  //errorno = status;
  if (status != 0) {
    debug.println("Parameter extraction failed");
    while(1);
  }

  //Once params are extracted, we can release eeMLX90641 array

  //MLX90641_SetRefreshRate(MLX90641_address, 0x02); //Set rate to 2Hz
  MLX90641_SetRefreshRate(MLX90641_address, 0x03); //Set rate to 4Hz
  //MLX90641_SetRefreshRate(MLX90641_address, 0x07); //Set rate to 64Hz    
}

void takeImage(){
  long startTime = millis();
 
  for (byte x = 0 ; x < 2 ; x++) {
    int status = MLX90641_GetFrameData(MLX90641_address, MLX90641Frame);
    float vdd = MLX90641_GetVdd(MLX90641Frame, &MLX90641);
    float Ta = MLX90641_GetTa(MLX90641Frame, &MLX90641);
    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
    float emissivity = 0.95;
 
    MLX90641_CalculateTo(MLX90641Frame, &MLX90641, emissivity, tr, MLX90641To);
  }
  long stopTime = millis();
  for (int x = 0 ; x < 192 ; x++) {
    debug.print(MLX90641To[x], 2);
    debug.print(",");
  }
  debug.println("");    
}
boolean isConnected() {
 
  Wire.beginTransmission((uint8_t)MLX90641_address);
  if (Wire.endTransmission() != 0) {
    return (false);    //Sensor did not ACK
  }
  return (true);
}