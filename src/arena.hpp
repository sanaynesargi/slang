#pragma once

class ArenaAllocator {
public:
    inline explicit ArenaAllocator(size_t bytes)
        : m_size(bytes) {
        m_buffer = static_cast<std::byte*>(malloc(m_size));
        m_offset = m_buffer;
    }

    template<typename T>
    inline T* alloc() {
        void* offset = m_offset;
        m_offset += sizeof(T);
        return static_cast<T*>(offset);
    }

    // cannot be copied (any copy is just deleted)
    inline ArenaAllocator(const ArenaAllocator& other) = delete;

    // cannot be assigned to (any assignment is deleted)
    inline ArenaAllocator operator = (const ArenaAllocator& other) = delete;

    // free allocation in destructor
    inline ~ArenaAllocator() {
        free(m_buffer);
    }

private:
    size_t m_size;
    std::byte* m_buffer;
    std::byte* m_offset;
};