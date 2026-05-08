import matplotlib.pyplot as plt
import csv
from numpy import double

temperaturedatasources = ['Task 4/Data Set 1/temperature_data.csv', 'Task 4/Data Set 2/temperature_data.csv', 'Task 4/Data Set 3/temperature_data.csv']
dftdatasources = ['Task 4/Data Set 1/dft_data.csv', 'Task 4/Data Set 2/dft_data.csv', 'Task 4/Data Set 3/dft_data.csv']

for s in range(len(temperaturedatasources)): ##repeats for both data sets
# PLOT 1 - Temperature data over time
    with open(temperaturedatasources[s], 'r') as file:
        reader = csv.reader(file)
        time = []
        temperature = []
        for row in reader:
            time.append(float(row[0]))
            temperature.append(float(row[1]))
        file.close()

    with open(dftdatasources[s], 'r') as file:
        reader = csv.reader(file)
        frequency = []
        magnitude = []
        for row in reader:
            frequency.append(float(row[0]))
            magnitude.append(float(row[1]))
        file.close()


    # PLOT 1 - Temperature data over time
    plt.plot(time, temperature)
    plt.xlabel('Time (s)')
    plt.ylabel('Temperature (C)')
    plt.title('Temperature Data Collection')
    plt.show()


    # PLOT 2 - magnitude vs frequency from DFT



    plt.plot(frequency, magnitude)
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('Magnitude')
    plt.title('Frequency Spectrum')
    plt.show()


    # PLOT 3 - Smoothed temperature data vs time

        #find moving average with window size of 5
    smoothedTemp = [0] * len(temperature)
    windowsize = 5 #the range of values around a data point used to make the average (including this data point). Must be odd
    maxpointdist = (windowsize - 1)/2 #amount of points on either side of the data point used to make the average. 
    for i in range(int(maxpointdist), int(len(temperature)-maxpointdist)):
        for x in range(int(i - maxpointdist),int(i - maxpointdist + windowsize)):
            smoothedTemp[i] += temperature[x] #sums nearest 5 points
        smoothedTemp[i] = smoothedTemp[i]/windowsize # divides by window size to get average

    print(temperature)
    print(smoothedTemp)
    plt.plot(time, temperature, color='gray', label='Original Data', alpha=0.75) # plot original data in gray
    plt.plot(time[2: (len(time)-4):2], smoothedTemp[2: (len(smoothedTemp)-4):2], color='red', linestyle='--', label='Smoothed Data') # plot smoothed data in red dashed line
    plt.xlabel('Time (s)')
    plt.ylabel('Smoothed Temperature (C)')
    plt.title('Smoothed Temperature Data')
    plt.legend()
    #set color of line to red and make it dashed
    plt.show()

    # PLOT 4 - Temperature data histogram
    plt.hist(temperature, bins=20, color='blue', edgecolor='black')
    plt.xlabel('Temperature (C)')
    plt.ylabel('Frequency')
    plt.title('Temperature Data Distribution')
    plt.show()

    # PLOT 5 - Consecutive temperature change vs time
    temperatureChange = [0] * len(temperature)
    for i in range(1, len(temperature)):
        temperatureChange[i] = temperature[i] - temperature[i-1]

    plt.plot(time[1:], temperatureChange[1:])
    plt.xlabel('Time (s)')
    plt.ylabel('Consecutive Temperature Change (C)')
    plt.title('Consecutive Temperature Change Distribution')
    plt.show()