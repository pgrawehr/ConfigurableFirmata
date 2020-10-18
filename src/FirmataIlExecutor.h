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
#include <ObjectStack.h>

#define IL_EXECUTE_NOW 0
#define IL_LOAD 1
#define IL_DECLARE 2

#define MAX_METHODS 10
#define MAX_PARAMETERS 10

#define METHOD_STATIC 1
#define METHOD_VIRTUAL 2
#define METHOD_SPECIAL 4

struct IlCode
{
	byte methodFlags;
	byte methodLength;
	byte maxLocals; // the maximum of (number of local variables, execution stack size)
	byte numArgs;
	byte* methodIl;
	uint32_t methodToken;
};

class ExecutionState
{
	public:
	short _pc;
	ObjectStack _executionStack;
	uint32_t* _locals;
	uint32_t* _args;
	
	// Next inner execution frame (the innermost frame is being executed) 
	ExecutionState* _next;
	ExecutionState(int maxLocals, int argCount) : _executionStack(maxLocals), _pc(0)
	{
		_next = NULL;
		_locals = NULL;
		if (maxLocals > 0)
		{
			_locals = (uint32_t*)malloc(maxLocals * sizeof(uint32_t));
			memset(_locals, 0, maxLocals * sizeof(uint32_t));
		}
	}
	~ExecutionState()
	{
		free(_locals);
	}
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
	void LoadIlDeclaration(byte codeReference, byte flags, byte maxLocals, byte argc, byte* argv);
	void DecodeParametersAndExecute(byte codeReference, byte argc, byte* argv);
	bool ExecuteIlCode(ExecutionState *state, int codeLength, byte* pCode, int argc, uint32_t* argList, uint32_t* returnValue);
	int ResolveToken(uint32_t token);
	uint32_t DecodeUint32(byte* argv);
	IlCode _methods[MAX_METHODS];
};


#endif 
