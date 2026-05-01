#include <Arduino.h>

// put function declarations here:
float* collect_temperature_data();
int TemperatureSensorPin = A0; // Example sensor pin  
float TemperatureReading();
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
int SamplingRateDelay = 1000;
int SampleCollectionTime = 180000; // 180 seconds

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  Serial.println("Starting temperature data collection...");
}

void loop() {
  // put your main code here, to run repeatedly:
  float* data = collect_temperature_data();
  for(int i = 0; i < SampleCollectionTime/SamplingRateDelay; i++)
  {
    Serial.print(data[i]);
    Serial.print(", ");
  }
  Serial.println();
}

float TemperatureReading() //gets the temperature reading from the sensor and converts it to Celsius
{
  int a = analogRead(TemperatureSensorPin);
  float R = 1023.0/a-1.0;
  R = R0*R;
  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
  Serial.print("temperature = ");
  Serial.println(temperature);
  delay(100); - 273.15;
  return temperature;
}

float* collect_temperature_data()
{
//collects data for 180 seconds and stores it in an array
int Loops = SampleCollectionTime/SamplingRateDelay;
float* temperature_data = new float[Loops];
int i = 0;
while(i<Loops)
{
temperature_data[i] = TemperatureReading();
Serial.print(i);
delay(SamplingRateDelay);
i++;
}
return temperature_data;
}