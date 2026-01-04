#pragma once
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <string>

#include <Siv3D.hpp>
#include <nlohmann/json.hpp>


class Config {
    public:
        // Load from config.json
        static void LoadConfig(const std::string& path);

        static inline int GetSeed0() { return config_data_["seeds"]["seed0"].get<int>(); }
        static inline int GetSeed1() { return config_data_["seeds"]["seed1"].get<int>(); }
    
        static inline int GetWindowWidth() { return config_data_["ui_size"]["window"]["width"].get<int>(); }
        static inline int GetWindowHeight() { return config_data_["ui_size"]["window"]["height"].get<int>(); }
        static inline std::u32string GetTitle() { return Unicode::FromUTF8(config_data_["ui_size"]["window"]["title"].get<std::string>()).toUTF32(); }
    
        static inline int GetCanvasUIX() { return config_data_["ui_size"]["canvas_ui"]["x"].get<int>(); }
        static inline int GetCanvasUIY() { return config_data_["ui_size"]["canvas_ui"]["y"].get<int>(); }
        static inline int GetCanvasUISize() { return config_data_["ui_size"]["canvas_ui"]["size"].get<int>(); }
        static inline int GetCanvasUIRatio() { return ui_ratio_; }

        // layout_size: the inner usable area
        // canvas_size: the outer usable area (= margin + layout_size + margin)
        static inline int GetCanvasMargin() { return config_data_["canvas"]["margin"].get<int>(); }
        static inline int GetLayoutSize() { return config_data_["canvas"]["layout_size"].get<int>(); }
        static inline int GetCanvasSize() { return GetCanvasMargin() * 2 + GetLayoutSize(); }
        static inline int GetNumCanvases() {
            if (config_data_["canvas"].contains("num_canvases")) {
                return std::max(1, config_data_["canvas"]["num_canvases"].get<int>());
            }
            return 1;
        }

        static inline int GetUnplacedUIX() { return config_data_["ui_size"]["unplaced_ui"]["x"].get<int>(); }
        static inline int GetUnplacedUIY() { return config_data_["ui_size"]["unplaced_ui"]["y"].get<int>(); }
        static inline int GetUnplacedUIWidth() { return config_data_["ui_size"]["unplaced_ui"]["width"].get<int>(); }
        static inline int GetUnplacedUIHeight() { return config_data_["ui_size"]["unplaced_ui"]["height"].get<int>(); }

        static inline int GetUnplacedIconSize() { return config_data_["unplaced_list"]["icon_size"].get<int>(); }
        static inline int GetUnplacedMargin() { return config_data_["unplaced_list"]["margin"].get<int>(); }
        static inline int GetUnplacedIconRatio() { return icon_ratio_; }
        static double GetPlacedWheelRotateRad();
        static int GetUnplacedScrollStep();

        static inline int GetControlUIX() { return config_data_["ui_size"]["control_ui"]["x"].get<int>(); }
        static inline int GetControlUIY() { return config_data_["ui_size"]["control_ui"]["y"].get<int>(); }
        static inline int GetControlUIWidth() { return config_data_["ui_size"]["control_ui"]["width"].get<int>(); }
        static inline int GetControlUIHeight() { return config_data_["ui_size"]["control_ui"]["height"].get<int>(); }

        static s3d::Array<s3d::FilePath> GetOutputsRoots();
        static s3d::FilePath GetLayoutExportsDir();
        static s3d::FilePath GetMeasureTempDir();
        static s3d::FilePath GetMeasureOutputsDir();
        static s3d::FilePath GetMeasureScriptPath();
        static bool GetCleanupMeasureOutputsOnStartup();
        static bool GetCleanupMeasureTempOnStartup();
        static const nlohmann::json* GetPatternShapeConfig(const std::string& key);
        static int GetPatternShapeLayoutSize();
    
        static inline bool GetGenIsLarge() { return config_data_["generate_polygon"]["is_large"].get<bool>(); }
        static inline bool GetGenIsRect() { return config_data_["generate_polygon"]["is_rectangle"].get<bool>(); }
        static int GetGenCount();
        static int GetGenVertexMin();
        static int GetGenVertexMax();
        static double GetGenRadiusMean();
        static double GetGenRadiusVar();
        static double GetGenRectLargeMin();
        static double GetGenRectLargeMax();
        static double GetGenRectSmallMin();
        static double GetGenRectSmallMax();
        static double GetGenMinExtentLarge();
        static double GetGenMinExtentSmall();

        static inline int GetFontBaseSize() { return config_data_["font"]["font_base"]["size"].get<int>(); }
        static inline std::u32string GetFontBaseType() { return Unicode::FromUTF8(config_data_["font"]["font_base"]["type"].get<std::string>()).toUTF32(); }

        static inline int GetButtonWidth() { return config_data_["button"]["width"].get<int>(); }
        static inline int GetButtonHeight() { return config_data_["button"]["height"].get<int>(); }
        static inline int GetButtonMargin() { return config_data_["button"]["margin"].get<int>(); }
    
        static inline int GetOptSteps() { return config_data_["hyperparameter"]["steps"].get<int>(); }
        static std::array<double, 5> GetHyperparameterP();
        static std::array<double, 5> GetHyperparameterNoP();
        static long long GetHyperparameterNpPen();
        static long long GetHyperparameterOrPen();
        static long long GetHyperparameterOlPen();
        static double GetHyperparameterStTmp();
        static double GetHyperparameterEnTmp();
        static int GetMaxPolygonCount();
        static double GetRectSimilarityCalcStep();
        static double GetTextureScaleDivisor();
        static double GetSeamAllowance();

    private:
        static nlohmann::json config_data_;
        static nlohmann::json pattern_shape_data_;
        static double ui_ratio_;
        static double icon_ratio_;
    };
