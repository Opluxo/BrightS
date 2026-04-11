# BrightS Operating System Build System
# Makefile wrapper for CMake

.PHONY: all build clean test debug release install uninstall help

# Build configuration
BUILD_DIR ?= build
BUILD_TYPE ?= Release

# Default target
all: build

# Create build directory and configure with CMake
configure:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ..

# Build all targets
build: configure
	cd $(BUILD_DIR) && make -j$(shell nproc)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Run tests
test: build
	cd $(BUILD_DIR) && ctest --output-on-failure

# Debug build
debug:
	$(MAKE) BUILD_TYPE=Debug all

# Release build
release:
	$(MAKE) BUILD_TYPE=Release all

# Run in QEMU
run: build
	cd $(BUILD_DIR) && make run

# Debug with QEMU
run-debug: build
	cd $(BUILD_DIR) && make run-debug

# Install (if supported)
install: build
	cd $(BUILD_DIR) && make install

# Uninstall (if supported)
uninstall:
	cd $(BUILD_DIR) && make uninstall

# Clean and rebuild
rebuild: clean build

# Show help
help:
	@echo "BrightS Operating System Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build all components (default)"
	@echo "  build      - Build all targets"
	@echo "  clean      - Remove build artifacts"
	@echo "  test       - Run test suite"
	@echo "  debug      - Build with debug symbols"
	@echo "  release    - Build optimized release"
	@echo "  run        - Run system in QEMU"
	@echo "  run-debug  - Run with GDB debugging"
	@echo "  install    - Install system files"
	@echo "  uninstall  - Remove installed files"
	@echo "  rebuild    - Clean and rebuild"
	@echo "  help       - Show this help"
	@echo ""
	@echo "Configuration:"
	@echo "  BUILD_DIR  - Build directory (default: build)"
	@echo "  BUILD_TYPE - CMake build type (default: Release)"

# Development targets
.PHONY: docs format lint

# Generate documentation
docs:
	@echo "Generating documentation..."
	# Add documentation generation commands here

# Format code
format:
	@echo "Formatting code..."
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i

# Lint code
lint:
	@echo "Running linter..."
	find . -name "*.c" -o -name "*.h" | xargs clang-tidy --quiet

# Show build info
info:
	@echo "BrightS Build Information"
	@echo "========================"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "Build type: $(BUILD_TYPE)"
	@echo "Parallel jobs: $(shell nproc)"
	@echo "CMake version: $(shell cmake --version | head -1)"
	@echo "Compiler: $(shell cc --version | head -1)"

# Aliases for common operations
.PHONY: b t r rd

b: build
t: test
r: run
rd: run-debug