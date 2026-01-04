#include "ui/ui_manager.hpp"

#include "core/canvas_model.hpp"
#include "ui/ui_constants.hpp"


using namespace s3d;

void UIManager::ChangePatternShape(PatternShapeType shape) {
    if (InOperation() || !model_.ChangePatternShape(shape)) {
        return;
    }

    selected_type_ = SelectType::None;
    selected_index_ = -1;
}

String UIManager::GetPatternShapeLabel(PatternShapeType shape) {
    switch (shape) {
        case PatternShapeType::Square:
            return U"Square";
        case PatternShapeType::Triangle:
            return U"Triangle";
        case PatternShapeType::Donut:
            return U"Donut";
        case PatternShapeType::Tshirt:
            return U"T-Shirt";
        case PatternShapeType::Hexagon:
            return U"Hexagon";
        case PatternShapeType::Ellipse:
            return U"Ellipse";
        case PatternShapeType::Polygon:
            return U"Polygon";
    }
    return U"";
}

void UIManager::DrawPatternShapeDialog() {
    const bool in_operation = InOperation();
    if (!is_pattern_dialog_open_ || in_operation) {
        if (in_operation) {
            is_pattern_dialog_open_ = false;
        }
        return;
    }

    bool skipCloseThisFrame = pattern_dialog_just_opened_;
    pattern_dialog_just_opened_ = false;

    Array<PatternShapeType> shapes = {
        PatternShapeType::Square,
        PatternShapeType::Triangle,
        PatternShapeType::Donut,
        PatternShapeType::Tshirt,
        PatternShapeType::Hexagon,
        PatternShapeType::Ellipse,
        PatternShapeType::Polygon
    };

    const int dialog_padding_x = 15;
    const int dialog_padding_y = 10;
    const int dialog_header_height = 45;
    const int close_button_size = 25;
    const int close_button_margin = 10;
    const int close_button_offset = close_button_size + close_button_margin;
    const int dialog_offset_x = 30;
    const int dialog_offset_y = 30;

    const int columns = 2;
    const int dialog_width = 320;
    const int button_height = 40;
    const int spacing = 12;
    const int rows = (shapes.size() + columns - 1) / columns;
    const int dialog_height = 60 + rows * button_height + (rows - 1) * spacing;
    int base_x = Config::GetCanvasUIX() + Config::GetCanvasUISize() + dialog_offset_x;
    int base_y = Config::GetCanvasUIY() + dialog_offset_y;

    Rect dialog_rect(base_x, base_y, dialog_width, dialog_height);
    dialog_rect.draw(Palette::Whitesmoke);
    dialog_rect.drawFrame(2, Palette::Black);

    FontRef(FontId::Bold)(U"Select Pattern Shape").draw(dialog_rect.x + dialog_padding_x,
                                                       dialog_rect.y + dialog_padding_y, Palette::Black);

    const int button_width = (dialog_width - 50) / columns;

    for (int index = 0; index < shapes.size(); ++index) {
        int row = index / columns;
        int col = index % columns;
        Rect button_rect(dialog_rect.x + dialog_padding_x + col * (button_width + spacing),
                         dialog_rect.y + dialog_header_height + row * (button_height + spacing),
                         button_width, button_height);
        bool is_selected = (model_.GetCurrentPatternShape() == shapes[index]);
        ColorF fill_color = is_selected ? Palette::Orange : Palette::Aliceblue;
        button_rect.draw(fill_color);
        button_rect.drawFrame(1, Palette::Black);
        const Font &labelFont = FontRefOr(FontId::Small, FontId::Base);
        labelFont(GetPatternShapeLabel(shapes[index])).drawAt(button_rect.center(), Palette::Black);

        if (button_rect.leftClicked()) {
            ChangePatternShape(shapes[index]);
            is_pattern_dialog_open_ = false;
            pattern_dialog_just_opened_ = false;
        }
    }

    Rect close_rect(dialog_rect.x + dialog_width - close_button_offset,
                    dialog_rect.y + dialog_padding_y, close_button_size, close_button_size);
    close_rect.draw(Palette::Indianred);
    close_rect.drawFrame(1, Palette::Black);
    FontRef(FontId::Bold)(U"X").drawAt(close_rect.center(), Palette::White);
    if (close_rect.leftClicked()) {
        is_pattern_dialog_open_ = false;
        pattern_dialog_just_opened_ = false;
    }

    if (!skipCloseThisFrame && MouseL.down() && !dialog_rect.mouseOver()) {
        is_pattern_dialog_open_ = false;
    }
}

UIManager::ScrapLoadMode UIManager::NextScrapLoadMode(ScrapLoadMode mode) {
    switch (mode) {
        case ScrapLoadMode::Preset:
            return ScrapLoadMode::Cutter;
        case ScrapLoadMode::Cutter:
            return ScrapLoadMode::Generator;
        case ScrapLoadMode::Generator:
        default:
            return ScrapLoadMode::Preset;
    }
}

String UIManager::GetScrapModeLabel() {
    switch (scrap_load_mode_) {
        case ScrapLoadMode::Preset:
            return U"Preset";
        case ScrapLoadMode::Cutter:
            return U"Cropper";
        case ScrapLoadMode::Generator:
        default:
            return U"Polygon";
    }
}
