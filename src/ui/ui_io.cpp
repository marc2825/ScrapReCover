#include "ui/ui_manager.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <limits>

#include <nlohmann/json.hpp>

#include "core/canvas_model.hpp"
using namespace s3d;

void UIManager::CanvasSave() {
    auto &model = model_;
    if (selected_type_ == UIManager::SelectType::Unplaced) {
        auto result = System::MessageBoxOKCancel(U"Save Layout?");

        if (result == MessageBoxResult::Cancel) {
            save_capture_state_ = SaveCaptureState::Idle;
            return;
        }
    }

    if (is_saving_) {
        if (FileSystem::Exists(U"Screenshot/" + screenshot_path_)) {
            std::cout << "Screenshot found: " << screenshot_path_.narrow() << '\n';
        } else {
            std::cerr << "Screenshot missing, capturing current frame: "
                      << screenshot_path_.narrow()
                      << '\n';
            ScreenCapture::SaveCurrentFrame(screenshot_path_);
            if (!FileSystem::Exists(U"Screenshot/" + screenshot_path_)) {
                std::cerr << "Failed to create screenshot: "
                          << screenshot_path_.narrow()
                          << '\n';
                return;
            }
        }

        int canvas_ui_x = Config::GetCanvasUIX();
        int canvas_ui_y = Config::GetCanvasUIY();
        int canvas_margin = Config::GetCanvasMargin();
        double canvas_ui_ratio = Config::GetCanvasUIRatio();
        int layout_size = Config::GetLayoutSize();
        int canvas_size = Config::GetCanvasSize();

        Rect crop_rect =
        Rect(canvas_ui_x + (canvas_margin-1) * canvas_ui_ratio, canvas_ui_y + (canvas_margin-1) * canvas_ui_ratio,
             (layout_size + 2) * canvas_ui_ratio, (layout_size + 2) * canvas_ui_ratio);

        const int dpi = 300;
        const int image_size = static_cast<int>(static_cast<double>(canvas_size) * dpi / 25.4); // inch -> mm

        Image image(U"Screenshot/" + screenshot_path_);
        Image cropped = image.flipped().clipped(crop_rect);
        Image resized = cropped.scaled(image_size, image_size);

        const int current_index = screenshot_index_;
        const FilePath layout_exports_root = FileSystem::FullPath(Config::GetLayoutExportsDir());
        FileSystem::CreateDirectories(layout_exports_root);
        const String timestamp = FileSystem::BaseName(screenshot_path_);
        const String layout_tag = timestamp.isEmpty()
            ? Format(current_index)
            : Format(current_index) + U"_" + timestamp;

        const FilePath layout_image_path =
            FileSystem::PathAppend(layout_exports_root, U"Layout_{}.png"_fmt(layout_tag));
        resized.savePNG(layout_image_path);

        const FilePath layout_dir =
            FileSystem::PathAppend(layout_exports_root, U"Layout_{}"_fmt(layout_tag));
        FileSystem::CreateDirectories(layout_dir);

        const FilePath layout_image_relative = U"layout.png";
        const FilePath layout_image_full = FileSystem::PathAppend(layout_dir, layout_image_relative);
        resized.savePNG(layout_image_full);

        const FilePath texture_previews_dir = FileSystem::PathAppend(layout_dir, U"texture_previews");
        FileSystem::CreateDirectories(texture_previews_dir);

        const auto &placed_polygons = model.GetPlacedPolygonsConst();
        const double seam_allowance = Config::GetSeamAllowance();
        const double layout_units_per_mm = 0.1; // 1mm = 1/10 layout unit
        const double expand_margin_mm = seam_allowance / layout_units_per_mm;

        // Output metadata
        nlohmann::json layout_json;
        layout_json["timestamp"] = screenshot_path_.narrow();
        layout_json["scrap_count"] = static_cast<int>(placed_polygons.size());
        layout_json["layout"] = {
            {"index", model.GetCurrentCanvasIndex()},
            {"image_path", layout_image_relative.narrow()},
            {"dpi", dpi},
            {"layout_units", canvas_size},
            {"sheet_size", layout_size},
            {"offset", canvas_margin},
            {"expand_margin_mm", expand_margin_mm}
        };
        layout_json["photo"] = {
            {"path", layout_image_relative.narrow()},
            {"dpi", dpi}
        };

        // Output individual scraps' info (metadata, preview image)
        nlohmann::json scrap_array = nlohmann::json::array();
        std::vector<int> layer_order;
        layer_order.reserve(placed_polygons.size());

        for (size_t layer = 0; layer < placed_polygons.size(); ++layer) {
            const MyPolygon &scrap = placed_polygons[layer];
            nlohmann::json scrap_json;
            scrap_json["index"] = scrap.GetIndex();
            scrap_json["layer"] = static_cast<int>(layer);

            const Vec2 center = scrap.GetCenter();
            scrap_json["center_cm"] = { {"x", center.x}, {"y", center.y} };
            scrap_json["rotation_radians"] = scrap.GetRotation();
            scrap_json["rotation_degrees"] = Math::ToDegrees(scrap.GetRotation());

            String texture_preview_relative;
            bool texture_preview_ready = false;
            if (const auto &texture_path_opt = scrap.GetTexturePath();
                texture_path_opt.has_value() && FileSystem::Exists(texture_path_opt.value())) {
                const String preview_filename = U"scrap_{}_preview.png"_fmt(scrap.GetIndex());
                const FilePath preview_full_path = texture_previews_dir + preview_filename;

                Image source(texture_path_opt.value());
                if (!source.isEmpty()) {
                    constexpr int preview_max_size = 256;
                    const int width = source.width();
                    const int height = source.height();
                    const int max_dim = std::max(width, height);

                    Image preview_image;
                    if (max_dim > preview_max_size) { // align the image size with preview_max_size 
                        const double scale = static_cast<double>(preview_max_size) / static_cast<double>(max_dim);
                        const int scaled_w = std::max(1, static_cast<int>(std::round(width * scale)));
                        const int scaled_h = std::max(1, static_cast<int>(std::round(height * scale)));
                        preview_image = source.scaled(scaled_w, scaled_h);
                    } else {
                        preview_image = source;
                    }

                    preview_image.save(preview_full_path);
                    texture_preview_relative = U"texture_previews/" + preview_filename;
                    texture_preview_ready = true;
                } else if (FileSystem::Copy(texture_path_opt.value(), preview_full_path)) {
                    texture_preview_relative = U"texture_previews/" + preview_filename;
                    texture_preview_ready = true;
                }
            }

            scrap_json["texture_preview_path"] = texture_preview_ready
                ? nlohmann::json(texture_preview_relative.narrow())
                : nlohmann::json(nullptr);

            // Export the pre-shrink polygon to undo seam allowance.
            const Polygon expanded_polygon = scrap.GetOriginalPolygon()
                .rotated(scrap.GetRotation())
                .movedBy(scrap.GetCenter());

            nlohmann::json outer_vertices = nlohmann::json::array();
            for (const auto &vertex : expanded_polygon.outer()) {
                outer_vertices.push_back({ {"x", vertex.x}, {"y", vertex.y} });
            }
            scrap_json["expanded_polygon_outer"] = outer_vertices;

            if (expanded_polygon.hasHoles()) {
                nlohmann::json holes_array = nlohmann::json::array();
                for (const auto &hole : expanded_polygon.inners()) {
                    nlohmann::json hole_vertices = nlohmann::json::array();
                    for (const auto &vertex : hole) {
                        hole_vertices.push_back({ {"x", vertex.x}, {"y", vertex.y} });
                    }
                    holes_array.push_back(hole_vertices);
                }
                scrap_json["expanded_polygon_holes"] = holes_array;
            }

            scrap_array.push_back(scrap_json);
            layer_order.push_back(scrap.GetIndex());
        }

        layout_json["scraps"] = scrap_array;
        layout_json["layer_order"] = layer_order;

        const FilePath layout_json_path = FileSystem::PathAppend(layout_dir, U"layout.json");
        std::ofstream json_stream(layout_json_path.narrow());
        if (json_stream) {
            json_stream << layout_json.dump(4);
        } else {
            std::cerr << Unicode::ToUTF8(U"[Save Layout] Failed to write metadata: ")
                      << layout_json_path.narrow()
                      << '\n';
        }

        FileSystem::Remove(U"Screenshot/" + screenshot_path_);
        screenshot_index_++;

        is_saving_ = false;
        std::cout << Unicode::ToUTF8(U"[Save Layout] Saved layout: ")
                  << layout_json_path.narrow()
                  << '\n';
    }
    else {
        if (save_capture_state_ != SaveCaptureState::Ready) {
            return;
        }

        screenshot_path_ = DateTime::Now().format(U"yyyyMMdd_HHmmss") + U".png";
        ScreenCapture::SaveCurrentFrame(screenshot_path_);
        is_saving_ = true;
        save_capture_state_ = SaveCaptureState::Idle;
    }
}

bool UIManager::LaunchMeasureScrap(const String& mode, const int index) {
    auto &unplaced_polygons = model_.GetUnplacedPolygons();
    tempFolderPath_ = FileSystem::FullPath(Config::GetMeasureTempDir());
    if (!FileSystem::IsDirectory(tempFolderPath_)) {
        FileSystem::CreateDirectories(tempFolderPath_);
    }

    // Export the current scrap data for editing.
    if (mode == U"edit" && index >= 0) {
        const FilePath points_path = FileSystem::PathAppend(tempFolderPath_, U"polygon_points.json");
        const FilePath texture_path = FileSystem::PathAppend(tempFolderPath_, U"polygon_texture.png");
        FileSystem::Remove(points_path);
        FileSystem::Remove(texture_path);
        
        if (selected_type_ == SelectType::Unplaced && selected_index_ >= 0) {
            const auto& poly = unplaced_polygons[selected_index_].GetOriginalPolygon();
            const auto& vertices = poly.vertices();
            double scale_divisor = std::max(1.0, Config::GetTextureScaleDivisor());
            const auto& texture_path_opt = unplaced_polygons[selected_index_].GetTexturePath();
            const bool has_texture = texture_path_opt.has_value()
                && FileSystem::Exists(texture_path_opt.value());
            if (has_texture) {
                const auto& texturePath = texture_path_opt.value();
                Image texture_image(texturePath);
                const RectF bounds = poly.boundingRect();
                if (!texture_image.isEmpty() && bounds.w > 0.0 && bounds.h > 0.0) {
                    const double scale_x = texture_image.width() / bounds.w;
                    const double scale_y = texture_image.height() / bounds.h;
                    scale_divisor = (scale_x + scale_y) * 0.5;
                }
            }

            Array<Vec2> scaled_vertices;
            scaled_vertices.reserve(vertices.size());
            double min_x = std::numeric_limits<double>::max();
            double min_y = std::numeric_limits<double>::max();
            double max_x = std::numeric_limits<double>::lowest();
            double max_y = std::numeric_limits<double>::lowest();
            for (const auto& v : vertices) {
                const Vec2 scaled(v.x * scale_divisor, v.y * scale_divisor);
                scaled_vertices.push_back(scaled);
                min_x = std::min(min_x, scaled.x);
                min_y = std::min(min_y, scaled.y);
                max_x = std::max(max_x, scaled.x);
                max_y = std::max(max_y, scaled.y);
            }

            Array<Vec2> adjusted_vertices;
            adjusted_vertices.reserve(scaled_vertices.size());

            if (has_texture) {
                const Vec2 offset(-min_x, -min_y);
                for (const auto& v : scaled_vertices) {
                    adjusted_vertices.push_back(v + offset);
                }
                FileSystem::Copy(texture_path_opt.value(), texture_path);
            } else {
                Array<Vec2> centroid_vertices = scaled_vertices;
                Polygon scaled_poly(centroid_vertices);
                if (scaled_poly.isEmpty()) {
                    std::reverse(centroid_vertices.begin(), centroid_vertices.end());
                    scaled_poly = Polygon(centroid_vertices);
                }
                const Vec2 centroid = scaled_poly.centroid();
                const double half_width = std::max(centroid.x - min_x, max_x - centroid.x);
                const double half_height = std::max(centroid.y - min_y, max_y - centroid.y);
                const int image_width = std::max(1, static_cast<int>(std::ceil(half_width * 2.0)));
                const int image_height = std::max(1, static_cast<int>(std::ceil(half_height * 2.0)));
                const Vec2 offset(image_width * 0.5 - centroid.x, image_height * 0.5 - centroid.y);
                for (const auto& v : scaled_vertices) {
                    adjusted_vertices.push_back(v + offset);
                }

                Array<Vec2> draw_vertices = adjusted_vertices;
                Polygon draw_poly(draw_vertices);
                if (draw_poly.isEmpty()) {
                    std::reverse(draw_vertices.begin(), draw_vertices.end());
                    draw_poly = Polygon(draw_vertices);
                }

                Image generated_texture(image_width, image_height, Color(0, 0, 0, 0));
                const RectF bounds = draw_poly.boundingRect();
                const int left = std::max(0, static_cast<int>(std::floor(bounds.x)));
                const int top = std::max(0, static_cast<int>(std::floor(bounds.y)));
                const int right = std::min(image_width, static_cast<int>(std::ceil(bounds.x + bounds.w)));
                const int bottom = std::min(image_height, static_cast<int>(std::ceil(bounds.y + bounds.h)));
                const Color fill_color = unplaced_polygons[selected_index_].GetColor();
                for (int y = top; y < bottom; ++y) {
                    for (int x = left; x < right; ++x) {
                        if (draw_poly.contains(Vec2(x + 0.5, y + 0.5))) {
                            generated_texture[y][x] = fill_color;
                        }
                    }
                }
                generated_texture.save(texture_path);
            }

            nlohmann::json points_json = nlohmann::json::array();
            for (const auto& point : adjusted_vertices) {
                points_json.push_back({point.x, point.y});
            }
            std::ofstream points_stream(points_path.narrow());
            if (points_stream) {
                points_stream << points_json.dump(2);
            } else {
                std::cerr << "Failed to write polygon_points.json: "
                          << points_path.narrow()
                          << '\n';
            }
        }
    }

    const FilePath script_path = FileSystem::FullPath(Config::GetMeasureScriptPath());
    if (!FileSystem::Exists(script_path)) {
        System::MessageBoxOK(U"Measure script not found:\n" + script_path, U"Execution Error");
        return false;
    }
    try {
        namespace fs = std::filesystem;
        fs::path path(script_path.toWstr());
        
        // chmod +x
        fs::permissions(path, 
            fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, 
            fs::perm_options::add);
    }
    catch (const std::exception& e) {
        std::cerr << "Warning: Failed to change permissions: " << e.what() << std::endl;
    }

    const String temp_dir = FileSystem::FullPath(Config::GetMeasureTempDir());
    const String outputs_dir = FileSystem::FullPath(Config::GetMeasureOutputsDir());

    String args;
    if (mode == U"edit") { // edit mode
        args = U"--mode edit --index " + Format(index);
    }
    else { // crop mode (register new scrap)
        args = U"--mode new";
    }
    args += U" --temp-dir \"" + temp_dir + U"\"";
    args += U" --outputs-dir \"" + outputs_dir + U"\"";

    // Execute Python script
    ChildProcess process(script_path, args, Pipe::StdInOut);
    if (!process.isValid()) {
        System::MessageBoxOK(U"Failed to run the cropper UI.\nPlease check Python and file permissions.",
                             U"Execution Error");
        return false;
    }

    process.wait();

    std::string outputStr;
    try {
        std::ostringstream ss;
        ss << process.istream().rdbuf();
        outputStr = ss.str();
    }
    catch (...) {
        outputStr = "Failed to read error message.";
    }

    if (process.getExitCode() == 0) {
        lastEditedScrapFolder_ = tempFolderPath_;
        return true;
    }

    const String errorMsg = Unicode::FromUTF8(outputStr);
    System::MessageBoxOK(U"Python Script Error:\n" + errorMsg, U"Execution Error");
    return false;
}

void UIManager::CutScrap() {
    auto &unplaced_polygons = model_.GetUnplacedPolygons();
    if (selected_type_ != SelectType::Unplaced ||
        selected_index_ < 0 || selected_index_ >= unplaced_polygons.size()) {
        return;
    }
    
    int polyIndex = unplaced_polygons[selected_index_].GetIndex();
    
    FilePath oldTexturePath;
    bool hasOldTexture = false;
    if (unplaced_polygons[selected_index_].GetTexturePath().has_value()) {
        oldTexturePath = unplaced_polygons[selected_index_].GetTexturePath().value();
        hasOldTexture = true;
    }
    
    Color originalColor = unplaced_polygons[selected_index_].GetColor();
    
    // Edit mode
    if (LaunchMeasureScrap(U"edit", polyIndex)) {
        FilePath jsonPath = FileSystem::PathAppend(tempFolderPath_, U"polygon_points.json");
        FilePath texturePath = FileSystem::PathAppend(tempFolderPath_, U"polygon_texture.png");
        
        if (FileSystem::Exists(jsonPath) && FileSystem::Exists(texturePath)) {
            try {
                JSON json = JSON::Load(jsonPath);
                if (!json) {
                    return;
                }
                
                Array<Vec2> vertices;
                for (const auto& point : json.arrayView()) {
                    double x = point[0].get<double>();
                    double y = point[1].get<double>();
                    vertices.emplace_back(Vec2(x, y));
                }
                
                Polygon cur_poly = Polygon(vertices);
                if(cur_poly.isEmpty()) { // If clockwise.
                    std::reverse(vertices.begin(), vertices.end());
                    cur_poly = Polygon(vertices);
                }
                
                if(cur_poly.area() == 0) {
                    return;
                }
                
                // Save edited polygon to permanent location.
                const String outputFolder = Config::GetMeasureOutputsDir() +
                    U"edited_scrap_" + Format(polyIndex) + U"_" + DateTime::Now().format(U"yyyyMMdd_HHmmss");
                FileSystem::CreateDirectories(outputFolder);
                
                FileSystem::Copy(jsonPath, outputFolder + U"/polygon_points.json");
                FileSystem::Copy(texturePath, outputFolder + U"/polygon_texture.png");
                
                // Replace original polygon.
                unplaced_polygons[selected_index_] = MyPolygon(
                    cur_poly, 
                    polyIndex,
                    originalColor,
                    outputFolder + U"/polygon_texture.png"
                );
                
                unplaced_polygons[selected_index_].RasterizePolygon();
                
                // Remove old files if present.
                if (hasOldTexture) {
                    FilePath oldFolderPath = FileSystem::ParentPath(oldTexturePath);
                    
                    for (const auto& file : FileSystem::DirectoryContents(oldFolderPath, Recursive::No)) {
                        FileSystem::Remove(file);
                    }
                    
                    FileSystem::Remove(oldFolderPath);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[Edit Scrap] Failed to load edited data: " << e.what() << '\n';
                System::MessageBoxOK(U"Failed to load edited scrap data.\nPlease check the generated files.",
                                   U"Edit Error");
            }
        }
    }
}

String UIManager::GetLoadButtonLabel() {
    switch (scrap_load_mode_) {
    case ScrapLoadMode::Preset:
        return U"Load Preset";
    case ScrapLoadMode::Cutter:
        return U"Load Cropper";
    case ScrapLoadMode::Generator:
    default:
        return U"Load Polygon";
    }
}
