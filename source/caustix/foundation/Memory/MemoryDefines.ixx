module;

#include <concepts>
#include <source_location>

export module Foundation.Memory.MemoryDefines;

import Foundation.Memory.Allocators.Allocator;

export namespace Caustix {
    template <typename A>
    requires std::derived_from<A, Allocator>
    void* calloca(sizet size, A* allocator,  const std::source_location location = std::source_location::current()) {
        return allocator->allocate( size, 1, location.file_name(), location.line() );
    }

    template <typename A>
    requires std::derived_from<A, Allocator>
    u8* callocam(sizet size, A* allocator,  const std::source_location location = std::source_location::current()) {
        return static_cast<u8*>(allocator->allocate( size, 1, location.file_name(), location.line() ));
    }

    template <typename A>
    requires std::derived_from<A, Allocator>
    void cfree(void* pointer, A* allocator){
        allocator->deallocate(pointer);
    }

    constexpr sizet ckilo(sizet size) {
        return size * 1024;
    }

    constexpr sizet cmega(sizet size) {
        return size * 1024 * 1024;
    }

    constexpr sizet cgiga(sizet size) {
        return size * 1024 * 1024 * 1024;
    }
}