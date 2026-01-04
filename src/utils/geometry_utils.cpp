#include "utils/geometry_utils.hpp"

#include <algorithm>
#include <cmath>


bool PointInTriangleStrict(const s3d::Vec2& p,
                           const s3d::Vec2& a,
                           const s3d::Vec2& b,
                           const s3d::Vec2& c,
                           double eps) {
    s3d::Vec2 ab = b - a;
    s3d::Vec2 bc = c - b;
    s3d::Vec2 ca = a - c;
    double d1 = ab.cross(p - a);
    double d2 = bc.cross(p - b);
    double d3 = ca.cross(p - c);

    if (std::abs(d1) <= eps || std::abs(d2) <= eps || std::abs(d3) <= eps) {
        return false;
    }

    bool hasPositive = (d1 > eps) && (d2 > eps) && (d3 > eps);
    bool hasNegative = (d1 < -eps) && (d2 < -eps) && (d3 < -eps);
    return hasPositive || hasNegative;
}

bool PointInConvexPolygonStrict(const s3d::Vec2& p,
                                const s3d::Array<s3d::Vec2>& vertices,
                                double eps) {
    if (vertices.size() < 3) {
        return false;
    }

    bool hasPositive = false;
    bool hasNegative = false;

    for (size_t i = 0; i < vertices.size(); ++i) {
        const s3d::Vec2& current = vertices[i];
        const s3d::Vec2& next = vertices[(i + 1) % vertices.size()];
        double cross = (next - current).cross(p - current);
        if (std::abs(cross) <= eps) {
            return false;
        }
        if (cross > 0) {
            hasPositive = true;
        } else if (cross < 0) {
            hasNegative = true;
        }
        if (hasPositive && hasNegative) {
            return false;
        }
    }
    return true;
}

bool PointInRectStrict(const s3d::Vec2& p, const s3d::RectF& rect, double eps) {
    return (p.x > rect.x + eps &&
            p.x < rect.x + rect.w - eps &&
            p.y > rect.y + eps &&
            p.y < rect.y + rect.h - eps);
}

bool PointInAxisAlignedSquareStrict(const s3d::Vec2& p, double min, double max) {
    return (p.x > min && p.x < max && p.y > min && p.y < max);
}

bool PointInAnnulusStrict(const s3d::Vec2& p,
                          const s3d::Vec2& center,
                          double inner,
                          double outer,
                          double eps) {
    const double dx = p.x - center.x;
    const double dy = p.y - center.y;
    const double r2 = dx * dx + dy * dy;
    const double outer2 = outer * outer;
    const double inner2 = inner * inner;
    return (r2 > inner2 + eps) && (r2 < outer2 - eps);
}

bool PointInEllipseStrict(const s3d::Vec2& p,
                          const s3d::Vec2& center,
                          double a,
                          double b,
                          double eps) {
    const double dx = p.x - center.x;
    const double dy = p.y - center.y;
    const double value = (dx * dx) / (a * a) + (dy * dy) / (b * b);
    return value < 1.0 - eps;
}

bool PointInPolygonStrict(const s3d::Vec2& p,
                          const s3d::Array<s3d::Vec2>& vertices,
                          double eps) {
    const int count = static_cast<int>(vertices.size());
    if (count < 3) {
        return false;
    }

    if (eps > 0.0) {
        const double eps_sq = eps * eps;
        for (int i = 0; i < count; ++i) {
            const s3d::Vec2& a = vertices[i];
            const s3d::Vec2& b = vertices[(i + 1) % count];
            if (DistanceSquaredPointSegment(p, a, b) <= eps_sq) {
                return false;
            }
        }
    }

    bool inside = false;
    for (int i = 0, j = count - 1; i < count; j = i++) {
        const s3d::Vec2& vi = vertices[i];
        const s3d::Vec2& vj = vertices[j];
        const bool intersects =
            ((vi.y > p.y) != (vj.y > p.y)) &&
            (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y) + vi.x);
        if (intersects) {
            inside = !inside;
        }
    }
    return inside;
}

s3d::Array<s3d::Vec2> BuildRegularPolygonVertices(const s3d::Vec2& center,
                                                  double radius,
                                                  int sides,
                                                  double start_angle_rad) {
    s3d::Array<s3d::Vec2> vertices;
    if (sides < 3) {
        return vertices;
    }
    vertices.reserve(sides);
    const double step = 2.0 * s3d::Math::Pi / static_cast<double>(sides);
    for (int k = 0; k < sides; ++k) {
        const double angle = start_angle_rad + step * k;
        vertices.emplace_back(center.x + radius * std::cos(angle),
                              center.y + radius * std::sin(angle));
    }
    return vertices;
}

double DistanceSquaredPointSegment(const s3d::Vec2& point,
                                   const s3d::Vec2& a,
                                   const s3d::Vec2& b) {
    const s3d::Vec2 ab = b - a;
    const double ab_len_sq = ab.dot(ab);
    if (ab_len_sq <= 0.0) {
        const s3d::Vec2 ap = point - a;
        return ap.dot(ap);
    }
    const double t = std::clamp((point - a).dot(ab) / ab_len_sq, 0.0, 1.0);
    const s3d::Vec2 projection = a + ab * t;
    const s3d::Vec2 diff = point - projection;
    return diff.dot(diff);
}
