/*
 * contrib/hstore/hstore_gin.c
 */
#include "postgres.h"

#include "access/gin.h"
#include "access/skey.h"
#include "catalog/pg_type.h"
#include "utils/hsearch.h"

#include "hstore.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(gin_compare_hstore_hash);
Datum		gin_compare_hstore_hash(PG_FUNCTION_ARGS);

Datum
gin_compare_hstore_hash(PG_FUNCTION_ARGS)
{
	uint64		arg1 = (uint64)PG_GETARG_INT64(0);
	uint64		arg2 = (uint64)PG_GETARG_INT64(1);
	int32		result;

	if (arg1 < arg2)
		result = -1;
	else if (arg1 == arg2)
		result = 0;
	else
		result = 1;

	PG_RETURN_INT32(result);
}

PG_FUNCTION_INFO_V1(gin_compare_partial_hstore_hash);
Datum		gin_compare_partial_hstore_hash(PG_FUNCTION_ARGS);

Datum
gin_compare_partial_hstore_hash(PG_FUNCTION_ARGS)
{
	uint64		partial_key = PG_GETARG_INT64(0);
	uint64		key = PG_GETARG_INT64(1);
	int32		result;

	if ((key & 0xFFFFFFFF00000000) > partial_key)
	{
		result = 1;
	}
	else
	{
		if ((key & 0xFFFFFFFF00000000) == partial_key)
			result = 0;
		else
			result = -1;
	}

	PG_RETURN_INT32(result);
}

static uint64
get_entry_hash(HEntry *hsent, char *ptr, int i)
{
	uint64 result = 0;

	result |= (uint64) tag_hash(HS_KEY(hsent, ptr, i), HS_KEYLEN(hsent, i)) << 32;

	if (HS_VALISNULL(hsent, i))
		result |= (uint64) tag_hash(HS_VAL(hsent, ptr, i), HS_VALLEN(hsent, i));

	return result;
}

static uint64
get_key_hash(text *key)
{
	uint64 result = 0;

	result |= (uint64) tag_hash(VARDATA_ANY(key), VARSIZE_ANY_EXHDR(key)) << 32;

	return result;
}

PG_FUNCTION_INFO_V1(gin_extract_hstore_hash);
Datum		gin_extract_hstore_hash(PG_FUNCTION_ARGS);

Datum
gin_extract_hstore_hash(PG_FUNCTION_ARGS)
{
	HStore	   *hs = PG_GETARG_HS(0);
	int32	   *nentries = (int32 *) PG_GETARG_POINTER(1);
	Datum	   *entries = NULL;
	HEntry	   *hsent = ARRPTR(hs);
	char	   *ptr = STRPTR(hs);
	int			count = HS_COUNT(hs);
	int			i;

	*nentries = count;
	if (count)
		entries = (Datum *) palloc(sizeof(Datum) * count);

	for (i = 0; i < count; ++i)
		entries[i] = Int64GetDatum(get_entry_hash(hsent, ptr, i));

	PG_RETURN_POINTER(entries);
}

PG_FUNCTION_INFO_V1(gin_extract_hstore_query_hash);
Datum		gin_extract_hstore_query_hash(PG_FUNCTION_ARGS);

Datum
gin_extract_hstore_query_hash(PG_FUNCTION_ARGS)
{
	int32	   *nentries = (int32 *) PG_GETARG_POINTER(1);
	StrategyNumber strategy = PG_GETARG_UINT16(2);
	bool	  **pmatch = (bool **) PG_GETARG_POINTER(3);
	int32	   *searchMode = (int32 *) PG_GETARG_POINTER(6);
	Datum	   *entries;

	if (strategy == HStoreContainsStrategyNumber)
	{
		/* Query is an hstore, so just apply gin_extract_hstore... */
		entries = (Datum *)
			DatumGetPointer(DirectFunctionCall2(gin_extract_hstore_hash,
												PG_GETARG_DATUM(0),
												PointerGetDatum(nentries)));
		/* ... except that "contains {}" requires a full index scan */
		if (entries == NULL)
			*searchMode = GIN_SEARCH_MODE_ALL;
	}
	else if (strategy == HStoreExistsStrategyNumber)
	{
		text	   *query = PG_GETARG_TEXT_PP(0);

		*nentries = 1;
		entries = (Datum *) palloc(sizeof(Datum));
		*pmatch = (bool *) palloc(sizeof(bool));
		entries[0] = Int64GetDatum(get_key_hash(query));
		(*pmatch)[0] = true;
	}
	else if (strategy == HStoreExistsAnyStrategyNumber ||
			 strategy == HStoreExistsAllStrategyNumber)
	{
		ArrayType  *query = PG_GETARG_ARRAYTYPE_P(0);
		Datum	   *key_datums;
		bool	   *key_nulls;
		int			key_count;
		int			i,
					j;

		deconstruct_array(query,
						  TEXTOID, -1, false, 'i',
						  &key_datums, &key_nulls, &key_count);

		entries = (Datum *) palloc(sizeof(Datum) * key_count);
		*pmatch = (bool *) palloc(sizeof(bool) * key_count);

		for (i = 0, j = 0; i < key_count; ++i)
		{
			/* Nulls in the array are ignored, cf hstoreArrayToPairs */
			if (key_nulls[i])
				continue;
			(*pmatch)[j] = true;
			entries[j++] = Int64GetDatum(get_key_hash(DatumGetTextPP(key_datums[i])));
		}

		*nentries = j;
		/* ExistsAll with no keys should match everything */
		if (j == 0 && strategy == HStoreExistsAllStrategyNumber)
			*searchMode = GIN_SEARCH_MODE_ALL;
	}
	else
	{
		elog(ERROR, "unrecognized strategy number: %d", strategy);
		entries = NULL;			/* keep compiler quiet */
	}

	PG_RETURN_POINTER(entries);
}

PG_FUNCTION_INFO_V1(gin_consistent_hstore_hash);
Datum		gin_consistent_hstore_hash(PG_FUNCTION_ARGS);

Datum
gin_consistent_hstore_hash(PG_FUNCTION_ARGS)
{
	bool	   *check = (bool *) PG_GETARG_POINTER(0);
	StrategyNumber strategy = PG_GETARG_UINT16(1);

	/* HStore	   *query = PG_GETARG_HS(2); */
	int32		nkeys = PG_GETARG_INT32(3);

	/* Pointer	   *extra_data = (Pointer *) PG_GETARG_POINTER(4); */
	bool	   *recheck = (bool *) PG_GETARG_POINTER(5);
	bool		res = true;
	int32		i;

	if (strategy == HStoreContainsStrategyNumber)
	{
		/*
		 * Index doesn't have information about correspondence of keys and
		 * values, so we need recheck.	However, if not all the keys are
		 * present, we can fail at once.
		 */
		*recheck = true;
		for (i = 0; i < nkeys; i++)
		{
			if (!check[i])
			{
				res = false;
				break;
			}
		}
	}
	else if (strategy == HStoreExistsStrategyNumber)
	{
		/* Existence of key is guaranteed in default search mode */
		*recheck = false;
		res = true;
	}
	else if (strategy == HStoreExistsAnyStrategyNumber)
	{
		/* Existence of key is guaranteed in default search mode */
		*recheck = false;
		res = true;
	}
	else if (strategy == HStoreExistsAllStrategyNumber)
	{
		/* Testing for all the keys being present gives an exact result */
		*recheck = false;
		for (i = 0; i < nkeys; i++)
		{
			if (!check[i])
			{
				res = false;
				break;
			}
		}
	}
	else
		elog(ERROR, "unrecognized strategy number: %d", strategy);

	PG_RETURN_BOOL(res);
}
