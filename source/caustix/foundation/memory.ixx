module;

#include <concepts>
#include <cstdlib>
#include <source_location>
#include "external/tlsf/tlsf.h"

#define CAUSTIX_IMGUI

#if defined CAUSTIX_IMGUI
#include "imgui.h"
#endif

export module foundation.memory;

export import foundation.platform;
import foundation.service;
import foundation.assert;
import foundation.log;


export namespace caustix {

    // Memory Methods /////////////////////////////////////////////////////
    void    memory_copy( void* destination, void* source, sizet size );

    //  Calculate aligned memory size.
    sizet   memory_align( sizet size, sizet alignment );

    // Memory Structs /////////////////////////////////////////////////////
    struct MemoryStatistics {
        sizet   allocated_bytes;
        sizet   total_bytes;

        u32     allocation_count;

        void add( sizet a ) {
            if ( a ) {
                allocated_bytes += a;
                ++allocation_count;
            }
        }
    };

     struct Allocator {
        virtual ~Allocator() { }
        virtual void*   allocate( sizet size, sizet alignment ) = 0;
        virtual void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) = 0;

        virtual void    deallocate( void* pointer ) = 0;
    };

    struct HeapAllocator : public Allocator {

        ~HeapAllocator() override = default;

        void    init( sizet size );
        void    shutdown();

#if defined CAUSTIX_IMGUI
        void    debug_ui();
#endif

        void*   allocate( sizet size, sizet alignment ) override;
        void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) override;

        void    deallocate( void* pointer ) override;

        void*   tlsf_handle;
        void*   memory;
        sizet   allocated_size = 0;
        sizet   max_size = 0;

    };

    struct StackAllocator : public Allocator {

        void    init( sizet size );
        void    shutdown();

        void*   allocate( sizet size, sizet alignment ) override;
        void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) override;

        void    deallocate( void* pointer ) override;

        [[nodiscard]] sizet   get_marker() const;
        void    free_marker( sizet marker );

        void    clear();

        u8*     memory          = nullptr;
        sizet   total_size      = 0;
        sizet   allocated_size  = 0;

    };

    struct DoubleStackAllocator : public Allocator {

        void    init( sizet size );
        void    shutdown();

        void*   allocate( sizet size, sizet alignment ) override;
        void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) override;
        void    deallocate( void* pointer ) override;

        void*   allocate_top( sizet size, sizet alignment );
        void*   allocate_bottom( sizet size, sizet alignment );

        void    deallocate_top( sizet size );
        void    deallocate_bottom( sizet size );

        sizet   get_top_marker();
        sizet   get_bottom_marker();

        void    free_top_marker( sizet marker );
        void    free_bottom_marker( sizet marker );

        void    clear_top();
        void    clear_bottom();

        u8*     memory          = nullptr;
        sizet   total_size      = 0;
        sizet   top             = 0;
        sizet   bottom          = 0;

    };

    //
    // Allocator that can only be reset.
    //
    struct LinearAllocator : public Allocator {

        ~LinearAllocator() override = default;

        void    init( sizet size );
        void    shutdown();

        void*   allocate( sizet size, sizet alignment ) override;
        void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) override;

        void    deallocate( void* pointer ) override;

        void    clear();

        u8*     memory          = nullptr;
        sizet   total_size      = 0;
        sizet   allocated_size  = 0;
    };

    //
    // DANGER: this should be used for NON runtime processes, like compilation of resources.
    struct MallocAllocator : public Allocator {
        void*   allocate( sizet size, sizet alignment ) override;
        void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) override;

        void    deallocate( void* pointer ) override;
    };

    // Memory Service /////////////////////////////////////////////////////
    //
    //
    struct MemoryServiceConfiguration {
        sizet                       maximum_dynamic_size = 32 * 1024 * 1024;    // Defaults to max 32MB of dynamic memory.
    };

    struct MemoryService : public Service<MemoryService> {

        void    init( void* configuration );
        void    shutdown();

#if defined CAUSTIX_IMGUI
        void    imgui_draw();
#endif

        // Frame allocator
        LinearAllocator     scratch_allocator;
        HeapAllocator       system_allocator;

        //
        // Test allocators.
        void    test();

        static constexpr cstring    k_name = "caustix_memory_service";

    };

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

// Define this and add StackWalker to heavy memory profile
//#define CAUSTIX_MEMORY_STACK

#define HEAP_ALLOCATOR_STATS

#if defined (CAUSTIX_MEMORY_STACK)
#include "external/StackWalker.h"
#endif

namespace caustix {
    //#define CAUSTIX_MEMORY_DEBUG
#if defined (CAUSTIX_MEMORY_DEBUG)
    #define hy_mem_assert(cond) hy_assert(cond)
#else
    #define hy_mem_assert(cond)
#endif

    static size_t s_size = cmega(32) + tlsf_size() + 8;

    // Walker methods
    static void exit_walker( void* ptr, size_t size, int used, void* user );
    static void imgui_walker( void* ptr, size_t size, int used, void* user );

    void MemoryService::init( void* configuration ) {

        info( "Memory Service Init\n" );
        MemoryServiceConfiguration* memory_configuration = static_cast< MemoryServiceConfiguration* >( configuration );
        system_allocator.init( memory_configuration ? memory_configuration->maximum_dynamic_size : s_size );
    }

    void MemoryService::shutdown() {

        system_allocator.shutdown();

        info( "Memory Service Shutdown\n" );
    }

    void exit_walker( void* ptr, size_t size, int used, void* user ) {
        MemoryStatistics* stats = ( MemoryStatistics* )user;
        stats->add( used ? size : 0 );

        if ( used )
            info( "Found active allocation %p, %llu\n", ptr, size );
    }

#if defined CAUSTIX_IMGUI
    void imgui_walker( void* ptr, size_t size, int used, void* user ) {

        u32 memory_size = ( u32 )size;
        cstring memory_unit = "b";
        if ( memory_size > 1024 * 1024 ) {
            memory_size /= 1024 * 1024;
            memory_unit = "Mb";
        }
        else if ( memory_size > 1024 ) {
            memory_size /= 1024;
            memory_unit = "kb";
        }
        ImGui::Text( "\t%p %s size: %4llu %s\n", ptr, used ? "used" : "free", memory_size, memory_unit );

        MemoryStatistics* stats = ( MemoryStatistics* )user;
        stats->add( used ? size : 0 );
    }


    void MemoryService::imgui_draw() {

        if ( ImGui::Begin( "Memory Service" ) ) {

            system_allocator.debug_ui();
        }
        ImGui::End();
    }
#endif

    void HeapAllocator::init( sizet size ) {
        // Allocate
        memory = malloc( size );
        max_size = size;
        allocated_size = 0;

        tlsf_handle = tlsf_create_with_pool( memory, size );

        info( "HeapAllocator of size %llu created\n", size );
    }

    void HeapAllocator::shutdown() {

        // Check memory at the application exit.
        MemoryStatistics stats{ 0, max_size };
        pool_t pool = tlsf_get_pool( tlsf_handle );
        tlsf_walk_pool( pool, exit_walker, ( void* )&stats );

        if ( stats.allocated_bytes ) {
            error( "HeapAllocator Shutdown.\n===============\nFAILURE! Allocated memory detected. allocated %llu, total %llu\n===============\n\n", stats.allocated_bytes, stats.total_bytes );
        } else {
            info( "HeapAllocator Shutdown - all memory free!\n" );
        }

        // TODO : Find good alternative for RASSERTM
        CASSERT(stats.allocated_bytes == 0);
//        RASSERTM( stats.allocated_bytes == 0, "Allocations still present. Check your code!" );

        tlsf_destroy( tlsf_handle );

        free( memory );
    }

#if defined CAUSTIX_IMGUI
    void HeapAllocator::debug_ui() {

        ImGui::Separator();
        ImGui::Text( "Heap Allocator" );
        ImGui::Separator();
        MemoryStatistics stats{ 0, max_size };
        pool_t pool = tlsf_get_pool( tlsf_handle );
        tlsf_walk_pool( pool, imgui_walker, ( void* )&stats );

        ImGui::Separator();
        ImGui::Text( "\tAllocation count %d", stats.allocation_count );
        ImGui::Text( "\tAllocated %llu K, free %llu Mb, total %llu Mb", stats.allocated_bytes / (1024 * 1024), ( max_size - stats.allocated_bytes ) / ( 1024 * 1024 ), max_size / ( 1024 * 1024 ) );
    }
#endif

#if defined (CAUSTIX_MEMORY_STACK)
    class RaptorStackWalker : public StackWalker {
    public:
        RaptorStackWalker() : StackWalker() {}
    protected:
        virtual void OnOutput( LPCSTR szText ) {
            rprint( "\nStack: \n%s\n", szText );
            StackWalker::OnOutput( szText );
        }
    }; // class RaptorStackWalker

    void* HeapAllocator::allocate( sizet size, sizet alignment ) {

        /*if ( size == 16 )
        {
            RaptorStackWalker sw;
            sw.ShowCallstack();
        }*/

        void* mem = tlsf_malloc( tlsf_handle, size );
        rprint( "Mem: %p, size %llu \n", mem, size );
        return mem;
    }
#else
    void* HeapAllocator::allocate( sizet size, sizet alignment ) {
#if defined (HEAP_ALLOCATOR_STATS)
        void* allocated_memory = alignment == 1 ? tlsf_malloc( tlsf_handle, size ) : tlsf_memalign( tlsf_handle, alignment, size );
        sizet actual_size = tlsf_block_size( allocated_memory );
        allocated_size += actual_size;

        /*if ( size == 52224 ) {
            return allocated_memory;
        }*/
        return allocated_memory;
#else
        return tlsf_malloc( tlsf_handle, size );
#endif // HEAP_ALLOCATOR_STATS
    }
#endif

    void* HeapAllocator::allocate( sizet size, sizet alignment, cstring file, i32 line ) {
        return allocate( size, alignment );
    }

    void HeapAllocator::deallocate( void* pointer ) {
#if defined (HEAP_ALLOCATOR_STATS)
        sizet actual_size = tlsf_block_size( pointer );
        allocated_size -= actual_size;

        tlsf_free( tlsf_handle, pointer );
#else
        tlsf_free( tlsf_handle, pointer );
#endif
    }

    // LinearAllocator /////////////////////////////////////////////////////////
    void LinearAllocator::init( sizet size ) {

        memory = ( u8* )malloc( size );
        total_size = size;
        allocated_size = 0;
    }

    void LinearAllocator::shutdown() {
        clear();
        free( memory );
    }

    void* LinearAllocator::allocate( sizet size, sizet alignment ) {
        CASSERT( size > 0 );

        const sizet new_start = memory_align( allocated_size, alignment );
        CASSERT( new_start < total_size );
        const sizet new_allocated_size = new_start + size;
        if ( new_allocated_size > total_size ) {
            hy_mem_assert( false && "Overflow" );
            return nullptr;
        }

        allocated_size = new_allocated_size;
        return memory + new_start;
    }

    void* LinearAllocator::allocate( sizet size, sizet alignment, cstring file, i32 line ) {
        return allocate( size, alignment );
    }

    void LinearAllocator::deallocate( void*  ) {
        // This allocator does not allocate on a per-pointer base!
    }

    void LinearAllocator::clear() {
        allocated_size = 0;
    }

    // Memory Methods /////////////////////////////////////////////////////////
    void memory_copy( void* destination, void* source, sizet size ) {
        memcpy( destination, source, size );
    }

    sizet memory_align( sizet size, sizet alignment ) {
        const sizet alignment_mask = alignment - 1;
        return ( size + alignment_mask ) & ~alignment_mask;
    }

    // MallocAllocator ///////////////////////////////////////////////////////
    void* MallocAllocator::allocate( sizet size, sizet alignment ) {
        return malloc( size );
    }

    void* MallocAllocator::allocate( sizet size, sizet alignment, cstring file, i32 line ) {
        return malloc( size );
    }

    void MallocAllocator::deallocate( void* pointer ) {
        free( pointer );
    }

    // StackAllocator ////////////////////////////////////////////////////////
    void StackAllocator::init( sizet size ) {
        memory = (u8*)malloc( size );
        allocated_size = 0;
        total_size = size;
    }

    void StackAllocator::shutdown() {
        free( memory );
    }

    void* StackAllocator::allocate( sizet size, sizet alignment ) {
        CASSERT( size > 0 );

        const sizet new_start = memory_align( allocated_size, alignment );
        CASSERT( new_start < total_size );
        const sizet new_allocated_size = new_start + size;
        if ( new_allocated_size > total_size ) {
            hy_mem_assert( false && "Overflow" );
            return nullptr;
        }

        allocated_size = new_allocated_size;
        return memory + new_start;
    }

    void* StackAllocator::allocate( sizet size, sizet alignment, cstring file, i32 line ) {
        return allocate( size, alignment );
    }

    void StackAllocator::deallocate( void* pointer ) {

        CASSERT( pointer >= memory );
        CASSERT( pointer < memory + total_size );
        CASSERT( pointer < memory + allocated_size );
//        RASSERTM( pointer < memory + total_size, "Out of bound free on linear allocator (outside bounds). Tempting to free %p, %llu after beginning of buffer (memory %p size %llu, allocated %llu)", ( u8* )pointer, ( u8* )pointer - memory, memory, total_size, allocated_size );
//        RASSERTM( pointer < memory + allocated_size, "Out of bound free on linear allocator (inside bounds, after allocated). Tempting to free %p, %llu after beginning of buffer (memory %p size %llu, allocated %llu)", ( u8* )pointer, ( u8* )pointer - memory, memory, total_size, allocated_size );

        const sizet size_at_pointer = ( u8* )pointer - memory;

        allocated_size = size_at_pointer;
    }

    sizet StackAllocator::get_marker() const {
        return allocated_size;
    }

    void StackAllocator::free_marker( sizet marker ) {
        const sizet difference = marker - allocated_size;
        if ( difference > 0 ) {
            allocated_size = marker;
        }
    }

    void StackAllocator::clear() {
        allocated_size = 0;
    }

    // DoubleStackAllocator //////////////////////////////////////////////////
    void DoubleStackAllocator::init( sizet size ) {
        memory = ( u8* )malloc( size );
        top = size;
        bottom = 0;
        total_size = size;
    }

    void DoubleStackAllocator::shutdown() {
        free( memory );
    }

    void* DoubleStackAllocator::allocate( sizet size, sizet alignment ) {
        CASSERT( false );
        return nullptr;
    }

    void* DoubleStackAllocator::allocate( sizet size, sizet alignment, cstring file, i32 line ) {
        CASSERT( false );
        return nullptr;
    }

    void DoubleStackAllocator::deallocate( void* pointer ) {
        CASSERT( false );
    }

    void* DoubleStackAllocator::allocate_top( sizet size, sizet alignment ) {
        CASSERT( size > 0 );

        const sizet new_start = memory_align( top - size, alignment );
        if ( new_start <= bottom  ) {
            hy_mem_assert( false && "Overflow Crossing" );
            return nullptr;
        }

        top = new_start;
        return memory + new_start;
    }

    void* DoubleStackAllocator::allocate_bottom( sizet size, sizet alignment ) {
        CASSERT( size > 0 );

        const sizet new_start = memory_align( bottom, alignment );
        const sizet new_allocated_size = new_start + size;
        if ( new_allocated_size >= top ) {
            hy_mem_assert( false && "Overflow Crossing" );
            return nullptr;
        }

        bottom = new_allocated_size;
        return memory + new_start;
    }

    void DoubleStackAllocator::deallocate_top( sizet size ) {
        if ( size > total_size - top ) {
            top = total_size;
        } else {
            top += size;
        }
    }

    void DoubleStackAllocator::deallocate_bottom( sizet size ) {
        if ( size > bottom ) {
            bottom = 0;
        } else {
            bottom -= size;
        }
    }

    sizet DoubleStackAllocator::get_top_marker() {
        return top;
    }

    sizet DoubleStackAllocator::get_bottom_marker() {
        return bottom;
    }

    void DoubleStackAllocator::free_top_marker( sizet marker ) {
        if ( marker > top && marker < total_size ) {
            top = marker;
        }
    }

    void DoubleStackAllocator::free_bottom_marker( sizet marker ) {
        if ( marker < bottom ) {
            bottom = marker;
        }
    }

    void DoubleStackAllocator::clear_top() {
        top = total_size;
    }

    void DoubleStackAllocator::clear_bottom() {
        bottom = 0;
    }
}