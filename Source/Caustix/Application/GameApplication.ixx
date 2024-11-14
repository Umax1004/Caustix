module;

#include <imgui.h>
#include <cglm/util.h>

export module Application.GameApplication;

export import Application.Application;
import Application.Input;
import Application.Window;
import Application.Graphics.Renderer;
import Application.Graphics.GPUDevice;
import Application.Graphics.ImGuiService;
import Foundation.Services.MemoryService;
import Foundation.Services.ServiceManager;
import Foundation.Memory.MemoryDefines;
import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.Allocators.StackAllocator;
import Foundation.ResourceManager;
import Foundation.Log;
import Application.Graphics.CommandBuffer;

export namespace Caustix {
    struct GameApplication : public Application {
        GameApplication( const ApplicationConfiguration& configuration);

        void    Shutdown() override;
        bool    MainLoop() override;

        void    FixedUpdate( f32 delta ) override;
        void    VariableUpdate( f32 delta ) override;

        void    FrameBegin() override;
        void    FrameEnd() override;

        void    Render( f32 interpolation, CommandBuffer* gpuCommands ) override;

        void    OnResize( u32 new_width, u32 new_height );

        f64             m_accumulator     = 0.0;
        f64             m_currentTime     = 0.0;
        f32             m_step            = 1.0f / 60.0f;

        Window*         m_window          = nullptr;
        InputService*   m_input           = nullptr;
        Renderer*       m_renderer        = nullptr;
        ImGuiService*   m_imgui           = nullptr;
        MemoryService*  m_memoryService   = nullptr;
        GpuDevice*      m_gpu             = nullptr;
        StackAllocator  m_scratchAllocator;
    };
}

namespace Caustix {
    static void InputOsMessagesCallback(void *os_event, void *user_data) {
        InputService *input = (InputService *)user_data;
        input->OnEvent(os_event);
    }

    GameApplication::GameApplication(const ApplicationConfiguration& configuration)
    : Application(configuration)
    , m_scratchAllocator(cmega(8)){

        MemoryServiceConfiguration memoryConfiguration;
        ServiceManager::GetInstance()->AddService(MemoryService::Create(memoryConfiguration), MemoryService::m_name);
        m_memoryService = ServiceManager::GetInstance()->Get<MemoryService>();
        Allocator *allocator = &m_memoryService->m_systemAllocator;

        WindowConfiguration wconf{1280, 800, "Caustix Test", allocator};
        ServiceManager::GetInstance()->AddService(Window::Create(wconf), Window::m_name);
        m_window = ServiceManager::GetInstance()->Get<Window>();

        ServiceManager::GetInstance()->AddService(InputService::Create(allocator), InputService::m_name);
        m_input = ServiceManager::GetInstance()->Get<InputService>();

        // Callback register
        m_window->RegisterOsMessagesCallback(InputOsMessagesCallback, m_input);

        // Graphics
        DeviceCreation deviceCreation;
        deviceCreation.SetWindow(m_window->m_width, m_window->m_height, m_window->m_platformHandle).SetAllocator(allocator).SetLinearAllocator(&m_scratchAllocator);

        ServiceManager::GetInstance()->AddService(GpuDevice::Create(deviceCreation), GpuDevice::m_name);
        m_gpu = ServiceManager::GetInstance()->Get<GpuDevice>();

        ResourceManager resourceManager(allocator, nullptr);

        ServiceManager::GetInstance()->AddService(Renderer::Create({m_gpu, allocator}), Renderer::m_name);
        m_renderer = ServiceManager::GetInstance()->Get<Renderer>();
        m_renderer->SetLoaders(&resourceManager);

        ImGuiServiceConfiguration imGuiServiceConfiguration{m_gpu, m_window->m_platformHandle};
        ServiceManager::GetInstance()->AddService(ImGuiService::Create(imGuiServiceConfiguration),ImGuiService::m_name);
        m_imgui = ServiceManager::GetInstance()->Get<ImGuiService>();

        info("GameApplication created successfully!");
    }

    void GameApplication::Shutdown() {
        info("GameApplication shutdown");

        m_imgui->Shutdown();
        m_renderer->Shutdown();

        m_window->UnregisterOsMessagesCallback(InputOsMessagesCallback);
    }

    bool GameApplication::MainLoop() {
        // Fix your timestep
        m_accumulator = m_currentTime = 0.0;

        // Main loop
        while ( !m_window->m_requestedExit ) {
            // New frame
            if ( !m_window->m_minimized ) {
                m_renderer->BeginFrame();
            }
            m_input->NewFrame();

            m_window->HandleOsMessages();

            if ( m_window->m_resized ) {
                m_renderer->ResizeSwapchain( m_window->m_width, m_window->m_height );
                OnResize( m_window->m_width, m_window->m_height );
                m_window->m_resized = false;
            }
            // This MUST be AFTER os messages!
            m_imgui->NewFrame();

            // TODO: frametime
            f32 delta_time = ImGui::GetIO().DeltaTime;

            //hprint( "Dt %f\n", delta_time );
            delta_time = glm_clamp( delta_time, 0.0f, 0.25f );

            m_accumulator += delta_time;

            // Various updates
            m_input->Update( delta_time );

            while ( m_accumulator >= m_step ) {
                FixedUpdate( m_step );

                m_accumulator -= m_step;
            }

            VariableUpdate( delta_time );

            if ( !m_window->m_minimized ) {
                // Draw debug UIs
                ServiceManager::GetInstance()->Get<MemoryService>()->ImguiDraw();

                CommandBuffer* gpuCommands = m_renderer->GetCommandBuffer( QueueType::Graphics, true );
                gpuCommands->PushMarker( "Frame" );

                const f32 interpolation_factor = glm_clamp( (f32)(m_accumulator / m_step), 0.0f, 1.0f );
                Render( interpolation_factor, gpuCommands );

                m_imgui->Render( *gpuCommands );

                gpuCommands->PopMarker();

                // Send commands to GPU
                m_renderer->QueueCommandBuffer( gpuCommands );

                m_renderer->EndFrame();
            }
            else {
                ImGui::Render();
            }

            // Prepare for next frame if anything must be done.
            FrameEnd();
        }

        return true;
    }

    void GameApplication::FixedUpdate(f32 delta)
    {

    }

    void GameApplication::VariableUpdate(f32 delta)
    {

    }

    void GameApplication::Render(f32 interpolation, CommandBuffer* gpuCommands)
    {

    }

    void GameApplication::OnResize(u32 new_width, u32 new_height)
    {

    }

    void GameApplication::FrameBegin()
    {

    }

    void GameApplication::FrameEnd()
    {

    }
}
