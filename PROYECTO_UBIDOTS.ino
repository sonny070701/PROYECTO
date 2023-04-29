/* Include Libraries */
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TinyGPSPlus.h>
#include "DHT.h"

/* Constantes y Macros */
#define WIFISSID "Xd"
#define PASSWORD "123456789" 

#define TOKEN "BBFF-9s4ssNAd21GHdhjCLusJn3oSi8mipR" 
#define MQTT_CLIENT_NAME "SONNY" 

/* Macros de sensores */
#define SOUND_SPEED 0.034 
#define CM_TO_INCH 0.393701
#define RST_PIN  22    
#define SS_PIN  21     
#define RedLed 14 
#define DHT_pin 14

/* Variables (etiquetas) */
#define VARIABLE_LABEL_dist "distancia" 
#define VARIABLE_LABEL_puls "RPM" 
#define VARIABLE_LABEL_posi "posicion"
#define VARIABLE_LABEL_temp "temperatura" 
#define VARIABLE_LABEL_hum "humedad" 

/* Nombre del dispositivo */
#define DEVICE_LABEL "proyecto_iot" 

/* Configuracion del broker */
char mqttBroker[]= "industrial.api.ubidots.com";
char payload[200]; 
char topic[150];

/* Space to store values to send */
char str_dist[10];
char str_puls[10];
char str_posi[100];
char str_temp[10];
char str_hum[10];

/* HC-SR05 */
const int trigPin= 25;
const int echoPin= 26;
long duration;
float distanceCm;

/* PULSENSOR */
int PulsePin = 36;
int Signal;           
int Threshold = 2200;

/* GPS */
TinyGPSPlus gps;

/* DHT */
DHT dht1(DHT_pin, DHT11);

// RFID
MFRC522 mfrc522(SS_PIN, RST_PIN); 
byte ActualUID[4];
byte USER[4] = {0xCE, 0x78, 0xFF, 0xB8}; 
byte ADMIN[4]= {0xCD, 0x78, 0xFF, 0xB8};

/* Funciones auxiliares */
WiFiClient ubidots;
PubSubClient client(ubidots);

void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);
  Serial.println(topic);
}

void reconnect() {
  
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

boolean compareArray(byte array1[],byte array2[]) {
  if(array1[0] != array2[0])return(false);
  if(array1[1] != array2[1])return(false);
  if(array1[2] != array2[2])return(false);
  if(array1[3] != array2[3])return(false);
  return(true);
}

void setup() {
  /* Monitor serial */
  Serial.begin(9600);
  Serial2.begin(9600);
  
  /* Configuracion de ubidots */
  WiFi.begin(WIFISSID, PASSWORD);
  Serial.println();
  Serial.print("Wait for WiFi...");
  
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }

  /* Configuracion del Sensor RFID */
  SPI.begin();        
  mfrc522.PCD_Init(); 

  /* Configuracion del HSR04 */
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 

  dht1.begin();

  /* Configuracion del broker de MQTT */
  Serial.println("");
  Serial.printf("WiFi Connected, IP address: %s\n", WiFi.localIP());
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);
}

void loop(){
  
  if (!client.connected()){
    reconnect();
  }
  
  delay(1000);
  MQTT_send_data();

  Serial.println("Control de acceso:");
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
        Serial.println("Acceso concedido -> USUARIO...");
        
        /* Variables de estados */
        int d = Sensor_choques();
        float h1 = dht1.readHumidity();
        float t1 = dht1.readTemperature();

        /* Imprimir en la consola del usuario */
        Serial.print(" La humedad detectada en el cargamento: ");
        Serial.println(h1); 
        Serial.print(" La temperatura detectada en el cargamento: ");
        Serial.println(t1);
        Serial.print(" Distancia del sensor: ");
        Serial.println(d);
        
        delay(1000);
      }
      else if(compareArray(ActualUID,ADMIN)) {
        Serial.println("Acceso concedido ADMINISTRADOR...");

        /* Variables de estados */
        char cargamento[30] = "Cargamento: Televisiones";
        Signal = analogRead(PulsePin);
        int d = Sensor_choques();
        float h1 = dht1.readHumidity();
        float t1 = dht1.readTemperature();

        /* Imprimir en la consola del administrador */
        Serial.print(" La humedad detectada en el cargamento: ");
        Serial.println(h1); 
        Serial.print(" La temperatura detectada en el cargamento: ");
        Serial.println(t1);
        Serial.printf("RPM del conductor: %d\n", Signal);
        Serial.printf(" Distancia del sensor: %d\n", d);
        Serial.printf(cargamento);
        Driver_state();
        gps = displayInfo();
        delay(1000);
      }
     }
     else {
        Serial.println("Acceso denegado...");
     }
  }
  mfrc522.PICC_HaltA();
  Serial.print(" Leyendo datos de los sensores: .........\n");
  delay(1000);
}

int Sensor_choques(void){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration= pulseIn(echoPin, HIGH);
  distanceCm = duration* SOUND_SPEED/2;

  if(distanceCm < 5){
    Serial.print("Hubo un choque");
  }
  return distanceCm;
}


TinyGPSPlus displayInfo(void){
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
  return gps;
}

void Driver_state(void){
  if (Signal > Threshold){
    char message[] = "CONDUCTOR HERIDO";
    Serial.println(message);
  } else {
    char message[] = "CONDUCTOR SIN DAÑOS";
    Serial.println(message);
    }
}

void MQTT_send_data(void){
  
  /* Publica en el topic de distancia */
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL_dist); 

  /* Obtener datos del sensor */
  Serial.print(" Iniciando el sistema: .........\n");
  int d1 = Sensor_choques();
  dtostrf(d1, 4, 2, str_dist);

  sprintf(payload, "%s {\"value\": %s", payload, str_dist);
  sprintf(payload, "%s } }", payload); 
  Serial.println("  Enviando distancia al device en ubidots via MQTT....");
  client.publish(topic, payload);

  /*******************************************************************/
   /* Publica en el topic de pulsos */
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL_puls); 

  /* Obtener datos del sensor */
  Signal = analogRead(PulsePin);
  Serial.println("  Enviando RPM al device en ubidots via MQTT....");
  dtostrf(Signal, 4, 2, str_puls);
  
  sprintf(payload, "%s {\"value\": %s", payload, str_puls);
  sprintf(payload, "%s } }", payload); 
  client.publish(topic, payload);
  
  /*******************************************************************/
  /* Publica en el topic de temperatura */
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); 
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL_temp); 

  /* Obtener datos del sensor */
  float t1 = dht1.readTemperature();
  Serial.println("  Enviando Temperatura al device en ubidots via MQTT....");
  dtostrf(t1, 4, 2, str_temp);
  
  sprintf(payload, "%s {\"value\": %s", payload, str_temp);
  sprintf(payload, "%s } }", payload); 
  client.publish(topic, payload);

  /*******************************************************************/
  /* Publica en el topic de humedad */
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL_hum); 
  
  float h1 = dht1.readHumidity();
  Serial.println("  Enviando Humedad al device en ubidots via MQTT....");
  dtostrf(h1, 4, 2, str_hum);
  
  sprintf(payload, "%s {\"value\": %s", payload, str_hum); // formatea el mensaje a publicar
  sprintf(payload, "%s } }", payload); // cierra el mensaje
  client.publish(topic, payload);
  client.loop();

  Serial.println("Waiting to read .....");
  delay(10000);
}
