#ifndef AD779x_h
#define AD779x_h

#if (ARDUINO >= 100)
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class AD779x
{
	public:
		AD779x(uint8_t csPin);
		void init(); //initialize AD7792 with CS pin
		void begin();
		void IEXC(uint8_t value); //excitation current selection 10uA, 100uA, 1000uA
		long read(uint8_t ch); //uint32_t read(byte ch); //for ad7793 type = true;
		void calibrate();
		void selectCH(uint8_t ch);
		void selectMode();
		uint8_t getID();
		void reset();
		void end(void);
		uint16_t readMode(); //read mode register
		uint16_t readCFG(); //read configuration register
		uint8_t readST(); //read status register
	private:
		uint8_t _cs;
		boolean _type; //if true, for AD7793
		uint8_t transfer(uint8_t data);
};
#endif