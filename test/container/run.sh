#!/bin/sh
set -eu

# Starts the test cluster built into the image.
sudo -u postgres "/usr/lib/postgresql/$PG_MAJOR/bin/pg_ctl" -D "/etc/postgresql/$PG_MAJOR/test" \
    -o "-c config_file=/etc/postgresql/$PG_MAJOR/test/postgresql.conf" \
    -w -l /tmp/postgres.log start

# Create a temporary working directory
# pg-build-test builds in the current directory
workdir=$(mktemp -d)
cp -a /src/. "$workdir"
cd "$workdir"
rm -rf build
make -s clean

status=0
BASH_XTRACEFD=3 GNUMAKEFLAGS=-s pg-build-test 3>/dev/null || status=$?

rm -rf /out/results
if [ -d results ]; then cp -r results /out/; fi

exit "$status"
