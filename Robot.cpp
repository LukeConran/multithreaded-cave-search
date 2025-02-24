#include "pch.h"
#include "Robot.h"

Robot::Robot(HANDLE p, int i)
    : pipe(p), index(i), bufferSize(DEFAULT_BUFFER_SIZE), buffer(new char[bufferSize]), currentNode(nullptr) {
}

Robot::Robot(Robot&& other) noexcept
    : pipe(other.pipe), index(other.index), bufferSize(other.bufferSize), buffer(std::move(other.buffer)), currentNode(other.currentNode) {
    other.pipe = nullptr;
    other.currentNode = nullptr;
}

void handleRobot(Robot& robot) {
    CommandRobotHeader cmd = { CONNECT }; // No room ID needed for CONNECT

    if (!WriteFile(robot.getPipe(), &cmd, sizeof(cmd), NULL, NULL)) {
        printf("Error sending CONNECT to robot %d: %d\n", robot.getIndex(), GetLastError());
        return;
    }

    DWORD bytesRead;
    if (!ReadFile(robot.getPipe(), robot.getBuffer(), robot.getBufferSize(), &bytesRead, NULL)) {
        printf("Error reading response from robot %d: %d\n", robot.getIndex(), GetLastError());
        return;
    }

    // Handle dynamic buffer resizing if more data is available
    DWORD bytesAvailable = 0;
    if (!PeekNamedPipe(robot.getPipe(), NULL, 0, NULL, &bytesAvailable, NULL)) {
        printf("Error peeking pipe for robot %d: %d\n", robot.getIndex(), GetLastError());
        return;
    }

    if (bytesAvailable > 0) {
        char* newBuffer = new char[bytesRead + bytesAvailable];
        memcpy(newBuffer, robot.getBuffer(), bytesRead);
        delete[] robot.getBuffer();
        robot.setBuffer(newBuffer);
        robot.setBufferSize(bytesRead + bytesAvailable);

        DWORD extraBytesRead = 0;
        if (!ReadFile(robot.getPipe(), robot.getBuffer() + bytesRead, bytesAvailable, &extraBytesRead, NULL)) {
            printf("Error reading additional response from robot %d: %d\n", robot.getIndex(), GetLastError());
            return;
        }
        bytesRead += extraBytesRead;
    }

    ResponseRobotHeader* response = reinterpret_cast<ResponseRobotHeader*>(robot.getBuffer());

    if (response->status != STATUS_OK) {
        printf("Robot %d: CONNECT failed with status %d\n", robot.getIndex(), response->status);
        return;
    }

    if (response->len != 1) {
        printf("Robot %d: Expected 1 starting room, got %d\n", robot.getIndex(), response->len);
        return;
    }

    DWORD* startNode = reinterpret_cast<DWORD*>(robot.getBuffer() + sizeof(ResponseRobotHeader));
    robot.setCurrentNode(startNode); // Store the starting room ID (DWORD)
}

void cleanupRobot(Robot& robot) {
    CommandRobotHeader cmd = { DISCONNECT }; // No room ID for DISCONNECT

    if (!WriteFile(robot.getPipe(), &cmd, sizeof(cmd), NULL, NULL)) {
        printf("Error disconnecting robot %d: %d\n", robot.getIndex(), GetLastError());
    }
    CloseHandle(robot.getPipe());
}