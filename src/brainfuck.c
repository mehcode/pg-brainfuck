#include "postgres.h"
#include "fmgr.h"

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
    const text* input        = PG_GETARG_TEXT_PP(0);
    const text* program_text = PG_GETARG_TEXT_PP(1);

    // Compile the Brainfuck program into a sequence of operations.
    pgbf_program* program =
        pgbf_compile(VARDATA_ANY(program_text), VARSIZE_ANY_EXHDR(program_text));

    // Execute the Brainfuck program and return its output as TEXT.
    PG_RETURN_TEXT_P(
        pgbf_execute(program, VARDATA_ANY(input), VARSIZE_ANY_EXHDR(input))
    );
}
