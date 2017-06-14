#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define LINE_SIZE       10000
#define LINE_COUNT      10000

int main(void) {
    const char ALPHABET[] = "()abcdefghijklmnopqrstuvwxyz";
    size_t alphabet_length = strlen(ALPHABET);
    size_t line_idx, char_idx, line_size, alpha, failure_alpha_idx;
    ssize_t opened;

    srand(time(NULL));

    for (line_idx = 1; line_idx <= LINE_COUNT; ++line_idx) {
        opened = 0;
        failure_alpha_idx = 0;
        line_size = 1 + (rand() % LINE_SIZE);

        for (char_idx = 1; (char_idx <= line_size); ++char_idx) {
            alpha = rand() % alphabet_length;

            if (0 == alpha) {
                ++opened;
            }
            else if (1 == alpha) {
                --opened;
            }

            fprintf(stdout, "%c", ALPHABET[alpha]);

            if (!failure_alpha_idx && (opened < 0)) {
                failure_alpha_idx = char_idx;
            }
        }

        fprintf(stdout, "\n");

        if (failure_alpha_idx) {
            fprintf(stderr, "Failure at line number %zu near character number %zu due to superfluous closing bracket\n",
                    line_idx, failure_alpha_idx);
        }
        else if (0 == opened) {
            fprintf(stderr, "Line %zu ok\n", line_idx);
        }
        else {
            fprintf(stderr, "Failure at line number %zu near character number %zu due to lack of closing bracket\n",
                    line_idx, line_size);
        }
    }

    return 0;
}

