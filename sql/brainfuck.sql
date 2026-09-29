-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION brainfuck" to load this file. \quit

CREATE FUNCTION brainfuck(input text, program text)
    RETURNS text
    AS 'MODULE_PATHNAME'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION plbrainfuck_call_handler()
    RETURNS language_handler
    AS 'MODULE_PATHNAME'
    LANGUAGE C;

CREATE FUNCTION plbrainfuck_validator(oid)
    RETURNS void
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT;

CREATE TRUSTED LANGUAGE plbrainfuck
    HANDLER plbrainfuck_call_handler
    VALIDATOR plbrainfuck_validator;

CREATE FUNCTION brainfuck_fdw_handler()
    RETURNS fdw_handler
    AS 'MODULE_PATHNAME'
    LANGUAGE C;

CREATE FUNCTION brainfuck_fdw_validator(text[], oid)
    RETURNS void
    AS 'MODULE_PATHNAME'
    LANGUAGE C STRICT;

CREATE FOREIGN DATA WRAPPER brainfuck_fdw
    HANDLER brainfuck_fdw_handler
    VALIDATOR brainfuck_fdw_validator;
