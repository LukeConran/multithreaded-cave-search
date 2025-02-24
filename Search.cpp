#include "pch.h"
#include "Search.h"
#include "ProcessManager.h"

void Search::Run(int robotIndex, DWORD processId, Ubase* storage, Stats& stats,
    std::mutex& mtx, HANDLE eventQuit, HANDLE semaphore, Discovered& discovered) {
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE)) {
        printf("Failed to set thread %d to idle priority: %d\n", robotIndex, GetLastError());
    }

    stats.incrementTotalThreads();

    HANDLE robotPipe = connectToRobot(robotIndex, processId);
    if (robotPipe == INVALID_HANDLE_VALUE) {
        printf("Error connecting to robot %d\n", robotIndex);
        stats.decrementTotalThreads();
        return;
    }

    Robot robot(robotPipe, robotIndex);
    handleRobot(robot);

    {
        std::lock_guard<std::mutex> lock(mtx);
        if (discovered.checkAdd(robot.getCurrentNode())) {
            storage->push(robot.getCurrentNode(), 0);
            stats.recordDiscoveredRoom();
            ReleaseSemaphore(semaphore, 1, NULL);
        }
    }

    HANDLE waitHandles[2] = { eventQuit, semaphore };
    while (true) {
        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0) break;

        UnexploredRoom room;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (storage->size() == 0) continue;
            room = storage->pop();
            stats.recordExploredRoom();
        }

        stats.incrementActiveThreads();

        CommandRobotHeader moveCommand = { MOVE };
        DWORD roomIDs[1] = { room.ID }; // Batch size of 1 for simplicity, need to change TODO
        DWORD commandSize = sizeof(CommandRobotHeader) + sizeof(DWORD) * 1;
        char* commandBuffer = new char[commandSize];
        memcpy(commandBuffer, &moveCommand, sizeof(CommandRobotHeader));
        memcpy(commandBuffer + sizeof(CommandRobotHeader), roomIDs, sizeof(DWORD) * 1);

        if (!WriteFile(robot.getPipe(), commandBuffer, commandSize, NULL, NULL)) {
            printf("Error sending MOVE command to robot %d\n", robot.getIndex());
            delete[] commandBuffer;
            break;
        }
        delete[] commandBuffer;

        DWORD bytesRead = 0;
        if (!ReadFile(robot.getPipe(), robot.getBuffer(), robot.getBufferSize(), &bytesRead, NULL)) {
            printf("Error reading response from robot %d: %d\n", robot.getIndex(), GetLastError());
            return;
        }

        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(robot.getPipe(), NULL, 0, NULL, &bytesAvailable, NULL)) {
            printf("Error peeking robot pipe for %d: %d\n", robot.getIndex(), GetLastError());
            return;
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
                return;
            }
            bytesRead += bytesAvailable;
        }

        char* bufferPtr = robot.getBuffer();
        ResponseRobotHeader* response = reinterpret_cast<ResponseRobotHeader*>(bufferPtr);
        bufferPtr += sizeof(ResponseRobotHeader);

        if (response->status != STATUS_OK) {
            printf("Robot %d: Error for room %u - Status %d\n", robotIndex, room.ID, response->status);
            stats.decrementActiveThreads();
            continue; // Skip this room and try the next
        }


        //if (0 == 1) {} if I want to turn off the exit

        if (response->len == 0) { // Exit found
            std::lock_guard<std::mutex> lock(mtx);
            if (WaitForSingleObject(eventQuit, 0) != WAIT_OBJECT_0) {
                printf("Thread %d: found exit %u, steps %d, distance %d\n", robotIndex, room.ID, stats.getExploredRooms(), room.distance);
                SetEvent(eventQuit);
            }
        }
        else {
            std::lock_guard<std::mutex> lock(mtx);
            DWORD* neighbors = reinterpret_cast<DWORD*>(bufferPtr);
            for (DWORD i = 0; i < response->len; i++) {
                if (discovered.checkAdd(neighbors[i])) {
                    storage->push(neighbors[i], room.distance + 1);
                    stats.recordDiscoveredRoom();
                    ReleaseSemaphore(semaphore, 1, NULL);
                }
            }
            if (storage->size() == 0 && stats.getActiveThreads() == 1) {
                printf("There is no exit to this cave.\n");
                SetEvent(eventQuit);
            }
        }

        stats.decrementActiveThreads();
    }

    cleanupRobot(robot);
    stats.decrementTotalThreads();
}

void Search::StatsThread(Stats& stats, HANDLE eventQuit) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    auto nextPrintTime = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto timeUntilPrint = std::chrono::duration_cast<std::chrono::milliseconds>(nextPrintTime - now).count();

        if (timeUntilPrint <= 0) {
            stats.printStats();
            nextPrintTime = now + std::chrono::seconds(2);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        DWORD waitResult = WaitForSingleObject(eventQuit, static_cast<DWORD>((((timeUntilPrint) < (static_cast<int64_t>(100))) ? (timeUntilPrint) : (static_cast<int64_t>(100)))));
        if (waitResult == WAIT_OBJECT_0) {
            stats.printFinalStats();
            break;
        }
    }
}