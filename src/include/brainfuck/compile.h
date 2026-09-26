#ifndef BRAINFUCK_COMPILE_H
#define BRAINFUCK_COMPILE_H

#include "brainfuck/program.h"

/**
 * Compiles Brainfuck source into a program.
 * Raises an error on compilation failure (such as mismatched brackets).
 */
pgbf_program*
pgbf_compile(const char* s, size_t len);

#endif // BRAINFUCK_COMPILE_H
