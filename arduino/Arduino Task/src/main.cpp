#include <Arduino.h>


//TODO:
//implement moving average
//calc frequency dft array on fly instead of stored in array 
//comment code better
//remove prints

// put function declarations here:
void collect_temperature_data();
int TemperatureSensorPin = A0; // sensor pin  
double TemperatureReading();
void decide_power_mode(int high_fluctuations);
void SetSampleRate(double delay);
void apply_dft();
bool determine_fluctuations();
void send_data_to_pc();
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
const double significant_fluctuation = 0.5; //value which is considered the threshold for high change of temperature in one minute
const double fluctuation_threshold = 0.5; //threshold for if a fluctuation is high enough to change modes
double SamplingRateDelay; //delay between samples
int inactive_cycles = 0; // how many following cycles of low fluctuations
const double SampleCollectionTime = 60000; //how long to temp collect data 
double Sampling_frequency; // Sampling frequency in Hz
int NumSamples; // Number of samples to collect
//static arrays set to max size to avoid memory issues with dynamic allocation
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
void loop() {
  
  collect_temperature_data();
  bool high_fluctuations = determine_fluctuations();
  apply_dft();
  send_data_to_pc();
  decide_power_mode(high_fluctuations);
  
}

void SetSampleRate(double delay)
{ 
  //set minimum and maximum delay to keep within acceptable range
    //decided on range of 0.1Hz - 4Hz
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
  NumSamples = int(SampleCollectionTime / SamplingRateDelay); 
  if (NumSamples > 60) //limit number of samples to 60 to avoid memory issues
  {
    NumSamples = 60;
  }
  Serial.print("Sampling rate set to ");
  Serial.print(Sampling_frequency);
  Serial.println(" Hz");
}



bool determine_fluctuations()
{
  //determines the range of the data to find if there is significant fluctuations
    //decided on using range instead of consecutive change as with differing sample rates, "significant change" can be different
  double min_temp = temperature_data[0];
  double max_temp = temperature_data[0];
    for (int i =0;i<NumSamples;i++)
  {
    if (temperature_data[i]<min_temp)
    {
      min_temp = temperature_data[i];
    }
    else if(temperature_data[i]>max_temp)
    {
      max_temp = temperature_data[i];
    }
  }

  double range = max_temp - min_temp;
  if (range > significant_fluctuation)
  {
    return true;
  }
  else 
  {
    return false;
  }
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

double find_dominant_frequency()
{
  //find dominant frequency
  int dominant_index = 1; //assume lowest frequency is dominant to avoid freq=0 issues
  double current_max_magnitude = magnitude[1];

   for (int i = 1; i < NumSamples/2; i++)//only need to check first half of dft results because of nyquist theorem
  {
    if (current_max_magnitude - magnitude[i] < -0.01)//if magnitude[i] > current max magnitude. Have to use this instead of > to avoid issues with very small differences due to noise
    {
      current_max_magnitude = magnitude[i];
      dominant_index = i;
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

  if (high_fluctuations)
  {
    new_sample_delay = 1000; //set to active mode
    inactive_cycles = 0; //reset inactive cycle count
    Serial.println("High fluctuations detected, setting to active mode");
  }
  else
  {
    new_sample_delay = 5000; //set to idle mode
    inactive_cycles++;
    if (inactive_cycles >= 3) //if there are 3 consecutive cycles of low fluctuations, set to power-down mode
    {
      new_sample_delay = 10000; //set to power-down mode
      //decided on 10 second delay as it gives enough readings in 1m for some analysis 
    }
  }
  Serial.print("inactive cycles: ");
  Serial.println(inactive_cycles);
  delay(100); //short delay to stop serial output corruption
  
  //then adjust the sample delay dynamically based on dominant frequency
    //if the dominant frequency is more than half the sampling freq, increase to 2x for nyquists theorum
    if (1000/new_sample_delay < dominant_frequency*2)
    {
      new_sample_delay = new_sample_delay * 0.75;//increase the rate of sampling
      Serial.println("sampling frequency is lower than nyquist of dominant frequency, increasing sample rate");
    }
    else if (1000/new_sample_delay>8*dominant_frequency)// if the dominant frequency is much lower than sampling rate
    {
      //decreases sample rate slightly to save power
      new_sample_delay = new_sample_delay * 1.5; //increase delay by 50%
      Serial.println("Dominant frequency is much lower than sampling frequency, decreasing sample rate to save power");
    }

  //set new delay
  delay(100); //short delay to stop serial output corruption
   SetSampleRate(new_sample_delay);
}
