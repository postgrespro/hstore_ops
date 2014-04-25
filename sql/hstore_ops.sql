CREATE EXTENSION hstore;
CREATE EXTENSION hstore_ops;

set escape_string_warning=off;

CREATE TABLE testhstore (h hstore);
\copy testhstore from 'data/hstore.data'

create index hidx on testhstore using gin(h gin_hstore_hash_ops);
set enable_seqscan=off;

select count(*) from testhstore where h @> 'wait=>NULL';
select count(*) from testhstore where h @> 'wait=>CC';
select count(*) from testhstore where h @> 'wait=>CC, public=>t';
select count(*) from testhstore where h ? 'public';
select count(*) from testhstore where h ?| ARRAY['public','disabled'];
select count(*) from testhstore where h ?& ARRAY['public','disabled'];
