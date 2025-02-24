#pragma once

#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include "pch.h"
#include "Messages.h"

CommandCC initializeCommandCC(const char* planet, const char* cave, const char* robots);
HANDLE startCCProcess(CommandCC& center, PROCESS_INFORMATION& pi);
void cleanupCC(HANDLE CC);
HANDLE connectToRobot(int robotIndex, DWORD processId);

#endif