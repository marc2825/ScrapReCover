#pragma once
#include <array>


struct Hyperparameter {
    static std::array<double, 5> P;
    static std::array<double, 5> noP;
    static long long np_pen;
    static long long or_pen;
    static long long ol_pen;
    static double sttmp;
    static double entmp;

    // Load values from config.json (falls back to built-in defaults).
    static void LoadFromConfig();
};
