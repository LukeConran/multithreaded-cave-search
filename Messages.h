#pragma once

#ifndef MESSAGES_H
#define MESSAGES_H

#include "pch.h"
#include "Common.h"

#pragma pack(push,1)
struct CommandRobotHeader {
    DWORD command;
};
#pragma pack(pop)

#pragma pack(push,1)
struct ResponseRobotHeader {
    DWORD status : 3; // 3 bits for status
    DWORD len : 29;   // 29 bits for neighbor count
};
#pragma pack(pop)

#pragma pack(push,1)
struct CommandCC {
    unsigned char command : 2;
    unsigned char planet : 6;
    DWORD cave;
    unsigned short robots;
};
#pragma pack(pop)

#pragma pack(push,1)
struct ResponseCC {
    DWORD status;
    char msg[64];
};
#pragma pack(pop)

//#pragma pack(push,1)
//struct CommandRobot {
//    DWORD command;
//    uint64_t room;
//};
//#pragma pack(pop)
//
//#pragma pack(push,1)
//struct ResponseRobot {
//    DWORD status;
//    char msg[64];
//};
//#pragma pack(pop)

//#pragma pack(push,1)
//struct NodeTuple64 {
//    uint64_t node;
//    float intensity;
//};
//#pragma pack(pop)

#pragma pack(push,1)
struct UnexploredRoom {
    DWORD ID;
    int distance;
    float intensity = 0.0f;
};
#pragma pack(pop)

#endif