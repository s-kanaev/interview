#include "ParenthesisChecker.h"

#include <assert.h>

ParenthesisChecker::ParenthesisChecker()
: _opening('('),
  _closing(')'),
  _state(State::wait_for_open)
{
    /* empty */
}

ParenthesisChecker::~ParenthesisChecker() {
    /* empty */
}

bool ParenthesisChecker::reset(char opening, char closing) {
    if (opening == closing) {
        return false;
    }

    _opening = opening;
    _closing = closing;

    _state = State::wait_for_open;
    _context = Context();
}

bool ParenthesisChecker::validate(const char *str, size_t length) {
    const char *current = str;
    const char *last = str + length;
    bool result = true;

    for (; result && current != last; ++current, ++_context.position) {
        switch (_state) {
            case State::wait_for_open :
                result = _validateWaitForOpen(*current);
                break;

            case State::wait_for_any :
                result = _validateWaitForAny(*current);
                break;

            default:
                assert(0);
                break;
        }

        if (!result) {
            break;
        }
    }

    return result;
}

bool ParenthesisChecker::done() {
    return !_context.openIndex;
}

void ParenthesisChecker::_opened() {
    ++ _context.openIndex;
}

void ParenthesisChecker::_closed() {
    -- _context.openIndex;
}

bool ParenthesisChecker::_validateWaitForOpen(char chr) {
    if (chr == _opening) {
        _opened();

        _state = State::wait_for_any;
    }
    else if (chr == _closing) {
        return false;
    }

    return true;
}

bool ParenthesisChecker::_validateWaitForAny(char chr) {
    if (chr == _opening) {
        _opened();
    }
    else if (chr == _closing) {
        _closed();

        if (!_context.openIndex) {
            _state = State::wait_for_open;
        }
    }

    return true;
}
