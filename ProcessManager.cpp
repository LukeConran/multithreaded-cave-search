#include "pch.h"
#include "ProcessManager.h"

CommandCC initializeCommandCC(const char* planet, const char* cave, const char* robots) {
    CommandCC cmd{};
    cmd.command = CONNECT;
    cmd.planet = static_cast<unsigned char>(atoi(planet));
    cmd.cave = static_cast<DWORD>(atoi(cave));
    cmd.robots = static_cast<unsigned short>(atoi(robots));
    return cmd;
}

HANDLE startCCProcess(CommandCC& center, PROCESS_INFORMATION& pi) {
    STARTUPINFO s{};
    GetStartupInfo(&s);
    printf("Starting CC.exe...\n");
    char path[] = "CC-hw2-2.4.exe";
    if (!CreateProcess(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &s, &pi)) {
        printf("Error %d starting %s\n", GetLastError(), path);
        return INVALID_HANDLE_VALUE;
    }
    char pipename[128];
    sprintf_s(pipename, "\\\\.\\pipe\\CC-%X", pi.dwProcessId);
    while (!WaitNamedPipe(pipename, INFINITE)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DELAY_MS));
    }
    printf("Connecting to CC with planet %u, cave %u, robots %u...\n", center.planet, center.cave, center.robots);

    // now open the pipe
    HANDLE CC = CreateFile(pipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (CC == INVALID_HANDLE_VALUE) {
        printf("error creating file\n");
        return INVALID_HANDLE_VALUE;
    }

    ResponseCC response = { 0, {0} };

    DWORD bytesWritten, bytesRead;
    if (!WriteFile(CC, &center, sizeof(CommandCC), &bytesWritten, NULL) || bytesWritten != sizeof(CommandCC)) {
        printf("Write error: %d\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }
    if (!ReadFile(CC, &response, sizeof(ResponseCC), &bytesRead, NULL) || bytesRead != sizeof(ResponseCC)) {
        printf("Read error: %d\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    printf("CC says: status = %d, msg = '%s'\n", response.status, response.msg);
    if (response.status == 0)
    {
        printf("Connecting error, quitting...\n");
        return INVALID_HANDLE_VALUE;
    }

    return CC;
}

void cleanupCC(HANDLE CC) {
    // Cleanup CC
    printf("Waiting for CC.exe to quit...\n");
    CommandCC disconnectCommand = { DISCONNECT, 0, 0, 0 };
    if (!WriteFile(CC, &disconnectCommand, sizeof(disconnectCommand), NULL, NULL)) {
        printf("Error disconnecting CC: %d\n", GetLastError());
    }
    CloseHandle(CC);
}

HANDLE connectToRobot(int robotIndex, DWORD processId) {
    char robotPipeName[128];
    sprintf_s(robotPipeName, "\\\\.\\pipe\\CC-%X-robot-%X", processId, robotIndex);

    while (!WaitNamedPipe(robotPipeName, INFINITE)) {
        Sleep(100);
    }

    //printf("Connecting to robot %d...\n", robotIndex);

    HANDLE robotPipe = CreateFile(robotPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (robotPipe == INVALID_HANDLE_VALUE) {
        printf("Error opening robot pipe %s: %d\n", robotPipeName, GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    return robotPipe;
}