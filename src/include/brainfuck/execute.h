#ifndef BRAINFUCK_EXECUTE_H
#define BRAINFUCK_EXECUTE_H

#include "brainfuck/program.h"

/**
 * Executes the `program` on `input` and returns the output as a `StringInfo`.
 */
StringInfo
pgbf_execute(const pgbf_program* program, const char* input, size_t input_len);

#endif // BRAINFUCK_EXECUTE_H
