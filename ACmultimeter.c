

#include <stdio.h>
#include <at89lp51rd2.h>
#include <math.h>

#define CLK    22118400L // clock frequency in Hz
#define BAUD     115200L // Baud rate of UART in bps
#define ONE_USEC (CLK/1000000L) // Timer reload for one microsecond delay

#if (CLK/(16L*BAUD))>0x100
#error Can not set baudrate
#endif
#define BRG_VAL (0x100-(CLK/(16L*BAUD)))


#define ADC_CE  P2_0
#define BB_MOSI P2_1
#define BB_MISO P2_2
#define BB_SCLK P2_3


#define LCD_RS P3_2
#define LCD_E  P3_3
#define LCD_D4 P3_4
#define LCD_D5 P3_5
#define LCD_D6 P3_6
#define LCD_D7 P3_7
#define CHARS_PER_LINE 16

char mystr[CHARS_PER_LINE+1];






unsigned char _c51_external_startup(void)
{

	AUXR=0B_0001_0001; // 1152 bytes of internal XDATA, P4.4 is a general purpose I/O

	// If the ports are configured in compatibility mode, this is not needed.
	P0M0=0; P0M1=0;
	P1M0=0; P1M1=0;
	P2M0=0; P2M1=0;
	P3M0=0; P3M1=0;
	
	
		// Initialize the pins used for SPI
	ADC_CE=0;  // Disable SPI access to MCP3008 analog to difital converter
	BB_SCLK=0; // Resting state of SPI clock is '0'
	BB_MISO=1; // Write '1' to MISO before using as input
	
	// Configure Serial Port and Baud Rate
    PCON|=0x80;
	SCON = 0x52;
    BDRCON=0;
    
    #if (CLK/(16L*BAUD))>0x100
    #error Can not set baudrate
    #endif
    
    BRL=BRG_VAL;
    BDRCON=BRR|TBCK|RBCK|SPD;
    
	CLKREG=0x00; // TPS=0000B
    
    return 0;
}



//8 bit SPI interface
unsigned char SPIWrite(unsigned char out_byte)
{
	// In the 8051 architecture both ACC and B are bit addressable!
	ACC=out_byte;
	
	BB_MOSI=ACC_7; BB_SCLK=1; B_7=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_6; BB_SCLK=1; B_6=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_5; BB_SCLK=1; B_5=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_4; BB_SCLK=1; B_4=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_3; BB_SCLK=1; B_3=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_2; BB_SCLK=1; B_2=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_1; BB_SCLK=1; B_1=BB_MISO; BB_SCLK=0;
	BB_MOSI=ACC_0; BB_SCLK=1; B_0=BB_MISO; BB_SCLK=0;
	
	return B;
}






void wait_us (unsigned char x)
{
	unsigned int j;
	
	TR0=0; // Stop timer 0
	TMOD&=0xf0; // Clear the configuration bits for timer 0
	TMOD|=0x01; // Mode 1: 16-bit timer
	
	if(x>5) x-=5; // Subtract the overhead
	else x=1;
	
	j=-ONE_USEC*x;
	TF0=0;
	TH0=j/0x100;
	TL0=j%0x100;
	TR0=1; // Start timer 0
	while(TF0==0); //Wait for overflow
}

void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) wait_us(250);
}


/*Read 10 bits from the MCP3008 ADC converter*/
unsigned int volatile GetADC(unsigned char channel)
{
	unsigned int adc;
	unsigned char spid;

	ADC_CE=0; // Activate the MCP3008 ADC.
	
	SPIWrite(0x01);// Send the start bit.
	spid=SPIWrite((channel*0x10)|0x80);	//Send single/diff* bit, D2, D1, and D0 bits.
	adc=((spid & 0x03)*0x100);// spid has the two most significant bits of the result.
	spid=SPIWrite(0x00);// It doesn't matter what we send now.
	adc+=spid;// spid contains the low part of the result. 
	
	ADC_CE=1; // Deactivate the MCP3008 ADC.
		
	return adc;
}


#define VREF 4.096






void LCD_pulse (void)
{
	LCD_E=1;
	wait_us(40);
	LCD_E=0;
}

void LCD_byte (unsigned char x)
{

	ACC=x; //Send high nible
	LCD_D7=ACC_7;
	LCD_D6=ACC_6;
	LCD_D5=ACC_5;
	LCD_D4=ACC_4;
	LCD_pulse();
	wait_us(40);
	ACC=x; //Send low nible
	LCD_D7=ACC_3;
	LCD_D6=ACC_2;
	LCD_D5=ACC_1;
	LCD_D4=ACC_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS=1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS=0;
	LCD_byte(x);
	waitms(5);
}

void LCD_4BIT (void)
{
	LCD_E=0; // Resting state of LCD's enable is zero
	//LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, bit clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}

float measure_period (void) {
	//float output_voltage=(GetADC(0)*VREF)/1023.0;
	float half_period;
	float period;
	int overflow_counter;
	
	TMOD&=0B_1111_0000; //sets the timer 0 as a 16-bit timer
	TMOD|=0B_0000_0001; //sets time 0 as a 16 bit timer
	
	TL0=0; TH0=0; overflow_counter=0;

	//zero cross with hysteresis
	while(GetADC(0)*VREF/1023.0>0.0);
	while(GetADC(0)*VREF/1023.0<0.2); 
	//zero cross detected
	TF0=0; //clears the overflow flag
	TR0=1; //start timer 0
	//while((GetADC(0)*VREF)/1023.0>3.9) //waits for signal to be zero again
	while(GetADC(0)*VREF/1023.0>0.0)
	{
		if(TF0) {TF0=0; overflow_counter++;}
		//printf("third loop reached\n");	
	}
	//stop timer
	TR0=0;
	half_period=overflow_counter*65536.0+TH0*256.0+TL0; //the 24-bit number [myof-TH0-TL0]
	//printf("out of all loops\n");
	//units are clock cycles
	period=2*half_period;
	//convert to seconds
	period=period/CLK;
	return period;
	
}



float find_peak(int port){
		int i;
		float max=0;
		float voltage;
		for(i=0;i<200;i++){
			voltage=GetADC(port)*VREF/1023.0;
			if(voltage>max){max=voltage;}
		}
		return max;				
}

float measure_phase(float frequency){
	
	//float output_voltage=(GetADC(0)*VREF)/1023.0;
	float time_diff;
	int overflow_counter;
	
	TMOD&=0B_1111_0000; //sets the timer 0 as a 16-bit timer
	TMOD|=0B_0000_0001; //sets time 0 as a 16 bit timer

	TL0=0; TH0=0; overflow_counter=0;

	while(GetADC(4)*VREF/1023.0>0.0);
	while(GetADC(4)*VREF/1023.0<0.2);//zero cross detected at pin4
	TF0=0; //clears the overflow flag
	TR0=1; //start timer 0
	
	
		while(GetADC(4)*VREF/1023.0>0.0)
	{
		
		if(TF0) {TF0=0; overflow_counter++;}
		//printf("third loop reached\n");	
		if(GetADC(0)*VREF/1023.0>0.0){
			break;//break out the loop if zero cross is detected on the input that is ahead
		}
	}

	//stop timer
	TR0=0;
	
	time_diff=(overflow_counter*65536.0+TH0*256.0+TL0)/CLK;
	
	return time_diff*360*frequency+5; //adding 5 accounts for error where phase is chronically low by 5 or so degrees
	

}






//compares two strings, returns 0 if they are equal and 1 if they are not

int compare(char a[],char b[])  
{  
    int flag=0,i=0;  // integer variables declaration  
    while(a[i]!='\0' &&b[i]!='\0')  // while loop  
    {  
       if(a[i]!=b[i])  
       {  
           flag=1;  
           break;  
       }  
       i++;  
    }  
    if(flag==0)  
    return 0;  
    else  
    return 1;  
}  














void main (void)
{
	float frequency,peak_voltage_1,peak_voltage_2,phase_shift,previous_voltage_1=0.0,previous_voltage_2 = 0.0;

	//initialize as 100 so that value is at similar order of magnitude to frequencies being measured
	
	unsigned char j;
	char c;
		
	//char output_buffer[16];
	int i;
	
	//printf("reset\n");
	
	waitms(500); // Gives time to putty to start before sending text
	
	// Configure the LCD
	LCD_4BIT();
	
   	// Display something in the LCD
	//LCDprint("LCD 4-bit test:", 1, 1);
	//LCDprint("Hello, World!", 2, 1);
	
	// Send text to putty
	//printf("LCD test.\nType something and press <Enter>\n(it will show in the LCD, %d characters max): ", CHARS_PER_LINE);
	

	//main loop
	while(1)
	{
	

	//measure the peak voltage of signal 1
for(i=0;i<2;i++){		
	peak_voltage_1=find_peak(0);
	
	if(peak_voltage_1<previous_voltage_1){
		peak_voltage_1=previous_voltage_1;
	}
	else{
		previous_voltage_1=peak_voltage_1;
	}
	
	}
	
	
	
	
	//measure the peak voltage of signal 2
	peak_voltage_2=find_peak(4);
	
	if(peak_voltage_2<previous_voltage_2){
		peak_voltage_2=previous_voltage_2;
	}
	else{
		previous_voltage_2=peak_voltage_2;
	}
	
	
	//RMS VALUES ARE STILL NAMED PEAK VOLTAGE TO CONSERVE MEMORY
	peak_voltage_1=peak_voltage_1 * 0.707016;
	peak_voltage_2=peak_voltage_2 * 0.707016;

	frequency=1.0/measure_period();


	phase_shift=measure_phase(frequency);
	

	//for testing the ADC ports
	/*
	for(i=0;i<8;i++){
	peak_voltage_1=find_peak(i);
		printf("port %d gives peak %f\n",i,peak_voltage_1);
	}

//for testing outputs on LCD without CLI
*/

	//printf("peak is %f\n",peak_voltage_2);
	

	//printf("rms V1 is %f\n", rms_1);
	//printf("rms V2 is %f\n", rms_2);
	

	
	/*phase_shift=find_phase_shift2(frequency);
	printf("Phase shift is %f\n",phase_shift);
	*/
	
	
	
	/*
	sprintf(mystr,"%1.2fHZ%1.1fV%1.1fV",frequency,peak_voltage_1,peak_voltage_2);
	
	LCDprint(mystr, 1, 1);
	
	sprintf(mystr,"phase %2.1f deg",phase_shift);
		
	LCDprint(mystr, 2, 1);
	*/

	
	
	
	//code for putty CLI



		if(RI)
		{

			for(j=0; j<CHARS_PER_LINE; j++)
			{
				c=getchar();
				if((c=='\n')||(c=='\r'))
				{
					mystr[j]=0;
					//LCDprint(mystr, 2, 1);
					//now that string is null terminated we want to print corresponding value
					if(compare(mystr,"frequency")==0){
						//print frequency if input is frequency
						sprintf(mystr,"%3.3f hertz",frequency);
						LCDprint("Frequency", 1, 1);
						LCDprint(mystr,2,1);
					}
					if(compare(mystr,"period")==0){				
 						//print period if input is period
						sprintf(mystr,"%3.3f ms",(1.0/frequency)*1000.0);
						LCDprint("Period", 1, 1);
						LCDprint(mystr,2,1);
 					}
 					if(compare(mystr,"rms1")==0){				
 						//print rms1 if input is rms1
						sprintf(mystr,"%3.3f Volts%",peak_voltage_1);
						LCDprint("Voltage 1 rms", 1, 1);
						LCDprint(mystr,2,1);
 					}
 					 if(compare(mystr,"rms2")==0){				
 						//print rms2 if input is rms2
						sprintf(mystr,"%3.3f Volts",peak_voltage_2);
						LCDprint("Voltage 2 rms", 1, 1);
						LCDprint(mystr,2,1);
 					}
 					//peak_voltage stores rms so it must be converted back into actual peak
 					 if(compare(mystr,"peak1")==0){				
 						//print peak1 if input is peak1
						sprintf(mystr,"%3.3f Volts",peak_voltage_1/0.707016);
						LCDprint("Voltage 1 peak", 1, 1);
						LCDprint(mystr,2,1);
 					}
 					 if(compare(mystr,"peak2")==0){				
 						//print peak2 if input is peak2
						sprintf(mystr,"%3.3f Volts",peak_voltage_2/0.707016);
						LCDprint("Voltage 2 peak", 1, 1);
						LCDprint(mystr,2,1);
 					}
 					 if(compare(mystr,"phase shift")==0){				
 						//print phase if input is phase
						sprintf(mystr,"%3.3f degrees",phase_shift);
						LCDprint("Phase shift is", 1, 1);
						LCDprint(mystr,2,1);
 					}
 					else{
 						//LCDprint("error",1,1);
 						//LCDprint("illegal input",2,1);
 					}
 					
					break;
				}
				mystr[j]=c;
			}
			if(j==CHARS_PER_LINE)
			{
				mystr[j]=0;
				LCDprint("line size exceeded", 1, 1);
				LCDprint("hit enter to reset", 2, 1);
			}
			printf("\nType command: ");
		}




			
			

	waitms(10);
	/*
	printf("%f",frequency);
	printf("\n");
	printf("%f",peak_voltage_1);
		printf("\n");
	printf("%f",peak_voltage_2);
		printf("\n");
	printf("%f",phase_shift);
		printf("\n");
	//
	}*/	

	
	}
	

	
}





