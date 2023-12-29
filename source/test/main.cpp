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

using StringBuffer = std::basic_string<char, std::char_traits<char>, Caustix::STLAdaptor<char>>;

int main( int argc, char** argv ) {
    Caustix::StackAllocator instanceAllocator(Caustix::cmega(2));
    Caustix::ServiceManager::GetInstance()->AddService(Caustix::MemoryService::Create({Caustix::cmega(2)}), Caustix::MemoryService::m_name);
    Caustix::Allocator* allocator = &Caustix::ServiceManager::GetInstance()->Get<Caustix::MemoryService>()->m_systemAllocator;

    Caustix::WindowConfiguration wconf{ 1280, 800, "Raptor Test", allocator };
    Caustix::ServiceManager::GetInstance()->AddService(Caustix::Window::Create(wconf), Caustix::Window::m_name);
    Caustix::Window* window = Caustix::ServiceManager::GetInstance()->Get<Caustix::Window>();

    Caustix::ServiceManager::GetInstance()->AddService(Caustix::InputService::Create(allocator), Caustix::InputService::m_name);
    Caustix::InputService* inputHandler = Caustix::ServiceManager::GetInstance()->Get<Caustix::InputService>();
//    window->RegisterOsMessagesCallback()
//    auto memory = Caustix::MemoryService::GetInstance();
//    Caustix::StackAllocator scratchAllocator(Caustix::cmega(8));
    return 0;
}