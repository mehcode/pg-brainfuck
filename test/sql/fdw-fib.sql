CREATE SERVER fdw_fib FOREIGN DATA WRAPPER brainfuck_fdw;

-- Prints the Fibonacci numbers from 0, one per line, forever.

CREATE FOREIGN TABLE fib (line text) SERVER fdw_fib OPTIONS (program $$
>++++++++++>+>+[
    [+++++[>++++++++<-]>.<++++++[>--------<-]+<<<]>.>>[
        [-]<[>+<-]>>[<<+>+>-]<[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
            [>+<-[>+<-[>+<-[>[-]>+>+<<<-[>+<-]]]]]]]]]]]+>>>
    ]<<<
]
$$);

-- The 4th number.

EXPLAIN (COSTS OFF) SELECT line FROM fib LIMIT 1 OFFSET 3;

SELECT line FROM fib LIMIT 1 OFFSET 3;

-- The first number with at least 200 digits.

EXPLAIN (COSTS OFF) SELECT line FROM fib WHERE length(line) >= 200 LIMIT 1;

SELECT line FROM fib WHERE length(line) >= 200 LIMIT 1;

DROP SERVER fdw_fib CASCADE;
