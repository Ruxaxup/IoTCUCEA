#include <Wire.h>
#include <TSL2561.h> //Luminosidad
#include <Adafruit_MPL115A2.h> //Barometro

Adafruit_MPL115A2 barometro; //Barometro
TSL2561 luminosidad(TSL2561_ADDR_FLOAT); //Luminosidad
const int sampleWindow = 50; //50 ms = 20 Hz
unsigned int sample;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  barometro.begin();
}

void loop() {
  //Variables para presion y temperatura
  float presion = 0, temperatura = 0;
  //Variables para luminosidad
  uint32_t luminosidadCompleta;
  //Variables microfono
  unsigned long startMillis = millis();
  unsigned int peekToPeek = 0;
  unsigned int signalMax = 0, signalMin = 1024;
  
  //Lectura de presion y temperatura
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
  //Lectura de luz
  luminosidadCompleta = luminosidad.getFullLuminosity();
  luminosidadCompleta = luminosidadCompleta & 0xFFFF;
  //Mostramos la luminosidad completa
  Serial.print("Full: ");
  Serial.print(luminosidadCompleta);
  Serial.println(" lumenes");
  Serial.println("/*************************/");
  
  //Lectura de ruido
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
  Serial.print(peekToPeek);
  Serial.print("Ruido: ");
  Serial.print(volts);
  Serial.println(" V");
  Serial.println("/--------------------/");
  delay(5000);
  
}
