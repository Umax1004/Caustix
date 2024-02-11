import Application.GameApplication;

int main(int argc, char **argv) {
    using namespace Caustix;

    ApplicationConfiguration configuration;
    configuration.m_height = 800;
    configuration.m_width = 1280;
    configuration.m_name = "Caustix Demo Application";
    GameApplication game_application(configuration);
    game_application.Run();
    game_application.Shutdown();
}