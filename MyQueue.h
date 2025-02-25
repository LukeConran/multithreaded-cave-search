#pragma once

#include "pch.h"

class MyQueue {
	HANDLE heap; // custom heap
	DWORD* buf; // buffer pointer
	DWORD head, tail; // usual head/tail offsets
	DWORD spaceAllocated; // buffer size
	DWORD sizeQ; // number of items in Q
public:
	void Push(DWORD item); // single push
	DWORD Pop(DWORD* array, int batchSize); // batch pop
};