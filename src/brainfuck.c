#include "postgres.h"

#include "fmgr.h"

#ifdef PG_MODULE_MAGIC_EXT
PG_MODULE_MAGIC_EXT(.name = "brainfuck", .version = PGBF_VERSION);
#else
PG_MODULE_MAGIC;
#endif
