#include "Input.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/limits.h>

Input::Input()
: _fd(-1)
{
}

Input::Input(int fd,
             char delim,
             size_t maxBufferSize)
: _fd(fd),
  _delim(delim),
  _buffer(maxBufferSize + 1),
  _maxBufferSize(maxBufferSize)
{
    *reinterpret_cast<char *>(_buffer.data()) = '\0';
    _buffer.resize(1);
    _prevNewline = reinterpret_cast<char *>(_buffer.data()) + _buffer.size();
}

Input::~Input() {
    /* empty */
}

bool Input::readUntilNewline(char **line, ssize_t *len) {
    bool result;

    do {
        result = readAndDetectNewline(line, len);
    } while (!result && (len > 0) && line);

    return result;
}

bool Input::readAndDetectNewline(char **line, ssize_t *len) {
    if (_fd < 0) {
        *line = NULL;
        *len = 0;
        return false;
    }

    bool newlineDetected = (reinterpret_cast<char *>(_buffer.data()) != _prevNewline);

    if (_prevNewline >= (reinterpret_cast<const char *>(_buffer.data()) + _buffer.size()) - 1) {
        ssize_t bytesRead = _readAnotherBlock();

        if (0 == bytesRead) {
            *line = NULL;
            *len = 0;
            return false;
        }

        if (0 > bytesRead) {
            *line = NULL;
            *len = -1;
            return false;
        }
    }

    char *b = _prevNewline;
    _prevNewline = strchr(b, _delim);

    *line = b;

    if (newlineDetected) {
        /* b is the new line start */

        if (_prevNewline) {
            *len = _prevNewline - b + 1;
            *_prevNewline = '\0';
            ++_prevNewline;
        }
        else {
            *len = _buffer.size() - (b - reinterpret_cast<char *>(_buffer.data())) + 1;
        }

        return true;
    }

    /* b is not a new line start */
    if (_prevNewline) {
        *len = _prevNewline - b + 1;
        *_prevNewline = '\0';
        ++_prevNewline;
    }
    else {
        *len = _buffer.size() - (b - reinterpret_cast<char *>(_buffer.data())) + 1;

        _prevNewline = reinterpret_cast<char *>(_buffer.data()) + _buffer.size();
    }

    return false;
}

ssize_t Input::_readAnotherBlock() {
    if (!_buffer.resize(_maxBufferSize)) {
        errno = ENOMEM;
        return -1;
    }

    ssize_t readCount = read(_fd, _buffer.data(), _buffer.size());

    if (readCount <= 0) {
        return readCount;
    }

    _buffer.resize(readCount + 1);
    reinterpret_cast<char *>(_buffer.data())[readCount] = '\0';

    _prevNewline = reinterpret_cast<char *>(_buffer.data());

    return readCount;
}

