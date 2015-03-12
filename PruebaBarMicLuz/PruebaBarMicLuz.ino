#include <Wire.h>
#include <TSL2561.h> //Luminosidad
#include <Adafruit_MPL115A2.h> //Barometro
#include <Ethernet.h>
#define DEBUG 0
#define LUZ 		0x01	//0000	0001
#define PRESION 	0x02	//0000	0010
#define TEMPERATURA     0x04	//0000	0100
#define RUIDO 		0x08	//0000	1000
#define ETHERNET 	0x10	//0001  0000
#define SERVER	 	0x20	//0010  0000
byte errorFlag = 0x0;

Adafruit_MPL115A2 barometro; //Barometro
TSL2561 luminosidad(TSL2561_ADDR_FLOAT); //Luminosidad
const int sampleWindow = 50; //50 ms = 20 Hz
unsigned int sample;
int ledMic = 7;
int ledLuz = 12;
int ledBar = 13;
int ledError = 8;
/***** Ethernet ******/
#define thingName "Test-Galileo-1"
#define GALILEO 1
#define MAC_SIZE 6
//MAC esta escrita en la etiqueta del puerto Ethernet
byte mac[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE5, 0x86 };
byte mac1F7R[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE5, 0xFB };
byte mac1DN7[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE1, 0xA6 };
byte actualMac[6];
EthernetClient client;
char server[] = "www.dweet.io"; 
/********************/

boolean hasError(byte flag){
  return (errorFlag&flag) == flag;
}

void copyArray(byte* array_original, byte* arrayTo, int n){
  int i = 0;
  for(; i<n; i++){
    arrayTo[i] = array_original[i];
  }
}

boolean initializeEthernet(){
  switch(GALILEO){
    case 0:
      copyArray(actualMac,mac,MAC_SIZE);
    break;
    case 1:
      copyArray(actualMac,mac1F7R,MAC_SIZE);
    break;
    case 2:
      copyArray(actualMac,mac1DN7,MAC_SIZE);
    break;
  }
  //habilitamos el puerto ethernet de galileo
    if(DEBUG == 1) Serial.print("Setting up ethernet port");
  system("ifup eth0");
  delay(2000); 
    if(DEBUG == 1) Serial.print("done");
  
  //Start Ethernet connection
  if(Ethernet.begin(mac) == 0){
      if(DEBUG == 1) Serial.print("Failed to configure Ethernet using DHCP");
    return false;
  }
    if(DEBUG == 1) Serial.print("Connecting..."); 
  return true;
}

/** Recopila y manda las lecturas a Dweet **/
void sendDweet(float presion, float temperatura, double ruido,
               uint32_t lumens)
{
  ruido = 20 * log10(ruido / 5);
  if(DEBUG == 1) Serial.println("Dweeting...");
  if(client.connect(server,80)){
    if(hasError(SERVER)){
      errorFlag = errorFlag ^ SERVER;
    }
    //Estructura de los datos
    client.print("GET /dweet/for/");
    client.print(thingName);
    client.print("?pressure=");
    client.print(presion);
    client.print("&temp=");
    client.print(temperatura);
    client.print("&noise=");
    client.print(ruido);
    client.print("&lumens=");
    client.print(lumens);
    client.println(" HTTP/1.1");
    
    //Indicador de host y cierre de la conexion
    client.println("Host: dweet.io");
    client.println("Connection: close");
    client.println();
    client.flush();  
    
    //Leemos la respuesta
    if(client.available()){
      char c = client.read();
      Serial.print(c);
    }
    client.stop();
      if(DEBUG == 1) Serial.println("Dweet sent");
    }else{
        errorFlag = errorFlag | SERVER;
        if(DEBUG == 1) Serial.println("Connection to Dweet failed");
    }
}

float getPressure(){
  float presion = barometro.getPressure();
  return presion;
}

float getTemperature(){
  float temperature = barometro.getTemperature();
  return temperature;
}

void printToSerialTempPress(float temp, float pressure){
  //Mostramos datos leidos
  Serial.print("Presion (kPa): ");
  Serial.print(pressure, 4);
  Serial.println(" kPa");
  Serial.print("Temperatura (*C): ");
  Serial.print(temp, 1);
  Serial.println(" *C");
  Serial.println("/*************************/"); 
}

uint32_t getLuminosity(){
  uint32_t luminosidadCompleta = luminosidad.getFullLuminosity();
  luminosidadCompleta = luminosidadCompleta & 0xFFFF;
  return luminosidadCompleta;
}

void printToSerialLum(uint32_t lum){
  //Mostramos la luminosidad completa
  Serial.print("Full: ");
  Serial.print(lum);
  Serial.println(" lumenes");
  Serial.println("/*************************/");
}

float getNoise(){
  unsigned int peekToPeek = 0;
  unsigned int signalMax = 0, signalMin = 1024;
  unsigned long startMillis = millis();
  digitalWrite(ledMic, HIGH);
  
  while(millis() - startMillis < sampleWindow){
    sample = analogRead(0);
    if(sample < 1024){
      if(sample > signalMax){
        signalMax = sample;
      }else if(sample < signalMin){
        signalMin = sample;
      }
    }
  } 
  peekToPeek = signalMax - signalMin;  
  //double ruido = (peekToPeek * 3.3) / 1024;
  double ruido = 20 * log10(sample / 2);
  return ruido;
}

void printToSerialNoise(double noise){
   //Serial.print(peekToPeek);
  Serial.print("Ruido: ");
  Serial.print(noise);
  Serial.println(" dB");
  Serial.println("/--------------------/");
  Serial.println("");
}


void setup() {
  if(DEBUG == 1) Serial.begin(9600);
  //Ethernet initialization
  if(!initializeEthernet()){
    //No hacemos nada
    errorFlag = errorFlag | ETHERNET;
    while(true){
        delay(5000);
        if(initializeEthernet()){
          errorFlag = errorFlag ^ ETHERNET;
          break;
        }
    }
  }  
  // put your setup code here, to run once:
  barometro.begin();
  pinMode(ledMic, OUTPUT);
  digitalWrite(ledMic,LOW);
  pinMode(ledLuz, OUTPUT);
  digitalWrite(ledLuz,LOW);
  pinMode(ledBar, OUTPUT);    
  digitalWrite(ledBar,LOW);
  pinMode(ledError, OUTPUT);
  digitalWrite(ledError, LOW);
}

void loop() {
  //Variables para presion y temperatura
  float presion = 0, temperatura = 0;
  //Variables para luminosidad
  uint32_t luminosidadCompleta = -1;
  //Variables microfono
  double ruido = 0;
  
  
  //Lectura de presion y temperatura
  digitalWrite(ledBar, HIGH);
  presion = getPressure();
  if(presion == 0){
    //error de lectura Presion
    errorFlag = errorFlag | PRESION;
  }else{
    if(hasError(PRESION)){
      errorFlag = errorFlag ^ PRESION;
    }
  }
  temperatura = getTemperature();
  if(temperatura == 0){
    //error de lectura temperatura
    errorFlag = errorFlag | TEMPERATURA;
  }else{
    if(hasError(TEMPERATURA)){
      errorFlag = errorFlag ^ TEMPERATURA;
    }
  }
  if(DEBUG == 1) printToSerialTempPress(temperatura, presion);   
  delay(1000);
  digitalWrite(ledBar, LOW);
  /******************************************************/
  //Lectura de luz
  digitalWrite(ledLuz,HIGH);
  luminosidadCompleta = getLuminosity();
  if(luminosidadCompleta == -1){
    //error de lectura Luz
    errorFlag = errorFlag | LUZ;
  }else{
    if(hasError(LUZ)){
      errorFlag = errorFlag ^ RUIDO;
    }
  }
  if(DEBUG == 1) printToSerialLum(luminosidadCompleta);
  delay(1000);
  digitalWrite(ledLuz,LOW);
  /******************************************************/
  //Lectura de ruido
  ruido = getNoise();
  if(ruido == 0){
    //error de lectura Ruido
    errorFlag = errorFlag | RUIDO;
  }else{
    if(hasError(RUIDO)){
      errorFlag = errorFlag ^ RUIDO;
    }
  }
  if(DEBUG == 1) printToSerialNoise(ruido);
  delay(1000);
  digitalWrite(ledMic, LOW);
  
  //Empujar los datos a Dweet
  sendDweet(presion, temperatura, ruido, luminosidadCompleta);
  
}
