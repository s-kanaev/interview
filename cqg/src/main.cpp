#include "ParenthesisChecker.h"
#include "Input.h"

#include <iostream>
#include <sstream>

#include <unistd.h>
#include <errno.h>
#include <string.h>

#define RET_OK                  0
#define RET_READ_FAILURE        1
#define RET_NOT_ALL_OK          2

void printStatus(size_t line, bool isValid, const ParenthesisChecker::Context &ctx) {
    std::stringstream ss;

    if (isValid) {
        ss << "Line "
           << line
           << " ok";
    }
    else {
        ss << "Failure at line number "
           << line
           << " near character number "
           << ctx.position
           << " due to ";

        if (ctx.openIndex > 0) {
            ss << "lack of";
        }
        else {
            ss << "superfluous";
        }

        ss << " closing bracket";
    }

    std::cout << ss.str() << std::endl;
}

int main(int argc, char **argv) {
    ParenthesisChecker checker;
    Input input(STDIN_FILENO);
    char *line = NULL;
    ssize_t len = 0;
    size_t lineCounter = 1;
    bool allOk = true;

    bool newLineFound = false;
    bool stillValid = true;

    input.start(&line, &len);

    while (line) {
        stillValid = checker.validate(line, len);
        allOk = allOk && stillValid;

        if (!stillValid) {
            printStatus(lineCounter, stillValid, checker.getContext());
            newLineFound = input.readUntilNewline(&line, &len, newLineFound);

            checker.reset();
            ++lineCounter;
            continue;
        }

        if (newLineFound) {
            stillValid = checker.done();
            allOk = allOk && stillValid;
            printStatus(lineCounter, stillValid, checker.getContext());

            checker.reset();
            ++lineCounter;
        }

        newLineFound = input.readAndDetectNewline(&line, &len);
    }

    if (!line && errno) {
        std::cerr << "Read error: "
                  << strerror(errno)
                  << " at line "
                  << lineCounter;

        return RET_READ_FAILURE;
    }

    return allOk ? RET_OK : RET_NOT_ALL_OK;
}
