#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <WiFi.h> 
#include<EloquentTinyML.h>
//#include <ESP8266WiFi.h> 
//#include <ESP8266WebServer.h>
#include "MLX90641_API.h"
#include "MLX9064X_I2C_Driver.h"
#include "MasModel.h"

#define NUMBER_OF_INPUTS 4
#define NUMBER_OF_OUTPUTS 3
// in future projects you may need to tweek this value: it's a trial and error process
#define TENSOR_ARENA_SIZE 2*1024

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml(MasModel);

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
//ESP8266WebServer server(80);

#define RST_PIN    15   
#define SS_PIN     2   

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

int estado=0;
int cont = 0;
int id;
float temp[192];
float udder_temp[4];
float* predicted;
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
void handle_root();
void handle_finished();
void extract_temp();
void forward_network();
boolean is_finished = false;
String header;

void setup() 
{
  Serial.begin(115200);   // Inicia a serial
  setupRFID();
  setupWIFI();
  setupMLX();
}


void loop() 
{
  /*is_finished = false;
  for(int i=0;i<192;i++){
    temp[i] = 28.35;
  }
  generatePacket();
  //while(!is_finished) server.handleClient();
  while(!is_finished) messageWIFI();
  Serial.println("---------------------------------------------------------------------------------------------------------------------------");
  Serial.println("------------------------------------------------------------------------------------------------------------------------------------------------------------------");
 */
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
    generatePacket();
    messageWIFI();
    if(is_finished) {
      Serial.println("OPA!!!!");
      Serial.println("-------------------------------------------------");
      //Função para extrair a temperatura
      extract_temp();
      //4 vezes
      cont++;
      if(cont==4) {
        for (int i=0;i<4;i++) Serial.println(udder_temp[i]);
        estado =3;
      }
      delay(5000);
      is_finished = false;
    }
  }
  if(estado==3){
    forward_network();
    cont = 0;
    is_finished = false;
    estado = 0;
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

  //server.on("/", handle_root);
  //server.on("/finished", handle_finished);
 
  server.begin();
}

/*void handle_root(){
  server.send(200, "text/plain", packet);
}

void handle_finished(){
  server.send(200, "text/plain", "ok");
  is_finished = true;
}*/

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
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/plain");
            client.println();
            client.print(packet);
            client.println();
            ret = true;
            if(header.indexOf("GET /finished")>=0) is_finished = true;
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client Disconnected.");
  }
  return ret;
}

void generatePacket(){
  packet = "";
  //packet.concat(String(id));
  //packet.concat(";");
  Serial.print("cont: ");
  Serial.println(cont+1);
  packet.concat(String(cont+1));
  packet.concat(";");
  for(int i=0;i<192;i++){
    packet.concat(String(temp[i]));
    if( i!=191) packet.concat(",");
  }
  //packet = "27.44,27.13,26.93,27.05,27.09,27.24,27.17,27.25,27.26,27.08,27.12,27.14,27.20,27.21,27.35,27.37,28.09,27.74,27.75,27.61,27.67,27.70,27.81,27.79,27.27,27.18,27.11,27.26,27.32,27.44,27.46,27.49,31.03,30.96,30.70,30.16,29.67,29.05,28.39,28.13,27.84,27.48,27.30,27.51,27.50,27.58,27.53,27.57,33.37,33.62,33.62,33.43,33.05,32.46,31.39,30.26,29.48,28.69,28.73,28.37,27.93,27.58,27.61,27.63,33.98,34.30,34.37,34.48,34.43,34.38,34.03,33.48,32.97,32.44,32.90,31.53,28.42,27.94,27.75,27.36,34.40,34.61,34.51,34.61,34.70,34.72,34.65,34.55,34.46,34.20,33.97,33.45,30.32,28.24,27.74,27.69,34.29,34.32,34.57,34.79,34.72,34.69,34.82,34.84,34.73,34.60,34.42,33.81,31.35,28.41,27.86,27.57,34.40,34.54,34.45,34.73,34.83,34.85,34.86,34.93,34.74,34.62,34.02,32.84,29.62,28.04,27.94,27.60,34.29,34.61,34.70,34.73,34.76,34.73,34.75,34.83,34.45,34.04,32.81,30.41,28.49,28.04,27.93,27.51,34.19,34.60,34.81,34.63,34.75,34.71,34.71,34.46,34.16,32.66,30.61,29.03,28.29,28.00,27.83,27.54,34.21,34.51,34.58,34.58,34.45,34.54,34.40,33.73,32.11,30.24,28.95,28.31,27.97,27.60,27.34,27.08,33.15,33.97,34.05,33.97,34.32,34.13,33.08,31.29,29.77,28.94,28.36,28.14,27.66,27.33,26.88,27.06";
  Serial.println(packet);
  //Serial.println(packet);
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

void extract_temp(){
  float result = temp[87]*0.1 + temp[88]*0.1 + temp[103]*0.7 + temp[104]*0.1;
  udder_temp[cont] = result;
  Serial.print("Teste de temp :");
  Serial.println(result);
}

void forward_network(){
  float input[4] = {37.5, 38.0,  38.3, 38.0 };
  predicted = ml.predict(udder_temp);
  //Serial.print("Res(");
  ////Serial.print(x);
  //Serial.print(") = ");
  //Serial.print(y);
  Serial.print("\t predicted: ");
  Serial.println(predicted[0]);
  Serial.println(predicted[1]);
  Serial.println(predicted[2]);
  delay(1000);
}
