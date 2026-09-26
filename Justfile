alias fmt := format
alias tidy := check-tidy

# Clean the built and generated files.
clean:
    @rm -rf build/ .cache/

# Build the extension.
build:
    @mkdir -p build
    @make -s -C build -f ../Makefile COPT=-Werror all

# Generate compile_commands.json for IDE/clangd support.
lsp:
    @mkdir -p build
    @make -s -C build -f ../Makefile lsp

# Format .c and .h files to project standard.
format:
    @make -s format

# Build with warnings as errors and run clang-tidy.
check: build check-tidy

# Run clang-tidy static analysis, failing on any warning.
check-tidy:
    @mkdir -p build
    @make -s -C build -f ../Makefile tidy

# Run the regression test suite for the specified PostgreSQL major version.
test PG_MAJOR:
    @echo -e "\x1b[34m::\x1b[0m \x1b[1mTesting \`brainfuck\` against Postgres v{{ PG_MAJOR }} ...\x1b[0m"
    @mkdir -p build/test-postgres-{{ PG_MAJOR }}
    @docker build -q -t pg-brainfuck-test:{{ PG_MAJOR }} test/container --build-arg PG_MAJOR={{ PG_MAJOR }} > /dev/null
    @docker run --rm -v "$PWD:/src:ro" -v "$PWD/build/test-postgres-{{ PG_MAJOR }}:/out" \
        -e AS_USER="$USER" -e LOCAL_UID="$(id -u)" \
        pg-brainfuck-test:{{ PG_MAJOR }}

# Run the regression test suite for all supported PostgreSQL major versions.
test-all:
    #!/bin/sh
    for v in 14 15 16 17 18 19; do
        {{ just_executable() }} test "$v"
        echo -e '\n'
    done
