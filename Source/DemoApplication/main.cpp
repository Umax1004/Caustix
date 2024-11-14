#include <filesystem>

import DemoApplication;

import Foundation.Log;
import Foundation.glTF;

int main(int argc, char **argv) {
    using namespace Caustix;

    if (argc < 2) {
        info("Usage: chapter1 [path to glTF model]");
        auto data = std::filesystem::current_path();
        // info("{}", data.c_str());
        InjectDefault3DModel(argc, argv);
    }

    ApplicationConfiguration configuration;
    configuration.m_height = 800;
    configuration.m_width = 1280;
    configuration.m_name = "Caustix Demo Application";
    DemoApplication gameApplication(configuration, argv);
    gameApplication.Run();
    gameApplication.Shutdown();
}