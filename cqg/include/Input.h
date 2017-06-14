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
    int             _fd;
    char            _delim;
    char            *_prevNewline;
    Buffer          _buffer;
    size_t          _maxBufferSize;

    /**
     * Read another chunk and reset \c _prevNewline to its start
     * \return the same as \c read (2) does including with errno values
     */
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
     * Notify the instance that user is about to start reading.
     * The same rules as for \c readAndDetectNewline
     * for \c *line and \c *len apply here
     */
    void start(char **line, ssize_t *len);

    /**
     * Try to read some data from \c fd and detect a newline
     * with delimiter provided whilst construction
     * \param [out] line where to put line start ptr
     * \param [out] len line length w/o newline or length of chunk read
     * \return \c true on new line started
     *
     * Upon return \c *line will contain a pointer to chunk read
     * of length \c *len. The chunk will represent a part of single line.
     * Each line ends with the function returning \c true.
     *
     * If after a return \c *line is equal to \c NULL then
     * there was a read failure and \c *len will subsequently
     * be less than zero. See \c errno for error code.
     *
     * If \c *line is not nil and \c *len equals to zero
     * then there was an empty line.
     *
     * The chunk might not be null-terminated.
     */
    bool readAndDetectNewline(char **line, ssize_t *len);

    /**
     * Skip the rest of the line.
     * The same as for \c readAndDetectNewline rules
     * for \c *line and \c *len apply here
     */
    void readUntilNewline(char **line, ssize_t *len);
};

#endif /* INPUT_H */
