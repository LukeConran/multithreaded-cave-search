#pragma once

#ifndef DISCOVERED_H
#define DISCOVERED_H

#include "pch.h"
#include "Messages.h"
#include <set>

class Discovered {
private:
    std::set<DWORD> discoveredSet;

public:
    bool checkAdd(DWORD roomID);
    int size() const;
};

#endif