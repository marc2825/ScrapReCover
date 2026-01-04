#include "core/polygon.hpp"

#include <algorithm>


double MyPolygon::regular_shape_preference_ = 0.0;
bool MyPolygon::draw_original_ = false;

void MyPolygon::SetDrawOriginal(bool enabled) {
    draw_original_ = enabled;
}

bool MyPolygon::IsDrawOriginal() {
    return draw_original_;
}

// Shrink the scrap slightly to account for the seam allowance (restored when saving the layout). 
// The contour defines the effective area processed by the system.
void MyPolygon::ApplySeamAllowance() {
    const double allowance = Config::GetSeamAllowance();
    if (allowance <= 0.0) {
        poly_ = original_poly_;
        return;
    }

    Polygon shrunk = original_poly_.calculateBuffer(-allowance);
    if (shrunk.isEmpty()) {
        poly_ = original_poly_;
        return;
    }

    poly_ = shrunk;
}

const Polygon& MyPolygon::GetDrawPolygon() const {
    return draw_original_ ? original_poly_ : poly_;
}

void MyPolygon::SetCenter(Vec2 c) { 
    center_ = c;
    RasterizePolygon();
}

void MyPolygon::SetCenterMousePos(Vec2 c, const Vec2& offset, double scale) {
    c -= offset;
    SetCenter(c / scale);
}

void MyPolygon::SetRotation(double r) {
    rotation_ = r;
    //RasterizePolygon(); // Omitted here. Add back for general use.
}

void MyPolygon::SetSelectionPriority(double weight) {
    selection_priority_ = weight;
    CalcSelectionWeight();
}

void MyPolygon::CalcSelectionWeight() {
    selection_weight_ = selection_priority_ * (pow(rect_similarity_, regular_shape_preference_));
}

double MyPolygon::GetRegularShapePreference() {
    return regular_shape_preference_;
}

void MyPolygon::Move(const Vec2& v) {
    center_ += v;
    RasterizePolygon();
}

void MyPolygon::Rotate(const double angle) { 
    rotation_ += angle;
    RasterizePolygon();
}

// Store grid points strictly inside the polygon.
void MyPolygon::RasterizePolygon() {
    rasterized_points_.clear();
    Vec2 center = GetCenter();
    poly_.rotate(rotation_);

    Rect rect = poly_.boundingRect().asRect();
    int xlen = rect.w;
    int ylen = rect.h;

    // is_in: whether each grid vertex lies inside the polygon.
    std::vector<std::vector<bool>> is_in(xlen + 1, std::vector<bool>(ylen + 1, false));
    for (int dx = 0; dx <= xlen; ++dx) {
        for (int dy = 0; dy <= ylen; ++dy) {
            int x = rect.x + dx;
            int y = rect.y + dy;
            is_in[dx][dy] = poly_.contains(Vec2(x, y));
        }
    }

    // Add a pixel if all four corners lie inside the polygon.
    for (int dx = 0; dx < xlen; ++dx) {
        for (int dy = 0; dy < ylen; ++dy) {
            if (is_in[dx][dy] && is_in[dx+1][dy] && is_in[dx][dy+1] && is_in[dx+1][dy+1]) {
                rasterized_points_.push_back(Point(rect.x + dx + (int)round(center.x), rect.y + dy + (int)round(center.y)));
            }
        }
    }

    poly_.rotate(-rotation_);
}

void MyPolygon::Draw(const Vec2& offset, double scale) const {
    const Polygon& draw_poly = GetDrawPolygon();
    if(texture_.has_value()) {
        texture_.value().scaled(scale / Config::GetTextureScaleDivisor()).rotated(rotation_).drawAt(center_ * scale + offset);
    }
    else {
        draw_poly.scaled(scale).rotated(rotation_).draw(center_ * scale + offset, color_);
    }
}

void MyPolygon::DrawFrame(const Vec2& offset, double scale, const int width, const Color color) const {
    const Polygon& draw_poly = GetDrawPolygon();
    draw_poly.scaled(scale).rotated(rotation_).drawFrame(center_ * scale + offset, width, color);
}

Polygon MyPolygon::CalcDisplayPolygon(const Vec2& offset, double scale) const {
    const Polygon& draw_poly = GetDrawPolygon();
    Polygon display_polygon = draw_poly;
    display_polygon.rotate(rotation_);
    display_polygon.scale(scale);
    display_polygon.moveBy(center_ * scale + offset);

    return display_polygon;
}

void MyPolygon::LoadTexture() {
    if (texture_path_) {
        texture_ = Texture(texture_path_.value());
        if (texture_->isEmpty()) {
            texture_ = std::nullopt;
        }
    } else {
        texture_ = std::nullopt;
    }
}

void MyPolygon::CalcEdgesLength() {
    double total_length = 0.0;
    const auto& vertices = poly_.vertices();
    int len = vertices.size();
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vec2 current = vertices[i];
        const Vec2 next = vertices[(i + 1) % vertices.size()];
        total_length += current.distanceFrom(next);
    }

    edges_length_ = total_length;
}

void MyPolygon::CalcRectSimilarity(float step) {
    if (step <= 0.0) {
        step = 1.0;
    }
    double areaPoly = poly_.area();
    double maxRatio = 0.0;

    for (double angle = 0.0; angle < 360.0; angle += step) {
        Polygon rotated = poly_.rotated(ToRadians(angle));

        RectF rect = rotated.boundingRect();
        double bbArea = rect.w * rect.h;
        if (bbArea > 0) {
            double ratio = areaPoly / bbArea;
            maxRatio = std::max(maxRatio, ratio);
        }
    }
    
    rect_similarity_ = maxRatio;
}
