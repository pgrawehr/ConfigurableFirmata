/*
  Frequency.cpp - Firmata library
  Copyright (C) 2006-2008 Hans-Christoph Steiner.  All rights reserved.
  Copyright (C) 2010-2011 Paul Stoffregen.  All rights reserved.
  Copyright (C) 2009 Shigeru Kobayashi.  All rights reserved.
  Copyright (C) 2013 Norbert Truchsess. All rights reserved.
  Copyright (C) 2009-2015 Jeff Hoefs.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  Last updated by Jeff Hoefs: November 15th, 2015
*/

#include <ConfigurableFirmata.h>
#include "Frequency.h"

Frequency *FrequencyFirmataInstance;

Frequency::Frequency()
{
	_nextCallback = nullptr;
	FrequencyFirmataInstance = this;
	for (int i = 0; i < TOTAL_PORTS; i++)
	{
		_activeBitmasks[i] = 0;
		_lastValues[i] = 0;
	}
	for (int i = 0; i < TOTAL_PINS; i++)
	{
		_ticks[i] = 0;
	}
  _reportDelay = 0;
  _lastReport = millis();
  _nextCallback = Firmata.attach(REPORT_DIGITAL, FrequencyIsr);
}

void Frequency::FrequencyIsr(byte port, int value)
{
	// The ISR can't be interrupted by the main routine, therefore this is thread safe
	FrequencyFirmataInstance->FrequencyIsrInternal(port, value);
}

void Frequency::FrequencyIsrInternal(byte port, int value)
{
	// Count up if a bit changed that is active
	int xoredValues = _lastValues[port] ^ value;

	// Could be more than one
	int pinsToUpdate = _activeBitmasks[port] & xoredValues;
	int shifted = 0;
	int shiftedValue = value;
	while (pinsToUpdate)
	{
		// Calculate pin number from port and bit that changed
		int pin = port * 8;
		if (pinsToUpdate & 1)
		{
			pin += shifted;
			// Count if the value changed to 1 on rising, or changed to low on falling
			if ((_mode == INTERRUPT_MODE_RISING && (shiftedValue & 1)) ||
				(_mode == INTERRUPT_MODE_FALLING && ((shiftedValue & 1) == 0)) ||
				(_mode == INTERRUPT_MODE_CHANGE))
			{
				_ticks[pin]++;
			}
		}
		shifted++;
		pinsToUpdate = pinsToUpdate >> 1;
		shiftedValue = shiftedValue >> 1;
	}
	
	_lastValues[port] = value;
	if (_nextCallback != nullptr)
	{
		_nextCallback(port, value);
	}
}

boolean Frequency::handleSysex(byte command, byte argc, byte* argv)
{
  if (command != FREQUENCY_COMMAND)
  {
	  return false;
  }
  if (argc >= 2)
  {
	  byte frequencyCommand = argv[0];
	  byte pin = argv[1];
	  // Clear
	  if (frequencyCommand == FREQUENCY_SUBCOMMAND_CLEAR)
	  {
		  _activeBitmasks[pin / 8] &= ~(1 << (pin & 7));
		  _ticks[pin] = 0; // Because that is easier to check during reporting
	  }
  }
  if (argc >= 5) // Expected: A command byte, a pin, the mode and a packed short
  {
	  byte frequencyCommand = argv[0];
	  byte pin = argv[1];
	  byte mode = argv[2]; // see below
	  int32_t ms = (argv[4] << 7) | argv[3];
	  // Set or query
	  if (frequencyCommand == FREQUENCY_SUBCOMMAND_QUERY)
	  {
		  if (pin >= TOTAL_PINS)
		  {
			  Firmata.sendString(F("Invalid pin number for frequency command"));
			  return true;
		  }
		  
		  if (ms > 0)
		  {
			  _lastReport = millis();
			  _reportDelay = ms;
		  }

		  _mode = mode;
			  
			  // Firmata.sendStringf(F("Frequency mode enabled with delay %ld on pin %ld"), 8, (int32_t)_reportDelay, (int32_t)pin);
		  _activeBitmasks[pin / 8] |= (1 << (pin & 7));
	  	 
		  Firmata.setPinMode(pin, PIN_MODE_FREQUENCY);
		  reportValue(pin);
	  }
  }
  return true;
}

void Frequency::report(bool elapsed)
{
	int mi = millis();
	if (mi < _lastReport || mi > (_lastReport + _reportDelay))
	{
		for (int i = 0; i < TOTAL_PINS; i++)
		{
			if (_ticks[i] != 0)
			{
				reportValue(i);
			}
		}
		_lastReport = mi;
	}
}

void Frequency::reportValue(int pin)
{
	int32_t currentTime = millis();
	// Clear the interrupt flag, so that we can read out the counter
	noInterrupts();
	int32_t ticks = _ticks[pin];
	interrupts();
	Firmata.startSysex();
	Firmata.write(FREQUENCY_COMMAND);
	Firmata.write(FREQUENCY_SUBCOMMAND_REPORT);
	Firmata.write(pin);
	Firmata.sendPackedUInt32(currentTime);
	Firmata.sendPackedUInt32(ticks);
	Firmata.endSysex();
}

boolean Frequency::handlePinMode(byte pin, int mode)
{
  int interruptPin = digitalPinToInterrupt(pin);
  if (IS_PIN_DIGITAL(pin) && interruptPin >= 0) 
  {
    if (mode == PIN_MODE_FREQUENCY) {
      return true;
    } else if (pin == _activePin)
	{
      detachInterrupt(interruptPin);
	  _activePin = -1;
    }
  }
  return false;
}

void Frequency::handleCapability(byte pin)
{
  int interrupt = digitalPinToInterrupt(pin);
  if (IS_PIN_DIGITAL(pin) && interrupt >= 0) {
    Firmata.write((byte)PIN_MODE_FREQUENCY);
    Firmata.write((byte)0); // 4 byte clock, 4 byte timestamp
  }
}

void Frequency::reset()
{
	if (_activePin >= 0)
	{
		detachInterrupt(digitalPinToInterrupt(_activePin));
		_activePin = -1;
	}
}
