#include "Input.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>

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
  _buffer(maxBufferSize),
  _maxBufferSize(maxBufferSize)
{
    *reinterpret_cast<char *>(_buffer.data()) = '\0';
    _buffer.resize(1);
    _prevNewline = reinterpret_cast<char *>(_buffer.data()) + _buffer.size();
}

Input::~Input() {
    /* empty */
}

bool Input::readUntilNewline(char **line, ssize_t *len,
                             bool alreadyNewLine) {
    bool result;

    if (!alreadyNewLine) {
        do {
            result = readAndDetectNewline(line, len);
        } while (!result && *line);
    }

    if (!(*line))
    {
        /* some failure obviously has hapened */
        return false;
    }

    return readAndDetectNewline(line, len);
}

void Input::start(char **line, ssize_t *len) {
    if (_fd < 0) {
        errno = EBADF;
        *line = NULL;
        *len = 0;
        return;
    }

    *line = reinterpret_cast<char *>(_buffer.data());
    *len = 0;
}

bool Input::readAndDetectNewline(char **line, ssize_t *len) {
    if (_fd < 0) {
        errno = EBADF;
        *line = NULL;
        *len = 0;
        return false;
    }

    const char *bufferStart = reinterpret_cast<const char *>(_buffer.data());
    const char *bufferEnd = bufferStart + _buffer.size();

    /* read another chunk if required */
    if (!_prevNewline || (_prevNewline >= bufferEnd)) {
        ssize_t bytesRead = _readAnotherBlock();

        if (0 == bytesRead) {
            /* an eof obviously */
            *line = NULL;
            *len = 0;
            return false;
        }

        if (0 > bytesRead) {
            *line = NULL;
            *len = -1;
            return false;
        }
    }   /* if (_prevNewline >= bufferEnd) { */

    /* check if line break is in the chunk read */
    char *b = _prevNewline;
    size_t sizeLeft = _buffer.size() - (b - reinterpret_cast<char *>(_buffer.data()));
    char *nl = reinterpret_cast<char *>(memchr(b, _delim, sizeLeft));

    *line = b;

    _prevNewline = nl;

    if (_prevNewline) {
        /* skip the delimiting character */
        *len = _prevNewline - b;
        *_prevNewline = '\0';
        ++_prevNewline;
    }   /* if (_prevNewline) { */
    else {
        *len = sizeLeft;

        /* instigate another read operation upon subsequent call */
        _prevNewline = reinterpret_cast<char *>(_buffer.data()) + _buffer.size();
    }   /* if (_prevNewline) { - else */

    return !!nl;
}

ssize_t Input::_readAnotherBlock() {
    if (!_buffer.resize(_maxBufferSize)) {
        errno = ENOMEM;
        return -1;
    }

    errno = 0;
    ssize_t readCount = read(_fd, _buffer.data(), _buffer.size());

    if (readCount <= 0) {
        return readCount;
    }

    _buffer.resize(readCount);

    _prevNewline = reinterpret_cast<char *>(_buffer.data());

    return readCount;
}

