module;

#include <memory>

export module foundation.service;

export namespace caustix {
    template<typename T>
    struct Service {
        static T* instance() {
            static std::unique_ptr<T> instance;
            return instance.get();
        }
        virtual void init( void* configuration ) { }
        virtual void shutdown() { }
    };
}