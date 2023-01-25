#ifndef INPUT_H
#define INPUT_H

#include "Buffer.h"

#include <sys/types.h>
#include <linux/limits.h>

/**
 * Input stream representation with variable delimiter
 */
class Input {
private:
    int _fd;
    char _delim;
    char *_prevNewline;
    Buffer _buffer;
    size_t _maxBufferSize;

    ssize_t _readAnotherBlock();

public:
    /**
     * Construct an uninitialized object
     */
    Input();

    /**
     * Constructor
     * \param fd input file descriptor
     * \param delim delimiter character
     * \param initialBufferSize initial input buffer size, will increased by 1
     */
    Input(int fd,
          char delim = '\n',
          size_t maxBufferSize = PIPE_BUF);
    ~Input();

    Input(Input &&rhs) = default;
    Input(const Input &rhs) = delete;

    Input &operator=(Input &&rhs) = default;
    Input &operator=(const Input &rhs) = delete;

    int fd() const {
        return _fd;
    }

    /**
     * Try to read some data from \c fd and detect a newline
     * with delimiter provided whilst construction
     * \param [out] line where to put line start ptr
     * \param [out] len line length w/o newline or length of chunk read
     * \return \c true on new line started (except for the very first line)
     */
    bool readAndDetectNewline(char **line, ssize_t *len);

    /**
     * Skip any intermediate chunks read until a newline is hit
     */
    bool readUntilNewline(char **line, ssize_t *len);
};

#endif /* INPUT_H */
