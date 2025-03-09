#pragma once

#ifndef SEARCH_H
#define SEARCH_H

#include "pch.h"
#include "Robot.h"
#include "Storage.h"
#include "Stats.h"
#include "Discovered.h"
#include <mutex>
#include <memory>

class Search {
private:
    CRITICAL_SECTION cs;
    Discovered discovered;
    Stats stats;
    std::unique_ptr<Ubase> storage;
    HANDLE eventQuit;
    HANDLE semaphore;
    std::mutex mutex;
    int numRooms;

public:
    Search(int planet) : stats(planet) {
        InitializeCriticalSection(&cs);
        storage = std::make_unique<Ubreadth>();
        stats.setStorage(storage.get());
        eventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
        numRooms = static_cast<int>(pow(2, planet));
        semaphore = CreateSemaphore(NULL, 0, numRooms, NULL);
    }

    ~Search() {
        CloseHandle(eventQuit);
        CloseHandle(semaphore);
        DeleteCriticalSection(&cs);
    }

    void Run(int robotIndex, DWORD processId);
    void StatsThread();

    // Getter methods if needed
    Stats& getStats() { return stats; }
    HANDLE getEventQuit() { return eventQuit; }
};

#endif