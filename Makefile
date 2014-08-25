# contrib/hstore_ops/Makefile

MODULE_big = hstore_ops
OBJS = hstore_compat.o hstore_ops.o

EXTENSION = hstore_ops
DATA = hstore_ops--1.1.sql hstore_ops--1.0--1.1.sql

REGRESS = hstore_ops

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/hstore_ops
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
