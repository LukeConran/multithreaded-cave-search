#pragma once

#ifndef STORAGE_H
#define STORAGE_H

#include "pch.h"
#include "Messages.h"

class Ubase {
public:
    virtual ~Ubase() = default;
    virtual void push(DWORD roomID, int distance) = 0;
    virtual UnexploredRoom pop() = 0;
    virtual int size() = 0;
};

class Ubreadth : public Ubase {
private:
    std::queue<std::pair<DWORD, int>> roomsQueue;
public:
    void push(DWORD roomID, int distance) override;
    UnexploredRoom pop() override;
    int size() override;
};

#endif