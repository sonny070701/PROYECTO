/* LIBRERIAS */
#include <SPI.h>
#include <MFRC522.h>
#include <TinyGPSPlus.h>

/* VARIABLES DEL SISTEMA */

// HC-SR05
const int trigPin= 25;
const int echoPin= 26;
long duration;
float distanceCm;

// PULSENSOR
int PulsePin = 0;
int Signal;           
int Threshold = 2200;

// BANDERAS DE INFORMACION 
boolean Colisionflag = 0;

/* MACROS */
// RFID
#define RST_PIN  22    
#define SS_PIN  21     
#define RedLed 14 

// HC-SR05
#define SOUND_SPEED 0.034 
#define CM_TO_INCH 0.393701

// GPS
TinyGPSPlus gps;

// RFID
MFRC522 mfrc522(SS_PIN, RST_PIN); 
byte ActualUID[4];
byte USER[4] = {0xCD, 0x78, 0xFF, 0xB8}; 
byte ADMIN[4]= {0x00, 0x78, 0x00, 0xB8};

void setup(){
  Serial.begin(9600); // Startstheserial communication
  Serial2.begin(9600);
  SPI.begin();        //Iniciamos el Bus SPI
  mfrc522.PCD_Init(); // Iniciamos el MFRC522
  pinMode(trigPin, OUTPUT); // Sets thetrigPinas anOutput
  pinMode(echoPin, INPUT); // Sets theechoPinas anInput
  pinMode(RedLed, OUTPUT);
  
  Serial.println("Control de acceso:");
}
void loop(){
  delay(500);

  if(mfrc522.PICC_IsNewCardPresent()){
    if ( mfrc522.PICC_ReadCardSerial()){
      Serial.print(F("Card UID:"));
      for (byte i = 0; i < mfrc522.uid.size; i++){
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        ActualUID[i]=mfrc522.uid.uidByte[i];
      }
      Serial.print("     ");
       if(compareArray(ActualUID,USER))
      {
        Serial.println("Acceso concedido USUARIO...");
        while(1){
          Sensor_choques();
          displayInfo();;
          delay(1000);
        }
      }
      else if(compareArray(ActualUID,ADMIN))
      {
        Serial.println("Acceso concedido ADMINISTRADOR...");
        while(1){
          Sensor_choques();
          Driver_state();
          //Monitoreo_carga();
          delay(500);
        }
      }
     else {
        Serial.println("Acceso denegado...");
     }
    }
  }
  mfrc522.PICC_HaltA();
}

boolean compareArray(byte array1[],byte array2[]) {
  if(array1[0] != array2[0])return(false);
  if(array1[1] != array2[1])return(false);
  if(array1[2] != array2[2])return(false);
  if(array1[3] != array2[3])return(false);
  return(true);
}

void Sensor_choques(void){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration= pulseIn(echoPin, HIGH);
  distanceCm= duration* SOUND_SPEED/2;
  Serial.printf("Distance(cm): ");
  Serial.println(distanceCm);

  if(distanceCm < 5){
    Colisionflag = 1;
  }
}

void Driver_state(void){
  if (Colisionflag == 1){
    char message[] = "CONDUCTOR HERIDO";
    Serial.println(message);
    Serial.println("Signos vitales :");
    while(1){
      Signal = analogRead(PulsePin);
      Serial.println(Signal);
      delay(100);  
    }
  }
}

void Monitoreo_carga(void){
  
  while (Serial2.available() > 0){
    if (gps.encode(Serial2.read()))
      displayInfo();
  }
  if (millis() > 5000 && gps.charsProcessed() < 10){
    Serial.println(F("No GPS detected: check wiring."));
  }
  displayInfo();
}

void displayInfo(void){
  Serial.print(F("Location: "));
  if (gps.location.isValid()){
    Serial.print("Lat: ");
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print("Lng: ");
    Serial.print(gps.location.lng(), 6);
    Serial.println();
  }  
  else{
    Serial.print(F("INVALID \n"));
  }
}
