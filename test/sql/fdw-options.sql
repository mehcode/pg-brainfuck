CREATE SERVER fdw_options FOREIGN DATA WRAPPER brainfuck_fdw;

-- `program` is required.
CREATE FOREIGN TABLE no_program (line text) SERVER fdw_options;

-- `program` must be a valid Brainfuck program.
CREATE FOREIGN TABLE unmatched (line text) SERVER fdw_options OPTIONS (program '[');

-- `rows` must be a positive integer.
CREATE FOREIGN TABLE zero_rows (line text) SERVER fdw_options OPTIONS (program '+', rows '0');
CREATE FOREIGN TABLE many_rows (line text) SERVER fdw_options OPTIONS (program '+', rows 'many');

DROP SERVER fdw_options CASCADE;
