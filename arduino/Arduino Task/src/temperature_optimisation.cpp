#include <Arduino.h>

// function declarations:
void collect_temperature_data();
int TemperatureSensorPin = A0; // sensor pin  
double TemperatureReading();
void decide_power_mode(double current_fluctuation);
double get_average_fluctuation(double current_fluctuation);
void SetSampleRate(double delay);
void apply_dft();
double determine_fluctuation();
void send_data_to_pc();

//constants 
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
const double fluctuation_threshold = 0.25; //threshold for if a fluctuation is high enough to change modes
const double SampleCollectionTime = 60000; //how long to collect 1 cycle of temp data 
//global variables
double SamplingRateDelay; //delay between samples
int inactive_cycles = 0; // how many following cycles of low fluctuation
double Sampling_frequency; // Sampling frequency in Hz
int NumSamples; // Number of samples to collect
//static arrays set to max size to avoid memory issues with dynamic allocation
double temperature_data[60];
double magnitude[60];
double fluctuations[10]; //has the fluctuations of previous cycles for moving average prediction
int fluctuation_index = 0; //index for fluctuations array

void setup() 
{
  Serial.begin(9600);
  SetSampleRate(1000); 
  Serial.println("Starting temperature data collection...");
}
void loop() 
{
  collect_temperature_data();
  double current_fluctuation = determine_fluctuation();
  apply_dft();
  send_data_to_pc();
  decide_power_mode(current_fluctuation);
}

void SetSampleRate(double delay)
{ 
  //set minimum and maximum delay to keep within acceptable fluctuation
    //decided on fluctuation of 0.1Hz - 1Hz
  if (delay < 1000)
  {
    delay = 1000; 
  }
  else if (delay > 10000)
  {
    delay = 10000; 
  }
  //calculate sampling frequency and num samples based on delay and set to global variables
  SamplingRateDelay = delay;
  Sampling_frequency = 1000/SamplingRateDelay;
  NumSamples = int(SampleCollectionTime / SamplingRateDelay); 
  if (NumSamples > 60) //limit number of samples to 60 to avoid memory issues
  {
    NumSamples = 60;
  }
  
}



double determine_fluctuation()
{
  //determines the fluctuation of the data to find if there is significant fluctuation
  double total_differences = 0;
  double average_fluctuation;
  for (int i = 1;i<=NumSamples;i++)//difference between consecutive samples 
  {
    total_differences = abs(temperature_data[i] - temperature_data[i-1]);
  }
  average_fluctuation = total_differences/NumSamples;//mean average
  return average_fluctuation;
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
  }
}

void apply_dft()
{//applies the dft formulae from the documentation to the collected data
  for (int k = 0; k < NumSamples; k++)
  {
    double real = 0;
    double imag = 0;
    for (int n = 0; n < NumSamples; n++)
    {
      //find real and imaginary parts of dft
        //only need to store as single variables as are not needed after magnitude is calculated
      real += temperature_data[n] * cos(2 * PI * k * n / NumSamples);
      imag -= temperature_data[n] * sin(2 * PI * k * n / NumSamples);
    }
    magnitude[k] = sqrt(real*real + imag*imag);//magnitude of complex parts
  }
}

void send_data_to_pc()
{//prints data in serial monitor
Serial.println("Frequency (Hz), Magnitude");
for (int i = 0; i < NumSamples; i++)
{
  Serial.print(double((double(i) * double(Sampling_frequency)) / double(NumSamples)));//calculates frequency for each bin
  //calculated now instead of in an array to save memory
  Serial.print(", ");
  Serial.println(magnitude[i]);
  delay(100);//delay to stop serial output corruption
}
Serial.println("Temperature Data:");
Serial.println("Time (s), Temperature (C)");
for (int i = 0; i < NumSamples; i++)
{
  Serial.print(i * SamplingRateDelay / 1000.0); // Time in seconds
  Serial.print(", ");
  Serial.println(temperature_data[i]);
  delay(100);//delay to stop serial output corruption
}
}

double find_dominant_frequency()
{
  //find dominant frequency
  int dominant_index = 1; //assume lowest frequency is dominant to avoid freq=0 issues
  double current_max_magnitude = magnitude[1];
  //searches for maximum magnitude
   for (int i = 1; i < NumSamples/2; i++)//only need to check first half of dft results because of nyquist theorem
  {
    if (current_max_magnitude - magnitude[i] < -0.01)//if magnitude[i] > current max magnitude. Have to use this instead of > to avoid issues with very small differences due to noise
    {
      current_max_magnitude = magnitude[i];
      dominant_index = i;
    }
  }
 //calculate frequency of dominant index to save memory instead of storing in an array
  return double((double(dominant_index) * double(Sampling_frequency)) / double(NumSamples));
}

double get_average_fluctuation(double current_fluctuation)
{
  //calculates a moving average of the last 10 averages
  double average = 0;
  if (fluctuation_index >= 10)//replaces the oldest element with the newest if the array is full (circular)
  {
    fluctuation_index = 0;
  }
  fluctuations[fluctuation_index] = current_fluctuation;
  fluctuation_index++;
  

  if (fluctuations[9] != 0)//only calculates an average if the array is full
  {
    double total = 0;
    for (int i = 0;i<10;i++)
    {
      total += fluctuations[i];
    }
    average = total/10;
  }
  return average;
}

void decide_power_mode(double current_fluctuation)
{
  //choose mode based on both domoinant frequency and fluctuation
  double new_sample_delay;
  //dft dominiant frequency
  double dominant_frequency = find_dominant_frequency();

  
  //first set mode based on fluctuation
  double mode_rate_delay;
  
  if (current_fluctuation > fluctuation_threshold)
  {
    mode_rate_delay = 1000; //set to active mode
    inactive_cycles = 0; //reset inactive cycle count
  }
  else
  {
     mode_rate_delay = 5000;//converge towards idle mode
    inactive_cycles++;
    if (inactive_cycles >= 5) //if there are 5 consecutive cycles of low fluctuation, set to power-down mode
    {
      mode_rate_delay = 10000; //set to power-down mode
      //decided on 10 second delay as it gives enough readings in 1min for some analysis 
    }
  }
  //converge toward mode's sample rate
  new_sample_delay = SamplingRateDelay + ((mode_rate_delay - SamplingRateDelay) * 0.5);

  //consider moving average of fluctuations to predict future high fluctuation
  double temp_fluctuation_avg = get_average_fluctuation(current_fluctuation);
  
  if (temp_fluctuation_avg >0)
  {
    if(temp_fluctuation_avg >0.15)//using lower boundaries for the average as it will be lower than current fluctuation average
    {
      mode_rate_delay = 1000;//predicted to be active
    }
    else if (temp_fluctuation_avg > 0.05)
    {
      mode_rate_delay = 5000;//predicted to be idle
    }
    else 
    {
      mode_rate_delay = 10000;//predicted to be power-down
    }
  }
  //adjust the new sample delay again using the same method but with a lower weight
  new_sample_delay = new_sample_delay + ((mode_rate_delay - new_sample_delay) * 0.25);
  
  //then adjust the sample delay dynamically based on dominant frequency
    //if the dominant frequency is more than half the sampling freq, increase to 2x for nyquists theorum
    if (1000/new_sample_delay < dominant_frequency*2)
    {
      new_sample_delay = new_sample_delay * 0.75;//increase the rate of sampling
    }
    else if (1000/new_sample_delay>8*dominant_frequency)// if the dominant frequency is much lower than sampling rate
    {
      //decreases sample rate slightly to save power
      new_sample_delay = new_sample_delay * 1.5; //increase delay by 50%
    }

  
    delay(100); //short delay to stop serial output corruption
   SetSampleRate(new_sample_delay);//set new delay
}
