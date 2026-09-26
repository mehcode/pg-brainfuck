srcdir := $(dir $(firstword $(MAKEFILE_LIST)))

EXTENSION := brainfuck
EXTVERSION := $(shell sed -n "s/^default_version *= *'\([^']*\)'.*/\1/p" $(srcdir)$(EXTENSION).control)

PG_CFLAGS = -std=c11 -Wpedantic -Wno-declaration-after-statement \
            -Wextra -Wshadow -Wformat=2 -Wold-style-definition \
            -Wconversion -Wno-sign-conversion -Wswitch-enum \
            -Wcast-qual -Wstrict-prototypes -Wmissing-declarations -Wundef \
            -Wwrite-strings -Wbad-function-cast -Wunused-macros \
            -Wnull-dereference -Walloca -Wshift-overflow=2 -Wformat-overflow=2 \
            -Wstringop-overflow=4 -Wformat-signedness

# Compiled into PG_MODULE_MAGIC_EXT
PG_CPPFLAGS = -DPGBF_VERSION='"$(EXTVERSION)"'

PG_CPPFLAGS += -I$(srcdir)src/include

# Mark the Postgres headers as system headers.
PG_CPPFLAGS += -isystem $(includedir_server)

REGRESS = $(patsubst $(srcdir)test/sql/%.sql,%,$(sort $(wildcard $(srcdir)test/sql/*.sql)))
REGRESS_OPTS += --inputdir=$(srcdir)test

sources := $(sort $(wildcard $(srcdir)src/*.c))
headers := $(sort $(wildcard $(srcdir)src/include/*/*.h))

MODULE_big = $(EXTENSION)
OBJS = $(patsubst $(srcdir)%.c,%.o,$(sources))
DATA_built = sql/$(EXTENSION)--$(EXTVERSION).sql

builddirs := $(abspath $(sort $(dir $(OBJS) $(DATA_built))))

PG_CONFIG ?= pg_config

# Clean up generated files.
EXTRA_CLEAN = compile_commands.json

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

$(DATA_built): sql/$(EXTENSION).sql
	cp $< $@

# Rebuild when the version changes
$(OBJS) $(OBJS:.o=.bc): $(EXTENSION).control

# Rebuild when headers change
$(OBJS) $(OBJS:.o=.bc): $(headers)

# PGXS doesn't create output subdirectories in a VPATH build.
$(OBJS) $(OBJS:.o=.bc) $(DATA_built): | $(builddirs)
$(builddirs):
	@mkdir -p $@

.PHONY: lsp # Generate compile_commands.json for IDE/clangd support.
lsp: compile_commands.json

# Requires https://github.com/rizsotto/Bear.
compile_commands.json: $(firstword $(MAKEFILE_LIST)) $(srcdir)$(EXTENSION).control $(srcdir)src
	@bear -- $(MAKE) -B -f $(firstword $(MAKEFILE_LIST)) all with_llvm=no

.PHONY: format # Format .c and .h files to project standard.
format: $(sources) $(headers)
	@clang-format --style=file:$(srcdir).clang-format -i $^

.PHONY: tidy # Run clang-tidy static analysis (requires compile_commands.json).
tidy: compile_commands.json
	@clang-tidy -p . --quiet --warnings-as-errors='*' \
		--extra-arg=-DUSE_ASSERT_CHECKING \
		--extra-arg=-Wno-unknown-warning-option \
		$(sources)
