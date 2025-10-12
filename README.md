# LLVM Code Obfuscator

A powerful LLVM-based code obfuscation tool for C/C++ programs that generates hardened binaries for Linux and Windows platforms with detailed reporting.

## 📋 Overview

This tool leverages LLVM's compiler infrastructure to apply multiple obfuscation techniques to C/C++ code, making reverse engineering significantly more difficult while maintaining functional correctness.

### Key Features

- ✅ **Multiple Obfuscation Techniques**
  - Bogus code injection (dead code that never executes)
  - Fake loop insertion (unreachable loops)
  - Instruction substitution (complex equivalents for simple operations)
  - Control flow obfuscation
  
- ✅ **Cross-Platform Support**
  - Linux native binaries
  - Windows cross-compilation support
  
- ✅ **Detailed Reporting**
  - Input parameter logging
  - Output file attributes
  - Obfuscation statistics
  - Code size impact analysis

## 🏗️ Project Structure

```
ollvm/
├── obfuscate                         # CLI tool executable
├── obfuscator_pass/                  # LLVM pass plugin
│   ├── Obfuscator.cpp               # Obfuscation logic
│   ├── CMakeLists.txt               # Build configuration
│   └── build/
│       └── ObfuscatorPass.so        # Compiled plugin
└── obfuscate.cpp                    # CLI tool source
```

## 🔧 Prerequisites

### Required Software

- **LLVM 20+** (with development headers)
- **Clang++** (C++ compiler with LLVM support)
- **CMake 3.10+** (optional, for build configuration)
- **MinGW-w64** (for Windows cross-compilation, optional)

### Installation on Arch Linux

```bash
sudo pacman -S llvm clang cmake
# For Windows cross-compilation:
sudo pacman -S mingw-w64-gcc
```

## 🚀 Building the Project

### Step 1: Build the LLVM Pass

```bash
cd ~/ollvm/obfuscator_pass

# Compile the obfuscation pass
clang++ -shared -fPIC \
    $(llvm-config --cxxflags) \
    -o build/ObfuscatorPass.so \
    Obfuscator.cpp \
    $(llvm-config --ldflags --libs core passes support)
```

Verify it was built:
```bash
ls -lh build/ObfuscatorPass.so
# Should show a file around 100KB
```

### Step 2: Build the CLI Tool

```bash
cd ~/ollvm

# Compile the CLI tool
clang++ -o obfuscate obfuscate.cpp -std=c++17
```

## 📖 Usage

### Basic Usage

```bash
./obfuscate input.cpp
```

This will:
1. Compile `input.cpp` to LLVM IR
2. Apply obfuscation transformations
3. Generate `input_obfuscated` executable
4. Create `obfuscation_report.txt`

### Advanced Usage

```bash
./obfuscate [options] <input.cpp>

Options:
  -o <file>       Output file name (default: <input>_obfuscated)
  -r <file>       Report file name (default: obfuscation_report.txt)
  -l <level>      Obfuscation level: low, medium, high (default: medium)
  --windows       Generate Windows executable
  --linux         Generate Linux executable (default)
  -h, --help      Show help message
```

### Examples

```bash
# Basic obfuscation
./obfuscate main.cpp

# Custom output name
./obfuscate main.cpp -o protected_app

# Generate Windows executable
./obfuscate main.cpp --windows -o app_win

# Custom report location
./obfuscate main.cpp -r logs/obfuscation_log.txt

# High obfuscation level (future feature)
./obfuscate main.cpp -l high
```

## 🔬 Manual Testing (Using LLVM Tools Directly)

For development and debugging:

```bash
# Step 1: Compile to LLVM IR
clang++ -emit-llvm -c main.cpp -o main.bc

# Step 2: Apply obfuscation
opt -load-pass-plugin=./obfuscator_pass/build/ObfuscatorPass.so \
    -passes="obfuscator-pass" \
    main.bc -o main_obf.bc 2>&1

# Step 3: View human-readable IR
llvm-dis main_obf.bc -o main_obf.ll
cat main_obf.ll

# Step 4: Compile to executable
clang++ main_obf.bc -o hello_obfuscated
```

## 📊 Report Format

The generated report includes:

```
========================================
LLVM Obfuscation Report
========================================
Generation Time: 2025-10-12 22:59:45
Tool Version: 1.0

--- Input Parameters ---
Input File: main.cpp
Output File: main_obfuscated
Obfuscation Level: medium
Target Platform: linux
String Encryption: Enabled
Bogus Code Injection: Enabled
Fake Loop Insertion: Enabled
Instruction Substitution: Enabled

--- Output File Attributes ---
File Size (original): 103 bytes
File Size (obfuscated): 16384 bytes
Size Increase: 16281 bytes
Methods Applied:
  - Control Flow Obfuscation
  - Bogus Code Insertion
  - Fake Loop Injection
  - Instruction Substitution

--- Obfuscation Statistics ---
Total Instructions Processed: 5
Total Basic Blocks: 1
String Obfuscations: 1
Bogus Code Blocks Added: 2
Fake Loops Inserted: 1
Instruction Substitutions: 0

--- Code Size Impact ---
Original Instructions: ~5
Bogus Instructions Added: ~11
Code Size Increase: ~220%

--- Obfuscation Cycles ---
Number of Passes Completed: 1
Functions Obfuscated: 1

========================================
Obfuscation completed successfully!
========================================
```

## 🛡️ Obfuscation Techniques Explained

### 1. **Bogus Code Injection**
Inserts unreachable code blocks with fake computations that appear legitimate but never execute.

**Example:**
```cpp
// Original
int x = 5;

// After obfuscation (simplified view)
int x = 5;
if (1 == 0) {  // Always false
    int fake1 = 42;
    int fake2 = fake1 + 13;
    // ... more fake operations
}
```

### 2. **Fake Loop Insertion**
Adds loops that look real but are controlled by impossible conditions.

**Example:**
```cpp
// Original
return result;

// After obfuscation
if (false) {  // Never executes
    for (int i = 0; i < 10; i++) {
        // fake computations
    }
}
return result;
```

### 3. **Instruction Substitution**
Replaces simple operations with mathematically equivalent but more complex ones.

**Example:**
```cpp
// Original
int sum = a + b;

// After obfuscation
int sum = a - (-b);  // Equivalent but more complex
```

### 4. **Control Flow Obfuscation**
Adds conditional branches that make the control flow graph more complex.

## 🎯 Assignment Requirements Checklist

### ✅ Core Requirements

- [x] **LLVM-based obfuscation** - Uses LLVM pass infrastructure
- [x] **C/C++ support** - Compiles C and C++ code
- [x] **Linux binaries** - Native Linux executable generation
- [x] **Windows binaries** - Cross-compilation support via MinGW
- [x] **Configurable parameters** - CLI options for customization
- [x] **Difficult to reverse** - Multiple obfuscation layers

### ✅ Report Requirements

- [x] **a. Input parameters logged** - All CLI options recorded
- [x] **b. Output file attributes** - Size, methods, platform
- [x] **c. Bogus code information** - Count and size impact
- [x] **d. Obfuscation cycles** - Number of passes completed
- [x] **e. String obfuscation count** - Encrypted strings tracked
- [x] **f. Fake loops count** - Inserted fake loops logged

### ✅ Deliverables

- [x] **Obfuscated executable** - Binary file generated
- [x] **Detailed report** - Text file with all metrics

## 🔍 Understanding LLVM IR

LLVM IR (Intermediate Representation) is the key to this obfuscation process.

### Compilation Pipeline

```
Source Code (.cpp) 
    ↓ [Clang Frontend]
LLVM IR (.bc/.ll)
    ↓ [Obfuscation Pass]
Obfuscated IR (.bc)
    ↓ [LLVM Backend]
Machine Code (executable)
```

### Viewing IR

```bash
# Compile to IR
clang++ -emit-llvm -S main.cpp -o main.ll

# View the IR
cat main.ll
```

Example IR for `int x = 5;`:
```llvm
%1 = alloca i32, align 4
store i32 5, ptr %1, align 4
```

## 🧪 Testing and Verification

### Test 1: Basic Functionality

```bash
# Create test file
cat > test.cpp << 'EOF'
#include <iostream>
int main() {
    int x = 10;
    int y = 20;
    int sum = x + y;
    std::cout << "Sum: " << sum << std::endl;
    return 0;
}
EOF

# Obfuscate it
./obfuscate test.cpp -o test_obf

# Run original
clang++ test.cpp -o test_orig
./test_orig
# Output: Sum: 30

# Run obfuscated
./test_obf
# Output: Sum: 30 (should be identical)
```

### Test 2: Verify Obfuscation

```bash
# Compare IR before and after
clang++ -emit-llvm -S test.cpp -o test_before.ll

opt -load-pass-plugin=./obfuscator_pass/build/ObfuscatorPass.so \
    -passes="obfuscator-pass" \
    test.bc -S -o test_after.ll

# Check the difference
diff test_before.ll test_after.ll
# Should show added bogus blocks, fake loops, etc.
```

### Test 3: Binary Size Increase

```bash
# Check original size
clang++ test.cpp -o test_orig
ls -lh test_orig

# Check obfuscated size
./obfuscate test.cpp -o test_obf
ls -lh test_obf

# Obfuscated should be larger due to bogus code
```

## 🐛 Troubleshooting

### Issue: "cannot find -lLLVMCore"

**Solution:** Use the direct clang++ compilation method instead of CMake:
```bash
cd obfuscator_pass
clang++ -shared -fPIC \
    $(llvm-config --cxxflags) \
    -o build/ObfuscatorPass.so \
    Obfuscator.cpp \
    $(llvm-config --ldflags --libs core passes support)
```

### Issue: Pass not found

**Solution:** Verify the plugin exports the correct symbol:
```bash
nm -D obfuscator_pass/build/ObfuscatorPass.so | grep llvmGetPassPluginInfo
```

Should show:
```
000000000000aea0 W llvmGetPassPluginInfo
```

### Issue: No obfuscation applied

**Solution:** Check if the pass is running:
```bash
opt -load-pass-plugin=./obfuscator_pass/build/ObfuscatorPass.so \
    -passes="obfuscator-pass" \
    -debug-pass-manager \
    main.bc -o main_obf.bc 2>&1
```

You should see:
```
Running pass: (anonymous namespace)::ObfuscatorPass on main
```

### Issue: Windows cross-compilation fails

**Solution:** Install MinGW toolchain:
```bash
sudo pacman -S mingw-w64-gcc
```

Or compile on Linux only (skip --windows flag).

## 📚 Technical Details

### LLVM Pass Types

This project uses a **Function Pass** which operates on individual functions:

```cpp
struct ObfuscatorPass : public PassInfoMixin<ObfuscatorPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        // Transformation logic here
        return PreservedAnalyses::none();
    }
};
```

### Why LLVM IR?

1. **Platform Independent**: Works on any target LLVM supports
2. **High Level**: Easier to analyze than assembly
3. **Low Level**: Close enough to machine code for effective obfuscation
4. **Standardized**: Well-documented format

### Obfuscation vs Optimization

- **Optimization**: Makes code faster/smaller
- **Obfuscation**: Makes code harder to understand

LLVM's optimization passes can be used in reverse to add complexity!

## 🚀 Future Enhancements

Potential improvements for the project:

1. **Advanced String Encryption**: XOR-based encryption with runtime decryption
2. **Control Flow Flattening**: Convert structured code to switch-based dispatch
3. **Opaque Predicates**: Conditions that are hard to evaluate statically
4. **Virtual Machine Obfuscation**: Convert code to custom bytecode
5. **Anti-Debugging**: Detect and prevent debugger attachment
6. **Symbol Stripping**: Remove function/variable names
7. **Multiple Pass Cycles**: Apply obfuscation repeatedly
8. **Configurable Intensity**: Fine-tune each technique independently

## 📖 References

- [LLVM Documentation](https://llvm.org/docs/)
- [Writing an LLVM Pass](https://llvm.org/docs/WritingAnLLVMPass.html)
- [LLVM IR Language Reference](https://llvm.org/docs/LangRef.html)
- [Code Obfuscation Techniques](https://en.wikipedia.org/wiki/Obfuscation_(software))

## 👨‍💻 Development Notes

### Building with Debug Info

```bash
clang++ -g -O0 -shared -fPIC \
    $(llvm-config --cxxflags) \
    -o build/ObfuscatorPass.so \
    Obfuscator.cpp \
    $(llvm-config --ldflags --libs core passes support)
```

### Verbose Output

```bash
./obfuscate main.cpp 2>&1 | tee obfuscation.log
```

### Clean Build

```bash
rm -rf obfuscator_pass/build/*
rm -f *.bc *.ll obfuscate
```

## 📝 License

This project is created for educational purposes as part of an academic assignment on software obfuscation using LLVM.

## ✨ Summary

This LLVM-based obfuscator provides:
- **Protection**: Multiple layers of code obfuscation
- **Transparency**: Detailed reporting of all transformations
- **Flexibility**: Configurable parameters and cross-platform support
- **Correctness**: Maintains program functionality while adding complexity

The tool successfully demonstrates practical application of LLVM's compiler infrastructure for software protection purposes.
