#include "ui/ui_manager.hpp"

#include <algorithm>

#include "algorithm/hyperparameter.hpp"
#include "core/canvas_model.hpp"
#include "ui/ui_constants.hpp"


using namespace s3d;

void UIManager::DrawFrames() {
    constexpr int kPanelFrameThickness = 2;
    constexpr int kSelectorFrameThickness = 2;
    unplaced_rect_.draw(Palette::Lightgray);
    unplaced_rect_.drawFrame(kPanelFrameThickness, Palette::Black);
    DrawUnplaced();
    
    constexpr int kHiderTopHeight = 10;
    constexpr int kHiderBottomOverlap = 10;
    constexpr int kHiderBottomExtraHeight = 40;
    Rect hider_bottom = Rect(0, Config::GetControlUIY() - kHiderBottomOverlap,
                             Config::GetWindowWidth(),
                             Config::GetControlUIHeight() + kHiderBottomExtraHeight);
    hider_bottom.draw(ui::WindowColor);
    Rect hider_top = Rect(0, 0, Config::GetWindowWidth(), kHiderTopHeight);
    hider_top.draw(ui::WindowColor);

    control_rect_.draw(Palette::Darkgray);
    control_rect_.drawFrame(kPanelFrameThickness, Palette::Black);

    canvas_outer_rect_.draw(Palette::White);
    layout_rect_.draw(Palette::White);
    DrawCanvas();

    int canvas_ui_x = Config::GetCanvasUIX();
    int canvas_ui_y = Config::GetCanvasUIY();
    int canvas_ui_size = Config::GetCanvasUISize();
    int selector_button_width = ui::LayoutSelectorButtonWidth;
    int selector_button_height = ui::LayoutSelectorButtonHeight;
    int selector_margin = ui::LayoutSelectorMargin;
    constexpr int kSelectorOffsetX = 10;
    constexpr int kSelectorOffsetY = 20;
    constexpr int kSelectorLabelPaddingX = 5;
    constexpr int kSelectorLabelOffsetY = 25;
    int selector_start_x = canvas_ui_x + canvas_ui_size + kSelectorOffsetX;
    int selector_start_y = canvas_ui_y + kSelectorOffsetY;

    const int canvas_count = static_cast<int>(model_.GetCanvasStateCount());
    if (canvas_selector_buttons_.size() != static_cast<size_t>(canvas_count)) {
        canvas_selector_buttons_.clear();
        for (int i = 0; i < canvas_count; ++i) {
            canvas_selector_buttons_.emplace_back(
                Rect(selector_start_x,
                     selector_start_y + i * (selector_button_height + selector_margin),
                     selector_button_width, selector_button_height));
        }
    }

    if (!canvas_selector_buttons_.empty()) {
        FontRef(FontId::Bold)(U"Pattern")
            .draw(canvas_selector_buttons_[0].x + kSelectorLabelPaddingX,
                  canvas_selector_buttons_[0].y - kSelectorLabelOffsetY,
                  Palette::Black);
    }

    for (size_t i = 0; i < canvas_selector_buttons_.size(); ++i) {
        const auto &rect = canvas_selector_buttons_[i];

        if (static_cast<int>(i) == model_.GetCurrentCanvasIndex()) {
            rect.draw(Palette::Orange);
        } else {
            rect.draw(Palette::White);
        }

        rect.drawFrame(kSelectorFrameThickness, Palette::Black);

        FontRef(FontId::Base)(static_cast<int>(i) + 1).drawAt(rect.center(), Palette::Black);

        if (!InOperation() && rect.leftClicked()) {
            SwitchCanvas(static_cast<int>(i));
        }
    }

    if (!canvas_selector_buttons_.empty()) {
        const auto &last = canvas_selector_buttons_.back();
        int spacing = ui::ButtonSpacing;
        int btn_height = std::max(ui::PlacedListItemHeight, last.h / 2);
        ButtonRect(ButtonId::AddCanvas) = Rect(last.x, last.y + last.h + spacing, last.w, btn_height);
        ButtonRect(ButtonId::RemoveCanvas) =
            Rect(last.x, ButtonRect(ButtonId::AddCanvas).y + btn_height + spacing, last.w, btn_height);

        if (Button(ButtonRect(ButtonId::AddCanvas), FontRef(FontId::Base), U"Add",
                   Palette::Black, Palette::Gray, Palette::Aliceblue, Palette::Gray,
                   !InOperation())) {
            AddCanvas();
        }

        bool canRemove = (canvas_count > 1) && !InOperation();
        if (Button(ButtonRect(ButtonId::RemoveCanvas), FontRef(FontId::Base), U"Erase",
                   Palette::Black, Palette::Gray, Palette::Aliceblue, Palette::Gray, canRemove)) {
            RemoveCurrentCanvas();
        }

        const int shape_y = progress_bar_.y + (progress_bar_.h - btn_height) / 2;
        Rect shape_rect(last.x,
                        shape_y,
                        last.w, btn_height);
        ColorF shapeColor = is_pattern_dialog_open_ ? Palette::Orange : Palette::Aliceblue;
        if (Button(shape_rect, FontRef(FontId::Base), U"Shape",
                   Palette::Black, Palette::Gray, shapeColor, Palette::Gray, !InOperation())) {
            if (!InOperation()) {
                bool open = !is_pattern_dialog_open_;
                is_pattern_dialog_open_ = open;
                pattern_dialog_just_opened_ = open;
            }
        }
        ButtonRect(ButtonId::PatternShape) = shape_rect;
    }

    if (!InOperation())
        DrawDrag();
}

void UIManager::DrawCanvasPlacedList() {
    placed_list_.Draw(model_, FontRef(FontId::Base), FontRef(FontId::Bold),
                      selected_type_, selected_index_);
}

void UIManager::UpdateCanvasPlacedList() {
    placed_list_.Update(model_, selected_type_, selected_index_,
                        is_dragging_, dragging_unplaced_index_, InOperation());
}

void UIManager::DrawIterationSlider(int control_ui_x, int control_ui_y, int button_margin, int header_height) {
    const int left_slider_x = control_ui_x + button_margin;
    constexpr int kSliderWidth = 250;
    constexpr int kSliderLabelWidth = 220;
    constexpr int kSliderPadding = 55;
    constexpr int kSliderBoxHeight = 35;
    constexpr int kSliderFrameThickness = 1;
    constexpr int kMinIterations = 2000;
    constexpr int kMaxIterations = 75000;
    const int slider_width = kSliderWidth;
    const int slider_value_width = slider_width - kSliderPadding;
    const int slider_label_width = kSliderLabelWidth + kSliderPadding;
    double max_iteration = model_.GetMaxIteration();

    if (SimpleGUI::Slider(U"Iterations: {:d}"_fmt((int)max_iteration),
        max_iteration, kMinIterations, kMaxIterations,
        Vec2(left_slider_x, control_ui_y + button_margin + header_height),
        slider_value_width, slider_label_width)) {
        if (!is_optimizing_) {
            model_.SetMaxIteration(static_cast<int>(max_iteration));
        }
    }
    iteration_box_ = Rect(left_slider_x, control_ui_y + button_margin + header_height,
                         slider_width + kSliderLabelWidth, kSliderBoxHeight);
    iteration_box_.drawFrame(kSliderFrameThickness, Palette::Black);
}

void UIManager::DrawChangeRateSlider(int control_ui_x, int control_ui_y, int button_margin, int header_height) {
    const int left_slider_x = control_ui_x + button_margin;
    constexpr int kSliderWidth = 250;
    constexpr int kSliderLabelWidth = 220;
    constexpr int kSliderPadding = 55;
    constexpr int kSliderBoxHeight = 35;
    constexpr int kSliderFrameThickness = 1;
    constexpr int kSliderRowSpacing = 50;
    constexpr int kChangeRateMax = 20000;
    const int slider_width = kSliderWidth;
    const int slider_value_width = slider_width - kSliderPadding;
    const int slider_label_width = kSliderLabelWidth + kSliderPadding;
    double start_temp = Hyperparameter::sttmp;

    if (SimpleGUI::Slider(
            U"Change Rate: {:d}"_fmt((int)start_temp), start_temp,
            Hyperparameter::entmp, kChangeRateMax,
            Vec2(left_slider_x, control_ui_y + button_margin + header_height + kSliderRowSpacing),
            slider_value_width, slider_label_width)) {
        if (!is_optimizing_) {
            Hyperparameter::sttmp = (int)start_temp;
        }
    }
    change_rate_box_ = Rect(left_slider_x,
                           control_ui_y + button_margin + header_height + kSliderRowSpacing,
                           slider_width + kSliderLabelWidth, kSliderBoxHeight);
    change_rate_box_.drawFrame(kSliderFrameThickness, Palette::Black);
}

void UIManager::DrawSelectionPrioritySlider(int unplaced_ui_x, int unplaced_ui_width, int control_ui_y, int button_margin, int header_height) {
    constexpr int kSliderWidth = 250;
    constexpr int kSliderLabelWidth = 120;
    constexpr int kSliderPadding = 20;
    constexpr int kSliderBoxHeight = 35;
    constexpr int kSliderFrameThickness = 1;
    constexpr int kRightPanelOffset = 225;
    constexpr double kSliderMin = 0.0;
    constexpr double kSliderMax = 50.0;
    const int slider_width = kSliderWidth;
    const int slider_value_width = slider_width - kSliderPadding;
    const int slider_label_width = kSliderLabelWidth + kSliderPadding;
    const int right_panel_x = unplaced_ui_x + unplaced_ui_width - (slider_width + kRightPanelOffset);
    double current_priority = 1.0;
    auto &unplaced_polygons = model_.GetUnplacedPolygons();
    if (selected_type_ == UIManager::SelectType::Unplaced && selected_index_ >= 0 &&
        selected_index_ < static_cast<int>(unplaced_polygons.size())) {
        current_priority = unplaced_polygons[selected_index_].GetSelectionPriority();
    }

    String display_label;
    if (selected_type_ == UIManager::SelectType::Unplaced && selected_index_ >= 0) {
        display_label = U"Scrap #{:d} Priority: {:.1f}"_fmt(
            unplaced_polygons[selected_index_].GetIndex(), current_priority);
    } else {
        display_label = U"Selection Priority: {:.1f}"_fmt(current_priority);
    }

    if (SimpleGUI::Slider(
            display_label, current_priority, kSliderMin, kSliderMax,
            Vec2(right_panel_x, control_ui_y + button_margin + header_height),
            slider_value_width, slider_label_width)) {
        if (!is_optimizing_) {
            if (selected_type_ == UIManager::SelectType::Unplaced && selected_index_ >= 0 &&
                selected_index_ < static_cast<int>(unplaced_polygons.size())) {
                unplaced_polygons[selected_index_].SetSelectionPriority(current_priority);
            }
        }
    }
    selection_priority_box_ = Rect(right_panel_x,
                                  control_ui_y + button_margin + header_height,
                                  slider_width + kSliderLabelWidth, kSliderBoxHeight);
    selection_priority_box_.drawFrame(kSliderFrameThickness, Palette::Black);
}

void UIManager::DrawRegularShapeSlider(int unplaced_ui_x, int unplaced_ui_width, int control_ui_y, int button_margin, int header_height) {
    constexpr int kSliderWidth = 250;
    constexpr int kSliderLabelWidth = 120;
    constexpr int kSliderPadding = 20;
    constexpr int kSliderBoxHeight = 35;
    constexpr int kSliderFrameThickness = 1;
    constexpr int kRightPanelOffset = 225;
    constexpr int kSliderRowSpacing = 50;
    constexpr int kResetButtonOffsetX = 255;
    constexpr int kResetButtonWidthPadding = 5;
    constexpr int kResetButtonHeightPadding = 15;
    constexpr double kRegShapeMin = -10.0;
    constexpr double kRegShapeMax = 50.0;
    const int slider_width = kSliderWidth;
    const int slider_value_width = slider_width - kSliderPadding;
    const int slider_label_width = kSliderLabelWidth + kSliderPadding;
    const int right_panel_x = unplaced_ui_x + unplaced_ui_width - (slider_width + kRightPanelOffset);
    const int button_width = Config::GetButtonWidth();
    const int button_height = Config::GetButtonHeight();
    double reg_shape_pref = MyPolygon::GetRegularShapePreference();
    auto &placed_polygons = model_.GetPlacedPolygons();
    auto &unplaced_polygons = model_.GetUnplacedPolygons();

    if (SimpleGUI::Slider(
            U"Reg Shape Pref: {:.1f}"_fmt(reg_shape_pref),
            reg_shape_pref, kRegShapeMin, kRegShapeMax,
            Vec2(right_panel_x, control_ui_y + button_margin + header_height + kSliderRowSpacing),
            slider_value_width, slider_label_width)) {
        if (!is_optimizing_) {
            MyPolygon::SetRegularShapePreference(reg_shape_pref);
            for (auto& polygon : unplaced_polygons) {
                polygon.CalcSelectionWeight();
            }
            for (auto& polygon : placed_polygons) {
                polygon.CalcSelectionWeight();
            }
        }
    }
    reg_shape_priority_box_ = Rect(right_panel_x,
                                  control_ui_y + button_margin + header_height + kSliderRowSpacing,
                                  slider_width + kSliderLabelWidth, kSliderBoxHeight);
    reg_shape_priority_box_.drawFrame(kSliderFrameThickness, Palette::Black);

    ButtonRect(ButtonId::ResetWeights) = Rect(
        right_panel_x + kSliderLabelWidth + kResetButtonOffsetX,
        control_ui_y + button_margin + header_height,
        button_width * 2 / 3 - kResetButtonWidthPadding,
        button_height * 2 - kResetButtonHeightPadding
    );

    if (Button(ButtonRect(ButtonId::ResetWeights), FontRef(FontId::Base), U" Reset\nWeights", Palette::Black,
            Palette::Lightgray, Palette::Aliceblue, Palette::Gray, !InOperation())) {
        if (!is_optimizing_) {
            MyPolygon::SetRegularShapePreference(0);
            for (auto& polygon : unplaced_polygons) {
                polygon.SetSelectionPriority(1.0);
            }
            for (auto& polygon : placed_polygons) {
                polygon.SetSelectionPriority(1.0);
            }
        }
    }
    ButtonRect(ButtonId::ResetWeights).drawFrame(kSliderFrameThickness, Palette::Black);
}

void UIManager::DrawSliders() {
    const int control_ui_x = Config::GetControlUIX();
    const int control_ui_y = Config::GetControlUIY();
    const int button_margin = Config::GetButtonMargin();
    const int unplaced_ui_x = Config::GetUnplacedUIX();
    const int unplaced_ui_width = Config::GetUnplacedUIWidth();
    constexpr int kHeaderHeight = 25;
    constexpr int kHeaderTextOffsetY = 5;
    constexpr int kSliderWidth = 250;
    constexpr int kRightPanelOffset = 225;
    const int header_height = kHeaderHeight;
    const int right_panel_x = unplaced_ui_x + unplaced_ui_width - (kSliderWidth + kRightPanelOffset);

    const int left_slider_x = control_ui_x + button_margin;

    FontRef(FontId::Bold)(U"Layout Control")
        .draw(left_slider_x, control_ui_y + kHeaderTextOffsetY, Palette::Black);

    FontRef(FontId::Bold)(U"Scrap Control")
        .draw(right_panel_x, control_ui_y + kHeaderTextOffsetY, Palette::Black);

    DrawIterationSlider(control_ui_x, control_ui_y, button_margin, header_height);
    DrawChangeRateSlider(control_ui_x, control_ui_y, button_margin, header_height);
    DrawSelectionPrioritySlider(unplaced_ui_x, unplaced_ui_width, control_ui_y, button_margin, header_height);
    DrawRegularShapeSlider(unplaced_ui_x, unplaced_ui_width, control_ui_y, button_margin, header_height);
}
