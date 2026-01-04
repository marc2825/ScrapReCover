#pragma once
#include <variant>

#include <Siv3D.hpp>

#include "ui/ui_types.hpp"

// Pattern definitions are experimental placeholders; adjust freely.
// Define shapes here and extend them as needed for new arbitrary pattern types.
namespace pattern_mask {

// Square interior [min, max) in both axes.
struct SquareDef {
    double min = 0.0;
    double max = 0.0;
};

// Triangle defined by three vertices and epsilon for edge strictness.
struct TriangleDef {
    s3d::Vec2 a;
    s3d::Vec2 b;
    s3d::Vec2 c;
    double eps = 0.0;
};

// Donut (annulus) with center, outer/inner radii and epsilon for boundary.
struct DonutDef {
    s3d::Vec2 center;
    double outer = 0.0;
    double inner = 0.0;
    double eps = 0.0;
};

// T-shirt-like shape composed from normalized rectangles plus a circular neck cutout.
struct TshirtDef {
    double size = 0.0;
    double eps = 0.0;
    s3d::RectF body;
    s3d::RectF shoulders;
    s3d::RectF left_sleeve;
    s3d::RectF right_sleeve;
    s3d::Vec2 neck_center;
    double neck_radius = 0.0;
    double neck_min_y = 0.0;
    double neck_max_y = 0.0;
};

// Regular hexagon defined by center and radius (vertices are placed every 60 degrees).
struct HexagonDef {
    s3d::Vec2 center;
    double radius = 0.0;
    double eps = 0.0;
};

// Arbitrary polygon with CCW vertices; eps trims the boundary as outside.
struct PolygonDef {
    s3d::Array<s3d::Vec2> vertices;
    double eps = 0.0;
};

// Axis-aligned ellipse with center and radii a (x) / b (y).
struct EllipseDef {
    s3d::Vec2 center;
    double a = 0.0;
    double b = 0.0;
    double eps = 0.0;
};

using PatternDefinition = std::variant<SquareDef, TriangleDef, DonutDef, TshirtDef, HexagonDef, PolygonDef, EllipseDef>;

PatternDefinition BuildPatternDefinition(ui::PatternShapeType shape, int layout_size);
bool IsPointInsidePattern(const PatternDefinition& def, const s3d::Vec2& point);
s3d::Grid<bool> BuildPatternMask(ui::PatternShapeType shape);
int CountPatternMaskCells(const s3d::Grid<bool>& mask);

} // namespace pattern_mask
