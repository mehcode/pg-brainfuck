-- https://www.hevanet.com/cristofd/brainfuck/tests.b

-- Tests for several obscure problems. Should output an "H".
SELECT brainfuck('', $$
[]++++++++++[>>+>+>++++++[<<+<+++>>>-]<<<<-]
"A*$";?@![#>>+<<]>[>>]<<<<[>++<[-]]>.>.
$$);
