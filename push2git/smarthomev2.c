 /*
 *  readMcpSensors.c:
 *  read value from ADC MCP3008
 *
 * Requires: wiringPi (http://wiringpi.com)
 * Copyright (c) 2015 http://shaunsbennett.com/piblog
 ***********************************************************************
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>

/*FROM LCD*/
#include <time.h>
#include <lcd.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#define	TRUE	            (1==1)
#define	FALSE	            (!TRUE)
#define CHAN_CONFIG_SINGLE  8
#define CHAN_CONFIG_DIFF    0
#define BUTTON		    3
#define MODUS_OPERANDI_PIN	25
#define DEBUG		    1


#define thermistorPin	    1    //ADC Channel 1
#define A 	3.354016e-03
#define B 	2.569850e-04
#define C 	2.620131e-06
#define D 	6.383091e-08
#define thermistorR25	10000    //R25 value for thermistor

//for das Ultra
#define TRIG 21
#define ECHO 22




float padResistance = 9982;	//measured resistance pad resistor

float temp;

float resistance, LnRes;
static int myFd ;

/*From LCD*/
static int lcdHandle;


/*
 * pingPong:
 *	Bounce a character - only on 4-line displays
 *********************************************************************************
 */

static void pingPong (int lcd, int cols)
{
  static int position = 0 ;
  static int dir      = 0 ;

  if (dir == 0)		// Setup
  {
    dir = 1 ;
    lcdPosition (lcdHandle, 0, 3) ;
    lcdPutchar  (lcdHandle, '*') ;
    return ;
  }

  lcdPosition (lcdHandle, position, 3) ;
  lcdPutchar (lcdHandle, ' ') ;
  position += dir ;

  if (position == cols)
  {
    dir = -1 ;
    --position ;
  }
  
  if (position < 0)
  {
    dir = 1 ;
    ++position ;
  }

  lcdPosition (lcdHandle, position, 3) ;
  lcdPutchar  (lcdHandle, '#') ;
}

void scrollMessage (const char* message, int line, int width)
{
  char buf [32] ;
  static int position = 0 ;
  static int timer = 0 ;

  if (millis () < timer)
    return ;

  timer = millis () + 200 ;

  strncpy (buf, &message [position], width) ;
  buf [width] = 0 ;
  lcdPosition (lcdHandle, 0, line) ;
  lcdPuts     (lcdHandle, buf) ;

  if (++position == (strlen (message) - width))
    position = 0 ;
}

//functions for UltraSonic

void ZonicSetup(){
	//wiringPiSetup();
	pinMode(TRIG, OUTPUT);
	pinMode(ECHO, INPUT);
	digitalWrite(TRIG, LOW);
	delay(30);
}


int getZonicCM(){
//	printf("in getCM\n");
	digitalWrite(TRIG,HIGH);
//	printf("trigger should be high\n");
	delayMicroseconds(20);
	digitalWrite(TRIG, LOW);
	
	while(digitalRead(ECHO)==LOW);
	long startTime = micros();
	while(digitalRead(ECHO) == HIGH);
	long travelTime = micros()-startTime;
	int distance = travelTime/58;

	return distance;
}


char *usage = "Usage: mcp3008 all|analogChannel[1-8] [-l] [-ce1] [-d]";
// -l   = load SPI driver,  default: do not load
// -ce1  = spi analogChannel 1, default:  0
// -d   = differential analogChannel input, default: single ended


//provided function by OddWires
float steinhartAndHart(int rawADC){
//	printf("rawADC value = %d\n", rawADC);
	resistance = padResistance*((1024.0/rawADC) - 1);
//	printf("resistance = %f\n", resistance);
	LnRes = log(resistance/thermistorR25);
	temp = 1/(A+(B*LnRes) + (C*pow(LnRes,2)) + (D*pow(LnRes,3)));
	return temp - 273.15;	//Convert Kelvin to Celsius
}

//send Email message
void sendEmailZonic(void){
	system("echo 'Ultrasonic detected suspicious activity' | mail -s 'UltraZon2000: Suspicious Activity' cklam19@gmail.com");
}

void sendEmailLDR(void){
	system("echo 'LDR detected suspicious activity' | mail -s 'LDR2000: Suspicious Activity' cklam19@gmail.com");
}
//get Weather Data
//can implement other cities if have time

char filestring [1000];
getWeather(void){
	FILE *f;
//	char wbuff2 [32];
/*	FILE *f = fopen("weather.txt","w");
	if (f==NULL){
		printf("error opening file!\n");
	}
*/
	system("curl api.openweathermap.org/data/2.5/weather?zip=92612,us >> weather.txt");
	system("cp weather.txt weather2.json");
	system("python cheatscript.py");
	f = fopen("weather2.txt","r");
	if (f==NULL){
		printf("error opening file!\n");
	}
	fgets(filestring,1000,f);
	printf("file_string: %s\n", filestring);
	fclose(f);
	//puts(filestring);
	//fprintf(f,"Test output for file: %s\n", wbuff);
	//sscanf(filestring,"\"description\":\"%s\"",wbuff);
	//sscanf(filestring,"{",wbuff);
	
	system("sudo rm weather.txt");
	//scan for weather description
}	

void loadSpiDriver()
{
    if (system("gpio load spi") == -1)
    {
        fprintf (stderr, "Can't load the SPI driver: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }
}

void spiSetup (int spiChannel)
{
    if ((myFd = wiringPiSPISetup (spiChannel, 1000000)) < 0)
    {
        fprintf (stderr, "Can't open the SPI bus: %s\n", strerror (errno)) ;
        exit (EXIT_FAILURE) ;
    }
}

int myAnalogRead(int spiChannel,int channelConfig,int analogChannel)
{
    if(analogChannel<0 || analogChannel>7)
        return -1;
    unsigned char buffer[3] = {1}; // start bit
    buffer[1] = (channelConfig+analogChannel) << 4;
    wiringPiSPIDataRW(spiChannel, buffer, 3);
    return ( (buffer[1] & 3 ) << 8 ) + buffer[2]; // get last 10 bits
}

//button debounce function

static void detectButton(int buttonVal){
	while (digitalRead(buttonVal) == HIGH)
	{ delay(1);}
	while (digitalRead(buttonVal) == LOW)
	{delay(1);}
	delay(300);
}

static int detectMode(int modeOp){
	while (digitalRead(MODUS_OPERANDI_PIN) == HIGH)
	{delay(1);}
	while (digitalRead(MODUS_OPERANDI_PIN) == LOW)
	{delay(1);}
	modeOp++;		//button has been pressed for different modes, home = 0, sec = 1
//	delay(300);
	return modeOp; 
}

int main (int argc, char *argv [])
{
    int loadSpi=FALSE;
    int analogChannel=0;
    int spiChannel=0;
    int channelConfig=CHAN_CONFIG_SINGLE;
    int print_once;
    int modus_operandi = 1;
    int mode_flag=1;
    int rows = 2;
    int cols = 16;
    int bits = 4; //4 bit mode
    char buf [32];
    char tempbuf [32];
    char weatherBuff;
    char *tempmsg;
    int lcd;
    struct tm *t ;
    time_t tim ;
    int i;
    int max_distance;
    int curr_distance;
    int avg_dist;
    int read_LDR;
  /*
    if (argc < 2)
    {
        fprintf (stderr, "%s\n", usage) ;
        return 1 ;
    }
    if((strcasecmp (argv [1], "all") == 0) )
        argv[1] = "0";
    if ( (sscanf (argv[1], "%i", &analogChannel)!=1) || analogChannel < 0 || analogChannel > 8 )
    {
        printf ("%s\n",  usage) ;
        return 1 ;
    }
    int i;
    for(i=2; i<argc; i++)
    {
        if (strcasecmp (argv [i], "-l") == 0 || strcasecmp (argv [i], "-load") == 0)
            loadSpi=TRUE;
        else if (strcasecmp (argv [i], "-ce1") == 0)
            spiChannel=1;
        else if (strcasecmp (argv [i], "-d") == 0 || strcasecmp (argv [i], "-diff") == 0)
            channelConfig=CHAN_CONFIG_DIFF;
    }
  */
    //
    if(loadSpi==TRUE)
        loadSpiDriver();
    wiringPiSetup () ;
    //set button input
   // pinMode(BUTTON, INPUT);	
    spiSetup(spiChannel);
    i = 0;
    print_once = 0;
    
    if (bits == 4)
	lcdHandle = lcdInit (rows, cols, 4, 11,15, 4,5,6,7,0,0,0,0) ;
    else
	lcdHandle = lcdInit (rows, cols, 8, 11,15, 0,1,2,3,4,5,6,7) ;

  if (lcdHandle < 0)
  {
    fprintf (stderr, "%s: lcdInit failed\n", argv [0]) ;
    return -1 ;
  }
//-------------Begin Infinite Loop
pinMode(27, OUTPUT);
//digitalWrite(27, HIGH);
pinMode(23, OUTPUT);
digitalWrite(27,LOW);
digitalWrite(23,LOW);
//sendEmail();
printf("grabbing weather data\n");
getWeather();
printf("weather is: %s\n", filestring);
delay(20);

/*set initial distance for ultrasonic sensor*/

ZonicSetup();
printf("Setting up UltraSonic Sensor\n");
max_distance = getZonicCM();

while(1){

//ZonicSetup();
if (digitalRead(MODUS_OPERANDI_PIN) == LOW)
	{mode_flag = (mode_flag+1)%2;
	delay(500);}
/*else if (digitalRead(MODUS_OPERANDI_PIN) == HIGH)
	{mode_flag = 0;}
*/
/*MODE_FLAG = 0.....HOME MODE
MODE_FLAG = 1......AWAY MODE */
/*MAYBE ADD SET APPLIANCE MODE*/
//printf("How fast is this while?\n");
//printf("\n");
/*HOME MODE
*
*Display Temperature and Date on LCD
*
*/
	if (mode_flag == 1){
		digitalWrite(27,LOW);
		digitalWrite(23,HIGH);


		tim = time (NULL) ;
		 t = localtime (&tim) ;

		 sprintf (buf, "%02d:%02d:%02d   %02d/%02d", t->tm_hour, t->tm_min, t->tm_sec, t->tm_mon, t->tm_mday) ;

		 //lcdPosition (lcdHandle, (cols - 8) / 2, 1) ;
		 lcdPosition (lcdHandle, 0, 1);
		 lcdPuts     (lcdHandle, buf) ;

//printf("got here1\n");
 //pingPong (lcd, cols) ;

		temp = steinhartAndHart(myAnalogRead(0,8,0));
		sprintf (tempbuf,"        TEMP:%.0fC   WEATHER: %s  MODE:HOME              ", temp,filestring); 
		tempmsg = tempbuf;
		lcdPosition (lcdHandle, 0, 1);
		scrollMessage(tempmsg,0, 16);
		//scrollMessage(weatherBuff, 0, 16);
		//lcdPuts (lcdHandle, tempbuf);
//		lcdPuts (lcdHandle, chr(223));
 
//	printf("mode_flag = %d\n", mode_flag);
/*		if (digitalRead(BUTTON) == LOW){
			printf("\n------HOME MODE--------\n");
			printf("Thermistor Output (Channel 1):\n");
			printf("-----------------\n");
			temp = steinhartAndHart(myAnalogRead(0,8,0));
			sprintf (tempbuf, "Temp: %.02d C", temp); 
			lcdPosition (lcdHandle, 0, 0);
			lcdPuts (lcdHandle, tempbuf);
			printf("Celsius: %.02f\n", temp);		//print out temperature in Celsius
			temp = (temp * 9.0)/5.0 + 32.0;
			printf("Fahrenheit: %.02f\n", temp);		//print out temperature in Fahrenheit
			//delay(170);
			}
*/
		}


/*AWAY MODE
*
*TAKE IN LDR AND SONIC SENSOR DATA
*
*/
	else {
		 curr_distance = getZonicCM();
		 read_LDR = myAnalogRead(0,8,1);
		 digitalWrite(23,LOW);
		 digitalWrite(27,HIGH);
		
		//printf("mode_flag = %d\n", mode_flag);
		//delay(500);
		//mode_flag = 0; //do something
	        //Tried to avg:
		/*for (avg_dist=0;avg_dist<=10;avg_dist++){
			curr_distance+=curr_distance;
		}*/
		//curr_distance = curr_distance/10;
		if (curr_distance < max_distance){
			if (curr_distance != 0){
				printf("Intruder at %d!\n",curr_distance);
				sendEmailZonic();
				delay(200);
				}
			}
		 delay(70);
		 //printf("%d cm\n", getZonicCM());
		 // delay(200);
		 
		 if (read_LDR > 550){
		     	printf("Detected Suspicious Lighting Activity\n");
			sendEmailLDR();
			delay(500);
			}
		 tim = time (NULL) ;
		 t = localtime (&tim) ;

		 sprintf (buf, "%02d:%02d:%02d   %02d/%02d", t->tm_hour, t->tm_min, t->tm_sec, t->tm_mon, t->tm_mday) ;

		 //lcdPosition (lcdHandle, (cols - 8) / 2, 1) ;
		 lcdPosition (lcdHandle, 0, 1);
		 lcdPuts     (lcdHandle, buf) ;


		temp = steinhartAndHart(myAnalogRead(0,8,0));
		sprintf (tempbuf, "                Temp:%.0fC   MODE:AWAY                 ", temp); 
		lcdPosition (lcdHandle, 0, 0);
		//lcdPuts (lcdHandle, tempbuf);
		scrollMessage(tempbuf,0,16);
//		printf("got here1\n");
/*ULTRASONIC SENSOR*/			
	//	printf("Distance: %dcm\n",getZonicCM());
	//	delay(2000);
		/*
		if (digitalRead(BUTTON) == LOW){
			printf("\n------AWAY MODE------\n");
			printf("LDR Output (Channel 2):\n");
			printf("LDR Output: %d\n", myAnalogRead(0,8,1));
			printf("-----------------\n");
			//delay(200);
		}
		*/
	}
}
return 0;
}
