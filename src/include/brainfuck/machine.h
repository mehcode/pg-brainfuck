#ifndef BRAINFUCK_MACHINE_H
#define BRAINFUCK_MACHINE_H

#include "lib/stringinfo.h"

#include "brainfuck/program.h"

typedef struct pgbf_machine {
    /** Index into the program. */
    size_t op;

    /** Reference to the input. Read via `,`. */
    const char* input;

    /** Size of input. */
    size_t input_len;

    /** Number of bytes read from the input. */
    size_t input_read;

    /** Working memory for the machine, the "tape". */
    uint8_t* mem;

    /** Size of the working memory. */
    size_t mem_len;

    /** Pointer into working memory, wrapping around as needed. */
    size_t ptr;
} pgbf_machine;

/** Initializes a new machine to execute a Brainfuck program. */
pgbf_machine
pgbf_machine_alloc(const char* input, size_t input_len);

/**
 * Executes a single step of the Brainfuck program.
 * Machine state is updated after each step.
 * Output is appended to the given StringInfo, as needed.
 * Returns `true` if the program can continue.
 */
bool
pgbf_machine_step(
    const pgbf_program* program,
    pgbf_machine* machine,
    StringInfo output
);

/** Executes a Brainfuck program until it terminates. */
void
pgbf_machine_run(const pgbf_program* program, pgbf_machine* machine, StringInfo output);

/**
 * Executes a Brainfuck program until it outputs `stop_byte` or terminates.
 * Returns `true` if the program stopped due to `stop_byte`.
 */
bool
pgbf_machine_run_until(
    const pgbf_program* program,
    pgbf_machine* machine,
    StringInfo output,
    char stop_byte
);

/** Frees the memory used by the machine. */
void
pgbf_machine_free(pgbf_machine* machine);

#endif // BRAINFUCK_MACHINE_H
