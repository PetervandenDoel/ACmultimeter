# ACmultimeter

Link to video demo
https://drive.google.com/file/d/16ZK31mSzWK8TsJp4SmsNh_zpA29mmwEY/view?usp=sharing
The project was demonstrated using a GEN2BB function generator and a BB2SCOPE oscilloscope (referred to as "Fraga Board" in the video)
They are interacted controlled and interacted with using a python script


THE PROJECT

An AC multimeter made with an ATMEL AT89LP51RC2 microcontroller and an MCP3008 analog to digital converter, programmed in C
Values will be displayed on an LCD and can be chosen through a command line interface, the program used for this is a serial console called PuTTY

The project will take in 2 sine waves of the same frequency and can return their peak values, rms values, phase difference in degrees, freqeuncy, and period
based on user inputs to a command line interface.

PEAK VOLTAGE

The peak and RMS voltage of the signals is measured by taking the maximum of 200 succeessive measurments with an MCP3008 analog to digital converter which data is retrieved from using 8 bit SPI. 

FREQUENCY

The period of a signal is measured by detecting a zero cross, waiting for the signal to cross 0.2V to ensure the signal will not cross 0 again due to noise, and starting the AT81's timer to wait until the singal crosses 0 again. Doubling the measured time will detrmine the period which can be used to find the frequency.

PHASE SHIFT

Phase shift is determined by waiting for the lagging signal (fed into ADC input 4 of the MCP3008) to cross 0, then waiting for the second signal to cross 0, and calculating phase shift based on the ratio of the time difference to the period of the signals. There was a chronic tendancy to underestimate the phase shift so 5 degrees is added to all measurements to improve accuracy.


COMMAND LINE INTERFACE

If input to the serial terminal (PuTTY) is modified, getchar() is used to read the input characters and place them into a string. If the string contains a newline or break (\n or \r), the string is compared with the strings ("RMS1","RMS2","peak1","peak2","period","frequency","phase"). If the user enters these keywords and presses enter, the desired measurment will be displayed on an LCD screen.
