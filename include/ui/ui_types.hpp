#pragma once

#include <Siv3D.hpp>


namespace ui {

enum class PatternShapeType { Square, Triangle, Donut, Tshirt, Hexagon, Ellipse, Polygon }; // Freely extend
enum class SelectType { None, Unplaced, Placed };
enum class ScrapLoadMode { Preset, Cutter, Generator };

struct LayoutPattern {
    s3d::String name;
    s3d::Array<s3d::Vec2> points;
};

} // namespace ui
