#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>

#define RST_PIN    D3    
#define SS_PIN     D4   

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance

int estado=0;

boolean validateID();
void setupRFID();

void setup() 
{
  Serial.begin(9600);   // Inicia a serial
  setupRFID();
}

void loop() 
{
  if(estado==0){
	  if (validateID()){
      estado = 1;
    }
  }
  if(estado == 1){
    Serial.println("Estado 1");
  }
  
}

void setupRFID(){
  SPI.begin();      // Inicia  SPI bus
  mfrc522.PCD_Init();   // Inicia MFRC522

  Serial.println("Aproxime o seu cartao do leitor...");
  Serial.println();
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
    Serial.println("vaca Identificada! - Vaca 1");
    Serial.println();
	return true;
  }
  else{
	return false;
  }
}