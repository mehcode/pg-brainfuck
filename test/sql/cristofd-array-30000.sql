-- https://www.hevanet.com/cristofd/brainfuck/tests.b

-- Goes to cell 30000 and reports from there with a "#".
SELECT brainfuck('', $$
++++[>++++++<-]>[>+++++>+++++++<<-]>>++++<[[>[[>>+<<-]<]>>>-]>-[>+>+<<-]>]
+++++[>+++++++<<++>-]>.<<.
$$);
