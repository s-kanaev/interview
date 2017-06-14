#ifndef PARENTHESIS_CHECKER_H
# define PARENTHESIS_CHECKER_H

#include <stddef.h>
#include <stdlib.h>

/**
 * Parenthis checker for single type of brackets
 * i.e. a single pair of opening and closing
 * brackets (e.g. '(' and ')').
 *
 * An instance is constructed with no arguments.
 * An instance must be reset before each and every
 * line to be processed.
 *
 * Processing is done through \c validate call.
 *
 * The call to \c validate method returns whether
 * the block is valid.
 *
 * The failure result is cumultive.
 * I.e. all consequent calls afyer a failure will fail.
 *
 * Due to a single pair of brackets in question there
 * is only a single failure reason: there is closing bracket
 * while checker is wating for the opening one.
 *
 * Typical usage:
 * \code
 * bool result = true;
 * while (input file has another line) {
 *     checker.reset('a', 'b');
 *     while (result && read block of line) {
 *         result = checker.validate(block.start, block.end);
 *     }
 *     result = checker.done();
 * }
 * \endcode
 */
class ParenthesisChecker {
public:
    /*** types ***/
    struct Context {
        ssize_t     openIndex   = 0;    /// current open bracket index
                                        /// less than nil indicates that
                                        /// there was a superfluous closing bracket
                                        /// in effect while positive value
                                        /// indicates that there is this many
                                        /// open brackets
        size_t      position    = 0;    /// position in the last block tested
    };

private:
    enum class State {
        wait_for_open,
        wait_for_any,
        finish          = wait_for_open,    /// the only valid state after
                                            /// a call to \c done
    };

    /*** data ***/
    char    _opening;
    char    _closing;
    State   _state;
    Context _context;

   /*** functions ***/
   bool _validateWaitForOpen(char chr);
   bool _validateWaitForAny(char chr);

   void _opened();
   void _closed();

public:
    /*** API ***/
    /**
     * c-tor
     * Defaults opening bracket to '('
     * and the closing one to ')'
     */
    ParenthesisChecker();
    ~ParenthesisChecker();

    /**
     * Reset the checker
     * \param opening open bracket symbol
     * \param closing closing bracker symbol
     * \return \c true if reset is successfull, \c false otherwise
     *
     * \c false is returned if \c opening is the same as \c closing
     *
     * If reset fails the state of the checker is not changed.
     */
    bool reset(char opening = '(',
               char closing = ')');

    /**
     * Validate the block
     * \param str block start
     * \param length block length
     * \return \c true if block is still valid
     */
    bool validate(const char *str, size_t length);

    /**
     * Notify the checker that the line is finished.
     * The result is the same as per \c validate call
     * except that current state is validated
     */
    bool done();

    const Context& getContext() const {
        return _context;
    }
};

#endif /* PARENTHESIS_CHECKER_H */
