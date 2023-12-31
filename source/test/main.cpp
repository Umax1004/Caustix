#include <iostream>
#include <vector>
#include <map>

import Foundation.Assert;
import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.Allocators.LinearAllocator;
import Foundation.Memory.Allocators.StackAllocator;
import Foundation.Memory.MemoryDefines;
import Foundation.Services.MemoryService;
import Foundation.Services.ServiceManager;
import Foundation.Services.Service;

import Application.Input;
import Application.Window;
import Application.Graphics.GPUDevice;

using StringBuffer = std::basic_string<char, std::char_traits<char>, Caustix::STLAdaptor<char>>;

static void InputOsMessagesCallback( void* os_event, void* user_data ) {
    Caustix::InputService* input = ( Caustix::InputService* )user_data;
    input->OnEvent( os_event );
}

int main( int argc, char** argv ) {
    Caustix::StackAllocator scratchAllocator(Caustix::cmega(8));

    Caustix::MemoryServiceConfiguration configuration;
    Caustix::ServiceManager::GetInstance()->AddService(Caustix::MemoryService::Create(configuration), Caustix::MemoryService::m_name);
    Caustix::Allocator* allocator = &Caustix::ServiceManager::GetInstance()->Get<Caustix::MemoryService>()->m_systemAllocator;

    Caustix::WindowConfiguration wconf{ 1280, 800, "Caustix Test", allocator };
    Caustix::ServiceManager::GetInstance()->AddService(Caustix::Window::Create(wconf), Caustix::Window::m_name);
    Caustix::Window* window = Caustix::ServiceManager::GetInstance()->Get<Caustix::Window>();

    Caustix::ServiceManager::GetInstance()->AddService(Caustix::InputService::Create(allocator), Caustix::InputService::m_name);
    Caustix::InputService* inputHandler = Caustix::ServiceManager::GetInstance()->Get<Caustix::InputService>();

    // Callback register
    window->RegisterOsMessagesCallback(InputOsMessagesCallback, inputHandler);

    // Graphics
    Caustix::DeviceCreation deviceCreation;
    deviceCreation.SetWindow( window->m_width, window->m_height, window->m_platformHandle ).SetAllocator( allocator ).SetLinearAllocator( &scratchAllocator );
    Caustix::ServiceManager::GetInstance()->AddService(Caustix::GpuDevice::Create(deviceCreation), Caustix::GpuDevice::m_name);
    Caustix::GpuDevice* gpu = Caustix::ServiceManager::GetInstance()->Get<Caustix::GpuDevice>();

    window->UnregisterOsMessagesCallback(InputOsMessagesCallback);
    return 0;
}