#include <Arduino.h>
//TODO:
//comment code better
//remove prints

// function declarations:
void collect_temperature_data();
int TemperatureSensorPin = A0; // sensor pin  
double TemperatureReading();
void SetSampleRate(double delay);
void apply_dft();
void send_data_to_pc();

//constants 
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
const double fluctuation_threshold = 0.25; //threshold for if a fluctuation is high enough to change modes
const double SampleCollectionTime = 180000; //how long to collect 1 cycle of temp data 
//global variables
double SamplingRateDelay; //delay between samples
int inactive_cycles = 0; // how many following cycles of low fluctuation
double Sampling_frequency; // Sampling frequency in Hz
const int NumSamples = 180; // Number of samples to collect 
      //NumSamples is constant in this compared to task 2 because we dont need dynamic allocation
      //maximum NumSamples found based on what the arduino can handle without crashing
//static arrays set to max size to avoid memory issues with dynamic allocation
double temperature_data[NumSamples];
double magnitude[NumSamples];

void setup() 
{
  Serial.begin(9600);
  SetSampleRate(1000); 
  Serial.println("Starting temperature data collection...");
}
void loop() 
{
  collect_temperature_data();
  apply_dft();
  send_data_to_pc();
}

void SetSampleRate(double delay)
{ 
  SamplingRateDelay = delay;
  Sampling_frequency = 1000/SamplingRateDelay;
}

double TemperatureReading() //gets the temperature reading from the sensor and converts it to Celsius
{
  int a = analogRead(TemperatureSensorPin);
  if (a == 0) //avoid divide by zero error
  {
    a = 1;
  }
  if (a >= 1023) //avoid divide by zero error
  {
    a = 1022;
  }
  double R = 1023.0/a-1.0;
  R = R0*R;
  double temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
  return temperature;
}

void collect_temperature_data()
{
//collects data for 180 seconds and stores it in an array
for (int i = 0; i < NumSamples; i++)
{
temperature_data[i] = TemperatureReading();
delay(SamplingRateDelay);
Serial.print("Temperature reading ");
Serial.print(i);
Serial.print(": ");
Serial.println(temperature_data[i]);
}
}

void apply_dft()
{
for (int k = 0; k < NumSamples; k++)
{
  double real = 0;
  double imag = 0;
  for (int n = 0; n < NumSamples; n++)
  {
    real += temperature_data[n] * cos(2 * PI * k * n / NumSamples);
    imag -= temperature_data[n] * sin(2 * PI * k * n / NumSamples);
  }
  magnitude[k] = sqrt(real*real + imag*imag);
}
}

void send_data_to_pc()
{
Serial.println("Frequency (Hz), Magnitude");
for (int i = 0; i < NumSamples; i++)
{
  Serial.print(double((double(i) * double(Sampling_frequency)) / double(NumSamples)), 3);//calculates frequency for each bin
  //calculated now instead of in an array to save memory
  Serial.print(", ");
  Serial.println(magnitude[i], 3);
  delay(100);
}
Serial.println("Temperature Data:");
Serial.println("Time (s), Temperature (C)");
for (int i = 0; i < NumSamples; i++)
{
  Serial.print(i * SamplingRateDelay / 1000.0); // Time in seconds
  Serial.print(", ");
  Serial.println(temperature_data[i]);
  delay(100);
}
}
