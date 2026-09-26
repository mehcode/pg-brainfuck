#include "postgres.h"
#include "miscadmin.h"

#include "brainfuck/execute.h"
#include "brainfuck/program.h"

/**
 * Size of the data table, in bytes, that the program can use.
 * Access will wrap around if needed.
 */
#define PGBF_DATA_SIZE 30000

void
pgbf_execute(
    const pgbf_program* program,
    const char* input,
    size_t input_len,
    StringInfo output
) {
    // <https://www.hevanet.com/cristofd/brainfuck/brainfuck.html>

    // The data table that is accessed by the program.
    uint8_t data[PGBF_DATA_SIZE] = { 0 };

    // The pointer into the data table, wrapping around as needed.
    size_t ptr = 0;

    // The index into the input string.
    size_t input_i = 0;

    for (size_t op_i = 0; op_i < program->nops; op_i++) {
        const pgbf_op* op = &program->ops[op_i];

        switch (op->opcode) {
        case PGBF_OP_INCREMENT:
            data[ptr] += 1;
            break;

        case PGBF_OP_DECREMENT:
            data[ptr] -= 1;
            break;

        case PGBF_OP_NEXT:
            ptr = (ptr + 1) % PGBF_DATA_SIZE;
            break;

        case PGBF_OP_PREV:
            if (ptr == 0) {
                ptr = PGBF_DATA_SIZE - 1;
            } else {
                ptr -= 1;
            }
            break;

        case PGBF_OP_INPUT:
            if (input_i >= input_len) {
                data[ptr] = 0;
                break;
            }

            data[ptr] = (uint8_t)input[input_i++];
            break;

        case PGBF_OP_OUTPUT:
            appendStringInfoChar(output, (char)data[ptr]);
            break;

        case PGBF_OP_LOOP_OPEN:
            // If the current cell is 0, jump to the end of the loop.
            if (data[ptr] == 0) {
                op_i = op->operand;
            }

            break;

        case PGBF_OP_LOOP_CLOSE:
            // If the current cell is not 0, jump to the start of the loop.
            if (data[ptr] != 0) {
                op_i = op->operand;

                // Check for and handle pending signals (such as a request to cancel).
                CHECK_FOR_INTERRUPTS();
            }

            break;
        }
    }
}
