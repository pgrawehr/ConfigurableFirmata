#ifndef OBJECTSTACK_H
#define OBJECTSTACK_H
#include "Arduino.h"

class ObjectStack
{
	// TODO: Replace int with void*
	int _maxSize;
	int _stackPtr = 0;
	int* data;
	public:
	ObjectStack(int maxSize)
	{
		_stackPtr = maxSize - 1;
		_maxSize = maxSize;
		data = (int*)malloc(_maxSize * sizeof(int));
	}
	
	~ObjectStack()
	{
		free(data);
		data = NULL;
	}
	
	void push(int object)
	{
		data[_stackPtr] = object;
		_stackPtr--;
	}
	
	int pop()
	{
		_stackPtr ++;
		return data[_stackPtr];
	}
	
	int peek()
	{
		return data[_stackPtr + 1];
	}
	
	bool empty()
	{
		return _stackPtr == _maxSize - 1;
	}
	
};
#endif