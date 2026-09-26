-- https://en.wikipedia.org/wiki/Brainfuck#Hello_World%21

SELECT brainfuck('', '++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.');

CREATE FUNCTION bf_hello_world()
RETURNS text
AS $$
BEGIN
    ++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.
END;
$$ LANGUAGE plbrainfuck;

SELECT bf_hello_world();
