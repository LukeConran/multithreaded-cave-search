#pragma once

#ifndef STATS_H
#define STATS_H

#include "pch.h"
#include "Storage.h"
#include <atomic>
#include <chrono>
#include <mutex>

class Stats {
private:
    std::atomic<int> totalRooms;
    std::atomic<int> discoveredRooms;
    std::atomic<int> exploredRooms;
    std::atomic<int> activeThreads;
    std::atomic<int> totalThreads;
    std::atomic<int> explorationRate;
    std::atomic<bool> cleanupComplete;

    Ubase* storage = nullptr;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastPrintTime;
    int lastExploredCount = 0;
    double lastTimePoint = 0.0;
    std::mutex statsMutex;

public:
    Stats(int planet);

    void setStorage(Ubase* s) { storage = s; }
    void recordDiscoveredRoom() { discoveredRooms++; }
    void recordExploredRoom() { exploredRooms++; }
    void incrementActiveThreads() { activeThreads++; }
    void decrementActiveThreads() { activeThreads--; }
    void incrementTotalThreads() { totalThreads++; }
    void decrementTotalThreads() { totalThreads--; }
    void setCleanupComplete() { cleanupComplete = true; }
    bool isCleanupComplete() const { return cleanupComplete.load(); }
    int getExploredRooms() { return exploredRooms.load(); }
    int getActiveThreads() { return activeThreads.load(); }

    double getElapsedSeconds() const;
    void printStats();
    void printFinalStats();
};

#endif