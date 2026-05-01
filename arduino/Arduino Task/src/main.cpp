#include <Arduino.h>


//NOTES FOR DEVELOPMENT:
//main mode is set by the consecutive fluctuations
//then check dominant frequency and adjust sample rate 
    //this needs finishing so that the sampling rate is always at least 2x the dominant frequency, 
    //also needs to adjust to lower sample rates if the dominant frequency is low, to save power
//implement moving average 

// put function declarations here:
double* collect_temperature_data();
int TemperatureSensorPin = A0; // Example sensor pin  
double TemperatureReading();
void decide_power_mode(double* dft_results);
void SetSampleRate(double delay);
double* apply_dft(double* temperature_data);
void determine_fluctuations(double* temp_data);
void send_data_to_pc(double* dft_results, double* temperature_data);
const int B = 4275000; // B value of the thermistor
const int R0 = 100000; // R0 = 100k
const double fluctuation_threshold; //threshold for if a fluctuation is high enough to change modes
double SamplingRateDelay;
int inactive_cycles = 0;
const double SampleCollectionTime = 30000; //how double to collect data for dft array
double Sampling_frequency; // Sampling frequency in Hz
int NumSamples; // Number of samples to collect

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  SetSampleRate(1000); 
  Serial.println("Starting temperature data collection...");
}

void SetSampleRate(double delay)
{
  SamplingRateDelay = delay;
  Sampling_frequency = 1000/SamplingRateDelay;
  NumSamples = SampleCollectionTime/SamplingRateDelay; 
  Serial.print("Sampling rate set to ");
  Serial.print(Sampling_frequency);
  Serial.println(" Hz");
}

void loop() {
  // put your main code here, to run repeatedly:
  double* temp_data = collect_temperature_data();
  determine_fluctuations(temp_data);
  double* dft_result = apply_dft(temp_data);
  send_data_to_pc(dft_result, temp_data);
  decide_power_mode(dft_result);
  delete[] temp_data;
  delete[] dft_result;
}

void determine_fluctuations(double* temp_data)
{
  double* fluctuations = new double[NumSamples - 1];
  for (int i = 1; i < NumSamples; i++)//finds the fluctuations in data between consecutive readings
  {
    fluctuations[i-1] = temp_data[i] - temp_data[i-1];
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
  if (high_fluctuations >= 3)
  {
    SetSampleRate(1000); //set to active mode
    inactive_cycles = 0; //reset inactive cycle count
  }
  else
  {
    SetSampleRate(5000); //set to idle mode
    inactive_cycles++;
    if (inactive_cycles >= 5) //if there are 3 consecutive cycles of low fluctuations, set to power-down mode
    {
      SetSampleRate(30000);
    }
  }

  delete[] fluctuations;
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

for (int k = 0; k < NumSamples; k++)
{
  real[k] = 0;
  imag[k] = 0;
  for (int n = 0; n < NumSamples; n++)
  {
    real[k] += temperature_data[n] * cos(2 * PI * k * n / NumSamples);
    imag[k] -= temperature_data[n] * sin(2 * PI * k * n / NumSamples);
  }
  Serial.print("k:");
  Serial.println(k);
  Serial.print("Real:");
  Serial.println(real[k]);
  Serial.print("Imaginary:");
  Serial.println(imag[k]);
  magnitude[k] = sqrt(real[k]*real[k] + imag[k]*imag[k]);
  F[k] = double((double(k) * double(Sampling_frequency)) / double(NumSamples));

  result[k] = F[k];
  result[k + NumSamples] = magnitude[k]; 
}
//free memory
delete[] real;
delete[] imag;
delete[] magnitude;
delete[] F;

  result[NumSamples] = 0;

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

double find_dominant_frequency(double* dft_results)
{
  //find dominant frequency
  int dominant_index = 0;
  double current_max_magnitude = 0;

   for (int i = 0; i < NumSamples/2; i++)//only need to check first half of dft results because of nyquist theorem
  {
    if (dft_results[i + NumSamples] > current_max_magnitude)
    {
      current_max_magnitude = dft_results[i + NumSamples];
      dominant_index = i;
    }
    else if (dft_results[i + NumSamples] == current_max_magnitude)
    {
      //if the data is multimodal take lower 
      if (dft_results[i] < dft_results[dominant_index])
      {
        dominant_index = i;
      }
    }
  }

  return dft_results[dominant_index];
}

void decide_power_mode(double* dft_results)
{
  //choose mode based on average frequency
  double new_sample_delay;
  double dominant_frequency = find_dominant_frequency(dft_results);
  Serial.print("Dominant Frequency: ");
  Serial.println(dominant_frequency);

  //adjust sample rate based on dominant frequency
  if (Sampling_frequency<dominant_frequency*2)
  {
    SetSampleRate(dominant_frequency*2); //active mode
  }
  
  // if (dominant_frequency > 0.5)
  // {
  //   new_sample_delay = 1000; //active mode
  //   inactive_cycles = 0; //reset inactive cycle count
  // }
  // else if (dominant_frequency > 0.1)
  // {
  //   new_sample_delay = 5000; //idle mode
  // }
  // else
  // {
  //   new_sample_delay = 30000; //power-down mode
  // }
    
  // //set delay based on mode
  // SetSampleRate(new_sample_delay);
}
