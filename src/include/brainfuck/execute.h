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

/**
 * Executes the Brainfuck `program` and returns the result as a `text` value.
 * Raises an error if the output is invalid according to the database's character
 * encoding.
 */
text*
pgbf_execute_to_text(const pgbf_program* program, const char* input, size_t input_len);

#endif // BRAINFUCK_EXECUTE_H
