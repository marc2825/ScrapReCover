#include "ui/ui_manager.hpp"

#include "algorithm/optimizer.hpp"
#include "core/canvas_model.hpp"
#include "utils/polygon_generator.hpp"


using namespace s3d;

bool UIManager::Button(const Rect &rect, const Font &font, const String &text,
                       const ColorF &font_color_enabled, const ColorF &font_color_disabled,
                       const ColorF &box_color_enabled, const ColorF &box_color_disabled,
                       bool enabled) {
    if (enabled && rect.mouseOver()) {
        Cursor::RequestStyle(CursorStyle::Hand);
    }

    constexpr double kButtonTextSizeRatio = 0.45;
    int sz = static_cast<int>(Config::GetButtonHeight() * kButtonTextSizeRatio);
    auto drawCenteredText = [&](const ColorF &color) {
        Array<String> lines = text.split(U'\n');
        if (lines.size() <= 1) {
            font(text).drawAt(sz, (rect.x + rect.w / 2), (rect.y + rect.h / 2), color);
            return;
        }

        Array<double> lineHeights;
        lineHeights.reserve(lines.size());
        double totalHeight = 0.0;
        const double lineGap = sz * 0.2;
        for (const auto &line : lines) {
            RectF region = font(line).region(sz);
            double h = region.h;
            lineHeights.push_back(h);
            totalHeight += h;
        }
        totalHeight += lineGap * (lines.size() - 1);

        double currentY = rect.y + rect.h / 2.0 - totalHeight / 2.0;
        double centerX = rect.x + rect.w / 2.0;
        for (size_t i = 0; i < lines.size(); ++i) {
            double lineCenterY = currentY + lineHeights[i] / 2.0;
            font(lines[i]).drawAt(sz, centerX, lineCenterY, color);
            currentY += lineHeights[i] + lineGap;
        }
    };

    if (enabled) {
        rect.draw(box_color_enabled);
        drawCenteredText(font_color_enabled);
    } else {
        rect.draw(box_color_disabled);
        drawCenteredText(font_color_disabled);
    }

    return (enabled && rect.leftClicked());
}

void UIManager::DrawButtons() {
    const int control_ui_x = Config::GetControlUIX();
    const int control_ui_y = Config::GetControlUIY();
    const int unplaced_ui_x = Config::GetUnplacedUIX();
    const int unplaced_ui_width = Config::GetUnplacedUIWidth();
    const int button_margin = Config::GetButtonMargin();
    const int button_width = Config::GetButtonWidth();
    const int button_height = Config::GetButtonHeight();
    constexpr int kButtonFrameThickness = 2;

    constexpr int kActionRow = 2;
    auto row_y = [&](int row) {
        return control_ui_y + button_margin + (button_height + button_margin) * row;
    };
    auto row_button = [&](int base_x, int row, int col) {
        return Rect(base_x + col * (button_width + button_margin),
                    row_y(row), button_width, button_height);
    };

    const int left_buttons_x = control_ui_x + button_margin;
    const int right_buttons_x =
        unplaced_ui_x + unplaced_ui_width - (button_width * 3 + button_margin * 2) - 5;

    ButtonRect(ButtonId::CanvasSave) = row_button(left_buttons_x, kActionRow, 0);
    ButtonRect(ButtonId::CanvasSave).drawFrame(kButtonFrameThickness, Palette::Black);

    ButtonRect(ButtonId::Optimize) = row_button(left_buttons_x, kActionRow, 1);
    ButtonRect(ButtonId::Optimize).drawFrame(kButtonFrameThickness, Palette::Black);

    ButtonRect(ButtonId::Reset) = row_button(left_buttons_x, kActionRow, 2);
    ButtonRect(ButtonId::Reset).drawFrame(kButtonFrameThickness, Palette::Black);

    ButtonRect(ButtonId::LoadImage) = row_button(right_buttons_x, kActionRow, 0);
    ButtonRect(ButtonId::LoadImage).drawFrame(kButtonFrameThickness, Palette::Black);

    ButtonRect(ButtonId::CutScraps) = row_button(right_buttons_x, kActionRow, 1);
    ButtonRect(ButtonId::CutScraps).drawFrame(kButtonFrameThickness, Palette::Black);

    ButtonRect(ButtonId::Delete) = row_button(right_buttons_x, kActionRow, 2);
    ButtonRect(ButtonId::Delete).drawFrame(kButtonFrameThickness, Palette::Black);

    bool save_clicked = Button(ButtonRect(ButtonId::CanvasSave), FontRef(FontId::Base), U"Save Layout", Palette::Black,
                               Palette::Lightgray, Palette::Aliceblue, Palette::Gray, true);
    if (save_clicked && !is_saving_ && !InOperation()) {
        save_capture_state_ = SaveCaptureState::Requested;
    }
    if ((is_saving_ || save_capture_state_ == SaveCaptureState::Ready) && !InOperation()) {
        CanvasSave();
    }

    String optimize_label = is_optimizing_ ? U"Cancel" : U"Optimize!";
    ColorF optimize_box_color = is_optimizing_ ? Palette::Lightcoral : Palette::Gold;
    bool optimize_enabled = is_optimizing_ || !InOperation();
    if (Button(ButtonRect(ButtonId::Optimize), FontRef(FontId::Base), optimize_label, Palette::Black,
               Palette::Lightgray, optimize_box_color, Palette::Gray, optimize_enabled)) {
        if (is_optimizing_) {
            // Restore state at optimization start.
            model_.RestoreCancelSnapshot();
            model_.ClearCancelSnapshot();
            is_optimizing_ = false;
            model_.CurrentCanvasState().mode = Mode::Waiting;
            model_.ResetIteration();
            OptInfo::Current().has_value = false;
        } else if (!InOperation()) {
            CanvasOptimize();
        }
    }

    if (Button(ButtonRect(ButtonId::Reset), FontRef(FontId::Base), U"Reset", Palette::Black,
               Palette::Lightgray, Palette::Aliceblue, Palette::Gray, true)) {
        if (!InOperation()) {
            CanvasReset();
        }
    }

    String loadButtonText = GetLoadButtonLabel();
    if (Button(ButtonRect(ButtonId::LoadImage), FontRef(FontId::Base), loadButtonText, Palette::Black,
               Palette::Lightgray, Palette::Aliceblue, Palette::Gray, !InOperation())) {
        if (!InOperation()) {
            if (scrap_load_mode_ == ScrapLoadMode::Preset) {
                model_.LoadAllScraps();
            } else if (scrap_load_mode_ == ScrapLoadMode::Cutter) {
                if (LaunchMeasureScrap(U"new", -1)) {
                    model_.LoadNewScraps();
                }
            } else {
                model_.GeneratePolygons(Config::GetGenCount());
            }
            selected_type_ = SelectType::None;
            selected_index_ = -1;
        }
    }

    bool canCutScrap = !InOperation() && selected_type_ == SelectType::Unplaced && selected_index_ >= 0;
    if (Button(ButtonRect(ButtonId::CutScraps), FontRef(FontId::Base), U"Cut Scraps", Palette::Black,
               Palette::Lightgray, Palette::Aliceblue, Palette::Gray, canCutScrap)) {
        CutScrap();
    }

    if (Button(ButtonRect(ButtonId::Delete), FontRef(FontId::Base), U"Delete Scraps", Palette::Black,
               Palette::Lightgray, Palette::Aliceblue, Palette::Gray, true)) {
        if (!InOperation()) {
            if (selected_type_ == SelectType::Unplaced &&
                model_.DeleteUnplacedScrapAt(selected_index_)) {
                selected_index_ = -1;
                selected_type_ = SelectType::None;
            }
        }
    }
}
