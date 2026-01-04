#pragma once
#include <Siv3D.hpp>

#include "core/canvas_model.hpp"
#include "ui/ui_types.hpp"


class PlacedListWidget {
    public:
        void Draw(const CanvasModel& model,
                const Font& font_base,
                const Font& font_bold,
                ui::SelectType selected_type,
                int selected_index);

        void Update(CanvasModel& model,
                    ui::SelectType& selected_type,
                    int& selected_index,
                    bool& is_dragging,
                    int& dragging_placed_index,
                    bool in_operation);

    private:
        struct State {
            int scroll_offset = 0;
            bool dragging_item = false;
            int drag_item_index = -1;
            double drag_item_offset = 0.0;
            int drop_target_index = -1;
            bool dragging_scrollbar = false;
            int drag_start_y = 0;
            int drag_start_scroll = 0;
        };

        Rect DrawDragHandle(const Rect& rect) const;

        State state_;
};
