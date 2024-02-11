export module Application.Application;

export import Foundation.Platform;
import Foundation.Services.ServiceManager;

export namespace Caustix {
    struct ApplicationConfiguration {
        u32                         m_width;
        u32                         m_height;

        cstring                     m_name = nullptr;

        bool                        init_base_services  = false;

        ApplicationConfiguration&   Width( u32 value ) { m_width = value; return *this; }
        ApplicationConfiguration&   Height( u32 value ) { m_height = value; return *this; }
        ApplicationConfiguration&   Name( cstring value ) { m_name = value; return *this; }
    };

    struct Application {
        explicit Application( const ApplicationConfiguration& configuration ) {}

        virtual void                Shutdown() {}
        virtual bool                MainLoop() { return false; }

        // Fixed update. Can be called more than once compared to rendering.
        virtual void                FixedUpdate( f32 delta ) {}
        // Variable time update. Called only once per frame.
        virtual void                VariableUpdate( f32 delta ) {}
        // Rendering with optional interpolation factor.
        virtual void                Render( f32 interpolation ) {}
        // Per frame begin/end.
        virtual void                FrameBegin() {}
        virtual void                FrameEnd() {}

        void                        Run();
    };
}

namespace Caustix {
    void Application::Run() {
        MainLoop();
    }
}