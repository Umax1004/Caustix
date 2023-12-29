module;

#include <unordered_map>
#include <string>
#include <memory>

export module Foundation.Services.ServiceManager;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Services.Service;
import Foundation.Platform;
import Foundation.Assert;
import Foundation.Log;

export namespace Caustix {
    using MapAllocator = STLAdaptor<std::pair<const u64, Service*>>;
    struct ServiceManager {
        ServiceManager() = default;

        void AddService(Service* service, cstring name);
        void RemoveService(cstring name);

        Service* GetService(cstring name);

        template<typename T>
        T* Get();

        static ServiceManager* GetInstance() {
            static std::unique_ptr<ServiceManager> instance{new ServiceManager{}};
            return instance.get();
        }

        std::unordered_map<u64, Service*> m_services;

        std::hash<cstring> m_hasher;
    };
}

namespace Caustix {
    void ServiceManager::AddService(Service* service, cstring name) {
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
            error("{} not found in the Service Manager", T::m_name);
            CASSERT(false);
        }
        return service;
    }
}