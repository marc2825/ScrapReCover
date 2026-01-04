#pragma once
#include <Siv3D.hpp>

#include "core/polygon.hpp"


MyPolygon PolygonGenerator();
MyPolygon PolygonGenerator(bool is_large, bool is_rectangle, int v_min, int v_max, double r_mean,
                           double r_var);
