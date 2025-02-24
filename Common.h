#ifndef COMMON_H
#define COMMON_H

#pragma once

#include <Windows.h>

#define CONNECT 0
#define DISCONNECT 1
#define MOVE 2
#define FAILURE 0
#define SUCCESS 1

#define STATUS_OK 0 // no error, command successful
#define STATUS_ALREADY_CONNECTED 1 // repeated attempt to connect
#define STATUS_INVALID_COMMAND 2 // command too short or invalid
#define STATUS_INVALID_ROOM 3 // room ID doesn't exist
#define STATUS_INVALID_BATCH_SIZE 4 // batch size too large or equals 0
#define STATUS_MUST_CONNECT 5 // first command must be CONNECT

const int DEFAULT_BUFFER_SIZE = 128;
const int SLEEP_DELAY_MS = 100;

#endif