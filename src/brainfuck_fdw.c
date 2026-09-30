// NOLINTBEGIN(readability-identifier-naming)
#include "postgres.h"
#include "access/attnum.h"
#include "access/reloptions.h"
#include "access/table.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_foreign_table.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "executor/executor.h"
#include "executor/tuptable.h"
#include "fmgr.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "nodes/execnodes.h"
#include "nodes/nodes.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "nodes/plannodes.h"
#include "nodes/value.h"
#include "optimizer/optimizer.h"
#include "optimizer/pathnode.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"

#if PG_VERSION_NUM >= 180000
#include "commands/explain_format.h"
#include "commands/explain_state.h"
#else
#include "commands/explain.h"
#endif

#if PG_VERSION_NUM >= 160000
#include "varatt.h"
#endif

#include "brainfuck/compile.h"
#include "brainfuck/machine.h"

typedef struct brainfuck_fdw_plan_state {
    const char* program;
} brainfuck_fdw_plan_state;

typedef struct brainfuck_fdw_execute_state {
    const pgbf_program* program;
    pgbf_machine machine;

    /** Output buffer of the machine, reused for each "line" of output.  */
    StringInfoData line;

    /** What `attnum` to use to push "line" */
    AttrNumber line_attnum;
} brainfuck_fdw_execute_state;

typedef enum brainfuck_fdw_option {
    BRAINFUCK_FDW_OPTION_PROGRAM,
    BRAINFUCK_FDW_OPTION_ROWS,
} brainfuck_fdw_option;

static int
brainfuck_fdw_parse_option_rows(DefElem* def) {
    const char* value = defGetString(def);
    int rows          = 0;

    if (!parse_int(value, &rows, 0, NULL) || rows <= 0) {
        ereport(
            ERROR,
            errcode(ERRCODE_FDW_INVALID_STRING_FORMAT),
            errmsg("invalid value for option \"%s\": %s", def->defname, value)
        );
    }

    return rows;
}

static bool
brainfuck_fdw_parse_option(const char* name, brainfuck_fdw_option* option) {
    Assert(option != NULL);

    if (strcmp(name, "program") == 0) {
        *option = BRAINFUCK_FDW_OPTION_PROGRAM;
        return true;
    }

    if (strcmp(name, "rows") == 0) {
        *option = BRAINFUCK_FDW_OPTION_ROWS;
        return true;
    }

    return false;
}

PG_FUNCTION_INFO_V1(brainfuck_fdw_validator);

Datum
brainfuck_fdw_validator(PG_FUNCTION_ARGS) {
    // untransformRelOptions converts the first argument to a List
    List* options = untransformRelOptions(PG_GETARG_DATUM(0));

    Oid context      = PG_GETARG_OID(1);
    bool has_program = false; // a `program` option is required

    if (context != ForeignTableRelationId) {
        if (options != NIL) {
            // We only support options for foreign tables
            DefElem* def = linitial_node(DefElem, options);
            ereport(
                ERROR,
                errcode(ERRCODE_FDW_INVALID_OPTION_NAME),
                errmsg("invalid option \"%s\"", def->defname)
            );
        }

        PG_RETURN_VOID();
    }

    ListCell* cell = NULL;
    foreach (cell, options) {
        // each DefElem node represents a key-value pair from the `options` map
        DefElem* def = lfirst_node(DefElem, cell);

        brainfuck_fdw_option option; // NOLINT(cppcoreguidelines-init-variables)
        if (!brainfuck_fdw_parse_option(def->defname, &option)) {
            ereport(
                ERROR,
                errcode(ERRCODE_FDW_INVALID_OPTION_NAME),
                errmsg("invalid option \"%s\"", def->defname)
            );
        }

        switch (option) {
        case BRAINFUCK_FDW_OPTION_PROGRAM: {
            // Compile and discard to ensure the source is valid.
            // If compilation fails, an error will be raised.
            const char* program = defGetString(def);
            pgbf_compile(program, strlen(program));
            has_program = true;
        } break;

        case BRAINFUCK_FDW_OPTION_ROWS:
            brainfuck_fdw_parse_option_rows(def);
            break;
        }
    }

    if (!has_program) {
        ereport(
            ERROR,
            errcode(ERRCODE_FDW_DYNAMIC_PARAMETER_VALUE_NEEDED),
            errmsg("option \"program\" is required for brainfuck_fdw")
        );
    }

    PG_RETURN_VOID();
}

/**
 * Checks that the table has a `line` column of type `text`.
 * No other columns are allowed at this time.
 */
static void
brainfuck_fdw_check_columns(Relation rel) {
    TupleDesc desc = RelationGetDescr(rel);
    bool has_line  = false;

    for (int i = 0; i < desc->natts; i++) {
        Form_pg_attribute attr = TupleDescAttr(desc, i);
        if (attr->attisdropped) {
            // Column is dropped (invisible), skip it.
            continue;
        }

        const char* name = NameStr(attr->attname);
        if (strcmp(name, "line") == 0) {
            has_line = true;

            if (attr->atttypid != TEXTOID) {
                // The "line" column must be of type text.
                ereport(
                    ERROR,
                    errcode(ERRCODE_FDW_INVALID_DATA_TYPE),
                    errmsg(
                        "column \"%s\" of foreign table \"%s\" must be of type "
                        "text",
                        name,
                        RelationGetRelationName(rel)
                    )
                );
            }
        } else {
            // No other columns supported.
            ereport(
                ERROR,
                errcode(ERRCODE_FDW_INVALID_COLUMN_NAME),
                errmsg(
                    "invalid column \"%s\" of foreign table \"%s\"",
                    name,
                    RelationGetRelationName(rel)
                )
            );
        }
    }

    if (!has_line) {
        // The "line" column is required.
        ereport(
            ERROR,
            errcode(ERRCODE_FDW_COLUMN_NAME_NOT_FOUND),
            errmsg(
                "foreign table \"%s\" must have a column \"line\"",
                RelationGetRelationName(rel)
            )
        );
    }
}

/**
 * Obtain relation size estimates.
 * Validates that the declared foreign table has *only* a "line" column.
 */
static void
brainfuck_fdw_get_foreign_rel_size(
    PlannerInfo* root,
    RelOptInfo* baserel,
    Oid foreigntableid
) {
    // The planner already has a lock on the table.
    Relation rel = table_open(foreigntableid, NoLock);

    // Ensure that only the "line" column exists.
    brainfuck_fdw_check_columns(rel);

    table_close(rel, NoLock);

    // Allocate space for private state for the plan phase.
    brainfuck_fdw_plan_state* state = palloc0(sizeof(brainfuck_fdw_plan_state));

    // The validator has already checked the options
    // Now we need to pull the values out so we can use them
    ForeignTable* table = GetForeignTable(foreigntableid);
    ListCell* cell      = NULL;
    double rows         = baserel->rows;

    foreach (cell, table->options) {
        DefElem* def = lfirst_node(DefElem, cell);

        brainfuck_fdw_option option; // NOLINT(cppcoreguidelines-init-variables)
        if (!brainfuck_fdw_parse_option(def->defname, &option)) {
            // validator rejects invalid options, so we should never get here
            elog(ERROR, "invalid option \"%s\"", def->defname);
        }

        switch (option) {
        case BRAINFUCK_FDW_OPTION_PROGRAM:
            state->program = defGetString(def);
            break;

        case BRAINFUCK_FDW_OPTION_ROWS:
            rows = brainfuck_fdw_parse_option_rows(def);
            break;
        }
    }

    Assert(state->program != NULL);

    baserel->tuples = rows;

    // Estimate the number of rows.
    baserel->rows =
        rows *
        // `clauselist_selectivity` guess what fraction of rows pass WHERE clauses
        clauselist_selectivity(root, baserel->baserestrictinfo, 0, JOIN_INNER, NULL);

    baserel->fdw_private = state;
}

static void
brainfuck_fdw_get_foreign_paths(
    PlannerInfo* root,
    RelOptInfo* baserel,
    Oid foreigntableid pg_attribute_unused()
) {
    const brainfuck_fdw_plan_state* state = baserel->fdw_private;

    size_t program_len = strlen(state->program);

    // Starting up will compile the program.
    Cost startup_cost = (Cost)program_len * cpu_operator_cost;

    // Each row runs it to the next line.
    Cost cpu_per_tuple = cpu_tuple_cost + baserel->baserestrictcost.per_tuple;
    Cost total_cost    = startup_cost + baserel->tuples * cpu_per_tuple;

    ForeignPath* path = create_foreignscan_path(
        root,
        baserel,
        NULL, // target
        baserel->rows,
#if PG_VERSION_NUM >= 180000
        0, // disabled_nodes
#endif
        startup_cost,
        total_cost,
        NIL,  // fdw_pathkeys: the rows have no sort order
        NULL, // required_outer: not parameterized
        NULL, // fdw_outerpath: only for joins
#if PG_VERSION_NUM >= 170000
        NIL, // fdw_restrictinfo
#endif
        NIL // fdw_private
    );

    add_path(baserel, (Path*)path);
}

/**
 * Builds the ForeignScan plan node.
 * Called once, at the end of planning.
 */
static ForeignScan*
brainfuck_fdw_get_foreign_plan(
    PlannerInfo* root pg_attribute_unused(),
    RelOptInfo* baserel,
    Oid foreigntableid pg_attribute_unused(),
    ForeignPath* best_path pg_attribute_unused(),
    List* tlist,
    List* scan_clauses,
    Plan* outer_plan
) {
    const brainfuck_fdw_plan_state* state = baserel->fdw_private;

    // `baserel->fdw_private` lasts only until planning ends (which is after we return)
    // the state must be a list, so we can't store our struct directly
    // BeginForeignScan consumes this and compiles the program
    List* fdw_private = list_make1(makeString(pstrdup(state->program)));

    // The foreign machine can't filter rows, so every WHERE clause is not pushed down
    // `scan_clauses` holds RestrictInfo nodes, `extract_actual_clauses` unwraps them
    // into plain expressions
    List* quals = extract_actual_clauses(scan_clauses, false);

    return make_foreignscan(
        tlist, // the columns to emit
        quals,
        baserel->relid,
        NIL, // fdw_exprs: no expressions to evaluate
        fdw_private,
        NIL,       // fdw_scan_tlist: rows are the shape of the table
        NIL,       // fdw_recheck_quals: we check no clauses
        outer_plan // only set for joins that are pushed down
    );
}

/**
 * Prepares to execute.
 * Compiles the program and allocates a machine.
 * Called at startup, in the per-query memory context.
 */
static void
brainfuck_fdw_begin_foreign_scan(ForeignScanState* node, int eflags) {
    if ((eflags & EXEC_FLAG_EXPLAIN_ONLY) != 0) {
        // Plain EXPLAIN (no ANALYZE) starts the executor but never runs.
        // The docs ask us to only do the minimum required.
        return;
    }

    // `ps.plan` is the ForeignScan node that GetForeignPlan just built
    const ForeignScan* plan = castNode(ForeignScan, node->ss.ps.plan);

    // The plan carries the source as a list of one string
    const char* source = strVal(linitial(plan->fdw_private));

    brainfuck_fdw_execute_state* state = palloc0(sizeof(brainfuck_fdw_execute_state));

    state->program = pgbf_compile(source, strlen(source));
    state->machine = pgbf_machine_alloc(NULL, 0);

    // IterateForeignScan reuses one buffer for every line of output
    // Allocated in the per-query memory context so it lives past the iteration
    initStringInfo(&state->line);

    // Find the `attnum` of the "line" column
    state->line_attnum =
        get_attnum(RelationGetRelid(node->ss.ss_currentRelation), "line");

    Assert(state->line_attnum != InvalidAttrNumber);

    node->fdw_state = state;
}

/**
 * Run the brainfuck machine and capture one row ("line") of output.
 */
static TupleTableSlot*
brainfuck_fdw_iterate_foreign_scan(ForeignScanState* node) {
    brainfuck_fdw_execute_state* state = node->fdw_state;

    // Docs say the ScanTupleSlot should be used to return the tuple
    TupleTableSlot* slot = node->ss.ss_ScanTupleSlot;
    ExecClearTuple(slot);

    // Prepare the line buffer to receive output.
    resetStringInfo(&state->line);
    appendStringInfoSpaces(&state->line, VARHDRSZ);

    // Resume machine execution where the last row left off.
    // Run until the machine terminates or prints a newline.
    bool has_newline =
        pgbf_machine_run_until(state->program, &state->machine, &state->line, '\n');

    int len = state->line.len - VARHDRSZ;
    if (has_newline) {
        // We received a newline-terminated line of output.
        // Trim the newline before returning the line.
        len -= 1;
    } else if (len == 0) {
        // The machine terminated without producing any more output.
        // Calling `pgbf_machine_run_until` on a terminated machine will always return
        // `false` immediately.
        return NULL;
    }

    // Check that the produced output is valid according to the database encoding.
    // Raises an error if the output is invalid.
    pg_verifymbstr(state->line.data + VARHDRSZ, len, false);

    // Fill in the variable-size header
    text* line = (text*)state->line.data;
    SET_VARSIZE(line, VARHDRSZ + len);

    // Initialize the full slot to `null`.
    memset(slot->tts_isnull, true, slot->tts_tupleDescriptor->natts * sizeof(bool));

    // Then fill in our "line" column.
    int index               = AttrNumberGetAttrOffset(state->line_attnum);
    slot->tts_isnull[index] = false;
    slot->tts_values[index] = PointerGetDatum(line);

    return ExecStoreVirtualTuple(slot);
}

/**
 * Restart the scan from the beginning.
 */
static void
brainfuck_fdw_rescan_foreign_scan(ForeignScanState* node) {
    brainfuck_fdw_execute_state* state = node->fdw_state;

    // Compiled program doesn't change but we need to reset the machine state.
    pgbf_machine_clear(&state->machine);
}

/**
 * Ends the scan, freeing resources if needed.
 */
static void
brainfuck_fdw_end_foreign_scan(ForeignScanState* node pg_attribute_unused()) {
    // Nothing to do. Anything allocated with palloc and friends is freed
    // for us and we don't allocate anything else.
}

/**
 * Lets the scan run inside a parallel worker.
 */
static bool
brainfuck_fdw_is_foreign_scan_parallel_safe(
    PlannerInfo* root pg_attribute_unused(),
    RelOptInfo* rel pg_attribute_unused(),
    RangeTblEntry* rte pg_attribute_unused()
) {
    // Running the program depends on no shared state outside of its
    // own query-local machine state.
    return true;
}

/**
 * Adds to the EXPLAIN output.
 * We count and export the number of STEPs taken by the machine.
 */
static void
brainfuck_fdw_explain_foreign_scan(ForeignScanState* node, ExplainState* es) {
    if (es->verbose) {
        // Only add the program source if the user requested verbose output.
        const ForeignScan* plan = castNode(ForeignScan, node->ss.ps.plan);
        ExplainPropertyText("Program", strVal(linitial(plan->fdw_private)), es);
    }

    if (es->analyze) {
        // ANALYZE runs the execute, that means `BeginForeignScan` ran in full
        const brainfuck_fdw_execute_state* state = node->fdw_state;
        ExplainPropertyUInteger("Steps", NULL, state->machine.steps, es);
    }
}

PG_FUNCTION_INFO_V1(brainfuck_fdw_handler);

Datum
brainfuck_fdw_handler(PG_FUNCTION_ARGS) {
    FdwRoutine* routine = makeNode(FdwRoutine);

    routine->GetForeignRelSize         = brainfuck_fdw_get_foreign_rel_size;
    routine->GetForeignPaths           = brainfuck_fdw_get_foreign_paths;
    routine->GetForeignPlan            = brainfuck_fdw_get_foreign_plan;
    routine->BeginForeignScan          = brainfuck_fdw_begin_foreign_scan;
    routine->EndForeignScan            = brainfuck_fdw_end_foreign_scan;
    routine->IterateForeignScan        = brainfuck_fdw_iterate_foreign_scan;
    routine->ReScanForeignScan         = brainfuck_fdw_rescan_foreign_scan;
    routine->IsForeignScanParallelSafe = brainfuck_fdw_is_foreign_scan_parallel_safe;
    routine->ExplainForeignScan        = brainfuck_fdw_explain_foreign_scan;

    PG_RETURN_POINTER(routine);
}

// NOLINTEND(readability-identifier-naming)
