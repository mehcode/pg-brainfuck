#include "postgres.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"

#include "brainfuck/machine.h"
#include "brainfuck/program.h"

/**
 * Size of the working memory, in bytes, that the program can use.
 * Access will wrap around if needed.
 */
#define PGBF_MACHINE_MEMORY_SIZE 30000

pgbf_machine
pgbf_machine_alloc(const char* input, size_t input_len) {
    uint8_t* mem = palloc0(PGBF_MACHINE_MEMORY_SIZE);

    return (pgbf_machine){ .input     = input,
                           .input_len = input_len,
                           .mem       = mem,
                           .mem_len   = PGBF_MACHINE_MEMORY_SIZE };
}

void
pgbf_machine_free(pgbf_machine* machine) {
    if (machine->mem != NULL) {
        pfree(machine->mem);
        machine->mem = NULL;
    }
}

void
pgbf_machine_clear(pgbf_machine* machine) {
    memset(machine->mem, 0, machine->mem_len);

    machine->op         = 0;
    machine->input_read = 0;
    machine->ptr        = 0;
}

bool
pgbf_machine_step(const pgbf_program* program, pgbf_machine* m, StringInfo output) {
    // <https://www.hevanet.com/cristofd/brainfuck/brainfuck.html>

    if (m->op >= program->nops) {
        // Out of operations to execute.
        return false;
    }

    const pgbf_op* op = &program->ops[m->op];

    switch (op->opcode) {
    case PGBF_OP_INCREMENT:
        m->mem[m->ptr] += 1;
        break;

    case PGBF_OP_DECREMENT:
        m->mem[m->ptr] -= 1;
        break;

    case PGBF_OP_NEXT:
        m->ptr = (m->ptr + 1) % m->mem_len;
        break;

    case PGBF_OP_PREV:
        if (m->ptr == 0) {
            m->ptr = m->mem_len - 1;
        } else {
            m->ptr -= 1;
        }

        break;

    case PGBF_OP_INPUT:
        if (m->input_read >= m->input_len) {
            m->mem[m->ptr] = 0;
            break;
        }

        m->mem[m->ptr] = (uint8_t)m->input[m->input_read++];
        break;

    case PGBF_OP_OUTPUT:
        appendStringInfoChar(output, (char)m->mem[m->ptr]);
        break;

    case PGBF_OP_LOOP_OPEN:
        // If the current cell is 0, jump to the end of the loop.
        if (m->mem[m->ptr] == 0) {
            m->op = op->operand;
        }

        break;

    case PGBF_OP_LOOP_CLOSE:
        // If the current cell is not 0, jump to the start of the loop.
        if (m->mem[m->ptr] != 0) {
            m->op = op->operand;
        }

        // Check for and handle pending signals (such as a request to cancel).
        CHECK_FOR_INTERRUPTS();

        break;
    }

    m->op += 1;
    m->steps += 1;

    return m->op < program->nops;
}

void
pgbf_machine_run(
    const pgbf_program* program,
    pgbf_machine* machine,
    StringInfo output
) {
    while (pgbf_machine_step(program, machine, output)) {
        // run until the program terminates
    }
}

bool
pgbf_machine_run_until(
    const pgbf_program* program,
    pgbf_machine* machine,
    StringInfo output,
    char stop_byte
) {
    for (;;) {
        int prev_output_len = output->len;
        bool has_more       = pgbf_machine_step(program, machine, output);

        if (output->len > prev_output_len &&
            output->data[prev_output_len] == stop_byte) {
            return true;
        }

        if (!has_more) {
            break;
        }
    }

    return false;
}
