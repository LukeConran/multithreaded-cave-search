#include "pch.h"
#include "Discovered.h"

bool Discovered::checkAdd(DWORD roomID) {
    if (discoveredSet.count(roomID) == 0) {
        discoveredSet.insert(roomID);
        return true;
    }
    return false;
}

int Discovered::size() const {
    return static_cast<int>(discoveredSet.size());
}