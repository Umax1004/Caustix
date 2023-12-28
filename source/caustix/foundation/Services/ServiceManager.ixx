module;

#include <map>
#include <string>
#include <memory>

export module Foundation.Services.ServiceManager;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Services.Service;
import Foundation.Platform;

export namespace Caustix {
    using MapAllocator = STLAdaptor<std::pair<const u64, Service*>>;
    struct ServiceManager {
        ServiceManager(Allocator* allocator);

        void AddService(Service* service, cstring name);
        void RemoveService(cstring name);

        Service* GetService(cstring name);

        template<typename T>
        T* Get();

        static ServiceManager* GetInstance() {
            static std::unique_ptr<ServiceManager> instance;
            return instance.get();
        }

        std::map<u64, Service*, std::less<u64>, MapAllocator> m_services;

        Allocator* m_allocator = nullptr;

        std::hash<cstring> m_hasher;
    };
}

namespace Caustix {
    ServiceManager::ServiceManager(Allocator *allocator)
    : m_allocator(allocator)
    , m_services(*allocator) {}

    void ServiceManager::AddService(Caustix::Service *service, cstring name) {
        u64 hashName = m_hasher(name);
        m_services.emplace(hashName, service);
    }

    void ServiceManager::RemoveService(cstring name) {
        u64 hashName = m_hasher(name);
        m_services.erase(hashName);
    }

    Service* ServiceManager::GetService(cstring name) {
        u64 hashName = m_hasher(name);
        return m_services.at(hashName);
    }

    template<typename T>
    inline T* ServiceManager::Get() {
        T* service = ( T* )GetService( T::m_name );
        if ( !service ) {
            AddService( T::GetInstance(), T::m_name );
        }

        return T::GetInstance();
    }
}