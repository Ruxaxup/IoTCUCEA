#include <Wire.h>
#include <TSL2561.h> //Luminosidad
#include <Adafruit_MPL115A2.h> //Barometro
#include <Ethernet.h>

Adafruit_MPL115A2 barometro; //Barometro
TSL2561 luminosidad(TSL2561_ADDR_FLOAT); //Luminosidad
const int sampleWindow = 50; //50 ms = 20 Hz
unsigned int sample;
int ledMic = 7;
int ledLuz = 12;
int ledBar = 13;

/***** Ethernet ******/
#define thingName "Test_Galileo_1"
//MAC esta escrita en la etiqueta del puerto Ethernet
byte mac[] = { 0x98, 0x4F, 0xEE, 0x00, 0xE5, 0x86 };
EthernetClient client;
char server[] = "www.dweet.io"; 
/********************/

boolean initializeEthernet(){
  
  //Start Ethernet connection
  if(Ethernet.begin(mac) == 0){
    Serial.print("Failed to configure Ethernet using DHCP");
    return false;
  }
  Serial.print("Connecting..."); 
  return true;
}

/** Recopila y manda las lecturas a Dweet **/
void sendDweet(float presion, float temperatura, float ruido,
               uint32_t lumens)
{

  if(client.connect(server,80)){
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
  }else{
    Serial.println("Connection to Dweet failed");
  }
}

void setup() {
  
  //Ethernet initialization
  if(!initializeEthernet()){
    //No hacemos nada
    while(true){}
  }
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  barometro.begin();
  pinMode(ledMic, OUTPUT);
  digitalWrite(ledMic,LOW);
  pinMode(ledLuz, OUTPUT);
  digitalWrite(ledLuz,LOW);
  pinMode(ledBar, OUTPUT);    
  digitalWrite(ledBar,LOW);
}

void loop() {
  //Variables para presion y temperatura
  float presion = 0, temperatura = 0;
  //Variables para luminosidad
  uint32_t luminosidadCompleta;
  //Variables microfono

  unsigned int peekToPeek = 0;
  unsigned int signalMax = 0, signalMin = 1024;
  
  //Lectura de presion y temperatura
  digitalWrite(ledBar, HIGH);
  presion = barometro.getPressure();
  temperatura = barometro.getTemperature();
  //Mostramos datos leidos
  Serial.print("Presion (kPa): ");
  Serial.print(presion, 4);
  Serial.println(" kPa");
  Serial.print("Temperatura (*C): ");
  Serial.print(temperatura, 1);
  Serial.println(" *C");
  Serial.println("/*************************/");  
  delay(1000);
  digitalWrite(ledBar, LOW);
  //Lectura de luz
  digitalWrite(ledLuz,HIGH);
  luminosidadCompleta = luminosidad.getFullLuminosity();
  luminosidadCompleta = luminosidadCompleta & 0xFFFF;
  //Mostramos la luminosidad completa
  Serial.print("Full: ");
  Serial.print(luminosidadCompleta);
  Serial.println(" lumenes");
  Serial.println("/*************************/");
  delay(1000);
  digitalWrite(ledLuz,LOW);

  //Lectura de ruido
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
  double volts = (peekToPeek * 3.3) / 1024;
  
  //Mostramos el ruido o lo que sea
  //Serial.print(peekToPeek);
  Serial.print("Ruido: ");
  Serial.print(volts);
  Serial.println(" V");
  Serial.println("/--------------------/");
  Serial.println("");
  delay(1000);
  digitalWrite(ledMic, LOW);
  
  //Empujar los datos a Dweet
  sendDweet(presion, temperatura, volts, luminosidadCompleta);
  
}
