/*
  FirmataIlExecutor

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

*/

#ifndef FirmataIlExecutor_h
#define FirmataIlExecutor_h

#include <ConfigurableFirmata.h>
#include <FirmataFeature.h>

#define IL_EXECUTE_NOW 0
#define IL_LOAD 1

#define MAX_METHODS 10
#define MAX_PARAMETERS 10

struct IlCode
{
	byte methodNumber;
	byte methodLength;
	byte* methodIl;
};


class FirmataIlExecutor: public FirmataFeature
{
  public:
    FirmataIlExecutor();
    boolean handlePinMode(byte pin, int mode);
    void handleCapability(byte pin);
    boolean handleSysex(byte command, byte argc, byte* argv);
	void reset();
 
  private:
    void LoadIlDataStream(byte codeReference, byte codeLength, byte offset, byte argc, byte* argv);
	void DecodeParametersAndExecute(byte codeReference, byte argc, byte* argv);
	
	IlCode _methods[MAX_METHODS];
};


#endif 
