#include "pch.h"
#include "Search.h"
#include "ProcessManager.h"
#include "ReadBuffer.h"

void Search::Run(int robotIndex, DWORD processId) {
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
        EnterCriticalSection(&cs);
        if (discovered.checkAdd(robot.getCurrentNode())) { //might be something to optimize (get just the ID's?)
            storage->push(robot.getCurrentNode(), 0);
            stats.recordDiscoveredRoom();
            //ReleaseSemaphore(semaphore, 1, NULL);
        }
        LeaveCriticalSection(&cs);
    }

    //HANDLE waitHandles[2] = { eventQuit, semaphore };
    while (true) {
         //DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        DWORD waitResult = WaitForSingleObject(eventQuit, 0);
        if (waitResult == WAIT_OBJECT_0) break;

        int batchSize;
        UnexploredRoom* batchOfRooms;
        {
            EnterCriticalSection(&cs);
            if (storage->size() == 0) {
                LeaveCriticalSection(&cs);
                continue;
            }
            batchSize = 10000 < storage->size() ? 10000 : storage->size();
            batchOfRooms = new UnexploredRoom[batchSize];
            for (int i = 0; i < batchSize; i++) {
                batchOfRooms[i] = storage->pop();
                stats.recordExploredRoom();
            }
            LeaveCriticalSection(&cs);
        }
        stats.incrementActiveThreads();

        CommandRobotHeader moveCommand = { MOVE };
        DWORD commandSize = sizeof(CommandRobotHeader) + sizeof(DWORD) * batchSize;
        char* commandBuffer = new char[commandSize];
        memcpy(commandBuffer, &moveCommand, sizeof(CommandRobotHeader));
        DWORD* roomIDs = new DWORD[batchSize]; // Can I somehow do this without as many heap allocated arrays?
        for (int i = 0; i < batchSize; i++) {
            roomIDs[i] = batchOfRooms[i].ID; // This has to be able to be done without a for loop somehow
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

        if (!ReadBuffer::ReadTheBuffer(robot)) {
            delete[] batchOfRooms;
            stats.decrementActiveThreads();
            break;
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
                continue;
            }

            if (response->len == 0) { // Exit found
                EnterCriticalSection(&cs);
                if (WaitForSingleObject(eventQuit, 0) != WAIT_OBJECT_0) {
                    printf("Thread %d: found exit %u, steps %d, distance %d\n", robotIndex, batchOfRooms[i].ID, stats.getExploredRooms(), batchOfRooms[i].distance);
                    SetEvent(eventQuit);
                }
                LeaveCriticalSection(&cs);
            }
            else {
                DWORD* neighbors = reinterpret_cast<DWORD*>(bufferPtr);
                {
                    EnterCriticalSection(&cs);
                    for (DWORD j = 0; j < response->len; j++) {
                        if (discovered.checkAdd(neighbors[j])) {
                            storage->push(neighbors[j], (batchOfRooms[i].distance) + 1);
                            stats.recordDiscoveredRoom();
                            //ReleaseSemaphore(semaphore, 1, NULL); // No need for semaphore
                        }
                    }
                    if (storage->size() == 0 && stats.getActiveThreads() == 1) {
                        printf("There is no exit to this cave.\n");
                        SetEvent(eventQuit);
                    }
                    LeaveCriticalSection(&cs);
                }
                bufferPtr += sizeof(DWORD) * response->len;
            }
        }
        
        delete[] batchOfRooms;
        stats.decrementActiveThreads();
    }

    cleanupRobot(robot);
    stats.decrementTotalThreads();
}

void Search::StatsThread() {
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