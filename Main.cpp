#include "pch.h"
#include "ProcessManager.h"
#include "Robot.h"
#include "Storage.h"
#include "Stats.h"
#include "Search.h"
#include "Discovered.h" // New include
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: program <planet> <cave> <numThreads> <searchAlgorithm>\n");
        return 1;
    }

    int planet = atoi(argv[1]), cave = atoi(argv[2]), numThreads = atoi(argv[3]);
    const char* searchAlgorithm = "BFS";
    Discovered discovered;
    Stats stats(planet);
    std::mutex mutex;
    std::unique_ptr<Ubase> storage = std::make_unique<Ubreadth>();

    stats.setStorage(storage.get());

    HANDLE eventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
    CommandCC center = initializeCommandCC(argv[1], argv[2], argv[3]);
    PROCESS_INFORMATION pi{};
    HANDLE CC = startCCProcess(center, pi);
    if (CC == INVALID_HANDLE_VALUE) return -1;

    int numRooms = static_cast<int>(pow(2, planet));
    HANDLE semaphore = CreateSemaphore(NULL, 0, numRooms, NULL);

    std::thread statsThread(&Search::StatsThread, std::ref(stats), eventQuit);

    std::vector<std::unique_ptr<std::thread>> threads;
    for (int i = 0; i < center.robots; i++) {
        threads.push_back(std::make_unique<std::thread>(&Search::Run, i, pi.dwProcessId, storage.get(),
            std::ref(stats), std::ref(mutex), eventQuit,
            semaphore, std::ref(discovered))); // Pass discovered
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (auto& thread : threads) thread->join();
    stats.setCleanupComplete();
    statsThread.join();

    cleanupCC(CC);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(eventQuit);
    CloseHandle(semaphore);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
    printf("Execution time %.2f seconds\n", duration.count() / 1000.0);

    return 0;
}