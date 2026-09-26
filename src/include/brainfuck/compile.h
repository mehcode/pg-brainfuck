#ifndef BRAINFUCK_COMPILE_H
#define BRAINFUCK_COMPILE_H

typedef enum pgbf_opcode {
    /** `+`: Increments the value of the current cell by 1. */
    PGBF_OP_INCREMENT,

    /** `-`: Decrements the value of the current cell by 1. */
    PGBF_OP_DECREMENT,

    /** `>`: Moves the pointer to the next cell. */
    PGBF_OP_NEXT,

    /** `<`: Moves the pointer to the previous cell. */
    PGBF_OP_PREV,

    /** `.`: Outputs the value of the current cell. */
    PGBF_OP_OUTPUT,

    /** `,`: Inputs a value into the current cell. */
    PGBF_OP_INPUT,

    /**
     * `[`: Jumps to the matching `PGBF_OP_LOOP_CLOSE` (`]`)
     * if the current cell is 0.
     */
    PGBF_OP_LOOP_OPEN,

    /**
     * `]`: Jumps to the matching `PGBF_OP_LOOP_OPEN` (`[`)
     * if the current cell is not 0.
     */
    PGBF_OP_LOOP_CLOSE,
} pgbf_opcode;

typedef struct pgbf_op {
    pgbf_opcode opcode;
    size_t operand;
} pgbf_op;

typedef struct pgbf_program {
    size_t nops;
    pgbf_op ops[FLEXIBLE_ARRAY_MEMBER];
} pgbf_program;

typedef enum pgbf_compile_result {
    PGBF_COMPILE_SUCCESS,
    PGBF_COMPILE_UNMATCHED_OPEN,  /**< Unmatched `[`. */
    PGBF_COMPILE_UNMATCHED_CLOSE, /**< Unmatched `]`. */
} pgbf_compile_result;

pgbf_compile_result
pgbf_compile(const char* s, size_t len, pgbf_program** program);

#endif
