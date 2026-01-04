#include "utils/pattern_mask.hpp"

#include <algorithm>
#include <cmath>

#include "ui/ui_constants.hpp"
#include "utils/config.hpp"
#include "utils/geometry_utils.hpp"


using namespace s3d;

namespace pattern_mask {
namespace {

const nlohmann::json* GetPatternConfig(const char* key) {
    return Config::GetPatternShapeConfig(key);
}

template <typename T>
T GetOr(const nlohmann::json& j, const char* key, const T& fallback) {
    if (!j.contains(key)) {
        return fallback;
    }
    try {
        return j.at(key).get<T>();
    } catch (...) {
        return fallback;
    }
}

Vec2 GetVec2Or(const nlohmann::json& j, const char* key, const Vec2& fallback) {
    if (!j.contains(key)) {
        return fallback;
    }
    const auto& value = j.at(key);
    if (value.is_array() && value.size() == 2) {
        try {
            return Vec2(value.at(0).get<double>(), value.at(1).get<double>());
        } catch (...) {
            return fallback;
        }
    }
    if (value.is_object()) {
        const double x = GetOr<double>(value, "x", fallback.x);
        const double y = GetOr<double>(value, "y", fallback.y);
        return Vec2(x, y);
    }
    return fallback;
}

Array<Vec2> GetVerticesOr(const nlohmann::json& j, const char* key, const Array<Vec2>& fallback) {
    if (!j.contains(key)) {
        return fallback;
    }
    const auto& value = j.at(key);
    if (!value.is_array()) {
        return fallback;
    }
    Array<Vec2> vertices;
    vertices.reserve(value.size());
    for (const auto& entry : value) {
        if (entry.is_array() && entry.size() == 2) {
            try {
                vertices.emplace_back(entry.at(0).get<double>(), entry.at(1).get<double>());
            } catch (...) {
                return fallback;
            }
        } else if (entry.is_object()) {
            const double x = GetOr<double>(entry, "x", 0.0);
            const double y = GetOr<double>(entry, "y", 0.0);
            vertices.emplace_back(x, y);
        } else {
            return fallback;
        }
    }
    if (vertices.size() < 3) {
        return fallback;
    }
    return vertices;
}

Array<Vec2> ScaleVertices(const Array<Vec2>& vertices, double scale) {
    Array<Vec2> scaled;
    scaled.reserve(vertices.size());
    for (const auto& vertex : vertices) {
        scaled.emplace_back(vertex.x * scale, vertex.y * scale);
    }
    return scaled;
}

bool IsInside(const SquareDef& def, const Vec2& point) {
    return PointInAxisAlignedSquareStrict(point, def.min, def.max);
}

bool IsInside(const TriangleDef& def, const Vec2& point) {
    return PointInTriangleStrict(point, def.a, def.b, def.c, def.eps);
}

bool IsInside(const DonutDef& def, const Vec2& point) {
    return PointInAnnulusStrict(point, def.center, def.inner, def.outer, def.eps);
}

bool IsInside(const TshirtDef& def, const Vec2& point) {
    const double nx = point.x / def.size;
    const double ny = point.y / def.size;
    const Vec2 normalized(nx, ny);
    bool inside = false;

    if (PointInRectStrict(normalized, def.body, def.eps)) {
        inside = true;
    }
    if (PointInRectStrict(normalized, def.shoulders, def.eps)) {
        inside = true;
    }
    if (PointInRectStrict(normalized, def.left_sleeve, def.eps)) {
        inside = true;
    }
    if (PointInRectStrict(normalized, def.right_sleeve, def.eps)) {
        inside = true;
    }

    const double dx = nx - def.neck_center.x;
    const double dy = ny - def.neck_center.y;
    const bool in_neck =
        (ny > def.neck_min_y - def.eps && ny < def.neck_max_y + def.eps &&
         (dx * dx + dy * dy < def.neck_radius * def.neck_radius));
    if (in_neck) {
        inside = false;
    }

    return inside;
}

bool IsInside(const HexagonDef& def, const Vec2& point) {
    constexpr double kStartAngle = Math::Pi / 6.0;
    Array<Vec2> vertices = BuildRegularPolygonVertices(def.center, def.radius, 6, kStartAngle);
    return PointInConvexPolygonStrict(point, vertices, def.eps);
}

bool IsInside(const PolygonDef& def, const Vec2& point) {
    return PointInPolygonStrict(point, def.vertices, def.eps);
}

bool IsInside(const EllipseDef& def, const Vec2& point) {
    return PointInEllipseStrict(point, def.center, def.a, def.b, def.eps);
}

} // namespace


PatternDefinition BuildPatternDefinition(ui::PatternShapeType shape, int layout_size) {
    const double s = static_cast<double>(layout_size);

    switch (shape) {
    case ui::PatternShapeType::Square: {
        const auto* config = GetPatternConfig("square");
        const double eps = config ? GetOr<double>(*config, "eps", ui::PatternEps) : ui::PatternEps;
        const double min_norm = config ? GetOr<double>(*config, "min", 0.0) : 0.0;
        const double max_norm = config ? GetOr<double>(*config, "max", 1.0) : 1.0;
        const double min_val = min_norm * s + eps;
        const double max_val = max_norm * s - eps;
        if (max_val <= min_val) {
            return SquareDef{ui::PatternEps, s - ui::PatternEps};
        }
        return SquareDef{min_val, max_val};
    }
    case ui::PatternShapeType::Triangle: {
        TriangleDef def;
        const auto* config = GetPatternConfig("triangle");
        const Vec2 a_norm = config ? GetVec2Or(*config, "a", Vec2(0.5, 0.0)) : Vec2(0.5, 0.0);
        const Vec2 b_norm = config ? GetVec2Or(*config, "b", Vec2(0.0, 1.0)) : Vec2(0.0, 1.0);
        const Vec2 c_norm = config ? GetVec2Or(*config, "c", Vec2(1.0, 1.0)) : Vec2(1.0, 1.0);
        def.a = a_norm * s;
        def.b = b_norm * s;
        def.c = c_norm * s;
        def.eps = config ? GetOr<double>(*config, "eps", ui::PatternEps) : ui::PatternEps;
        return def;
    }
    case ui::PatternShapeType::Donut: {
        DonutDef def;
        const auto* config = GetPatternConfig("donut");
        const Vec2 center_norm = config ? GetVec2Or(*config, "center", Vec2(0.5, 0.5)) : Vec2(0.5, 0.5);
        const double default_outer = 0.5 - 0.5 / s;
        const double outer_norm = config ? GetOr<double>(*config, "outer", default_outer) : default_outer;
        const double default_inner = default_outer * 0.5;
        const double inner_norm = config ? GetOr<double>(*config, "inner", default_inner) : default_inner;
        def.center = center_norm * s;
        def.outer = outer_norm * s;
        def.inner = inner_norm * s;
        def.eps = config ? GetOr<double>(*config, "eps", ui::PatternEps) : ui::PatternEps;
        return def;
    }
    case ui::PatternShapeType::Tshirt: { // hard-coded for experiment
        TshirtDef def;
        def.size = s;
        def.eps = 1e-3;
        def.body = RectF(0.25, 0.40, 0.50, 0.60);
        def.shoulders = RectF(0.10, 0.22, 0.80, 0.18);
        def.left_sleeve = RectF(0.05, 0.40, 0.20, 0.28);
        def.right_sleeve = RectF(0.75, 0.40, 0.20, 0.28);
        def.neck_center = Vec2(0.5, 0.30);
        def.neck_radius = 0.12;
        def.neck_min_y = 0.22;
        def.neck_max_y = 0.38;
        return def;
    }
    case ui::PatternShapeType::Hexagon: {
        HexagonDef def;
        const auto* config = GetPatternConfig("hexagon");
        const Vec2 center_norm = config ? GetVec2Or(*config, "center", Vec2(0.5, 0.5)) : Vec2(0.5, 0.5);
        const double radius_norm = config ? GetOr<double>(*config, "radius", 0.45) : 0.45;
        def.center = center_norm * s;
        def.radius = radius_norm * s;
        def.eps = config ? GetOr<double>(*config, "eps", ui::PatternEps) : ui::PatternEps;
        return def;
    }
    case ui::PatternShapeType::Polygon: {
        PolygonDef def;
        const auto* config = GetPatternConfig("polygon");
        const Array<Vec2> default_vertices = {Vec2(0.0, 0.0), Vec2(1.0, 0.0), Vec2(1.0, 1.0), Vec2(0.0, 1.0)};
        const Array<Vec2> vertices_norm = config ?
            GetVerticesOr(*config, "vertices", default_vertices) :
            default_vertices;
        def.vertices = ScaleVertices(vertices_norm, s);
        def.eps = config ? GetOr<double>(*config, "eps", ui::PatternEps) : ui::PatternEps;
        return def;
    }
    case ui::PatternShapeType::Ellipse: {
        EllipseDef def;
        const auto* config = GetPatternConfig("ellipse");
        const Vec2 center_norm = config ? GetVec2Or(*config, "center", Vec2(0.5, 0.5)) : Vec2(0.5, 0.5);
        const double a_norm = config ? GetOr<double>(*config, "a", 0.45) : 0.45;
        const double b_norm = config ? GetOr<double>(*config, "b", 0.35) : 0.35;
        def.center = center_norm * s;
        def.a = a_norm * s;
        def.b = b_norm * s;
        def.eps = config ? GetOr<double>(*config, "eps", 1e-3) : 1e-3;
        return def;
    }
    }
    return SquareDef{ui::PatternEps, s - ui::PatternEps};
}

bool IsPointInsidePattern(const PatternDefinition& def, const Vec2& point) {
    return std::visit([&](const auto& shape_def) { return IsInside(shape_def, point); }, def);
}

// Loose check: set flag if at least one vertex contains in the pattern (opposite to the strict check for scraps).
s3d::Grid<bool> BuildPatternMask(ui::PatternShapeType shape) {
    const int layout_size = Config::GetLayoutSize();
    const int pattern_layout_size = Config::GetPatternShapeLayoutSize();
    const int margin = Config::GetCanvasMargin();
    const int canvas_size = Config::GetCanvasSize();
    Grid<bool> mask(canvas_size, canvas_size, false);

    const PatternDefinition def = BuildPatternDefinition(shape, pattern_layout_size);

    for (int sy = 0; sy < layout_size; ++sy) {
        for (int sx = 0; sx < layout_size; ++sx) {
            bool valid = false;
            for (int dy = 0; dy <= 1 && !valid; ++dy) {
                for (int dx = 0; dx <= 1; ++dx) {
                    const double px = static_cast<double>(sx + dx);
                    const double py = static_cast<double>(sy + dy);
                    if (IsPointInsidePattern(def, Vec2(px, py))) {
                        valid = true;
                        break;
                    }
                }
            }
            mask[sx + margin][sy + margin] = valid;
        }
    }

    return mask;
}

int CountPatternMaskCells(const Grid<bool>& mask) {
    int count = 0;
    for (int x = 0; x < mask.width(); ++x) {
        for (int y = 0; y < mask.height(); ++y) {
            if (mask[x][y]) {
                count++;
            }
        }
    }
    return count;
}

} // namespace pattern_mask
