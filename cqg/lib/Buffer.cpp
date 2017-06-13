#include "Buffer.h"

#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>

#include <stdexcept>
#include <exception>

Buffer::Buffer(size_t initialSize)
: _size(initialSize),
  _userSize(initialSize),
  _data(NULL),
  _offset(0),
  _invalid(false)
{
    if (initialSize) {
        _data = malloc(initialSize);
        if (!_data) {
            _invalid = true;
        }
    }
}

Buffer::Buffer(size_t userSize, size_t realSize, void *data)
: _size(realSize),
  _userSize(userSize),
  _data(data),
  _offset(0),
  _invalid(false)
{
    if (_size < _userSize || (_size && !_data)) {
        _invalid = true;
    }
}

Buffer::Buffer(size_t userSize, size_t realSize, void *data, off_t offset)
: _size(realSize),
  _userSize(userSize),
  _data(data),
  _offset(offset),
  _invalid(false)
{
    if (_size < _userSize || (_size && !_data)) {
        _invalid = true;
    }
}

Buffer& Buffer::operator=(Buffer&& rhs) {
    if (this != &rhs) {
        _deallocate();

        _size = rhs._size;
        _userSize = rhs._userSize;
        _data = rhs._data;
        _offset = rhs._offset;

        rhs._size = rhs._userSize = 0;
        rhs._data = NULL;
        rhs._offset = 0;
    }

    return *this;
}

Buffer::Buffer(Buffer &&that)
: _size(that._size),
  _userSize(that._userSize),
  _data(that._data),
  _offset(that._offset)
{
    that._size = that._userSize = 0;
    that._data = NULL;
    that._offset = 0;
}

Buffer::~Buffer() {
    _deallocate();
}

void Buffer::_deallocate() {
    if (_data) {
        free(_data);
    }
}

void *Buffer::release() {
    void *r = _data;
    _size = _userSize = 0;
    _data = NULL;
    return r;
}

bool Buffer::resize(size_t newSize) {
    if (newSize <= _size) {
        _userSize = newSize;
        if (_offset > _userSize) {
            _offset = _userSize;
        }

        return true;
    }

    void *newBuf = realloc(_data, newSize);
    if (!newBuf) {
        _invalid = true;
        return false;
    }

    _size = _userSize = newSize;
    _data = newBuf;

    return true;
}

void *Buffer::data() {
    return _data;
}

size_t Buffer::size() const {
    return _userSize;
}

size_t Buffer::realSize() const {
    return _size;
}

bool Buffer::offset(off_t off) {
    if (off < 0 || off > _userSize) {
        _invalid = true;
        return false;
    }

    _offset = off;

    return true;
}

off_t Buffer::offset(void) const {
    return _offset;
}

bool Buffer::shift(off_t sft) {
    off_t offset = _offset + sft;
    if (offset < 0 || offset > _userSize) {
        _invalid = true;
        return false;
    }

    _offset = offset;

    return true;
}
