#include <string>

#include <Siv3D.hpp>

#include "algorithm/hyperparameter.hpp"
#include "core/canvas_model.hpp"
#include "core/polygon.hpp"
#include "ui/ui_manager.hpp"
#include "utils/config.hpp"


void Main() {
    CanvasModel &model = CanvasModel::Get();
    UIManager ui(model);

    const std::string config_path = "../assets/config.json";
    Config::LoadConfig(config_path);
    Hyperparameter::LoadFromConfig();
    model.GetRngOpt().seed(Config::GetSeed0());
    model.GetRngShapeGen().seed(Config::GetSeed1());

    while (System::Update()) {
        ui.SetMouseInfo(Cursor::Pos(), Mouse::Wheel());

        ui.CreateUI();

        ui.Operations();
    }
    
}
