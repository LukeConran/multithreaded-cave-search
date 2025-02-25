#include "pch.h"
#include "Search.h"
#include "ProcessManager.h"

void Search::Run(int robotIndex, DWORD processId, Ubase* storage, Stats& stats,
    std::mutex& mtx, HANDLE eventQuit, HANDLE semaphore, Discovered& discovered) {
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE)) {
        printf("Failed to set thread %d to idle priority: %d\n", robotIndex, GetLastError());
        return;
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
        if (discovered.checkAdd(robot.getCurrentNode())) { //might be something to optimize
            storage->push(robot.getCurrentNode(), 0);
            stats.recordDiscoveredRoom();
            ReleaseSemaphore(semaphore, 1, NULL);
        }
    }

    HANDLE waitHandles[2] = { eventQuit, semaphore };
    while (true) {
        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0) break;

        int batchSize;
        UnexploredRoom* batchOfRooms;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (storage->size() == 0) continue;
            batchSize = 10000 < storage->size() ? 10000 : storage->size();
            batchOfRooms = new UnexploredRoom[batchSize];
            for (int i = 0; i < batchSize; i++) {
                batchOfRooms[i] = storage->pop();
                stats.recordExploredRoom();
            }
        }
        stats.incrementActiveThreads();

        CommandRobotHeader moveCommand = { MOVE };
        DWORD commandSize = sizeof(CommandRobotHeader) + sizeof(DWORD) * batchSize;
        char* commandBuffer = new char[commandSize];
        memcpy(commandBuffer, &moveCommand, sizeof(CommandRobotHeader));
        DWORD* roomIDs = new DWORD[batchSize];
        for (int i = 0; i < batchSize; i++) {
            roomIDs[i] = batchOfRooms[i].ID;
        }
        memcpy(commandBuffer + sizeof(CommandRobotHeader), roomIDs, sizeof(DWORD) * batchSize);

        DWORD bytesWritten;
        if (!WriteFile(robot.getPipe(), commandBuffer, commandSize, &bytesWritten, NULL)) {
            printf("Error sending MOVE command to robot %d\n", robot.getIndex());
            delete[] commandBuffer;
            delete[] roomIDs;
            cleanupRobot(robot);
            break;
        }
        delete[] commandBuffer;
        delete[] roomIDs;

        DWORD bytesRead = 0;
        if (!ReadFile(robot.getPipe(), robot.getBuffer(), robot.getBufferSize(), &bytesRead, NULL)) {
            printf("Error reading response from robot %d: %d\n", robot.getIndex(), GetLastError());
            delete[] batchOfRooms;
            stats.decrementActiveThreads();
            break;
        }

        DWORD bytesAvailable = 0;
        if (!PeekNamedPipe(robot.getPipe(), NULL, 0, NULL, &bytesAvailable, NULL)) {
            printf("Error peeking robot pipe for %d: %d\n", robot.getIndex(), GetLastError());
            delete[] batchOfRooms;
            stats.decrementActiveThreads();
            break;
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
                delete[] batchOfRooms;
                stats.decrementActiveThreads();
                break;
            }
            bytesRead += bytesAvailable;
        }

        char* bufferPtr = robot.getBuffer();

        for (int i = 0; i < batchSize; i++) {
            ResponseRobotHeader* response = reinterpret_cast<ResponseRobotHeader*>(bufferPtr);
            bufferPtr += sizeof(ResponseRobotHeader);

            if (response->status != STATUS_OK) {
                const char* errorMsg = nullptr;
                switch (response->status) {
                case STATUS_INVALID_ROOM: errorMsg = "Invalid room ID"; break;
                case STATUS_INVALID_BATCH_SIZE: errorMsg = "Invalid batch size"; break;
                default: errorMsg = "Unknown error"; break;
                }
                printf("Robot %d: Error for room %u - %s (Status %d)\n", robotIndex, batchOfRooms[i].ID, errorMsg, response->status);
                continue; // Use continue instead of break to process remaining rooms
            }

            if (response->len == 0) { // Exit found
                std::lock_guard<std::mutex> lock(mtx);
                if (WaitForSingleObject(eventQuit, 0) != WAIT_OBJECT_0) {
                    printf("Thread %d: found exit %u, steps %d, distance %d\n", robotIndex, batchOfRooms[i].ID, stats.getExploredRooms(), batchOfRooms[i].distance);
                    SetEvent(eventQuit);
                }
            }
            else {
                std::lock_guard<std::mutex> lock(mtx);
                DWORD* neighbors = reinterpret_cast<DWORD*>(bufferPtr);
                for (DWORD j = 0; j < response->len; j++) {
                    if (discovered.checkAdd(neighbors[j])) {
                        storage->push(neighbors[j], (batchOfRooms[i].distance) + 1);
                        stats.recordDiscoveredRoom();
                        ReleaseSemaphore(semaphore, 1, NULL);
                        //do i need to increment bufferPtr here
                    }
                }
                bufferPtr += sizeof(DWORD) * response->len;
                if (storage->size() == 0 && stats.getActiveThreads() == 1) {
                    printf("There is no exit to this cave.\n");
                    SetEvent(eventQuit);
                }
            }
        }
        
        delete[] batchOfRooms;
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