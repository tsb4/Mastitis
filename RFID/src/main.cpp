#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h> 

const char* ssid = "Heim";
const char* password = "acgabensb";
WiFiServer server(80);

#define RST_PIN    D3    
#define SS_PIN     D4   

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

void setup() 
{
  Serial.begin(9600);   // Inicia a serial
  setupRFID();
  setupWIFI();
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
    Serial.println("Estado 2");
    for(int i=0;i<192;i++) temp[i] =10.31;
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