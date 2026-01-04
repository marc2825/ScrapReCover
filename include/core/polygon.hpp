#pragma once
#include <cmath>

#include <Siv3D.hpp>

#include "utils/config.hpp"


class MyPolygon {
    public:
            MyPolygon(const Polygon& p, int idx, Color c = Palette::Black, std::optional<FilePath> path = std::nullopt)
            : original_poly_(p), poly_(p), color_(c), index_(idx), texture_path_(path), selection_priority_(1.0) // Default weight to 1.0
            {
                ApplySeamAllowance();
                LoadTexture();
                CalcArea();
                CalcEdgesLength();
                CalcRectSimilarity(Config::GetRectSimilarityCalcStep());
                CalcSelectionWeight();
            }

        MyPolygon() : original_poly_(Polygon()), poly_(Polygon()), index_(-1), color_(Palette::Black), texture_path_(std::nullopt), selection_priority_(1.0) {}


        void SetCenter(Vec2 c);
        void SetCenterMousePos(Vec2 c, const Vec2& offset, double scale);
        void SetRotation(double r);
        void SetSelectionPriority(double weight);
        static void SetRegularShapePreference(double value) { regular_shape_preference_ = value; }
        static void SetDrawOriginal(bool enabled);
        static bool IsDrawOriginal();

        int GetIndex() const { return index_; }
        const Vec2& GetCenter() const { return center_; }
        double GetRotation() const { return rotation_; }
        const Color& GetColor() const { return color_; }
        const std::optional<FilePath>& GetTexturePath() const { return texture_path_; }
        double GetArea() const { return area_; }
        double GetEdgesLength() const { return edges_length_; }
        double GetRectSimilarity() const { return rect_similarity_; }
        double GetSelectionWeight() const { return selection_weight_; }
        double GetSelectionPriority() const { return selection_priority_; }
        const std::vector<Point>& GetRasterizedPoints() const { return rasterized_points_; }
        const Polygon& GetOriginalPolygon() const { return original_poly_; }
        const Polygon& GetPolygon() const { return poly_; }

        static void Initialize();
        void CalcSelectionWeight();
        static double GetRegularShapePreference();
        void Move(const Vec2& v);
        void Rotate(const double angle); 
        void RasterizePolygon();

        void Draw(const Vec2& offset, double scale) const;
        void DrawFrame(const Vec2& offset, double scale, const int width = 1,
                       const Color color = Palette::Black) const;
        Polygon CalcDisplayPolygon(const Vec2& offset, double scale) const;


    private:
        Polygon original_poly_;
        Polygon poly_;
        Color color_;
        int index_;
        std::optional<FilePath> texture_path_;
        std::optional<Texture> texture_;
        double rotation_ = 0.0; // Radians
        Vec2 center_; // Center position used in optimization (not UI space)

        double rect_similarity_; // Shape-specific, fixed
        double selection_priority_;  // Coefficient, mutable
        double selection_weight_; // = selection_priority * (rect_similarity ** reg_shape_pref (shared))

        double area_;
        double edges_length_;
        std::vector<Point> rasterized_points_; // Discrete representation of the polygon

        static double regular_shape_preference_;
        static bool draw_original_;

        void ApplySeamAllowance();
        const Polygon& GetDrawPolygon() const;
        void LoadTexture();

        void CalcArea() { area_ = poly_.area(); }
        void CalcEdgesLength();
        void CalcRectSimilarity(float step);
};
