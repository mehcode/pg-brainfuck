#ifndef BRAINFUCK_EXECUTE_H
#define BRAINFUCK_EXECUTE_H

#include "lib/stringinfo.h"

#include "brainfuck/program.h"

/**
 * Executes the Brainfuck `program`.
 * Reads from `input`, writes into `output`.
 */
void
pgbf_execute(
    const pgbf_program* program,
    const char* input,
    size_t input_len,
    StringInfo output
);

#endif // BRAINFUCK_EXECUTE_H
