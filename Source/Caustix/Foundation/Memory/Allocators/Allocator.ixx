export module Foundation.Memory.Allocators.Allocator;

export import Foundation.Platform;
export import Foundation.Assert;

export namespace Caustix {
    struct Allocator {
        virtual ~Allocator() = default;
        virtual void*   allocate( sizet size, sizet alignment ) = 0;
        virtual void*   allocate( sizet size, sizet alignment, cstring file, i32 line ) = 0;

        virtual void    deallocate( void* pointer ) = 0;
    };

    sizet MemoryAlign( sizet size, sizet alignment ) {
        const sizet alignment_mask = alignment - 1;
        return ( size + alignment_mask ) & ~alignment_mask;
    }

    template<typename T>
    class STLAdaptor
    {
    public:
        typedef T value_type;

        STLAdaptor() = delete;

        STLAdaptor(Allocator& allocator) noexcept
                :
                m_allocator(allocator)
        {

        }

        template<typename U>
        STLAdaptor(const STLAdaptor<U>& other) noexcept
                :
                m_allocator(other.m_allocator)
        {}

        [[nodiscard]] constexpr T* allocate(sizet n)
        {
            return reinterpret_cast<T*>
            (m_allocator.allocate(n * sizeof(T), sizeof(T)));
        }

        constexpr void deallocate(T* p, [[maybe_unused]] sizet n)
        noexcept
        {
            m_allocator.deallocate(p);
        }

        bool operator==(const STLAdaptor<T>& rhs) const noexcept
        {
            return *this == rhs;
        }

        bool operator!=(const STLAdaptor<T>& rhs) const noexcept
        {
            return *this != rhs;
        }


        Allocator& m_allocator;
    };
}

