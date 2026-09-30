# pg-brainfuck

Extends PostgreSQL with [Brainfuck].

- Provides the `brainfuck(input, program)` [function].

- Provides a `plbrainfuck` [procedural language] to write Brainfuck functions in SQL.

- Provides the `brainfuck_fdw` [foreign data wrapper] (FDW) to query a Brainfuck
  program's output as a table, one row per line.

## Install

Build and install the extension.

```shell
make
```

```shell
sudo make install
```

Then, create the extension in your database.

```sql
CREATE EXTENSION brainfuck;
```

## Usage

### Function

Execute an ad-hoc Brainfuck program.

```sql
-- https://en.wikipedia.org/wiki/Brainfuck#Hello_World%21
SELECT brainfuck('', $$
++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.
$$); -- E'Hello World!\n'
```

### Language

Define your own SQL functions in Brainfuck.

```sql
-- https://www.hevanet.com/cristofd/brainfuck/rot13.b
CREATE FUNCTION rot13(text)
RETURNS text
AS $$
BEGIN
,
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>++++++++++++++<-
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>>+++++[<----->-]<<-
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>++++++++++++++<-
[>+<-[>+<-[>+<-[>+<-[>+<-
[>++++++++++++++<-
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>>+++++[<----->-]<<-
[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
[>++++++++++++++<-
[>+<-]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]
]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]>.[-]<,]
END;
$$ LANGUAGE plbrainfuck;
```

Now you can use the `rot13` function.

```sql
SELECT rot13('Hello'); -- 'Uryyb'
```

### Foreign Data Wrapper

Create a new server with the `brainfuck_fdw` FDW.

```sql
CREATE SERVER brainfuck
FOREIGN DATA WRAPPER brainfuck_fdw;
```

Create a Brainfuck table, to map a Brainfuck program to line-by-line
output to a PostgreSQL table.

```sql
-- Prints the Fibonacci numbers from 0, one per line, forever.
-- https://www.hevanet.com/cristofd/brainfuck/fib.b
CREATE FOREIGN TABLE fib (line text) SERVER brainfuck OPTIONS (program $$
>++++++++++>+>+[
    [+++++[>++++++++<-]>.<++++++[>--------<-]+<<<]>.>>[
        [-]<[>+<-]>>[<<+>+>-]<[>+<-[>+<-[>+<-[>+<-[>+<-[>+<-
            [>+<-[>+<-[>+<-[>[-]>+>+<<<-[>+<-]]]]]]]]]]]+>>>
    ]<<<
]
$$);
```

Get the 4th Fibonacci number:

```sql
SELECT line FROM fib
LIMIT 1 OFFSET 3; -- '2'
```

Get the first number with at least 200 digits.

```sql
SELECT line FROM fib
WHERE length(line) >= 200 LIMIT 1; -- '105858027252...818468660472' (200 digits)
```

## License

Licensed under the [PostgreSQL License](LICENSE).

[Brainfuck]: https://en.wikipedia.org/wiki/Brainfuck
[function]: https://www.postgresql.org/docs/current/xfunc.html
[procedural language]: https://www.postgresql.org/docs/current/xplang.html
[foreign data wrapper]: https://www.postgresql.org/docs/current/ddl-foreign-data.html
