/* Program Oregon Written by David Wright, Jan 2015. */
/* thanks and plagarised from various internet sources on oregon sensors. Paul(DISK91.com), ALTelectronics for their document on      */
/* Oregon Scientific RF protocol v1.0, kevinmehall on github for rtldr-433m-sensor, Alexander Yerezeyev and more ....                 */
/* */
/* This Program reads a 433Mhz transmission from an Oregon version 1 Protocol Temperature Sensor */
/* */
/* Programming uses the logic of first detecting the preamble portion of transmission by counting the number of short 1 pulses         */
/* Next the progam monitors the 3 sync pulses and translates the last sync pulse to determine the first data message bit either 1 or 0 */
/* From knowing the first message data bit the program determines the next 31 message data bits using the logic as follows:-           */
/*   Two SHORT pulses means that the message data bit is the same as the previous message data bit (we know first so can do this)      */
/*   One LONG pulse means that the message data bit is opposite (inverse) of the previous message data bit.                            */
/* */
/* The resulting 32 bit message is reversed to form 8 nibbles(4 bit). These nibbles are decoded to produce the data values.            */
/* */
/* This program is quite basic and simplistic in that there is no error checking and only really gets temperature, channel and low bat */
/* I wrote this program to learn C programming from being a visual basic and java programmer and as an aid to learning to code         */
/* hardware interfaces on my Raspberry Pi B+. I found plenty of code for various Oregon sensors but none I could easily understand     */
/* from the C or Python code. Hence this program does nothing clever using bitwise or memory facilities or uses no rising clock edges  */
/* , falling clock edges, clock ticks etc. Just simple time measurement and ONs and OFFs. Also I dont do any hex conversions.          */
/* There are no hex conversions because for channel, temperature and low battery all the hex and decimal values are the same i.e.      */
/* only numbers 0 to 9 are used for each part of the message. The temp minus sign and low battery are determined from the raw binary   */
/* in nibble 2 (third nibble as it starts nibble zero).                                                                                */
/* */
/* If you wish this program can be improved on by adding error checking by using the checksum (hex conversion needed) and/or           */
/* by storing values and checking against the second transmission of the same message (all messages sent twice, I sleep through the    */
/* second transmission Zzzzz).                                                                                                         */
 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include<wiringPi.h> //wiringpi for pin reads
int thisPin;
int lastPin;
int preambleON;
int onShortLo;
int onShortHi;
int offShortLo;
int offShortHi;
int onLongLo;
int onLongHi;
int offLongLo;
int offLongHi;
int syncEnd0Lo;
int syncEnd0Hi;
long timeDiff;
long startedAt;
long endedAt;
int preambleFound;
int dataBits[32];
int myPin = 0; // wiringPi 0, Rasp B+ pin 11, GPIO 17


long getTime(long returnUsecs)
{
	struct timespec currentTime;
        long microSecs;
        long  secs;
	clock_gettime(CLOCK_MONOTONIC, &currentTime);
        microSecs = currentTime.tv_nsec  * 0.001;
        secs = currentTime.tv_sec;
        returnUsecs = microSecs +  secs * 1000;
	return(returnUsecs); 
}
int getPinValue(int returnPinValue)
{
        returnPinValue = digitalRead(myPin);
	return(returnPinValue);
}
int detectPreamble(int returnDetected)
{

	thisPin = getPinValue(thisPin);
	if (!(thisPin == lastPin))
	{
		endedAt = getTime(endedAt); // set timer end for last pin
		timeDiff = endedAt - startedAt; // reset time
		if (lastPin == 1)
		{
			if ((timeDiff >= onShortLo) && (timeDiff <= onShortHi)) // error of margin on pulse length
			{
				preambleON++; // looking for 12 short ON pulses.
			}
			else // not preamble
			{
				preambleON = 0;
			}
		}
		else // check is this preamble, lastPin off
		{


			if (preambleON < 11) // last preamble is special as next low not short(thisPin)
			{
				if ((timeDiff > offShortLo) && (timeDiff < offShortHi)) // off ok
				{
				}
				else // not preamble
				{
					preambleON = 0;
				}

			}
		}

	startedAt = endedAt; //  set timer start for this pin
	lastPin = thisPin;
	}
	if (preambleON == 12) 
	{
		returnDetected = 1;
	}
	else
	{
		returnDetected = 0;
	}
	return(returnDetected);
}
int getSync()
{
        // sync is long OFF, long ON, long OFF

        int sCount;
	sCount =  1;
        thisPin = getPinValue(thisPin);
        lastPin = thisPin; //looking for state changes
        while (sCount < 3) // 3 sync pulses
        {
           if (!(thisPin == lastPin))
           {
              	sCount ++;
		if (sCount == 3)
		{
              		startedAt = getTime(startedAt); // time this pulse to get first bit value
		}
              	lastPin = thisPin;
          }
           thisPin = getPinValue(thisPin); // poll the pin state
        }
        while ((thisPin == lastPin))
        {
             thisPin = getPinValue(thisPin);
        }
        endedAt = getTime(endedAt);
	timeDiff = endedAt - startedAt;
	startedAt = endedAt; // start timer for next bit. 
	if (timeDiff > syncEnd0Lo && timeDiff < syncEnd0Hi)
	{
		dataBits[0] = 0;
	}
	else
	{
		dataBits[0] = 1;
	}
        return;
}

int getData()
{
// get next 31 data bits, we determined bit 0 in SYNC
int i;
int l;
int s;
        i = 1; //first bit[0] was derived in Sync
	s = 0; // short pulse 
	l = 0; // long pulse

        while (i < 32)
        {
           if (!(thisPin == lastPin)) // lastPin and thisPin are from getSync.
           {
                endedAt = getTime(endedAt); //  timer started in getSync
                timeDiff = endedAt - startedAt;
		startedAt = endedAt; // next starts at this end
			if (lastPin == 0) //lastPin was OFF
			{
				if ((timeDiff > offShortLo) && (timeDiff < offShortHi))
					{ // short off detected
						s++;
						l=0;
					}
					if ((timeDiff > offLongLo) && (timeDiff < offLongHi))
					{ // long off detected
						l++;
						s=0;
					}
			}
			else // lastPin was ON
			{
					if ((timeDiff > onShortLo) && (timeDiff < onShortHi)) // half-time
					{ // short on detetcted
						s++;
						l=0;
					}
					if ((timeDiff > onLongLo) && (timeDiff < onLongHi)) // full-time
					{ // long on detected
						l++;
						s=0;
					}
			}
			if (s == 2)
			{ // 2 short pulses this bit equals previous bit (we know 1st bit from sync)
					dataBits[i] = dataBits[(i-1)];
					i++;
					s=0;
					l=0;
			}
			if (l == 1)
			{ // 1 long pulse this bit is inverse of previous bit (we know 1st bit from sync)
					if (dataBits[(i-1)] == 0)
					{
					   dataBits[i] = 1;
					}
					else
					{
					   dataBits[i] = 0;
					}
					l=0;
					s=0;
					i++;
			}
            // update  last pin to this pin value
                lastPin = thisPin;
           }
           thisPin = getPinValue(thisPin); // get pin value 
        }
	return;
}

int processData()
{
int x = 0;
int y = 0;
int z=  0;
int nStart;
int nFinish;
int nibbleValue;
int nibbleValues[8];
int temp;
int nib;
int nibble[4];
int lowBat=0;

	for (x=0;x<8;x++)
	{
		for (y=0;y<4;y++){nibble[y]=0;} //initialise
		nStart=(31-((x*4)+3)); // array index for nibble start
		nFinish=nStart+4;
		y = 3; // nibble index
		// Reverse the bits in message data (dataBits) to create 8 nibbles of 4 bits.
		for (z=nStart;z<nFinish;z++) // read 4 bits 
		{
			if (y >= 0)
			{
				nibble[y]=dataBits[z];//reverse bits, y starts at 3 back to 0
				y--;
			}
		}
		nibbleValue=0;
		nib=8;
		temp=0;
		for (z=0;z<4;z++) // convert this nibble to decimal from binary
		{
			temp=nibbleValue;
			nibbleValue=(nib * nibble[z]) + temp;
			temp=nib;
			if (temp > 1) nib = (temp / 2);

		}
		nibbleValues[x] = nibbleValue; // store nibble decimal values
	}

	// Print out the converted decimal nibble values
	for (x=0;x<8;x++)
	{
		if (x==6) //channel number conversion
		{
			 temp=99;
			 if (nibbleValues[x]==0) temp=1;
			 if (nibbleValues[x]==4) temp=2;
			 if (nibbleValues[x]==8) temp=3;
			 printf(" Channel     : %d\n",temp);
		}
		if (x==2)
		{
			printf(" Temperature : ");
			if ((nibbleValues[x]==10) || (nibbleValues[x]==2)) printf("-"); // 8 = low bat, 2=minus, 10=minus+low bat
			if ((nibbleValues[x]==8) || (nibbleValues[x]==10)) lowBat=1; else lowBat=0;

		}
		if ((x==3) || (x==4)) printf("%d",nibbleValues[x]);
		if (x==5)
		{
			printf(".");
			printf("%d degC.\n",nibbleValues[x]);
		}
		if (lowBat==1) printf("Low Battery. \n");
	}
	return;
}
int main(int argc,  char **argv)
{
int p;
//  on short 1720, on long 3180, off short 1219, off long 2680
// sync 1 off 4200, sync 2 on 5700, sync 3 off 5200 (sync 3 off long 6680)
        onShortLo = 1500;
        onShortHi = 2400;
        offShortLo = 970;
        offShortHi = 1950;
        onLongLo = 2980;
        onLongHi = 3880;
        offLongLo = 1950;
        offLongHi = 2900;
      //  syncBeginLo = 4000;
      //  syncBeginHi = 4400;
      //  syncLo = 5500;
      //  syncHi = 5900;
      //  syncEnd1Lo = 5000;
      //  syncEnd1Hi = 5400;
        syncEnd0Lo = 6480;
        syncEnd0Hi = 6880;
        wiringPiSetup();
        pinMode(myPin, INPUT);
	preambleFound = 0;
	preambleON = 0;
	printf("Waiting for transmission (approx. every 30s).\n");
	printf("Press Ctrl-C to exit.\n");
	while (1)
	{ // loop forever or ctrl-c

		thisPin = getPinValue(thisPin);
		lastPin = thisPin;
		endedAt = getTime(endedAt); // set initial timer end
		startedAt = endedAt;

		while (preambleFound == 0) // constantly monitor for preamble
		{
			preambleFound = detectPreamble(preambleFound);
		}
		if (preambleFound == 1)
		{
       		        getSync();
       	        	getData();
			processData();
			preambleFound = 0;
			preambleON = 0;
			sleep(10); // avoid second xmission of same data
		}
	}
	exit(0);
}
