#include "postgres.h"
#include "mb/pg_wchar.h"

#if PG_VERSION_NUM >= 160000
// In Postgres 16+, the VARDATA/VARSIZE macros moved to the `varatt.h` header.
#include "varatt.h"
#endif

#include "brainfuck/execute.h"
#include "brainfuck/machine.h"
#include "brainfuck/program.h"

text*
pgbf_execute(const pgbf_program* program, const char* input, size_t input_len) {
    // Allocate space for the output string, reserving VARHDRSZ bytes.
    StringInfo output = makeStringInfo();
    enlargeStringInfo(output, program->output_hint + VARHDRSZ);
    output->len = VARHDRSZ;

    // Execute the compiled program in a new machine.
    pgbf_machine m = pgbf_machine_alloc(input, input_len);
    pgbf_machine_run(program, &m, output);
    pgbf_machine_free(&m);

    // We need to return TEXT so raise an exception if the program produced
    // text that is invalid against the database character encoding.
    pg_verifymbstr(output->data + VARHDRSZ, output->len - VARHDRSZ, false);

    SET_VARSIZE(output->data, output->len);
    return (text*)output->data;
}
