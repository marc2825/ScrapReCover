#pragma once

#include <Siv3D.hpp>


bool PointInTriangleStrict(const s3d::Vec2& p,
                           const s3d::Vec2& a,
                           const s3d::Vec2& b,
                           const s3d::Vec2& c,
                           double eps);

bool PointInConvexPolygonStrict(const s3d::Vec2& p,
                                const s3d::Array<s3d::Vec2>& vertices,
                                double eps);

bool PointInRectStrict(const s3d::Vec2& p, const s3d::RectF& rect, double eps);

bool PointInAxisAlignedSquareStrict(const s3d::Vec2& p, double min, double max);

bool PointInAnnulusStrict(const s3d::Vec2& p,
                          const s3d::Vec2& center,
                          double inner,
                          double outer,
                          double eps);

bool PointInEllipseStrict(const s3d::Vec2& p,
                          const s3d::Vec2& center,
                          double a,
                          double b,
                          double eps);

bool PointInPolygonStrict(const s3d::Vec2& p,
                          const s3d::Array<s3d::Vec2>& vertices,
                          double eps);

// Builds regular polygon vertices ordered CCW with a given start angle (radians).
s3d::Array<s3d::Vec2> BuildRegularPolygonVertices(const s3d::Vec2& center,
                                                  double radius,
                                                  int sides,
                                                  double start_angle_rad);

double DistanceSquaredPointSegment(const s3d::Vec2& point,
                                   const s3d::Vec2& a,
                                   const s3d::Vec2& b);
