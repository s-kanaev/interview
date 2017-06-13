#include "ParenthesisChecker.h"
#include "Input.h"

#include <iostream>
#include <memory>

#include <unistd.h>
#include <errno.h>
#include <string.h>

#define RET_OK                  0
#define RET_READ_FAILURE        1
#define RET_NOT_ALL_OK          2

void printStatus(size_t line, bool isValid, const ParenthesisChecker::Context &ctx) {
    if (isValid) {
        std::cout << "Line "
                  << line
                  << " ok"
                  << std::endl;
        return;
    }
}

int main(int argc, char **argv) {
    ParenthesisChecker checker;
    Input input(STDIN_FILENO);
    char *line = NULL;
    ssize_t len = 0;
    size_t lineCounter = 1;
    bool allOk = true;

    bool newLineFound = input.readAndDetectNewline(&line, &len);
    bool stillValid = true;

    while (len > 0 && line) {
        if (newLineFound) {
            stillValid = checker.done();
            allOk = allOk && stillValid;
            printStatus(lineCounter, stillValid, checker.getContext());

            checker.reset();
            ++lineCounter;
        }

        stillValid = checker.validate(line, len);
        allOk = allOk && stillValid;

        if (!stillValid) {
            printStatus(lineCounter, stillValid, checker.getContext());
            newLineFound = input.readUntilNewline(&line, &len);
            continue;
        }

        newLineFound = input.readAndDetectNewline(&line, &len);
    }

    if (len < 0) {
        std::cerr << "Read error: "
                  << strerror(errno);

        return RET_READ_FAILURE;
    }

    return allOk ? RET_OK : RET_NOT_ALL_OK;
}
