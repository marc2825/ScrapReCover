#include "utils/config.hpp"

#include <filesystem>

nlohmann::json Config::config_data_;
nlohmann::json Config::pattern_shape_data_;
double Config::ui_ratio_;
double Config::icon_ratio_;

namespace {
template <typename T, typename Json>
T GetOr(const Json &j, const std::string &key, const T &fallback) {
    if (!j.contains(key)) {
        return fallback;
    }
    try {
        return j[key].template get<T>();
    } catch (...) {
        return fallback;
    }
}
} // namespace

void Config::LoadConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Failed to open config file: " << path << std::endl;
        return;
    }
    file >> config_data_;

    pattern_shape_data_ = nlohmann::json::object();
    try {
        const std::filesystem::path config_path(path);
        const std::filesystem::path pattern_path = config_path.parent_path() / "pattern_shape.json";
        std::ifstream pattern_file(pattern_path);
        if (pattern_file) {
            pattern_file >> pattern_shape_data_;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load pattern_shape.json: " << e.what() << std::endl;
    }

    ui_ratio_ = static_cast<double>(GetCanvasUISize()) / GetCanvasSize();
    icon_ratio_ = static_cast<double>(GetUnplacedIconSize()) / GetCanvasSize();

}

double Config::GetPlacedWheelRotateRad() {
    static const double kDefaultDegrees = 3.0;
    if (!config_data_.contains("input")) {
        return kDefaultDegrees * s3d::Math::Pi / 180.0;
    }
    const auto &input = config_data_["input"];
    const double degrees = GetOr<double>(input, "placed_wheel_rotate_deg", kDefaultDegrees);
    return degrees * s3d::Math::Pi / 180.0;
}

int Config::GetUnplacedScrollStep() {
    static const int kDefault = 200;
    if (!config_data_.contains("input")) {
        return kDefault;
    }
    const auto &input = config_data_["input"];
    return std::max(0, GetOr<int>(input, "unplaced_scroll_step", kDefault));
}

s3d::Array<s3d::FilePath> Config::GetOutputsRoots() {
    s3d::Array<s3d::FilePath> roots;
    if (!config_data_.contains("paths")) {
        return roots;
    }
    const auto &paths = config_data_["paths"];

    if (paths.contains("preset_outputs_roots")) {
        const auto &value = paths["preset_outputs_roots"];
        if (value.is_array()) {
            for (const auto &entry : value) {
                if (entry.is_string()) {
                    roots.push_back(Unicode::FromUTF8(entry.get<std::string>()));
                }
            }
        } else if (value.is_string()) {
            roots.push_back(Unicode::FromUTF8(value.get<std::string>()));
        }
        return roots;
    }

    if (paths.contains("outputs_roots")) {
        const auto &value = paths["outputs_roots"];
        if (value.is_array()) {
            for (const auto &entry : value) {
                if (entry.is_string()) {
                    roots.push_back(Unicode::FromUTF8(entry.get<std::string>()));
                }
            }
        } else if (value.is_string()) {
            roots.push_back(Unicode::FromUTF8(value.get<std::string>()));
        }
        return roots;
    }

    const auto legacy = GetOr<std::string>(paths, "outputs_root", "");
    if (!legacy.empty()) {
        roots.push_back(Unicode::FromUTF8(legacy));
    }
    return roots;
}

namespace {
s3d::FilePath NormalizeDirPath(const s3d::FilePath& path) {
    if (path.isEmpty()) {
        return path;
    }
    if (path.back() == U'/' || path.back() == U'\\') {
        return path;
    }
    return path + U"/";
}
} // namespace

s3d::FilePath Config::GetLayoutExportsDir() {
    static const s3d::FilePath kDefault = U"../LayoutExports/";
    if (!config_data_.contains("paths")) {
        return kDefault;
    }
    const auto &paths = config_data_["paths"];
    const std::string layout_dir = GetOr<std::string>(paths, "layout_exports_dir", "");
    if (!layout_dir.empty()) {
        return NormalizeDirPath(Unicode::FromUTF8(layout_dir));
    }
    return kDefault;
}

s3d::FilePath Config::GetMeasureTempDir() {
    static const s3d::FilePath kDefault = U"../input_scraps/cropper_temp/";
    if (!config_data_.contains("paths")) {
        return kDefault;
    }
    const auto &paths = config_data_["paths"];
    const std::string temp_dir = GetOr<std::string>(paths, "cropper_temp_dir", "");
    if (!temp_dir.empty()) {
        return NormalizeDirPath(Unicode::FromUTF8(temp_dir));
    }
    const std::string legacy_dir = GetOr<std::string>(paths, "measure_temp_dir", "");
    if (!legacy_dir.empty()) {
        return NormalizeDirPath(Unicode::FromUTF8(legacy_dir));
    }
    return kDefault;
}

s3d::FilePath Config::GetMeasureOutputsDir() {
    static const s3d::FilePath kDefault = U"../input_scraps/cropper_outputs/";
    if (!config_data_.contains("paths")) {
        return kDefault;
    }
    const auto &paths = config_data_["paths"];
    const std::string outputs_dir = GetOr<std::string>(paths, "cropper_outputs_dir", "");
    if (!outputs_dir.empty()) {
        return NormalizeDirPath(Unicode::FromUTF8(outputs_dir));
    }
    const std::string legacy_dir = GetOr<std::string>(paths, "measure_outputs_dir", "");
    if (!legacy_dir.empty()) {
        return NormalizeDirPath(Unicode::FromUTF8(legacy_dir));
    }
    return kDefault;
}

s3d::FilePath Config::GetMeasureScriptPath() {
    static const s3d::FilePath kDefault = U"../src/ui/measure_scrap/main.py";
    if (!config_data_.contains("paths")) {
        return kDefault;
    }
    const auto &paths = config_data_["paths"];
    const std::string script_path = GetOr<std::string>(paths, "cropper_script_path", "");
    if (!script_path.empty()) {
        return Unicode::FromUTF8(script_path);
    }
    const std::string legacy_path = GetOr<std::string>(paths, "measure_script_path", "");
    if (!legacy_path.empty()) {
        return Unicode::FromUTF8(legacy_path);
    }
    return kDefault;
}

bool Config::GetCleanupMeasureOutputsOnStartup() {
    static const bool kDefault = false;
    if (!config_data_.contains("startup_cleanup")) {
        return kDefault;
    }
    const auto &cleanup = config_data_["startup_cleanup"];
    return GetOr<bool>(cleanup, "measure_outputs", kDefault);
}

bool Config::GetCleanupMeasureTempOnStartup() {
    static const bool kDefault = true;
    if (!config_data_.contains("startup_cleanup")) {
        return kDefault;
    }
    const auto &cleanup = config_data_["startup_cleanup"];
    return GetOr<bool>(cleanup, "measure_temp", kDefault);
}

const nlohmann::json* Config::GetPatternShapeConfig(const std::string& key) {
    const nlohmann::json* root = nullptr;
    if (pattern_shape_data_.contains("pattern_shapes")) {
        root = &pattern_shape_data_;
    } else if (config_data_.contains("pattern_shapes")) {
        root = &config_data_;
    } else if (config_data_.contains("patterns")) {
        root = &config_data_;
    }

    if (!root) {
        return nullptr;
    }

    const auto &container = root->contains("pattern_shapes")
        ? (*root)["pattern_shapes"]
        : (*root)["patterns"];
    if (!container.contains(key)) {
        return nullptr;
    }
    const auto &value = container[key];
    if (!value.is_object()) {
        return nullptr;
    }
    return &value;
}

int Config::GetPatternShapeLayoutSize() {
    const int fallback = GetLayoutSize();
    if (!pattern_shape_data_.contains("layout_size")) {
        return fallback;
    }
    return std::max(1, GetOr<int>(pattern_shape_data_, "layout_size", fallback));
}

int Config::GetGenCount() {
    static const int kDefault = 1;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return std::max(1, GetOr<int>(gen, "count", kDefault));
}

int Config::GetGenVertexMin() {
    static const int kDefault = 4;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<int>(gen, "v_min", kDefault);
}

int Config::GetGenVertexMax() {
    static const int kDefault = 10;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<int>(gen, "v_max", kDefault);
}

double Config::GetGenRadiusMean() {
    static const double kDefault = 15.0;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "r_mean", kDefault);
}

double Config::GetGenRadiusVar() {
    static const double kDefault = 2.5;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "r_var", kDefault);
}

double Config::GetGenRectLargeMin() {
    static const double kDefault = 5.0;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "rect_large_min", kDefault);
}

double Config::GetGenRectLargeMax() {
    static const double kDefault = 20.0;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "rect_large_max", kDefault);
}

double Config::GetGenRectSmallMin() {
    static const double kDefault = 5.0;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "rect_small_min", kDefault);
}

double Config::GetGenRectSmallMax() {
    static const double kDefault = 10.0;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "rect_small_max", kDefault);
}

double Config::GetGenMinExtentLarge() {
    static const double kDefault = 2.5;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "min_extent_large", kDefault);
}

double Config::GetGenMinExtentSmall() {
    static const double kDefault = 1.0;
    if (!config_data_.contains("generate_polygon")) {
        return kDefault;
    }
    const auto &gen = config_data_["generate_polygon"];
    return GetOr<double>(gen, "min_extent_small", kDefault);
}

std::array<double, 5> Config::GetHyperparameterP() {
    static const std::array<double, 5> kDefault = {0.29, 0.30, 0.313, 0.081, 0.016};
    if (!config_data_.contains("hyperparameter")) {
        return kDefault;
    }
    const auto &hp = config_data_["hyperparameter"];
    if (!hp.contains("P") || !hp["P"].is_array() || hp["P"].size() != 5) {
        return kDefault;
    }
    std::array<double, 5> result = kDefault;
    for (size_t i = 0; i < 5; ++i) {
        try {
            result[i] = hp["P"].at(i).get<double>();
        } catch (...) {
            result[i] = kDefault[i];
        }
    }
    return result;
}

std::array<double, 5> Config::GetHyperparameterNoP() {
    static const std::array<double, 5> kDefault = {0.432, 0.447, 0, 0.121, 0};
    if (!config_data_.contains("hyperparameter")) {
        return kDefault;
    }
    const auto &hp = config_data_["hyperparameter"];
    if (!hp.contains("noP") || !hp["noP"].is_array() || hp["noP"].size() != 5) {
        return kDefault;
    }
    std::array<double, 5> result = kDefault;
    for (size_t i = 0; i < 5; ++i) {
        try {
            result[i] = hp["noP"].at(i).get<double>();
        } catch (...) {
            result[i] = kDefault[i];
        }
    }
    return result;
}

long long Config::GetHyperparameterNpPen() {
    static const long long kDefault = 13600;
    if (!config_data_.contains("hyperparameter")) {
        return kDefault;
    }
    const auto &hp = config_data_["hyperparameter"];
    return GetOr<long long>(hp, "np_pen", kDefault);
}

long long Config::GetHyperparameterOrPen() {
    static const long long kDefault = 250;
    if (!config_data_.contains("hyperparameter")) {
        return kDefault;
    }
    const auto &hp = config_data_["hyperparameter"];
    return GetOr<long long>(hp, "or_pen", kDefault);
}

long long Config::GetHyperparameterOlPen() {
    static const long long kDefault = 100;
    if (!config_data_.contains("hyperparameter")) {
        return kDefault;
    }
    const auto &hp = config_data_["hyperparameter"];
    return GetOr<long long>(hp, "ol_pen", kDefault);
}

double Config::GetHyperparameterStTmp() {
    static const double kDefault = 17500.0;
    if (!config_data_.contains("hyperparameter")) {
        return kDefault;
    }
    const auto &hp = config_data_["hyperparameter"];
    return GetOr<double>(hp, "sttmp", kDefault);
}

double Config::GetHyperparameterEnTmp() {
    static const double kDefault = 5.0;
    if (!config_data_.contains("hyperparameter")) {
        return kDefault;
    }
    const auto &hp = config_data_["hyperparameter"];
    return GetOr<double>(hp, "entmp", kDefault);
}

int Config::GetMaxPolygonCount() {
    static const int kDefault = 5000;
    if (!config_data_.contains("canvas")) {
        return kDefault;
    }
    const auto &canvas = config_data_["canvas"];
    return GetOr<int>(canvas, "max_polygons", kDefault);
}

double Config::GetRectSimilarityCalcStep() {
    static const double kDefault = 1.0;
    if (!config_data_.contains("polygon_config")) {
        return kDefault;
    }
    const auto &polygon = config_data_["polygon_config"];
    return GetOr<double>(polygon, "rect_similarity_calc_step", kDefault);
}

double Config::GetTextureScaleDivisor() {
    static const double kDefault = 11.0;
    if (!config_data_.contains("polygon_config")) {
        return kDefault;
    }
    const auto &polygon = config_data_["polygon_config"];
    return GetOr<double>(polygon, "texture_scale_divisor", kDefault);
}

double Config::GetSeamAllowance() {
    static const double kDefault = 0.0;
    if (!config_data_.contains("polygon_config")) {
        return kDefault;
    }
    const auto &polygon = config_data_["polygon_config"];
    return GetOr<double>(polygon, "seam_allowance", kDefault);
}
