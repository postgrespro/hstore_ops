/* contrib/hstore_ops/hstore_ops--1.1.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hstore_ops" to load this file. \quit

CREATE FUNCTION gin_compare_hstore_hash(int8, int8)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gin_compare_partial_hstore_hash(int8, int8, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gin_extract_hstore_hash(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gin_extract_hstore_query_hash(internal, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gin_consistent_hstore_hash(internal, int2, internal, int4, internal, internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS gin_hstore_hash_ops
FOR TYPE hstore USING gin
AS
	OPERATOR        7       @>,
	OPERATOR        9       ?(hstore,text),
	OPERATOR        10      ?|(hstore,text[]),
	OPERATOR        11      ?&(hstore,text[]),
	FUNCTION        1       gin_compare_hstore_hash(int8, int8),
	FUNCTION        2       gin_extract_hstore_hash(internal, internal),
	FUNCTION        3       gin_extract_hstore_query_hash(internal, internal, int2, internal, internal),
	FUNCTION        4       gin_consistent_hstore_hash(internal, int2, internal, int4, internal, internal),
	FUNCTION        5       gin_compare_partial_hstore_hash(int8, int8, int2, internal),
	STORAGE         int8;

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
