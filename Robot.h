#pragma once

#ifndef ROBOT_H
#define ROBOT_H

#include "pch.h"
#include "Messages.h"
#include <memory>

class Robot {
private:
    HANDLE pipe;
    int index;
    DWORD bufferSize;
    char* buffer;
    DWORD* currentNode; //I these need to be something else (for the LPWORD)
public:
    Robot(HANDLE p, int i);
    Robot(Robot&& other) noexcept;
    Robot(const Robot&) = delete;
    Robot& operator=(const Robot&) = delete;
    ~Robot() = default;
    HANDLE getPipe() const { return pipe; }
    int getIndex() const { return index; }
    char* getBuffer() { return buffer; }
    DWORD& getBufferSize() { return bufferSize; }
    void setBuffer(char* buf) { buffer = buf; }
    void setBufferSize(DWORD bufSize) { bufferSize = bufSize; }
    DWORD getCurrentNode() const { return *currentNode; } //I think the LPWORD and the following line need to be something else
    void setCurrentNode(DWORD* node) { currentNode = node; } //
};

void handleRobot(Robot& robot);
void cleanupRobot(Robot& robot);

#endif