#include "utils/polygon_generator.hpp"

#include <algorithm>
#include <random>

#include "core/canvas_model.hpp"
#include "utils/config.hpp"


MyPolygon PolygonGenerator() {
    return PolygonGenerator(Config::GetGenIsLarge(), Config::GetGenIsRect(),
                            Config::GetGenVertexMin(), Config::GetGenVertexMax(),
                            Config::GetGenRadiusMean(), Config::GetGenRadiusVar());
}

MyPolygon PolygonGenerator(bool is_large, bool is_rectangle, int v_min, int v_max, double r_mean,
                           double r_var) {

    Polygon polygon;
    const double rect_large_min = Config::GetGenRectLargeMin();
    const double rect_large_max = Config::GetGenRectLargeMax();
    const double rect_small_min = Config::GetGenRectSmallMin();
    const double rect_small_max = Config::GetGenRectSmallMax();
    const double min_extent_large = Config::GetGenMinExtentLarge();
    const double min_extent_small = Config::GetGenMinExtentSmall();

    if (is_rectangle) {
        double width;
        double height;
        std::uniform_real_distribution<double> dist_large(rect_large_min, rect_large_max);
        std::uniform_real_distribution<double> dist_small(rect_small_min, rect_small_max);
        if (is_large) {
            width = dist_large(CanvasModel::Get().GetRngShapeGen());
            height = dist_large(CanvasModel::Get().GetRngShapeGen());
        } else {
            width = dist_small(CanvasModel::Get().GetRngShapeGen());
            height = dist_small(CanvasModel::Get().GetRngShapeGen());
        }
        double x_min = -width / 2.0;
        double x_max = width / 2.0;
        double y_min = -height / 2.0;
        double y_max = height / 2.0;

        Array<Vec2> vertices = {Vec2(x_min, y_min), Vec2(x_max, y_min), Vec2(x_max, y_max),
                                Vec2(x_min, y_max)};

        polygon = Polygon(vertices);
    } else {
        std::uniform_int_distribution<int> vnum_dist(v_min, v_max);
        int v_num = vnum_dist(CanvasModel::Get().GetRngShapeGen());
        if (!is_large) {
            r_mean /= 2;
        }

        NormalDistribution<double> dist(r_mean, r_var);

        while (true) {
            double r1 = dist(CanvasModel::Get().GetRngShapeGen());
            double r2 = dist(CanvasModel::Get().GetRngShapeGen());
            double r_min_val = std::min(r1, r2);
            double r_max_val = std::max(r1, r2);
            r_min_val = std::max(1.0, r_min_val);

            Array<double> radii;
            radii.reserve(v_num);
            for (int i = 0; i < v_num; ++i) {
                std::uniform_real_distribution<double> radius_dist(r_min_val, r_max_val);
                radii.push_back(radius_dist(CanvasModel::Get().GetRngShapeGen()));
            }

            Array<double> angles;
            double pi2 = 2 * Math::Pi;
            std::uniform_real_distribution<double> angle_dist(0.0, pi2);
            for (int i = 0; i < v_num; ++i) {
                angles.push_back(angle_dist(CanvasModel::Get().GetRngShapeGen()));
            }
            sort(angles.begin(), angles.end()); // Argument Sorting

            Array<Vec2> points;
            points.reserve(v_num);
            for (int i = 0; i < v_num; ++i) {
                double x = radii[i] * cos(angles[i]);
                double y = radii[i] * sin(angles[i]);
                points.emplace_back(x, y);
            }

            double x_max_val = -1e9;
            double x_min_val = 1e9;
            double y_max_val = -1e9;
            double y_min_val = 1e9;
            for (const auto &p : points) {
                x_max_val = std::max(x_max_val, p.x);
                x_min_val = std::min(x_min_val, p.x);
                y_max_val = std::max(y_max_val, p.y);
                y_min_val = std::min(y_min_val, p.y);
            }

            if (is_large && ((x_max_val - x_min_val) < min_extent_large ||
                             (y_max_val - y_min_val) < min_extent_large)) {
                continue;
            }
            if (!is_large && ((x_max_val - x_min_val) < min_extent_small ||
                              (y_max_val - y_min_val) < min_extent_small)) {
                continue;
            }

            Polygon cur_poly(points);
            if (!cur_poly.contains(Vec2(0, 0))) {
                continue;
            }

            polygon = cur_poly;
            break;
        }
    }

    std::uniform_int_distribution<int> color_dist(0, 255);
    int r = color_dist(CanvasModel::Get().GetRngShapeGen());
    int g = color_dist(CanvasModel::Get().GetRngShapeGen());
    int b = color_dist(CanvasModel::Get().GetRngShapeGen());
    Color color(r, g, b);

    return MyPolygon(polygon, CanvasModel::Get().GetUnplacedPolygonNum(), color);
}
