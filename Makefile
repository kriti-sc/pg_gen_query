PGFILEDESC = "Extension to convert a text to tile format"
EXTENSION = pg_make_title
DATA = pg_make_title--1.0.sql
PG_CONFIG = pg_config
MODULES = pg_make_title
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

