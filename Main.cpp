#include "pch.h"
#include "ProcessManager.h"
#include "Robot.h"
#include "Storage.h"
#include "Stats.h"
#include "Search.h"
#include "Discovered.h"
#include <vector>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: program <planet> <cave> <numThreads> <searchAlgorithm>\n");
        return 1;
    }

    int planet = atoi(argv[1]), cave = atoi(argv[2]), numThreads = atoi(argv[3]);
    const char* searchAlgorithm = "BFS";

    // Create Search instance with planet parameter
    Search search(planet);

    CommandCC center = initializeCommandCC(argv[1], argv[2], argv[3]);
    PROCESS_INFORMATION pi{};
    HANDLE CC = startCCProcess(center, pi);
    if (CC == INVALID_HANDLE_VALUE) return -1;

    std::thread statsThread(&Search::StatsThread, &search);

    std::vector<std::unique_ptr<std::thread>> threads;
    for (int i = 0; i < center.robots; i++) {
        threads.push_back(std::make_unique<std::thread>(&Search::Run, &search, i, pi.dwProcessId));
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (auto& thread : threads) thread->join();
    search.getStats().setCleanupComplete();
    statsThread.join();

    cleanupCC(CC);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
    printf("Execution time %.2f seconds\n", duration.count() / 1000.0);

    return 0;
}