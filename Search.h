#pragma once

#ifndef SEARCH_H
#define SEARCH_H

#include "pch.h"
#include "Robot.h"
#include "Storage.h"
#include "Stats.h"
#include "Discovered.h"
#include <mutex>

class Search {
public:
    static void Run(int robotIndex, DWORD processId, Ubase* storage, Stats& stats,
        std::mutex& mtx, HANDLE eventQuit, HANDLE semaphore, Discovered& discovered);
    static void StatsThread(Stats& stats, HANDLE eventQuit);
};

#endif