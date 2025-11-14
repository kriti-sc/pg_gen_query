PGFILEDESC = "Extension to convert a text to tile format"
EXTENSION = pg_make_title
MODULES = pg_make_title
DATA = pg_make_title--1.0.sql

# Use C++ compiler
CC = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -fPIC

# Link C++ standard library explicitly
PG_CPPFLAGS = -I$(shell pg_config --includedir-server) \
               -I/opt/homebrew/opt/curl/include
SHLIB_LINK += -lc++ -L/opt/homebrew/opt/curl/lib -lcurl

# Force the final link step to use clang++
override LDFLAGS += -lc++

# Standard PostgreSQL build rules
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# Force link with C++ (clang++ instead of clang)
%.so: %.o
	$(CC) $(CXXFLAGS) $(LDFLAGS) -bundle -bundle_loader $(shell $(PG_CONFIG) --bindir)/postgres $< -o $@ $(SHLIB_LINK)