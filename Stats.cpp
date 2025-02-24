#include "pch.h"
#include "Stats.h"
#include <cmath>


Stats::Stats(int planet)
    : totalRooms(static_cast<int>(pow(2, planet))),
    discoveredRooms(0), exploredRooms(0), activeThreads(0), totalThreads(0),
    explorationRate(0), cleanupComplete(false) {
    startTime = lastPrintTime = std::chrono::steady_clock::now();
}

double Stats::getElapsedSeconds() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
}

void Stats::printStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    double elapsed = getElapsedSeconds();
    int currentExplored = exploredRooms.load();
    if (elapsed - lastTimePoint >= 1.0) {  // Update rate every second
        explorationRate = static_cast<int>((currentExplored - lastExploredCount) / (elapsed - lastTimePoint));
        lastExploredCount = currentExplored;
        lastTimePoint = elapsed;
    }
    int remainingRooms = (((0) > (totalRooms.load() - discoveredRooms.load())) ? (0) : (totalRooms.load() - discoveredRooms.load()));

    printf("[%.0fs] E %.2fK, U %.2fK, D %.2fK, %d/sec, active %d, run %d\n",
        elapsed, storage->size() / 1000.0, remainingRooms / 1000.0, currentExplored / 1000.0,
        explorationRate.load(), activeThreads.load(), totalThreads.load());
}

void Stats::printFinalStats() {
    while (!cleanupComplete.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::lock_guard<std::mutex> lock(statsMutex);
    double totalSeconds = getElapsedSeconds();
    int totalExplored = exploredRooms.load();
    int averageRate = static_cast<int>(totalExplored / totalSeconds);
    int remainingRooms = (((0) > (totalRooms.load() - discoveredRooms.load())) ? (0) : (totalRooms.load() - discoveredRooms.load()));

    printf("[final] E %.2fK, U %.2fK, D %.2fK, %d/sec, active %d, run %d\n",
        storage->size() / 1000.0, remainingRooms / 1000.0, totalExplored / 1000.0,
        averageRate, activeThreads.load(), totalThreads.load());
}