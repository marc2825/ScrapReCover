#pragma once
#include <random>


enum class OperationType { MOVE = 0, ROTATE = 1, SWAP = 2, ERASE = 3, ADD = 4 };

struct NeighborhoodOp {
    OperationType op;
    int idx1 = 0;
    int idx2 = 0;
    double angle = 0.0;
    int dir = 0;
    int pos_x = 0;
    int pos_y = 0;
};
