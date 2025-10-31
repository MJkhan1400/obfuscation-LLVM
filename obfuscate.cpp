#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <ctime>
#include <libgen.h> // For dirname()
#include <cstdio> // For std::remove
#include <cstring>

void printUsage(const char *progName) {
    std::cout << "LLVM Code Obfuscator - CLI Tool\n";
    std::cout << "================================\n\n";
    std::cout << "Usage: " << progName << " [options] <input.cpp>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -o <file>       Output file name (default: <input>_obfuscated)\n";
    std::cout << "  -r <file>       Report file name (default: obfuscation_report.txt)\n";
    std::cout << "  -l <level>      Obfuscation level: low, medium, high (default: medium)\n";
    std::cout << "  --windows       Generate Windows executable (cross-compile)\n";
    std::cout << "  --linux         Generate Linux executable (default)\n";
    std::cout << "  --emit-ll       Emit human-readable LLVM IR (.ll file)\n";
    std::cout << "  --no-bogus-blocks Disable bogus block obfuscation\n";
    std::cout << "  --no-fake-loops   Disable fake loop obfuscation\n";
    std::cout << "  --no-instr-sub    Disable instruction substitution obfuscation\n";
    std::cout << "  -f, --force       Force overwrite of existing output files\n";
    std::cout << "  -h, --help      Show this help message\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << progName << " main.cpp -o obfuscated_main\n";
    std::cout << "  " << progName << " main.cpp --windows -r report.txt\n";
}

long getFileSize(const std::string &filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string inputFile;
    std::string outputFile;
    std::string reportFile = "obfuscation_report.txt";
    std::string level = "medium";
    std::string platform = "linux";
    bool emitLL = false;
    bool enableBogusBlocks = true;
    bool enableFakeLoops = true;
    bool enableInstrSub = true;
    bool forceOverwrite = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "-r" && i + 1 < argc) {
            reportFile = argv[++i];
        } else if (arg == "-l" && i + 1 < argc) {
            level = argv[++i];
        } else if (arg == "--windows") {
            platform = "windows";
        } else if (arg == "--linux") {
            platform = "linux";
        } else if (arg == "--emit-ll") {
            emitLL = true;
        } else if (arg == "--no-bogus-blocks") {
            enableBogusBlocks = false;
        } else if (arg == "--no-fake-loops") {
            enableFakeLoops = false;
        } else if (arg == "--no-instr-sub") {
            enableInstrSub = false;
        } else if (arg == "-f" || arg == "--force") {
            forceOverwrite = true;
        } else if (arg[0] != '-') {
            inputFile = arg;
        }
    }
    
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Check if input file exists
    std::ifstream test(inputFile);
    if (!test.good()) {
        std::cerr << "Error: Input file '" << inputFile << "' not found\n";
        return 1;
    }
    test.close();

    // Create a build directory in the same location as the input file
    std::string buildDir = "build";
    mkdir(buildDir.c_str(), 0755);
    
    // Set default output name
    if (outputFile.empty()) {
        size_t dotPos = inputFile.find_last_of('.');
        if (dotPos != std::string::npos) {
            outputFile = inputFile.substr(0, dotPos) + "_obfuscated";
        } else {
            outputFile = inputFile + "_obfuscated";
        }
    }

    // Prepend build directory to output paths
    outputFile = buildDir + "/" + outputFile.substr(outputFile.find_last_of('/') + 1);
    if (reportFile.rfind(buildDir + "/", 0) != 0) { // Check if reportFile already starts with buildDir/
        reportFile = buildDir + "/" + reportFile;
    }


    // Check if output file exists
    if (!forceOverwrite) {
        std::ifstream checkExists(outputFile);
        if (checkExists.good()) {
            std::cerr << "Error: Output file '" << outputFile << "' already exists. Use -f to overwrite.\n";
            return 1;
        }
    }
    
    std::cout << "========================================\n";
    std::cout << "LLVM Code Obfuscator\n";
    std::cout << "========================================\n";
    std::cout << "Input File:      " << inputFile << "\n";
    std::cout << "Output File:     " << outputFile << "\n";
    std::cout << "Report File:     " << reportFile << "\n";
    std::cout << "Obfuscation:     " << level << "\n";
    std::cout << "Target Platform: " << platform << "\n";
    std::cout << "========================================\n\n";
    
    // Step 1: Compile to LLVM IR
    std::cout << "[1/5] Compiling to LLVM IR...\n";
    std::string bcFile = outputFile + ".bc";
    std::string cmd = "clang++ -emit-llvm -c " + inputFile + " -o " + bcFile;
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "Error: Compilation failed\n";
        return 1;
    }
    std::cout << "      Generated: " << bcFile << "\n";

    
    // Step 2: Apply obfuscation pass
    std::cout << "[2/5] Applying obfuscation transformations...\n";
    std::string obfBcFile = outputFile + "_obf.bc";
    
    // Get the directory where this binary is located
    std::string pluginPath = "obfuscator_pass/build/ObfuscatorPass.so";
    
        std::string passes = "obfuscator-pass";
    
    std::string optFlags = " -bogus-blocks=" + std::string(enableBogusBlocks ? "true" : "false") +
                           " -fake-loops=" + std::string(enableFakeLoops ? "true" : "false") +
                           " -instr-sub=" + std::string(enableInstrSub ? "true" : "false");

    std::string optLogFile = buildDir + "/opt_output.log";
    cmd = "opt -load-pass-plugin=./" + pluginPath + 
          " -passes='" + passes + "'" + optFlags + " " + 
          " -report-file=" + reportFile + 
          " " + bcFile + " -o " + obfBcFile;
    result = system(cmd.c_str());
	if (result != 0) {
    std::cerr << "Error: Obfuscation pass failed\n";
    std::cerr << "Make sure ObfuscatorPass.so is built\n";
    return 1;
}
    if (result != 0) {
        std::cerr << "Error: Obfuscation pass failed\n";
        std::cerr << "Make sure ObfuscatorPass.so is built\n";
        return 1;
    }
    std::cout << "      Generated: " << obfBcFile << "\n";
    
    // Step 3: Emit human-readable LLVM IR if requested
    if (emitLL) {
        std::cout << "[3/5] Emitting human-readable LLVM IR...\n";
        std::string llFile = outputFile + "_obf.ll";
        cmd = "llvm-dis " + obfBcFile + " -o " + llFile;
        result = system(cmd.c_str());
        if (result != 0) {
            std::cerr << "Error: llvm-dis failed\n";
        } else {
            std::cout << "      Generated: " << llFile << "\n";
        }
    }

    // Step 4: Generate executable
    std::cout << "[4/5] Generating executable...\n";
    if (platform == "windows") {
        // Cross-compile for Windows
        std::cout << "      Attempting Windows cross-compilation...\n";
        cmd = "x86_64-w64-mingw32-g++ " + obfBcFile + " -o " + outputFile + ".exe -static-libgcc -static-libstdc++";
        result = system(cmd.c_str());
        if (result != 0) {
            std::cerr << "      Warning: Windows cross-compilation failed.\n";
            std::cerr << "      Make sure mingw-w64 is installed: sudo pacman -S mingw-w64-gcc\n";
            std::cerr << "      Falling back to LLVM cross-compile...\n";
            cmd = "clang++ --target=x86_64-w64-mingw32 " + obfBcFile + " -o " + outputFile + ".exe 2>&1";
            result = system(cmd.c_str());
            if (result != 0) {
                std::cerr << "      Error: Windows compilation failed. Generating Linux binary instead.\n";
                platform = "linux";
                cmd = "clang++ " + obfBcFile + " -o " + outputFile;
                system(cmd.c_str());
            }
        }
    } else {
        // Compile for Linux
        cmd = "clang++ " + obfBcFile + " -o " + outputFile;
        result = system(cmd.c_str());
    }
    
    std::string finalBinary = outputFile + (platform == "windows" ? ".exe" : "");
    if (result == 0) {
        std::cout << "      ✓ Generated: " << finalBinary << "\n";
    }
    else {
        std::cerr << "      ✗ Compilation had issues\n";
    }
    
    // Step 5: Clean up intermediate files
    std::cout << "[5/5] Cleaning up intermediate files...\n";
    if (std::remove(bcFile.c_str()) != 0) {
        std::cerr << "      Warning: Could not delete " << bcFile << "\n";
    }
    // if (std::remove(obfBcFile.c_str()) != 0) {
    //     std::cerr << "      Warning: Could not delete " << obfBcFile << "\n";
    // }
    
    // Step 6: Summary
    std::cout << "[6/6] Done!\n\n";
    std::cout << "========================================\n";
    std::cout << "Obfuscation Complete!\n";
    std::cout << "========================================\n";
    std::cout << "Output binary: " << outputFile << (platform == "windows" ? ".exe" : "") << "\n";
    std::cout << "Report: " << reportFile << "\n";
    std::cout << "========================================\n";
    
    return 0;
}
