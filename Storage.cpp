#include "pch.h"
#include "Storage.h"


/// UBREADTH (BFS)
void Ubreadth::push(DWORD roomID, int distance) {
    roomsQueue.push({ roomID, distance });
}

UnexploredRoom Ubreadth::pop() {
    UnexploredRoom room;
    room.ID = roomsQueue.front().first;
    room.distance = roomsQueue.front().second;
    roomsQueue.pop();
    return room;
}

int Ubreadth::size() {
    return static_cast<int>(roomsQueue.size());
}