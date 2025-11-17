-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_gen_query" to load this file. \quit

CREATE FUNCTION pg_gen_query(text)
RETURNS TEXT
AS 'MODULE_PATHNAME', 'pg_gen_query'
LANGUAGE C STABLE;
    