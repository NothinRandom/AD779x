#include "AD779x.h"

//SPCR Settings:
// |   7  |   6   |  5   |  4   |  3   |   2  |   1  |   0  |
// | SPIE |  SPE  | DORD | MSTR | CPOL | CPHA | SPR1 | SPR0 |

// SPIE - Enables the SPI interrupt when 1
// SPE - Enables the SPI when 1
// DORD - Sends data least Significant Bit First when 1, most Significant Bit first when 0
// MSTR - Sets the Arduino in master mode when 1, slave mode when 0
// CPOL - Sets the data clock to be idle when high if set to 1, idle when low if set to 0
// CPHA - Samples data on the falling edge of the data clock when 1, rising edge when 0
// SPR1 and SPR0 - Sets the SPI speed, 00 is fastest (4MHz) 11 is slowest (250KHz)

// SPI2X SPR1 SPR0 SCK Freq.  Actual SCK speed @ 16Mhz
// 0     0    0    fosc/4     4Mhz
// 0     0    1    fosc/16    1Mhz
// 0     1    0    fosc/64    250Khz
// 0     1    1    fosc/128   125Khz
// 1     0    0    fosc/2     8Mhz
// 1     0    1    fosc/8     2Mhz
// 1     1    0    fosc/32    500Khz
// 1     1    1    fosc/64    125Khz

//COMMUNICATIONS register p14
//CR7 - !WEN
//CR6 - R/!W
//CR5 - RS2
//CR4 - RS1
//CR3 - RS0
//CR2 - CREAD
//CR1 - 0
//CR0 - 0

//	Register		RS2 RS1 RS0 	  p14
//Communications	 0   0   0	-> 00000000 0x00
//Status			 0	 0	 0	-> 01000000 0x40
//Mode				 0	 0	 1	-> 00001000 0x08
//Configuration		 0	 1	 0	-> 00010000 0x10
//Data				 0	 1	 1	-> 00011000 0x18
//ID				 1	 0	 0	-> 00100000 0x20
//IO				 1	 0	 1	-> 00101000 0x28
//Offset			 1	 1	 0	-> 00110000 0x30
//Full-Scale		 1	 1	 1	-> 00111000 0x38

AD779x::AD779x(uint8_t csPin)
{
	_cs = csPin;
	pinMode(SCK, OUTPUT);
	pinMode(MOSI, OUTPUT);
	pinMode(MISO, INPUT);
	pinMode(_cs, OUTPUT);
	digitalWrite(_cs, HIGH);
}

void AD779x::begin() //must be mode3
{
	//SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA)|(1<<SPR0); //mode3, 1MHz
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA); //mode3, 4MHz
}

//set up AD779x with initial settings
void AD779x::init()
{
	begin();
	reset(); //reset device
	delay(2);
	
	//grab ID to differentiate between AD7792 and AD7793
	_type = true;
	if(!(getID()%2)) //false: AD7792
		_type = false;		
} 

void AD779x::selectMode()
{
	
	digitalWrite(_cs, LOW); //pull CS low to engage
	
	transfer(0x08); //select MODE register (00001000)(0x08)
	//Write 16 bits to MODE register on page 16 Table 15
	transfer(0x20); //0x00 for continuous 
					//0x20 for single conversion	preferred
					//0x40 for idle
					//0x60 for power down
					//0x80 internal zero scale calibration
					//0xA0 internal full scale calibration
					//0xC0 system zero scale calibration
					//0xE0 system full scale calibration
	
	//update rate on page 16/17
	transfer(0x0C); //0x00    X    X
					//0x01  470Hz 4mS
					//0x02  242Hz 8mS
					//0x03  123Hz 16mS
					//0x04   62Hz 32mS 
					//0x05   50Hz 40mS
					//0x06   39Hz 48mS
					//0x07 33.2Hz 60mS 
					//0x08 19.6Hz 101mS -90dB for 60Hz	
					//0x09 16.7Hz 120mS -80dB for 50Hz
					//0x0A 16.7Hz 120mS -65dB for 50/60Hz
					//0x0B 12.5Hz 160mS -66dB for 50/60Hz
					//0x0C   10Hz 200mS -69dB for 50/60Hz
					//0x0D 8.33Hz 240mS -70dB for 50/60Hz  
					//0x0E 6.25Hz 320mS -72dB for 50/60Hz 
					//0x0F 4.17Hz 480mS -74dB for 50/60Hz 
	digitalWrite(_cs, HIGH);
}

//excitation current settings from p18/19
//IO REGISTER:
	//CR5 - 1
	//CR4 - 0
	//CR3 - 1
void AD779x::IEXC(uint8_t value) //works!
{
	if(value > 3)
		value = 0;
	digitalWrite(_cs, LOW);	
	transfer(0x28); //select IO register RS2=1, RS1= 0, RS0 = 1	0x28
	//Write 8 bits to IO register (IEXC1 -> IOUT1, IEXC2 -> IOUT2, 10uA excitation current) 	
	transfer(value); //0 -> 0uA
					 //1 -> 10uA
					 //2 -> 210uA
					 //3 -> 1000uA
	digitalWrite(_cs, HIGH);				 
}

//p18
uint8_t AD779x::getID() //WORKS!
{
	uint8_t output = 0;
	digitalWrite(_cs, LOW);
	transfer(0x60); //01100000 0x60 
	output = transfer(0x00); 	
	digitalWrite(_cs, HIGH);
	return output;
}

void AD779x::calibrate()
{
	digitalWrite(_cs, LOW);
	transfer(0x08); //select mode register
	delayMicroseconds(5);
	transfer(0x80);
	transfer(0x0F);
	transfer(0x08); //
	delayMicroseconds(5);
	transfer(0xA0);
	transfer(0x0F);
	delayMicroseconds(5);
	digitalWrite(_cs, HIGH);
}

//select adc channel
void AD779x::selectCH(uint8_t ch)
{
	if(ch > 2)
		ch = 2;
	ch |= 0x90;
	
	digitalWrite(_cs, LOW); //pull CS low to engage
	transfer(0x10); //select configuration register 00010000 (0x10)

	transfer(0x10); //Write 16 bits to CONFIGURATION register:
					//bias voltage: disabled 0 0
					//burnout current: disabled 0 
					//bipolar operation: 0 or 1?
					//boost: disabled 1
					//gain: disabled 0 0 0
					//top 8b: 00011000 (0x18), disable boost yields 00010000 (0x10)
					
	transfer(ch);   //internal reference 1
					//0 0
					//buffer 1
					//0
					//000 (CH1) 001(CH2) 010(CH3) 
					//channel select = AIN1(+) - AIN1(-)	
					//bottom 8b: 10010000 (0x90) 	10010001 (0x91) read CH2	10010010 (0x92) read CH3
	digitalWrite(_cs, HIGH); //pull CS high to disengage
}

uint16_t AD779x::readMode() //works!
{
	long output = 0;
	digitalWrite(_cs, LOW);
	transfer(0x48); //select MODE register (01001000)(0x48)
	output = (transfer(0x00) << 8) & 0xFF00; //grab top byte
	output += transfer(0x00) & 0xFF; //grab bottom byte	
	digitalWrite(_cs, HIGH);
	return output;
}

uint16_t AD779x::readCFG() //works!
{
	long output = 0;
	digitalWrite(_cs, LOW);
	transfer(0x50); //select CONFIGURATION register (01010000)(0x50)
	output = (transfer(0x00) << 8) & 0xFF00; //grab top byte
	output += transfer(0x00) & 0xFF; //grab bottom byte	
	digitalWrite(_cs, HIGH);
	return output;	
}

uint8_t AD779x::readST() //works!
{
	long output = 0;
	digitalWrite(_cs, LOW);
	transfer(0x40); //select STATUS register (01000000)(0x40)
	output = transfer(0x00); //grab byte
	digitalWrite(_cs, HIGH);
	return output;	
}

//read value from ADC, 
long AD779x::read(uint8_t ch)
{ 
	long value = 0;	//uint16_t byte1, byte2, byte3;
	
	selectCH(ch);
	selectMode();
	
	while(readST() > 127); //wait until !RDY
	
	digitalWrite(_cs, LOW);
	transfer(0x58); //select DATA register (01011000)(0x58) //one read	try (01011100)(0x5C) for CREAD
	
	if(_type) //if AD7793, do an extra byte for 24bit value;
		value = (transfer(0x00) << 16) & 0xFF0000;	
	value += (transfer(0x00) << 8) & 0xFF00; //grab top byte
	value += transfer(0x00) & 0xFF; //grab bottom byte	

	digitalWrite(_cs, HIGH); //pull CS high to disengage
	
	return value; //return( ((byte1<<8) | byte2) & 0xFFFF);
} 

uint8_t AD779x::transfer(uint8_t data)
{
	begin(); //set correct SPI settings for devices
	SPDR = data;
	while (!(SPSR & (1<<SPIF))); //wait until transfer is done
	return SPDR; //return received byte
}

//send 16b if neccessary
// void AD9833::send(uint16_t packet)
// {
    // transfer((uint8_t)(packet >> 8));
    // transfer((uint8_t)packet);
// }

void AD779x::reset() //p14
{
	digitalWrite(_cs, LOW); //reset IC by sending out 32b of 1s
	transfer(0xFF);
	transfer(0xFF);
	transfer(0xFF);
	transfer(0xFF);
	digitalWrite(_cs, HIGH);
}

//reconfigure the SPI MODE
void AD779x::end(void) //no need anymore?
{
	//SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL); //SPCR = B010111000; //enable spi, master, mode2 for ad9833
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA); //enable spi, master, mode3
}