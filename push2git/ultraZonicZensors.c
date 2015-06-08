#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#define TRUE 1
#define TRIG 21
#define ECHO 22

void setup(){
	wiringPiSetup();
	pinMode(TRIG, OUTPUT);
	pinMode(ECHO, INPUT);
	digitalWrite(TRIG, LOW);
	delay(30);
}

int getCM(){
	printf("in getCM\n");
	digitalWrite(TRIG,HIGH);
	printf("trigger should be high\n");
	delayMicroseconds(20);
	digitalWrite(TRIG, LOW);
	
	while(digitalRead(ECHO)==LOW);
	long startTime = micros();
	while(digitalRead(ECHO) == HIGH);
	long travelTime = micros()-startTime;
	int distance = travelTime/58;

	return distance;
}

int main(void){
	setup();
	while(1){
	printf("Distance: %dcm\n",getCM());
	delay(2000);
	}
	return 0;

}
