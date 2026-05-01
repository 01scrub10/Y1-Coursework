#include <Arduino.h>

// put function declarations here:
double* collect_temperature_data();
int TemperatureSensorPin = A0; // Example sensor pin  
double TemperatureReading();
void decide_power_mode();
double* apply_dft(double* temperature_data);
void send_data_to_pc(double* dft_results, double* temperature_data);
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
double SamplingRateDelay; 
const double SampleCollectionTime = 30000; //how double to collect data for dft array
const double Sampling_frequency = 1000/SamplingRateDelay; // Sampling frequency in Hz
const int NumSamples = SampleCollectionTime/SamplingRateDelay; // Number of samples to collect

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  SamplingRateDelay = 1000;
  Serial.println("Starting temperature data collection...");
}

void loop() {
  // put your main code here, to run repeatedly:
  double* temp_data = collect_temperature_data();
  double* dft_result = apply_dft(temp_data);
  send_data_to_pc(dft_result, temp_data);
}

double TemperatureReading() //gets the temperature reading from the sensor and converts it to Celsius
{
  int a = analogRead(TemperatureSensorPin);
  double R = 1023.0/a-1.0;
  R = R0*R;
  double temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
  return temperature;
}

double* collect_temperature_data()
{
//collects data for 180 seconds and stores it in an array

double* temperature_data = new double[NumSamples];
int i = 0;
while(i<NumSamples)
{
temperature_data[i] = TemperatureReading();
delay(SamplingRateDelay);
Serial.print("Collected data point ");
Serial.println(i);
i++;
}
return temperature_data;
}

double* apply_dft(double* temperature_data)
{
  double* real = new double[NumSamples];
  double* imag = new double[NumSamples];
  double* magnitude = new double[NumSamples];
  double* F = new double[NumSamples];
  double* result = new double[NumSamples * 2]; //first half for frequencies, second half for magnitudes

for (int k = 1; k < NumSamples; k++)
{
  real[k] = 0;
  imag[k] = 0;
  for (int n = 1; n < NumSamples; n++)
  {
    real[k] += temperature_data[n] * cos(2 * PI * k * n / NumSamples);
    imag[k] += temperature_data[n] * sin(2 * PI * k * n / NumSamples);
  }
  imag[k] = -imag[k]; //invert the imaginary part to match the standard DFT definition
  magnitude[k] = sqrt(real[k]*real[k] + imag[k]*imag[k]);
  F[k] = double((double(k) * double(Sampling_frequency)) / double(NumSamples));

  result[k] = F[k];
  result[k + NumSamples] = (2.0/NumSamples) * magnitude[k]; //amplitude scaling 

}


return result;
}

void send_data_to_pc(double* dft_results, double* temperature_data)
{
Serial.println("Frequency (Hz), Magnitude");
for (int i = 0; i < NumSamples; i++)
{
  Serial.print(dft_results[i]);
  Serial.print(", ");
  Serial.println(dft_results[i + NumSamples]);
}
Serial.println("Temperature Data:");
Serial.println("Time (s), Temperature (C)");
for (int i = 0; i < NumSamples; i++)
{
  Serial.print(i * SamplingRateDelay / 1000.0); // Time in seconds
  Serial.print(", ");
  Serial.println(temperature_data[i]);
}
}

