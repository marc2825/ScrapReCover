#include "ui/ui_manager.hpp"

#include <algorithm>

#include "algorithm/eval_functions.hpp"
#include "algorithm/optimizer.hpp"
#include "core/canvas_model.hpp"
#include "ui/ui_constants.hpp"
#include "utils/layout_utils.hpp"
#include "utils/pattern_mask.hpp"


using namespace s3d;

void UIManager::DrawCanvas() {
    auto &model = model_;
    auto &placed_polygons = model.GetPlacedPolygons();
    auto &canvas_placement_count = model.GetCanvasPlacementCount();
    auto &cur_empty_count = model.GetCurEmptyCount();
    const int cur_waste = model.GetCurWaste();
    const auto &pattern_mask = model.GetPatternMaskConst();

    double cell_ratio = Config::GetCanvasUIRatio();
    int margin = Config::GetCanvasMargin();
    int layout_size = Config::GetLayoutSize();
    int mask_width = static_cast<int>(pattern_mask.width());
    int mask_height = static_cast<int>(pattern_mask.height());
    const Vec2 canvas_offset(Config::GetCanvasUIX(), Config::GetCanvasUIY());
    const double canvas_scale = cell_ratio;

    constexpr double kValidFill = 0.85;
    constexpr double kMinOutlineThickness = 1.5;
    constexpr double kOutlineThicknessRatio = 0.1;
    constexpr int kPolygonFrameThickness = 1;
    constexpr int kProgressBarMarginX = 10;
    constexpr int kProgressBarHeight = 20;
    constexpr int kProgressBarBottomPadding = 5;
    constexpr int kProgressBarHorizontalPadding = 20;
    constexpr int kProgressTextOffsetY = 25;
    constexpr int kInfoStartXOffset = 30;
    constexpr int kInfoScrapToWasteOffset = 140;
    constexpr int kInfoWasteToEmptyOffset = 150;
    constexpr int kPatternLabelPaddingX = 5;
    constexpr int kPatternLabelMaxOffsetX = 130;
    constexpr int kPatternLabelOffsetY = 22;
    constexpr int kPreviewCellSize = 2;
    constexpr int kPreviewOffsetFromBottom = 90;

    ColorF valid_fill = ColorF(kValidFill); // light gray
    double line_thickness = std::max(kMinOutlineThickness, cell_ratio * kOutlineThicknessRatio);

    auto isValid = [&](int px, int py) {
        if (px < 0 || px >= mask_width || py < 0 || py >= mask_height) {
            return false;
        }
        return pattern_mask[px][py];
    };

    auto draw_pattern_mask = [&]() {
        for (int sy = 0; sy < layout_size; ++sy) {
            for (int sx = 0; sx < layout_size; ++sx) {
                int bx = sx + margin;
                int by = sy + margin;
                RectF cell_rect(layout_rect_.x + sx * cell_ratio,
                                layout_rect_.y + sy * cell_ratio,
                                cell_ratio, cell_ratio);
                if (!isValid(bx, by)) {
                    continue;
                }

                cell_rect.draw(valid_fill);

                double x0 = cell_rect.x;
                double y0 = cell_rect.y;
                double x1 = cell_rect.x + cell_rect.w;
                double y1 = cell_rect.y + cell_rect.h;

                if (!isValid(bx - 1, by)) {
                    Line(Vec2(x0, y0), Vec2(x0, y1)).draw(line_thickness, Palette::Black);
                }
                if (!isValid(bx + 1, by)) {
                    Line(Vec2(x1, y0), Vec2(x1, y1)).draw(line_thickness, Palette::Black);
                }
                if (!isValid(bx, by - 1)) {
                    Line(Vec2(x0, y0), Vec2(x1, y0)).draw(line_thickness, Palette::Black);
                }
                if (!isValid(bx, by + 1)) {
                    Line(Vec2(x0, y1), Vec2(x1, y1)).draw(line_thickness, Palette::Black);
                }
            }
        }
    };

    auto draw_placed_scraps = [&]() {
        for (int i = 0; i < placed_polygons.size(); ++i) {
            if (is_dragging_ && selected_type_ == UIManager::SelectType::Placed &&
                dragging_unplaced_index_ == i)
                continue;

            placed_polygons[i].Draw(canvas_offset, canvas_scale);
        }
    };

    auto draw_selection_and_rotation = [&]() {
        for (int i = 0; i < placed_polygons.size(); ++i) {
            if (is_dragging_ && selected_type_ == UIManager::SelectType::Placed &&
                selected_index_ == i) {
                temp_dragged_polygon_.DrawFrame(canvas_offset, canvas_scale,
                                               kPolygonFrameThickness, Palette::Red);
            } else if (selected_type_ == UIManager::SelectType::Placed &&
                       selected_index_ == i) {
                placed_polygons[i].DrawFrame(canvas_offset, canvas_scale,
                                             kPolygonFrameThickness, Palette::Red);
            } else {
                placed_polygons[i].DrawFrame(canvas_offset, canvas_scale,
                                             kPolygonFrameThickness, Palette::Black);
            }
        }
    };

    auto draw_progress_bar = [&]() {
        double progress = static_cast<double>(model.GetIteration()) / model.GetMaxIteration();
        progress_bar_.draw(Palette::Gray);
        progress_bar_inner_ =
            Rect(Config::GetCanvasUIX() + kProgressBarMarginX,
                 Config::GetCanvasUIY() + Config::GetCanvasUISize() -
                     kProgressBarBottomPadding - kProgressBarHeight,
                 (int)((Config::GetCanvasUISize() - kProgressBarHorizontalPadding) * progress),
                 kProgressBarHeight);
        progress_bar_inner_.draw(Palette::Green);
        FontRef(FontId::Bold)(U"{:.0f}%"_fmt(progress * 100))
            .draw(Config::GetCanvasUIX() + (Config::GetCanvasUISize() - kProgressBarHorizontalPadding) / 2,
                  Config::GetCanvasUIY() + Config::GetCanvasUISize() - kProgressTextOffsetY,
                  Palette::White);
    };

    auto draw_layout_info = [&]() {
        int polygons_num = static_cast<int>(placed_polygons.size());
        const int info_y = Config::GetCanvasUIY();
        int info_x = Config::GetCanvasUIX() + kInfoStartXOffset;
        FontRef(FontId::Bold)(U"No. scraps: {}"_fmt(polygons_num))
            .draw(info_x, info_y, Palette::Black);
        info_x += kInfoScrapToWasteOffset;
        FontRef(FontId::Bold)(U"Waste : {} cm²"_fmt(cur_waste))
            .draw(info_x, info_y, Palette::Blue);
        info_x += kInfoWasteToEmptyOffset;
        if (cur_empty_count == 0)
            FontRef(FontId::Bold)(U"Uncovered : {} cm²"_fmt(cur_empty_count))
                .draw(info_x, info_y, Palette::Black);
        else
            FontRef(FontId::Bold)(U"Uncovered : {} cm²"_fmt(cur_empty_count))
                .draw(info_x, info_y, Palette::Red);
    };

    auto draw_pattern_label = [&]() {
        String patternLabel = U"Pattern: {}"_fmt(GetPatternShapeLabel(model.GetCurrentPatternShape()));
        int patternTextX = std::max(progress_bar_.x + kPatternLabelPaddingX,
                                    progress_bar_.x + progress_bar_.w - kPatternLabelMaxOffsetX);
        int patternTextY = progress_bar_.y - kPatternLabelOffsetY;
        FontRef(FontId::Bold)(patternLabel).draw(patternTextX, patternTextY, Palette::Black);
    };

    auto draw_preview = [&]() {
        for(int i=0; i<layout_size; ++i) {
            for(int j=0; j<layout_size; ++j) {
                int bx = j + margin;
                int by = i + margin;
                Rect mini_rect(progress_bar_.x + kPreviewCellSize * j,
                               Config::GetCanvasUIY() + Config::GetCanvasUISize() -
                                   kPreviewOffsetFromBottom + kPreviewCellSize * i,
                               kPreviewCellSize, kPreviewCellSize);
                bool within = (bx >= 0 && bx < mask_width && by >= 0 && by < mask_height);
                if (!within || !pattern_mask[bx][by]) {
                    mini_rect.draw(Palette::Darkgray);
                } else if (canvas_placement_count[bx][by] == 0) {
                    mini_rect.draw(Palette::Red);
                } else {
                    mini_rect.draw(Palette::Gray);
                }
            }
        }
    };

    draw_pattern_mask();
    draw_placed_scraps();
    draw_selection_and_rotation();
    draw_progress_bar();
    draw_layout_info();
    draw_pattern_label();
    draw_preview();
}

void UIManager::UpdateCanvasSelectionAndRotation() {
    auto &model = model_;
    auto &placed_polygons = model.GetPlacedPolygons();
    auto &cur_loss = model.GetCurLoss();
    auto &cur_empty_count = model.GetCurEmptyCount();
    int cur_waste = model.GetCurWaste();
    int mouse_wheel = GetMouseWheel();
    const Vec2 canvas_offset(Config::GetCanvasUIX(), Config::GetCanvasUIY());
    const double canvas_scale = Config::GetCanvasUIRatio();

    for (int i = 0; i < placed_polygons.size(); ++i) {
        bool is_dragging_selected = is_dragging_ && selected_type_ == UIManager::SelectType::Placed &&
            selected_index_ == i;
        if (!is_dragging_selected &&
            selected_type_ == UIManager::SelectType::Placed && selected_index_ == i) {
            if (mouse_wheel != 0 &&
                !model.IsLocked(placed_polygons[selected_index_].GetIndex())) {
                for (const auto &p : placed_polygons[selected_index_].GetRasterizedPoints())
                    CanvasCountUpdate(p.x, p.y, -1, cur_loss, cur_empty_count, cur_waste);
                placed_polygons[i].Rotate(mouse_wheel * placed_wheel_rotate_rad_);
                for (const auto &p : placed_polygons[selected_index_].GetRasterizedPoints())
                    CanvasCountUpdate(p.x, p.y, 1, cur_loss, cur_empty_count, cur_waste);
            }
        }

        if (!InOperation() &&
            placed_polygons[i].CalcDisplayPolygon(canvas_offset, canvas_scale).leftClicked()) {
            if (!is_dragging_) {
                is_dragging_ = true;
                dragging_unplaced_index_ = i;
                temp_dragged_polygon_ = placed_polygons[i];

                selected_type_ = UIManager::SelectType::Placed;
                selected_index_ = i;
            }
        }
    }

    model.SetCurWaste(cur_waste);
}

void UIManager::UpdateDrag() {
    auto &model = model_;
    auto &placed_polygons = model.GetPlacedPolygons();
    auto &unplaced_polygons = model.GetUnplacedPolygons();
    auto &cur_loss = model.GetCurLoss();
    auto &cur_empty_count = model.GetCurEmptyCount();
    int cur_waste = model.GetCurWaste();
    const Vec2 canvas_offset(Config::GetCanvasUIX(), Config::GetCanvasUIY());
    const double canvas_scale = Config::GetCanvasUIRatio();

    Point mouse_pos = GetMousePos();

    if (is_dragging_) {
        temp_dragged_polygon_.SetCenterMousePos(mouse_pos, canvas_offset, canvas_scale);

        if (MouseL.up()) {
            if (selected_type_ == UIManager::SelectType::Unplaced) {
                if (canvas_outer_rect_.contains(mouse_pos)) { // Place a new scrap on the canvas.
                    placed_polygons.emplace_back(temp_dragged_polygon_);
                    placed_polygons.back().SetCenterMousePos(mouse_pos, canvas_offset, canvas_scale);
                    for (const auto &p : placed_polygons.back().GetRasterizedPoints())
                        CanvasCountUpdate(p.x, p.y, 1, cur_loss, cur_empty_count, cur_waste);

                    unplaced_polygons.erase(unplaced_polygons.begin() + dragging_unplaced_index_);

                    selected_type_ = UIManager::SelectType::Placed;
                    selected_index_ = static_cast<int>(placed_polygons.size()) - 1;
                }
            } else if (selected_type_ == UIManager::SelectType::Placed) {
                if (!canvas_outer_rect_.contains(mouse_pos)) { // Remove a scrap from the canvas.
                    unplaced_polygons.emplace_back(temp_dragged_polygon_);
                    model.SetLocked(temp_dragged_polygon_.GetIndex(), false);

                    for (const auto &p :
                         placed_polygons[dragging_unplaced_index_].GetRasterizedPoints())
                        CanvasCountUpdate(p.x, p.y, -1, cur_loss, cur_empty_count, cur_waste);
                    placed_polygons.erase(placed_polygons.begin() + dragging_unplaced_index_);

                    selected_type_ = UIManager::SelectType::Unplaced;
                    selected_index_ = static_cast<int>(unplaced_polygons.size()) - 1;
                } else { // Drag a scrap within the canvas.
                    for (const auto &p : placed_polygons[selected_index_].GetRasterizedPoints())
                        CanvasCountUpdate(p.x, p.y, -1, cur_loss, cur_empty_count, cur_waste);
                    placed_polygons[selected_index_].SetCenterMousePos(mouse_pos, canvas_offset, canvas_scale);
                    for (const auto &p : placed_polygons[selected_index_].GetRasterizedPoints())
                        CanvasCountUpdate(p.x, p.y, 1, cur_loss, cur_empty_count, cur_waste);
                }
            }

            is_dragging_ = false;
            dragging_unplaced_index_ = -1;
        }
        model.SetCurWaste(cur_waste);
        return;
    }

    if (MouseL.down()) {
        if (!ButtonRect(ButtonId::Delete).leftClicked()) {
            const Vec2 canvas_offset(Config::GetCanvasUIX(), Config::GetCanvasUIY());
            const double canvas_scale = Config::GetCanvasUIRatio();
            if (selected_type_ == UIManager::SelectType::Unplaced &&
                selected_index_ >= 0 &&
                !unplaced_polygons[selected_index_]
                    .CalcDisplayPolygon(canvas_offset, canvas_scale)
                    .leftClicked() &&
                !selection_priority_box_.leftClicked() &&
                !ButtonRect(ButtonId::CutScraps).leftClicked()) {
                selected_type_ = UIManager::SelectType::None;
                selected_index_ = -1;
            } else if (selected_type_ == UIManager::SelectType::Placed &&
                       selected_index_ >= 0 &&
                       !placed_polygons[selected_index_]
                            .CalcDisplayPolygon(canvas_offset, canvas_scale)
                            .leftClicked()) {
                selected_type_ = UIManager::SelectType::None;
                selected_index_ = -1;
            }
        }
    }

    model.SetCurWaste(cur_waste);
}

void UIManager::CanvasInitialize() {
    is_initializing_ = true;

    model_.CurrentCanvasState().mode = UIManager::Mode::InitialSolution;
}

void UIManager::CanvasOptimize() {
    auto &model = model_;
    auto &state = model.CurrentCanvasState();
    if (state.mode != UIManager::Mode::InitialSolution) {
        CanvasInitialize();
        return;
    }

    model.CaptureCancelSnapshot();

    is_optimizing_ = true;
    state.mode = UIManager::Mode::Optimization;

    InitializeOptimization();
}

void UIManager::FinalizeOptimization(bool applyBest) {
    auto &model = model_;
    auto &state = model.CurrentCanvasState();
    is_optimizing_ = false;
    state.mode = UIManager::Mode::Waiting;
    model.ResetIteration();

    auto &opt_state = OptInfo::Current();
    auto &placed_polygons = model.GetPlacedPolygons();
    auto &canvas_placement_count = model.GetCanvasPlacementCount();
    auto &cur_loss = model.GetCurLoss();
    int cur_waste = model.GetCurWaste();
    auto &cur_empty_count = model.GetCurEmptyCount();

    model.ClearCancelSnapshot();

    if (!applyBest || !opt_state.has_value) {
        return;
    }

    cur_loss = opt_state.min_loss;
    cur_waste = opt_state.min_waste;
    cur_empty_count = 0;
    placed_polygons = opt_state.min_placed_polygons;
    model.GetUnplacedPolygons() = opt_state.min_unplaced_polygons;
    canvas_placement_count = opt_state.min_canvas_placement_count;
    model.SetCurWaste(cur_waste);
}

void UIManager::CanvasReset() {
    auto result = System::MessageBoxOKCancel(U"Reset Canvas?");

    if (result == MessageBoxResult::Cancel) {
        return;
    }

    auto &model = model_;
    auto &state = model.CurrentCanvasState();
    auto &placed_polygons = model.GetPlacedPolygons();
    auto &canvas_placement_count = model.GetCanvasPlacementCount();
    auto &cur_loss = model.GetCurLoss();
    int cur_waste = model.GetCurWaste();
    auto &cur_empty_count = model.GetCurEmptyCount();
    model.ClearLockedFlags();

    while (!placed_polygons.empty()) {
        model.GetUnplacedPolygons().emplace_back(placed_polygons.back());
        placed_polygons.pop_back();
    }
    state.mode = UIManager::Mode::Waiting;

    int canvas_size = Config::GetCanvasSize();
    canvas_placement_count = Grid<int>(canvas_size, canvas_size, 0);
    state.valid_cell_count = pattern_mask::CountPatternMaskCells(state.pattern_mask);
    model.RefreshLayoutMetrics(state);
    cur_loss = state.cur_loss;
    cur_waste = state.cur_waste;
    cur_empty_count = state.cur_empty_count;
    model.SetCurWaste(cur_waste);

    auto &opt_state = OptInfo::Current();
    opt_state.has_value = false;
    opt_state.min_loss = state.cur_loss;
    opt_state.min_waste = state.cur_waste;
    opt_state.min_iter = 0;
    opt_state.min_placed_polygons.clear();
    opt_state.min_unplaced_polygons.clear();
    opt_state.min_canvas_placement_count = canvas_placement_count;
}
