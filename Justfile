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
