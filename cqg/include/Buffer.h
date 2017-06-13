#ifndef BUFFER_H
# define BUFFER_H

#include <stddef.h>
#include <sys/types.h>

/**
 * Buffer class.
 * Implements non-shrinkable policy buffer i.e. tries to only expand
 * and thus --- reduces realloc calls and memory fragmentation.
 */
class Buffer {
protected:
    size_t _size;                                           ///< allocated size
    size_t _userSize;                                       ///< size to report to user
    void *_data;                                            ///< allocated block
    off_t _offset;
    bool _invalid;

    void _deallocate();

public:
    /**
     * C-tor
     * \param initialSize initial buffer size
     * Nil initial size is permitted
     */
    Buffer(size_t initialSize = 0);
    Buffer(size_t userSize, size_t realSize, void *data);
    Buffer(size_t userSize, size_t realSize, void *data, off_t offset);
    ~Buffer();

    Buffer(Buffer &&rhs);
    Buffer(const Buffer &) = delete;

    Buffer &operator=(const Buffer &) = delete;
    Buffer &operator=(Buffer &&rhs);

    void *release();

    /**
     * Report user defined buffer size
     * i.e. the size which with \c resize
     * was called for the last time
     */
    size_t size() const;
    size_t realSize() const;

    /**
     * Fetch data pointer
     */
    void* data();

    /**
     * Resize buffer.
     * \param newSize buffer new size
     * Buffer won't shrink if \c newSize is
     * less than current buffer size but will
     * report \c newSize to user on call to \c size().
     */
    bool resize(size_t newSize);

    /**
     * For those who operate with the buffer
     * w/o its size changing we provide this fancy API
     */
    off_t offset(void) const;
    bool offset(off_t off);
    bool shift(off_t sft);
};

#endif /* BUFFER_H */
