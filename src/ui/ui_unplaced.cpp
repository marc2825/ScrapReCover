#include "ui/ui_manager.hpp"

#include <algorithm>

#include "algorithm/eval_functions.hpp"
#include "core/canvas_model.hpp"
#include "ui/ui_constants.hpp"
#include "utils/layout_utils.hpp"


using namespace s3d;

void UIManager::DrawUnplaced() {
    auto &model = model_;
    auto &unplaced_polygons = model.GetUnplacedPolygons();
    const int unplaced_ui_x = Config::GetUnplacedUIX();
    const int unplaced_ui_y = Config::GetUnplacedUIY();
    const int unplaced_ui_width = Config::GetUnplacedUIWidth();
    const int unplaced_ui_height = Config::GetUnplacedUIHeight();
    const int button_width = Config::GetButtonWidth();
    const int button_height = Config::GetButtonHeight();
    const int button_margin = Config::GetButtonMargin();
    const Vec2 canvas_offset(Config::GetCanvasUIX(), Config::GetCanvasUIY());
    const double canvas_scale = Config::GetCanvasUIRatio();

    const int icon_size = Config::GetUnplacedIconSize();
    const int margin = Config::GetUnplacedMargin();
    const int max_col = Config::GetUnplacedUIWidth() / (icon_size + margin);

    constexpr int weight_display_height = ui::UnplacedWeightLabelHeight;

    const int row_count = ceil_div(static_cast<int>(unplaced_polygons.size()), max_col);
    const int content_height =
        row_count * (icon_size + margin + weight_display_height); // Extra height for weight display
    const int max_scroll_offset =
        std::max(0, content_height - (unplaced_ui_height - 2 * margin));
    const int scroll_offset =
        std::clamp(cur_unplaced_scroll_offset_, 0, max_scroll_offset);

    {
        const Rect unplaced_area(unplaced_ui_x, unplaced_ui_y, unplaced_ui_width, unplaced_ui_height);
        unplaced_area.draw(Palette::Whitesmoke);
        unplaced_area.drawFrame(2, Palette::Black);
    }

    for (int i = 0; i < static_cast<int>(unplaced_polygons.size()); ++i) {
        const int col = i % max_col;
        const int row = i / max_col;

        const Rect item_rect(
            unplaced_ui_x + margin + col * (icon_size + margin),
            unplaced_ui_y + margin + row * (icon_size + margin + weight_display_height) -
                scroll_offset,
            icon_size, icon_size + weight_display_height);

        const Rect icon_rect(item_rect.x, item_rect.y, icon_size, icon_size);
        const Rect weight_rect(item_rect.x, item_rect.y + icon_size, icon_size,
                               weight_display_height);

        const bool is_selected = (selected_type_ == UIManager::SelectType::Unplaced &&
                                  selected_index_ == i);

        if (is_selected) {
            item_rect.draw(ColorF(0.9, 1.0, 0.9));
            item_rect.drawFrame(1, Palette::Red);
        } else {
            item_rect.draw(Palette::White);
            item_rect.drawFrame(1, Palette::Black);
        }

        if (is_selected) {
            icon_rect.draw(Palette::Lightgreen);
        } else {
            icon_rect.draw(Palette::White);
        }

        unplaced_polygons[i].SetCenterMousePos(icon_rect.center(), canvas_offset, canvas_scale);
        unplaced_polygons[i].Draw(canvas_offset, canvas_scale);

        const String weightText = U"Sel. Wt.: {:.3f}"_fmt(unplaced_polygons[i].GetSelectionWeight());

        if (is_selected) {
            weight_rect.draw(ColorF(0.9, 1.0, 0.9));
            FontRef(FontId::Bold)(weightText).drawAt(weight_rect.center(), Palette::Red);
        } else {
            weight_rect.draw(ColorF(0.95, 0.95, 0.95));
            FontRef(FontId::Bold)(weightText).drawAt(weight_rect.center(), Palette::Black);
        }
    }

    int polygons_num = static_cast<int>(unplaced_polygons.size());
    FontRef(FontId::Bold)(U"No. Scraps: {}"_fmt(polygons_num))
        .draw(Config::GetUnplacedUIX() + 10, Config::GetUnplacedUIY(), Palette::Black);

    const int load_mode_button_width = std::min(button_width * 2, unplaced_ui_width - button_margin * 2);
    const int load_mode_button_height = std::max(24, button_height / 2);
    const int button_x = unplaced_ui_x + unplaced_ui_width - load_mode_button_width - button_margin;
    const int button_y = unplaced_ui_y + unplaced_ui_height - load_mode_button_height - button_margin;
    ButtonRect(ButtonId::ScrapPreset) =
        Rect(button_x, button_y, load_mode_button_width, load_mode_button_height);

    String scrapModeButtonText = U"Load Mode: {}"_fmt(GetScrapModeLabel());
    Button(ButtonRect(ButtonId::ScrapPreset), FontRef(FontId::Base), scrapModeButtonText, Palette::Black,
           Palette::Lightgray, Palette::Aliceblue, Palette::Gray, !InOperation());
    ButtonRect(ButtonId::ScrapPreset).drawFrame(2, Palette::Black);

    constexpr int dropdown_mark_size = 6;
    constexpr int dropdown_mark_margin = 8;
    const Rect dropdown_button = ButtonRect(ButtonId::ScrapPreset);
    const double mark_center_x = dropdown_button.x + dropdown_button.w - dropdown_mark_margin - dropdown_mark_size;
    const double mark_center_y = dropdown_button.y + dropdown_button.h * 0.5;
    const Triangle dropdown_mark(
        Vec2(mark_center_x - dropdown_mark_size, mark_center_y - dropdown_mark_size * 0.6),
        Vec2(mark_center_x + dropdown_mark_size, mark_center_y - dropdown_mark_size * 0.6),
        Vec2(mark_center_x, mark_center_y + dropdown_mark_size * 0.6));
    dropdown_mark.draw(Palette::Black);
}

void UIManager::DrawDrag() {
    if (is_dragging_) {
        const Vec2 canvas_offset(Config::GetCanvasUIX(), Config::GetCanvasUIY());
        const double canvas_scale = Config::GetCanvasUIRatio();
        temp_dragged_polygon_.Draw(canvas_offset, canvas_scale);
    }
}

void UIManager::UpdateUnplacedInteraction() {
    auto &model = model_;
    auto &unplaced_polygons = model.GetUnplacedPolygons();
    const int unplaced_ui_x = Config::GetUnplacedUIX();
    const int unplaced_ui_y = Config::GetUnplacedUIY();
    const int unplaced_ui_width = Config::GetUnplacedUIWidth();
    const int unplaced_ui_height = Config::GetUnplacedUIHeight();

    const int icon_size = Config::GetUnplacedIconSize();
    const int margin = Config::GetUnplacedMargin();
    const int max_col = Config::GetUnplacedUIWidth() / (icon_size + margin);

    constexpr int weight_display_height = ui::UnplacedWeightLabelHeight;

    const int row_count = ceil_div(static_cast<int>(unplaced_polygons.size()), max_col);
    const int content_height = row_count * (icon_size + margin + weight_display_height);
    const int max_scroll_offset =
        std::max(0, content_height - (unplaced_ui_height - 2 * margin));
    cur_unplaced_scroll_offset_ =
        std::clamp(cur_unplaced_scroll_offset_, 0, max_scroll_offset);

    const Rect scroll_area(unplaced_ui_x + margin, unplaced_ui_y + margin,
                           unplaced_ui_width - 2 * margin, unplaced_ui_height - 2 * margin);

    if (scroll_area.mouseOver()) {
        cur_unplaced_scroll_offset_ =
            std::max(0, cur_unplaced_scroll_offset_ +
                            static_cast<int>(Mouse::Wheel() * unplaced_scroll_));
        cur_unplaced_scroll_offset_ =
            std::min(cur_unplaced_scroll_offset_, max_scroll_offset);
    }

    bool is_unplaced_selected = unplaced_rect_.leftClicked();
    if (is_unplaced_selected && !InOperation()) {
        for (int i = 0; i < static_cast<int>(unplaced_polygons.size()); ++i) {
            const int col = i % max_col;
            const int row = i / max_col;

            const Rect item_rect(
                unplaced_ui_x + margin + col * (icon_size + margin),
                unplaced_ui_y + margin + row * (icon_size + margin + weight_display_height) -
                    cur_unplaced_scroll_offset_,
                icon_size, icon_size + weight_display_height);

            if (item_rect.leftClicked()) {
                if (!is_dragging_) {
                    is_dragging_ = true;
                    dragging_unplaced_index_ = i;
                    temp_dragged_polygon_ = unplaced_polygons[i];

                    selected_type_ = UIManager::SelectType::Unplaced;
                    selected_index_ = i;
                }
            }
        }
    }

    if (!InOperation() && ButtonRect(ButtonId::ScrapPreset).leftClicked()) {
        scrap_load_mode_ = NextScrapLoadMode(scrap_load_mode_);
        selected_type_ = SelectType::None;
        selected_index_ = -1;
        is_pattern_dialog_open_ = false;
        pattern_dialog_just_opened_ = false;
    }
}
