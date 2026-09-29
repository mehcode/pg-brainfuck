CREATE SERVER fdw_explain FOREIGN DATA WRAPPER brainfuck_fdw;

-- Prints "A" and "B" on their own lines.
CREATE FOREIGN TABLE ab (line text)
    SERVER fdw_explain
    OPTIONS (program '++++++++[>++++++++<-]>+.<++++++++++.>+.<.');

EXPLAIN (COSTS OFF) SELECT * FROM ab;

-- The machine can't filter, so WHERE clauses stay in the plan.
EXPLAIN (COSTS OFF) SELECT * FROM ab WHERE line <> 'A';

-- A table has a single `line text` column.
CREATE FOREIGN TABLE int_line (line int) SERVER fdw_explain OPTIONS (program '+');
EXPLAIN (COSTS OFF) SELECT * FROM int_line;

DROP SERVER fdw_explain CASCADE;
