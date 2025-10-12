#!/bin/bash
# setup.sh - Build the LLVM Obfuscator project

set -e  # Exit on error

echo "========================================="
echo "LLVM Code Obfuscator - Setup Script"
echo "========================================="

# Check prerequisites
echo ""
echo "[1/4] Checking prerequisites..."
command -v clang++ >/dev/null 2>&1 || { echo "Error: clang++ not found"; exit 1; }
command -v llvm-config >/dev/null 2>&1 || { echo "Error: llvm-config not found"; exit 1; }
echo "      ✓ clang++ found: $(clang++ --version | head -n1)"
echo "      ✓ LLVM found: $(llvm-config --version)"

# Check for Windows cross-compilation support
if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    echo "      ✓ MinGW-w64 found (Windows support enabled)"
    WINDOWS_SUPPORT=true
else
    echo "      ⚠ MinGW-w64 not found (Windows support disabled)"
    echo "        Install with: sudo pacman -S mingw-w64-gcc"
    WINDOWS_SUPPORT=false
fi

# Build the LLVM pass
echo ""
echo "[2/4] Building LLVM obfuscation pass..."
cd obfuscator_pass
mkdir -p build

clang++ -shared -fPIC \
    $(llvm-config --cxxflags) \
    -o build/ObfuscatorPass.so \
    Obfuscator.cpp \
    $(llvm-config --ldflags --libs core passes support)

if [ ! -f "build/ObfuscatorPass.so" ]; then
    echo "      ✗ Failed to build ObfuscatorPass.so"
    exit 1
fi

echo "      ✓ Built: build/ObfuscatorPass.so ($(du -h build/ObfuscatorPass.so | cut -f1))"

# Verify the plugin symbol
if nm -D build/ObfuscatorPass.so | grep -q "llvmGetPassPluginInfo"; then
    echo "      ✓ Plugin symbol verified"
else
    echo "      ✗ Warning: Plugin symbol not found"
fi

cd ..

# Build the CLI tool
echo ""
echo "[3/4] Building CLI tool..."
clang++ -o obfuscate obfuscate.cpp -std=c++17

if [ ! -f "obfuscate" ]; then
    echo "      ✗ Failed to build obfuscate"
    exit 1
fi

chmod +x obfuscate
echo "      ✓ Built: obfuscate"

# Test the setup
echo ""
echo "[4/4] Testing the setup..."

# Create a test file if main.cpp doesn't exist
if [ ! -f "main.cpp" ]; then
    cat > main.cpp << 'EOF'
#include <iostream>

int main() {
    std::cout << "Hello, Obfuscation!" << std::endl;
    return 0;
}
EOF
    echo "      ✓ Created test file: main.cpp"
fi

# Compile to LLVM IR
clang++ -emit-llvm -c main.cpp -o main.bc 2>/dev/null
echo "      ✓ Compiled to LLVM IR: main.bc"

# Test the pass
opt -load-pass-plugin=./obfuscator_pass/build/ObfuscatorPass.so \
    -passes="obfuscator-pass" \
    main.bc -o main_obf.bc 2>&1 | grep -q "ObfuscatorPass"

if [ $? -eq 0 ]; then
    echo "      ✓ Obfuscation pass working!"
else
    echo "      ✗ Warning: Obfuscation pass test failed"
fi

# Compile the obfuscated version
clang++ main_obf.bc -o hello_obfuscated 2>/dev/null
if [ -f "hello_obfuscated" ]; then
    echo "      ✓ Generated test executable: hello_obfuscated"
fi

echo ""
echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo ""
echo "Usage:"
echo "  ./obfuscate main.cpp              # Basic usage"
echo "  ./obfuscate main.cpp -o output    # Custom output name"
echo "  ./obfuscate main.cpp --windows    # Windows binary"
echo "  ./obfuscate --help                # Show all options"
echo ""
echo "Files created:"
echo "  - obfuscator_pass/build/ObfuscatorPass.so  (LLVM pass)"
echo "  - obfuscate                                 (CLI tool)"
echo "  - hello_obfuscated                          (Test executable)"
echo ""
echo "Try running: ./hello_obfuscated"
echo "========================================="
