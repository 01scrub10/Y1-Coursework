#include <Arduino.h>


//NOTES FOR DEVELOPMENT:
//main mode is set by the consecutive fluctuations
//then check dominant frequency and adjust sample rate 
    //this needs finishing so that the sampling rate is always at least 2x the dominant frequency, 
    //also needs to adjust to lower sample rates if the dominant frequency is low, to save power
//implement moving average 

// put function declarations here:
void collect_temperature_data();
int TemperatureSensorPin = A0; // sensor pin  
double TemperatureReading();
void decide_power_mode(int high_fluctuations);
void SetSampleRate(double delay);
void apply_dft();
int determine_fluctuations();
void send_data_to_pc();
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
const double fluctuation_threshold = 0.5; //threshold for if a fluctuation is high enough to change modes
double SamplingRateDelay;
int inactive_cycles = 0;
const double SampleCollectionTime = 10000; //how long to temp collect data 
double Sampling_frequency; // Sampling frequency in Hz
int NumSamples; // Number of samples to collect
//static arrays set to max size to avoid memory issues with dynamic allocation
double fluctuations[60]; // changes in consecutive temperature readings
double temperature_data[60];
double real[60];
double imag[60];
double magnitude[60];
double F[60];

void setup() 
{
  Serial.begin(9600);
  SetSampleRate(1000); 
  Serial.println("Starting temperature data collection...");
}

void SetSampleRate(double delay)
{ 
  //set minimum and maximum delay to keep within acceptable range
  if (delay < 250)
  {
    delay = 250; 
  }
  else if (delay > 10000)
  {
    delay = 10000; 
  }
  SamplingRateDelay = delay;
  Sampling_frequency = 1000/SamplingRateDelay;
  NumSamples = 10; 
  if (NumSamples > 60) //limit number of samples to 60 to avoid memory issues
  {
    NumSamples = 60;
  }
  Serial.print("Sampling rate set to ");
  Serial.print(Sampling_frequency);
  Serial.println(" Hz");
}

void loop() {
  
  collect_temperature_data();
  int high_fluctuations = determine_fluctuations();
  apply_dft();
  send_data_to_pc();
  decide_power_mode(high_fluctuations);
  
}

int determine_fluctuations()
{
  
  for (int i = 1; i < NumSamples; i++)//finds the fluctuations in data between consecutive readings
  {
    fluctuations[i-1] = temperature_data[i] - temperature_data[i-1];
  }
  //finds if there are atleast 3 significant fluctuations in the data, to avoid outliers 
  int high_fluctuations = 0;
  for (int i = 0; i < NumSamples - 1; i++)
  {
    if (abs(fluctuations[i]) > fluctuation_threshold) //threshold for significant fluctuation
    {
      high_fluctuations++;
    }
  }
  
  return high_fluctuations;
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
  real[k] = 0;
  imag[k] = 0;
  for (int n = 0; n < NumSamples; n++)
  {
    real[k] += temperature_data[n] * cos(2 * PI * k * n / NumSamples);
    imag[k] -= temperature_data[n] * sin(2 * PI * k * n / NumSamples);
  }
  magnitude[k] = sqrt(real[k]*real[k] + imag[k]*imag[k]);
  F[k] = double((double(k) * double(Sampling_frequency)) / double(NumSamples));
}
}

void send_data_to_pc()
{
Serial.println("Frequency (Hz), Magnitude");
for (int i = 0; i < NumSamples; i++)
{
  Serial.print(F[i]);
  Serial.print(", ");
  Serial.println(magnitude[i]);
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

double find_dominant_frequency()
{
  //find dominant frequency
  int dominant_index = 1;
  double current_max_magnitude = 0;

   for (int i = 1; i < NumSamples/2; i++)//only need to check first half of dft results because of nyquist theorem
  {
    if (magnitude[i] > current_max_magnitude)
    {
      current_max_magnitude = magnitude[i];
      dominant_index = i;
    }
    else if (magnitude[i] == current_max_magnitude)
    {
      //if the data is multimodal take lower 
      if (F[i] < F[dominant_index])
      {
        dominant_index = i;
      }
    }
  }

  return F[dominant_index];
}

void decide_power_mode(int high_fluctuations)
{
  //choose mode based on both domoinant frequency and fluctuations
  double new_sample_delay;
  //dft dominiant frequency
  double dominant_frequency = find_dominant_frequency();
  Serial.print("Dominant Frequency: ");
  Serial.println(dominant_frequency);

  
  //first set mode based on fluctuations

  if (high_fluctuations >= 3)
  {
    new_sample_delay = 1000; //set to active mode
    inactive_cycles = 0; //reset inactive cycle count
    Serial.println("High fluctuations detected, setting to active mode");
  }
  else
  {
    new_sample_delay = 5000; //set to idle mode
    inactive_cycles++;
    if (inactive_cycles >= 5) //if there are 3 consecutive cycles of low fluctuations, set to power-down mode
    {
      new_sample_delay = 10000; //set to power-down mode
      //decided on 10 second delay as it gives enough readings in 1m for some analysis 
    }
  }
  Serial.print("inactive cycles: ");
  Serial.println(inactive_cycles);

  //then adjust the sample delay dynamically based on dominant frequency
    //if the dominant frequency is more than half the sampling freq, increase to 2x for nyquists theorum
    if (Sampling_frequency < 2 * dominant_frequency)
    {
      new_sample_delay = 1000 / (2 * dominant_frequency); //set to 2x the dominant frequency
    }
    else if (Sampling_frequency>8*dominant_frequency)// if the dominant frequency is much lower than sampling rate
    {
      //decreases sample rate proportionally to the difference between the dominant frequency and the sampling frequency, to save power
      new_sample_delay = SamplingRateDelay * (Sampling_frequency / (4 * dominant_frequency));
    }

  //set new delay
   SetSampleRate(new_sample_delay);
}
