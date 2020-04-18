/*
  SpiFirmata.h - Firmata library
  Copyright (C) 2006-2008 Hans-Christoph Steiner.  All rights reserved.
  Copyright (C) 2010-2011 Paul Stoffregen.  All rights reserved.
  Copyright (C) 2009 Shigeru Kobayashi.  All rights reserved.
  Copyright (C) 2013 Norbert Truchsess. All rights reserved.
  Copyright (C) 2009-2016 Jeff Hoefs.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

*/

#ifndef SpiFirmata_h
#define SpiFirmata_h

#include <ConfigurableFirmata.h>

#include <SPI.h>
#include "FirmataFeature.h"
#include "FirmataReporting.h"

#define SPI_MAX_DEVICES 8
#define MAX_SPI_BUF_SIZE 32

/* Spi data */
struct spi_device_config {
  byte deviceId;
  byte channel;
  byte dataMode;
  byte bitOrder;
  byte csPin;
  boolean enabled;
};

class SpiFirmata: public FirmataFeature
{
  public:
    SpiFirmata();
    boolean handlePinMode(byte pin, int mode);
    void handleCapability(byte pin);
    boolean handleSysex(byte command, byte argc, byte* argv);
    void reset();
    void report();

  private:
    spi_device_config config[SPI_MAX_DEVICES];

    void handleSpiRequest(byte command, byte argc, byte *argv);
	boolean handleSpiBegin(byte argc, byte *argv);
    boolean handleSpiConfig(byte argc, byte *argv);
    boolean enableSpiPins();
	void handleSpiTransfer(byte argc, byte *argv);
    void disableSpiPins();
	bool isSpiEnabled;
	
	byte data[MAX_SPI_BUF_SIZE];
};


SpiFirmata::SpiFirmata()
{
  isSpiEnabled = false;
}

boolean SpiFirmata::handlePinMode(byte pin, int mode)
{
  if (IS_PIN_SPI(pin)) {
    if (mode == PIN_MODE_SPI) {
      return true;
    } else if (isSpiEnabled) {
      // disable Spi so pins can be used for other functions
      // the following if statements should reconfigure the pins properly
      if (Firmata.getPinMode(pin) == PIN_MODE_SPI) {
        disableSpiPins();
      }
    }
  }
  return false;
}

void SpiFirmata::handleCapability(byte pin)
{
  if (IS_PIN_SPI(pin)) {
    Firmata.write(PIN_MODE_SPI);
    Firmata.write(1);
  }
}

boolean SpiFirmata::handleSysex(byte command, byte argc, byte *argv)
{
  switch (command) {
    case SPI_DATA:
		  if (argc < 1)
		  {
			  Firmata.sendString("Error in SPI_DATA command: empty");
			  return false;
		  }
        handleSpiRequest(argv[0], argc - 1, argv + 1);
        return true;
  }
  return false;
}

void SpiFirmata::handleSpiRequest(byte command, byte argc, byte *argv)
{
  switch (command) {
	  case SPI_BEGIN:
	    handleSpiBegin(argc, argv);
		break;
	  case SPI_DEVICE_CONFIG:
	    handleSpiConfig(argc, argv);
		break;
	  case SPI_END:
	    disableSpiPins();
		break;
	  case SPI_TRANSFER:
	    handleSpiTransfer(argc, argv);
		break;
  }
}

void SpiFirmata::handleSpiTransfer(byte argc, byte *argv)
{
	int j = 0;
	for (byte i = 4; i < argc; i += 2) {
        data[j++] = argv[i] + (argv[i + 1] << 7);
    }
	
	digitalWrite(10, LOW);
	Firmata.sendString("Sending to SPI:");
	for (int i = 0; i < j; i++)
	{
		Firmata.sendString(String(data[i], HEX).c_str());
	}
	
	SPI.transfer(data, j); 
	digitalWrite(10, HIGH);
	
	Firmata.sendString("From SPI:");
	for (int i = 0; i < j; i++)
	{
		Firmata.sendString(String(data[i], HEX).c_str());
	}
	
	// send slave address, register and received bytes
  Firmata.startSysex();
  Firmata.write(SPI_DATA);
  Firmata.write(SPI_REPLY);
  Firmata.write(0);
  Firmata.write(0x22);
  Firmata.write(j);
  for (int i = 0; i < j; i++)
  {
	  Firmata.sendValueAsTwo7bitBytes(data[i]);
  }
  Firmata.endSysex();
}

boolean SpiFirmata::handleSpiConfig(byte argc, byte *argv)
{
  config[0].csPin = 10;
}

boolean SpiFirmata::handleSpiBegin(byte argc, byte *argv)
{
  if (!isSpiEnabled) {
	  // Only channel 0 supported
    if (argc != 1 || *argv != 0) {
		Firmata.sendString("SPI_BEGIN: Only channel 0 supported");
		return false;
	}
    enableSpiPins();
	SPI.begin();
	Firmata.sendString("SPI_INIT done");

  }
  return isSpiEnabled;
}

boolean SpiFirmata::enableSpiPins()
{
  if (Firmata.getPinMode(PIN_SPI_MISO) == PIN_MODE_IGNORE) {
        return false;
  }
  // mark pins as Spi so they are ignore in non Spi data requests
  Firmata.setPinMode(PIN_SPI_MISO, PIN_MODE_SPI);
  pinMode(PIN_SPI_MISO, INPUT);
  
  if (Firmata.getPinMode(PIN_SPI_MOSI) == PIN_MODE_IGNORE) {
        return false;
  }
  // mark pins as Spi so they are ignore in non Spi data requests
  Firmata.setPinMode(PIN_SPI_MOSI, PIN_MODE_SPI);
  pinMode(PIN_SPI_MOSI, OUTPUT);
  
  if (Firmata.getPinMode(PIN_SPI_SCK) == PIN_MODE_IGNORE) {
        return false;
  }
  // mark pins as Spi so they are ignore in non Spi data requests
  Firmata.setPinMode(PIN_SPI_SCK, PIN_MODE_SPI);
  pinMode(PIN_SPI_SCK, OUTPUT);
  
  isSpiEnabled = true;
  return true;
}

/* disable the Spi pins so they can be used for other functions */
void SpiFirmata::disableSpiPins()
{
  isSpiEnabled = false;
  SPI.end();
}

void SpiFirmata::reset()
{
  if (isSpiEnabled) {
    disableSpiPins();
  }
}

void SpiFirmata::report()
{
}

#endif /* SpiFirmata_h */
