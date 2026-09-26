#include "postgres.h"

#include "brainfuck/compile.h"
#include "brainfuck/program.h"

pgbf_program*
pgbf_compile(const char* s, size_t len) {
    // <https://www.hevanet.com/cristofd/brainfuck/brainfuck.html>

    // Count of operations seen, used during each pass.
    size_t n = 0;

    // Current depth of nested brackets, used during each pass.
    size_t depth = 0;

    // The maximum depth of nested brackets. Sizes `stack` from the first pass.
    size_t max_depth = 0;

    // Tracks the opened bracket positions in the second pass.
    size_t* stack = NULL;

    // Count of output operations.
    // Used as a hint to the executor for the output allocation size.
    int n_output = 0;

    // Iterate once over the source to count operations and validate brackets.
    for (size_t i = 0; i < len; i++) {
        switch (s[i]) {
        case '[':
            depth += 1;
            max_depth = depth > max_depth ? depth : max_depth;
            n += 1;
            break;

        case ']':
            if (depth == 0) {
                ereport(
                    ERROR, errcode(ERRCODE_SYNTAX_ERROR), errmsg("unmatched \"]\"")
                );
            }

            depth -= 1;
            n += 1;
            break;

        case '.':
            n_output += 1;
            // fallthrough

        case '+':
        case '-':
        case '>':
        case '<':
        case ',':
            n += 1;
            break;

        default:
            break;
        }
    }

    if (depth != 0) {
        ereport(ERROR, errcode(ERRCODE_SYNTAX_ERROR), errmsg("unmatched \"[\""));
    }

    pgbf_program* program = palloc(sizeof(pgbf_program) + sizeof(pgbf_op) * n);
    program->nops         = n;
    program->output_hint  = n_output;

    stack = palloc(sizeof(size_t) * max_depth);
    depth = 0;

    n = 0;

    for (size_t i = 0; i < len; i++) {
        switch (s[i]) {
        case '+':
            program->ops[n++].opcode = PGBF_OP_INCREMENT;
            break;

        case '-':
            program->ops[n++].opcode = PGBF_OP_DECREMENT;
            break;

        case '>':
            program->ops[n++].opcode = PGBF_OP_NEXT;
            break;

        case '<':
            program->ops[n++].opcode = PGBF_OP_PREV;
            break;

        case '.':
            program->ops[n++].opcode = PGBF_OP_OUTPUT;
            break;

        case ',':
            program->ops[n++].opcode = PGBF_OP_INPUT;
            break;

        case '[':
            stack[depth++]           = n;
            program->ops[n++].opcode = PGBF_OP_LOOP_OPEN;
            break;

        case ']':
            depth -= 1;
            program->ops[stack[depth]].operand = n;
            program->ops[n].operand            = stack[depth];
            program->ops[n].opcode             = PGBF_OP_LOOP_CLOSE;
            n += 1;
            break;

        default:
            // From <https://www.hevanet.com/cristofd/brainfuck/brainfuck.html>
            // Unrecognized characters are ignored.
            break;
        }
    }

    Assert(depth == 0);
    Assert(n == program->nops);

    pfree(stack);

    return program;
}
