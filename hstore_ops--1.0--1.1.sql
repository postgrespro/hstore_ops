/* contrib/hstore_ops/hstore_ops--1.0--1.1.sql */

-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION hstore_ops UPDATE TO '1.1'" to load this file. \quit

CREATE OPERATOR CLASS gin_hstore_bytea_ops
FOR TYPE hstore USING gin
AS
	OPERATOR        7       @>,
	OPERATOR        9       ?(hstore,text),
	OPERATOR        10      ?|(hstore,text[]),
	OPERATOR        11      ?&(hstore,text[]),
	FUNCTION        1       byteacmp(bytea,bytea),
	FUNCTION        2       gin_extract_hstore(internal, internal),
	FUNCTION        3       gin_extract_hstore_query(internal, internal, int2, internal, internal),
	FUNCTION        4       gin_consistent_hstore(internal, int2, internal, int4, internal, internal),
	STORAGE         bytea;
