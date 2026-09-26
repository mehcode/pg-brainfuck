#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"

#if PG_VERSION_NUM >= 160000
// In Postgres 16+, the VARDATA/VARSIZE macros moved to the `varatt.h` header.
#include "varatt.h"
#endif

#include "brainfuck/compile.h"
#include "brainfuck/execute.h"

#ifdef PG_MODULE_MAGIC_EXT
PG_MODULE_MAGIC_EXT(.name = "brainfuck", .version = PGBF_VERSION);
#else
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(brainfuck); // NOLINT(readability-identifier-naming)

Datum
brainfuck(PG_FUNCTION_ARGS) {
    const text* input     = PG_GETARG_TEXT_PP(0);
    const text* program_s = PG_GETARG_TEXT_PP(1);

    // Compile the Brainfuck program into a sequence of operations.
    pgbf_program* program =
        pgbf_compile(VARDATA_ANY(program_s), VARSIZE_ANY_EXHDR(program_s));

    // Allocate space for the output string, reserving VARHDRSZ bytes.
    StringInfo output = makeStringInfo();
    enlargeStringInfo(output, program->output_hint + VARHDRSZ);
    output->len = VARHDRSZ;

    // Execute the compiled program. Infallible (outside of allocation failure).
    pgbf_execute(program, VARDATA_ANY(input), VARSIZE_ANY_EXHDR(input), output);

    // We need to return TEXT so raise an exception if the program produced
    // text that is invalid against the database character encoding.
    pg_verifymbstr(output->data + VARHDRSZ, output->len - VARHDRSZ, false);

    SET_VARSIZE(output->data, output->len);
    PG_RETURN_TEXT_P(output->data);
}
