#include "postgres.h"
#include "access/htup_details.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "utils/guc.h"
#include "utils/syscache.h"

#if PG_VERSION_NUM >= 160000
// In Postgres 16+, the VARDATA/VARSIZE macros moved to the `varatt.h` header.
#include "varatt.h"
#endif

#include "brainfuck/compile.h"
#include "brainfuck/execute.h"
#include "brainfuck/program.h"

#ifdef PG_MODULE_MAGIC_EXT
PG_MODULE_MAGIC_EXT(.name = "brainfuck", .version = PGBF_VERSION);
#else
PG_MODULE_MAGIC;
#endif

static text*
pgbf_execute_to_text(const pgbf_program* program, const char* input, size_t input_len) {
    // Allocate space for the output string, reserving VARHDRSZ bytes.
    StringInfo output = makeStringInfo();
    enlargeStringInfo(output, program->output_hint + VARHDRSZ);
    output->len = VARHDRSZ;

    // Execute the compiled program.
    pgbf_execute(program, input, input_len, output);

    // We need to return TEXT so raise an exception if the program produced
    // text that is invalid against the database character encoding.
    pg_verifymbstr(output->data + VARHDRSZ, output->len - VARHDRSZ, false);

    SET_VARSIZE(output->data, output->len);
    return (text*)output->data;
}

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
        pgbf_execute_to_text(program, VARDATA_ANY(input), VARSIZE_ANY_EXHDR(input))
    );
}

static void
plbrainfuck_check_signature(Form_pg_proc form) {
    if (form->proretset) {
        // Must not use SETOF.
        ereport(
            ERROR,
            errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
            errmsg("plbrainfuck functions cannot return sets")
        );
    }

    if (form->prokind != PROKIND_FUNCTION) {
        // Must be `CREATE FUNCTION` (not `CREATE PROCEDURE`, ...).
        ereport(
            ERROR,
            errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
            errmsg("plbrainfuck supports only regular functions")
        );
    }

    if (form->prorettype != TEXTOID) {
        // We only support TEXT return types.
        ereport(
            ERROR,
            errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
            errmsg("plbrainfuck functions must return text")
        );
    }

    if (form->pronargs > 1 ||
        (form->pronargs == 1 && form->proargtypes.values[0] != TEXTOID)) {
        // We only support zero or one TEXT argument.
        ereport(
            ERROR,
            errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
            errmsg("plbrainfuck functions must take zero or one argument of type TEXT")
        );
    }
}

static pgbf_program*
plbrainfuck_compile(HeapTuple proc) {
    // Get the procedure's source code.
    // This is what we need to compile and execute the Brainfuck program.
    bool isnull  = false;
    Datum prosrc = SysCacheGetAttr(PROCOID, proc, Anum_pg_proc_prosrc, &isnull);
    if (isnull) {
        elog(
            ERROR,
            "source is null for function %u",
            ((Form_pg_proc)GETSTRUCT(proc))->oid
        );
    }

    const text* source = DatumGetTextPP(prosrc);

    return pgbf_compile(VARDATA_ANY(source), VARSIZE_ANY_EXHDR(source));
}

PG_FUNCTION_INFO_V1(plbrainfuck_validator); // NOLINT(readability-identifier-naming)

Datum
plbrainfuck_validator(PG_FUNCTION_ARGS) {
    Oid fn_oid = PG_GETARG_OID(0);

    if (!CheckFunctionValidatorAccess(fcinfo->flinfo->fn_oid, fn_oid)) {
        PG_RETURN_VOID();
    }

    HeapTuple proc = SearchSysCache1(PROCOID, ObjectIdGetDatum(fn_oid));
    if (!HeapTupleIsValid(proc)) {
        elog(ERROR, "cache lookup failed for function %u", fn_oid);
    }

    plbrainfuck_check_signature((Form_pg_proc)GETSTRUCT(proc));

    if (check_function_bodies) {
        // Compile and discard to ensure the source is valid.
        // If compilation fails, an error will be raised.
        plbrainfuck_compile(proc);
    }

    ReleaseSysCache(proc);

    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(plbrainfuck_call_handler); // NOLINT(readability-identifier-naming)

Datum
plbrainfuck_call_handler(PG_FUNCTION_ARGS) {
    HeapTuple proc = SearchSysCache1(PROCOID, ObjectIdGetDatum(fcinfo->flinfo->fn_oid));
    if (!HeapTupleIsValid(proc)) {
        elog(ERROR, "cache lookup failed for function %u", fcinfo->flinfo->fn_oid);
    }

    plbrainfuck_check_signature((Form_pg_proc)GETSTRUCT(proc));

    const pgbf_program* program = plbrainfuck_compile(proc);

    ReleaseSysCache(proc);

    // Get the procedure's input argument, if any.
    // VARDATA_ANY does not check for NULL.
    const char* input = NULL;
    size_t input_len  = 0;
    if (PG_NARGS() == 1 && !PG_ARGISNULL(0)) {
        const text* input_text = PG_GETARG_TEXT_PP(0);

        input     = VARDATA_ANY(input_text);
        input_len = VARSIZE_ANY_EXHDR(input_text);
    }

    // Execute the Brainfuck program and return its output as TEXT.
    PG_RETURN_TEXT_P(pgbf_execute_to_text(program, input, input_len));
}
