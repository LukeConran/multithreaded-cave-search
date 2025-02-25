#include "pch.h"
#include "ReadBuffer.h"

bool ReadBuffer::ReadTheBuffer(Robot &robot) {
    DWORD bytesRead = 0;
    if (!ReadFile(robot.getPipe(), robot.getBuffer(), robot.getBufferSize(), &bytesRead, NULL)) {
        printf("Error reading response from robot %d: %d\n", robot.getIndex(), GetLastError());
        return false;
    }

    DWORD bytesAvailable = 0;
    if (!PeekNamedPipe(robot.getPipe(), NULL, 0, NULL, &bytesAvailable, NULL)) {
        printf("Error peeking robot pipe for %d: %d\n", robot.getIndex(), GetLastError());
        return false;
    }

    if (bytesAvailable > 0) {
        char* newBuffer = new char[bytesRead + bytesAvailable];
        robot.setBufferSize(bytesRead + bytesAvailable);
        memcpy(newBuffer, robot.getBuffer(), bytesRead);
        delete[] robot.getBuffer();
        robot.setBuffer(newBuffer);

        DWORD extraBytesRead = 0;
        if (!ReadFile(robot.getPipe(), robot.getBuffer() + bytesRead, bytesAvailable, &extraBytesRead, NULL)) {
            printf("Error reading remaining response from robot %d: %d\n", robot.getIndex(), GetLastError());
            return false;
        }
        bytesRead += bytesAvailable;
    }

    return true;
}