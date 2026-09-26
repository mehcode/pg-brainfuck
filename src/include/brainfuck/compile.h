#ifndef BRAINFUCK_COMPILE_H
#define BRAINFUCK_COMPILE_H

#include "brainfuck/program.h"

typedef enum pgbf_compile_result {
    PGBF_COMPILE_SUCCESS,
    PGBF_COMPILE_UNMATCHED_OPEN,  /**< Unmatched `[`. */
    PGBF_COMPILE_UNMATCHED_CLOSE, /**< Unmatched `]`. */
} pgbf_compile_result;

/**
 * Compiles Brainfuck source into a program.
 */
pgbf_compile_result
pgbf_compile(const char* s, size_t len, pgbf_program** program);

#endif // BRAINFUCK_COMPILE_H
