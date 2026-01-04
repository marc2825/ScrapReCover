#include <Siv3D.hpp>

#include "algorithm/hyperparameter.hpp"
#include "core/canvas_model.hpp"
#include "core/polygon.hpp"
#include "ui/ui_manager.hpp"
#include "utils/config.hpp"


void Main() {
    Config::LoadConfig("../assets/config.json"); // pwdからの相対位置->変える
    CanvasModel &model = CanvasModel::Get();
    model.GetRngOpt().seed(Config::GetSeed0());
    model.GetRngShapeGen().seed(Config::GetSeed1());
    Hyperparameter::LoadFromConfig();

    UIManager ui(model);
    MyPolygon::Initialize();

    while (System::Update()) {
        ui.SetMouseInfo(Cursor::Pos(), Mouse::Wheel());

        ui.CreateUI();

        ui.Operations();
    }
    
}
