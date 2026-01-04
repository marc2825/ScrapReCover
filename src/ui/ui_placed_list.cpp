#include "ui/ui_placed_list.hpp"

#include <algorithm>

#include "algorithm/optimizer.hpp"
#include "ui/ui_constants.hpp"
#include "utils/config.hpp"


using namespace s3d;

namespace {

struct PlacedListLayout {
    Rect list_area;
    Rect list_view_area;
    int item_height = 0;
    int max_visible_items = 0;
    int total_items = 0;
    int max_scroll_offset = 0;
    int start_item = 0;
    int end_item = 0;
    int visible_items = 0;
    int item_draw_width = 0;
};

constexpr int kPlacedListBottomPadding = 15;
constexpr int kPlacedListFrameThickness = 2;
constexpr int kPlacedListTitleOffsetY = 20;
constexpr int kPlacedListViewFrameThickness = 2;
constexpr int kPlacedListItemInsetX = 1;
constexpr int kPlacedListItemTextOffsetX = 25;
constexpr int kPlacedListItemTextOffsetY = -2;
constexpr int kPlacedListItemFrameThickness = 1;
constexpr int kPlacedListCheckboxOffsetX = 80;
constexpr int kPlacedListCheckboxOffsetY = 2;
constexpr int kPlacedListCheckboxFrameThickness = 1;
constexpr int kPlacedListCheckMarkStartX = 3;
constexpr int kPlacedListCheckMarkStartY = 10;
constexpr int kPlacedListCheckMarkMidX = 8;
constexpr int kPlacedListCheckMarkMidY = 15;
constexpr int kPlacedListCheckMarkEndX = 17;
constexpr int kPlacedListCheckMarkEndY = 5;
constexpr int kPlacedListCheckMarkThickness = 2;
constexpr int kPlacedListCheckboxLabelOffsetY = -3;
constexpr int kPlacedListDragPreviewFrameThickness = 2;
constexpr int kPlacedListDropIndicatorThickness = 2;

constexpr int kDragHandlePaddingX = 3;
constexpr int kDragHandlePaddingY = 2;
constexpr int kDragHandleLineInset = 3;
constexpr int kDragHandleFrameThickness = 1;
constexpr int kDragHandleLineThickness = 1;

// Bottom-center: Placed scraps list
PlacedListLayout BuildPlacedListLayout(int total_items, int scroll_offset) {
    PlacedListLayout layout;
    const int control_ui_y = Config::GetControlUIY();
    const int control_ui_height = Config::GetControlUIHeight();
    const int window_width = Config::GetWindowWidth();
    const int center_x = window_width / 2;
    const int list_y = control_ui_y;
    const int list_width = ui::PlacedListWidth;
    const int list_height = control_ui_height - kPlacedListBottomPadding;
    const int list_view_padding = ui::PlacedListPadding;

    layout.item_height = ui::PlacedListItemHeight;
    layout.max_visible_items = ui::PlacedListMaxVisibleItems;
    layout.total_items = total_items;
    layout.max_scroll_offset =
        std::max(0, (total_items - layout.max_visible_items) * layout.item_height);

    const int clamped_scroll = std::clamp(scroll_offset, 0, layout.max_scroll_offset);
    layout.start_item = clamped_scroll / layout.item_height;
    layout.end_item = std::min(layout.start_item + layout.max_visible_items, total_items);
    layout.visible_items = layout.end_item - layout.start_item;

    layout.list_area = Rect(center_x - list_width / 2 + list_view_padding,
                            list_y + list_view_padding,
                            list_width,
                            list_height + list_view_padding);
    layout.list_view_area = Rect(layout.list_area.x + list_view_padding,
                                 layout.list_area.y + ui::PlacedListTitleHeight,
                                 layout.list_area.w - 2 * list_view_padding,
                                 layout.list_area.h - (ui::PlacedListTitleHeight + list_view_padding));
    layout.item_draw_width = layout.list_view_area.w - 2 * list_view_padding;

    return layout;
}

Rect CalcPlacedListItemRect(const PlacedListLayout& layout, int item_index) {
    const int y_pos =
        layout.list_view_area.y + (item_index - layout.start_item) * layout.item_height;
    return Rect(layout.list_view_area.x + kPlacedListItemInsetX,
                y_pos, layout.item_draw_width, layout.item_height);
}

Rect CalcPlacedListCheckboxRect(const Rect& item_rect) {
    return Rect(item_rect.x + item_rect.w - kPlacedListCheckboxOffsetX,
                item_rect.y + kPlacedListCheckboxOffsetY,
                ui::CheckboxSize, ui::CheckboxSize);
}

Rect CalcDragHandleRect(const Rect& rect) {
    return Rect(rect.x + kDragHandlePaddingX, rect.y + kDragHandlePaddingY,
                ui::DragHandleSize, ui::DragHandleSize);
}

int CalcPlacedListDropSlot(const PlacedListLayout& layout, double pointer_local_y, int scroll_offset) {
    double pointer_absolute = pointer_local_y + scroll_offset;
    pointer_absolute = std::clamp(pointer_absolute, 0.0,
                                  static_cast<double>(layout.total_items * layout.item_height));
    return std::clamp(static_cast<int>(pointer_absolute / layout.item_height), 0, layout.total_items);
}

} // namespace

Rect PlacedListWidget::DrawDragHandle(const Rect& rect) const {
    Rect drag_handle_rect = CalcDragHandleRect(rect);
    drag_handle_rect.drawFrame(kDragHandleFrameThickness, Palette::Gray);
    const int line_start_x = drag_handle_rect.x + kDragHandleLineInset;
    const int line_end_x = drag_handle_rect.x + ui::DragHandleSize - kDragHandleLineInset;
    const int line_start_y = drag_handle_rect.y + ui::DragHandleLineSpacing;

    for (int j = 0; j < ui::DragHandleLineCount; ++j) {
        const int line_y = line_start_y + j * ui::DragHandleLineSpacing;
        Line(line_start_x, line_y, line_end_x, line_y)
            .draw(kDragHandleLineThickness, Palette::Gray);
    }

    return drag_handle_rect;
}

void PlacedListWidget::Draw(const CanvasModel& model,
                            const Font& font_base,
                            const Font& font_bold,
                            ui::SelectType selected_type,
                            int selected_index) {
    const int center_x = Config::GetWindowWidth() / 2;
    const int list_y = Config::GetControlUIY();

    const auto& placed_polygons = model.GetPlacedPolygonsConst();
    const int total_items = static_cast<int>(placed_polygons.size());

    PlacedListLayout layout = BuildPlacedListLayout(total_items, state_.scroll_offset);

    layout.list_area.draw(Palette::Lightgray);
    layout.list_area.drawFrame(kPlacedListFrameThickness, Palette::Black);

    font_bold(U"Placed Scraps List")
        .drawAt(center_x, list_y + kPlacedListTitleOffsetY, Palette::Black);

    layout.list_view_area.draw(Palette::White);
    layout.list_view_area.drawFrame(kPlacedListViewFrameThickness, Palette::Black);

    if (total_items == 0) {
        font_base(U"No scraps on layout")
            .drawAt(layout.list_view_area.center(), Palette::Gray);
        return;
    }

    for (int i = layout.start_item; i < layout.end_item; ++i) {
        Rect item_rect = CalcPlacedListItemRect(layout, i);

        const int polygon_index = placed_polygons[i].GetIndex();
        const bool is_locked_poly = model.IsLocked(polygon_index);
        const bool is_dragged_row =
            state_.dragging_item && (i == state_.drag_item_index);

        if (!is_dragged_row) {
            if (selected_type == ui::SelectType::Placed && selected_index == i) {
                item_rect.draw(ColorF(0.9, 1.0, 0.9));
                item_rect.drawFrame(kPlacedListItemFrameThickness, Palette::Red);
            } else {
                item_rect.draw(ColorF(0.98, 0.98, 0.98));
                item_rect.drawFrame(kPlacedListItemFrameThickness, Palette::Silver);
            }
        } else {
            item_rect.draw(ColorF(0.9, 0.95, 1.0, 0.4));
            item_rect.drawFrame(kPlacedListItemFrameThickness, Palette::Skyblue);
        }

        if (is_dragged_row) {
            continue;
        }

        DrawDragHandle(item_rect);

        String item_text = U"Scrap #{}"_fmt(polygon_index);
        font_base(item_text).draw(item_rect.x + kPlacedListItemTextOffsetX,
                                  item_rect.y + kPlacedListItemTextOffsetY,
                                  Palette::Black);

        Rect checkbox_rect = CalcPlacedListCheckboxRect(item_rect);
        checkbox_rect.drawFrame(kPlacedListCheckboxFrameThickness, Palette::Black);

        if (is_locked_poly) {
            checkbox_rect.draw(Palette::Lightgreen);
            Line(checkbox_rect.x + kPlacedListCheckMarkStartX,
                 checkbox_rect.y + kPlacedListCheckMarkStartY,
                 checkbox_rect.x + kPlacedListCheckMarkMidX,
                 checkbox_rect.y + kPlacedListCheckMarkMidY)
                .draw(kPlacedListCheckMarkThickness, Palette::Black);
            Line(checkbox_rect.x + kPlacedListCheckMarkMidX,
                 checkbox_rect.y + kPlacedListCheckMarkMidY,
                 checkbox_rect.x + kPlacedListCheckMarkEndX,
                 checkbox_rect.y + kPlacedListCheckMarkEndY)
                .draw(kPlacedListCheckMarkThickness, Palette::Black);
        }

        font_base(U"Lock")
            .draw(checkbox_rect.x + ui::CheckboxTextOffset,
                  checkbox_rect.y + kPlacedListCheckboxLabelOffsetY,
                  Palette::Black);
    }

    if (state_.dragging_item && state_.drag_item_index >= 0 &&
        state_.drag_item_index < static_cast<int>(placed_polygons.size())) { // while dragging
        const int drag_preview_top =
            std::clamp(static_cast<int>(Cursor::Pos().y - state_.drag_item_offset),
                       layout.list_view_area.y - layout.item_height,
                       layout.list_view_area.y + layout.list_view_area.h - layout.item_height);
        Rect drag_preview_rect(layout.list_view_area.x + kPlacedListItemInsetX,
                               drag_preview_top, layout.item_draw_width, layout.item_height);
        drag_preview_rect.draw(ColorF(0.8, 0.9, 1.0, 0.8));
        drag_preview_rect.drawFrame(kPlacedListDragPreviewFrameThickness, Palette::Blue);

        DrawDragHandle(drag_preview_rect);

        const int polygon_index = placed_polygons[state_.drag_item_index].GetIndex();
        String item_text = U"Scrap #{}"_fmt(polygon_index);
        font_base(item_text).draw(drag_preview_rect.x + kPlacedListItemTextOffsetX,
                                  drag_preview_rect.y + kPlacedListItemTextOffsetY,
                                  Palette::Black);

        int drop_visual =
            std::clamp(state_.drop_target_index - layout.start_item, 0, layout.visible_items);
        const int indicator_y = layout.list_view_area.y + drop_visual * layout.item_height;
        Line(layout.list_view_area.x, indicator_y,
             layout.list_view_area.x + layout.list_view_area.w, indicator_y)
            .draw(kPlacedListDropIndicatorThickness, Palette::Blue);
    } else if (state_.dragging_item && state_.drop_target_index >= 0) {
        int drop_visual =
            std::clamp(state_.drop_target_index - layout.start_item, 0, layout.visible_items);
        const int indicator_y = layout.list_view_area.y + drop_visual * layout.item_height;
        Line(layout.list_view_area.x, indicator_y,
             layout.list_view_area.x + layout.list_view_area.w, indicator_y)
            .draw(kPlacedListDropIndicatorThickness, Palette::Blue);
    }

    if (total_items > layout.max_visible_items) {
        const int scrollbar_width = ui::ScrollbarWidth;
        Rect scrollbar_track(layout.list_view_area.x + layout.list_view_area.w - scrollbar_width,
                             layout.list_view_area.y, scrollbar_width,
                             layout.list_view_area.h);
        scrollbar_track.draw(ColorF(0.85, 0.85, 0.85));

        const double visible_ratio =
            std::min(1.0, static_cast<double>(layout.max_visible_items) / total_items);
        const int thumb_height =
            std::max(ui::ScrollbarMinThumbHeight,
                     static_cast<int>(layout.list_view_area.h * visible_ratio));

        const int clamped_scroll =
            std::clamp(state_.scroll_offset, 0, layout.max_scroll_offset);
        double scroll_ratio = (layout.max_scroll_offset > 0)
                                  ? static_cast<double>(clamped_scroll) / layout.max_scroll_offset
                                  : 0.0;
        const int thumb_pos =
            static_cast<int>(scroll_ratio * (layout.list_view_area.h - thumb_height));

        Rect scroll_thumb(scrollbar_track.x, scrollbar_track.y + thumb_pos,
                          scrollbar_track.w, thumb_height);
        scroll_thumb.draw(ColorF(0.5, 0.5, 0.5));
    }
}

void PlacedListWidget::Update(CanvasModel& model,
                              ui::SelectType& selected_type,
                              int& selected_index,
                              bool& is_dragging,
                              int& dragging_placed_index,
                              bool in_operation) {
    auto& placed_polygons = model.GetPlacedPolygons();
    const int total_items = static_cast<int>(placed_polygons.size());

    if (total_items == 0) {
        return;
    }

    PlacedListLayout layout = BuildPlacedListLayout(total_items, state_.scroll_offset);

    if (layout.list_view_area.mouseOver()) {
        state_.scroll_offset += Mouse::Wheel() * layout.item_height;
    }
    state_.scroll_offset =
        std::clamp(state_.scroll_offset, 0, layout.max_scroll_offset);

    if (state_.dragging_item) {
        const int cursor_y = Cursor::Pos().y;
        if (cursor_y < layout.list_view_area.y + ui::AutoScrollMargin &&
            state_.scroll_offset > 0) {
            state_.scroll_offset =
                std::max(state_.scroll_offset - layout.item_height, 0);
        } else if (cursor_y > layout.list_view_area.y + layout.list_view_area.h -
                   ui::AutoScrollMargin &&
                   state_.scroll_offset < layout.max_scroll_offset) {
            state_.scroll_offset =
                std::min(state_.scroll_offset + layout.item_height, layout.max_scroll_offset);
        }
    }

    layout = BuildPlacedListLayout(total_items, state_.scroll_offset);

    auto apply_reorder = [&](int from_index, int insertion_slot) {
        if (from_index < 0 || from_index >= static_cast<int>(placed_polygons.size())) {
            return;
        }

        insertion_slot = std::clamp(insertion_slot, 0, static_cast<int>(placed_polygons.size()));

        MyPolygon moving_polygon = placed_polygons[from_index];
        placed_polygons.erase(placed_polygons.begin() + from_index);

        if (insertion_slot > from_index) {
            insertion_slot -= 1;
        }
        insertion_slot = std::clamp(insertion_slot, 0, static_cast<int>(placed_polygons.size()));
        placed_polygons.insert(placed_polygons.begin() + insertion_slot, moving_polygon);

        if (selected_type == ui::SelectType::Placed) {
            if (selected_index == from_index) {
                selected_index = insertion_slot;
            } else if (selected_index > from_index && selected_index <= insertion_slot) {
                selected_index -= 1;
            } else if (selected_index < from_index && selected_index >= insertion_slot) {
                selected_index += 1;
            }
        }

        auto& opt_state = OptInfo::Current();
        if (opt_state.has_value) {
            opt_state.min_placed_polygons = placed_polygons;
            opt_state.min_unplaced_polygons = model.GetUnplacedPolygons();
            opt_state.min_canvas_placement_count = model.GetCanvasPlacementCount();
        }

        const int min_visible_index = state_.scroll_offset / layout.item_height;
        const int max_visible_index = min_visible_index + layout.max_visible_items - 1;
        if (insertion_slot < min_visible_index) {
            state_.scroll_offset =
                std::clamp(insertion_slot * layout.item_height, 0, layout.max_scroll_offset);
        } else if (insertion_slot > max_visible_index) {
            state_.scroll_offset =
                std::clamp((insertion_slot - layout.max_visible_items + 1) * layout.item_height,
                           0, layout.max_scroll_offset);
        }
    };

    for (int i = layout.start_item; i < layout.end_item; ++i) {
        Rect item_rect = CalcPlacedListItemRect(layout, i);

        const int polygon_index = placed_polygons[i].GetIndex();
        const bool is_locked_poly = model.IsLocked(polygon_index);
        const bool is_dragged_row =
            state_.dragging_item && (i == state_.drag_item_index);

        if (is_dragged_row) {
            continue;
        }

        Rect checkbox_rect = CalcPlacedListCheckboxRect(item_rect);
        if (checkbox_rect.leftClicked()) {
            model.SetLocked(polygon_index, !is_locked_poly);
        }

        Rect drag_handle_rect = CalcDragHandleRect(item_rect);
        if (!state_.dragging_item && !is_locked_poly && !in_operation &&
            drag_handle_rect.mouseOver() && MouseL.down()) {
            state_.dragging_item = true;
            state_.drag_item_index = i;
            state_.drag_item_offset = Cursor::Pos().y - item_rect.y;
            state_.drop_target_index = i;
        }

        if (item_rect.leftClicked() && !is_locked_poly && !in_operation &&
            !state_.dragging_item) {
            selected_type = ui::SelectType::Placed;
            selected_index = i;
        }
    }

    if (state_.dragging_item && state_.drag_item_index >= 0 &&
        state_.drag_item_index < static_cast<int>(placed_polygons.size())) {
        const double pointer_local =
            (Cursor::Pos().y - state_.drag_item_offset +
             layout.item_height / 2.0) - layout.list_view_area.y;
        state_.drop_target_index =
            CalcPlacedListDropSlot(layout, pointer_local, state_.scroll_offset);

        if (!MouseL.pressed()) {
            if (state_.drop_target_index >= 0) {
                apply_reorder(state_.drag_item_index, state_.drop_target_index);
            }
            state_.dragging_item = false;
            state_.drag_item_index = -1;
            state_.drop_target_index = -1;
        }
    }

    if (total_items > layout.max_visible_items) {
        const int scrollbar_width = ui::ScrollbarWidth;
        Rect scrollbar_track(layout.list_view_area.x + layout.list_view_area.w - scrollbar_width,
                             layout.list_view_area.y, scrollbar_width,
                             layout.list_view_area.h);

        const double visible_ratio =
            std::min(1.0, static_cast<double>(layout.max_visible_items) / total_items);
        const int thumb_height =
            std::max(ui::ScrollbarMinThumbHeight,
                     static_cast<int>(layout.list_view_area.h * visible_ratio));

        double scroll_ratio = (layout.max_scroll_offset > 0)
                                  ? static_cast<double>(state_.scroll_offset) /
                                        layout.max_scroll_offset
                                  : 0.0;
        const int thumb_pos =
            static_cast<int>(scroll_ratio * (layout.list_view_area.h - thumb_height));

        Rect scroll_thumb(scrollbar_track.x, scrollbar_track.y + thumb_pos,
                          scrollbar_track.w, thumb_height);

        if (scroll_thumb.leftPressed()) {
            state_.dragging_scrollbar = true;
            state_.drag_start_y = Cursor::Pos().y;
            state_.drag_start_scroll = state_.scroll_offset;
        }

        if (state_.dragging_scrollbar) {
            if (MouseL.pressed()) {
                int drag_delta = Cursor::Pos().y - state_.drag_start_y;
                double scroll_scale = static_cast<double>(layout.max_scroll_offset) /
                                      std::max(1, layout.list_view_area.h - thumb_height);
                state_.scroll_offset =
                    std::clamp(state_.drag_start_scroll +
                                   static_cast<int>(drag_delta * scroll_scale),
                               0, layout.max_scroll_offset);
            } else {
                state_.dragging_scrollbar = false;
            }
        }
    }

    if (is_dragging && selected_type == ui::SelectType::Placed &&
        dragging_placed_index >= 0 &&
        dragging_placed_index < static_cast<int>(placed_polygons.size())) {
        int dragged_index = placed_polygons[dragging_placed_index].GetIndex();
        if (model.IsLocked(dragged_index)) {
            is_dragging = false;
            dragging_placed_index = -1;
        }
    }
}
