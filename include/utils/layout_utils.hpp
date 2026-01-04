#pragma once
#include <Siv3D.hpp>

#include "core/polygon.hpp"
#include "utils/config.hpp"


inline constexpr int dx[4] = {1, 0, -1, 0};
inline constexpr int dy[4] = {0, 1, 0, -1};

inline int ceil_div(int a, int b) { return (a + b - 1) / b; }

bool InLayout(int x, int y);
bool IsEmpty(int x, int y);
