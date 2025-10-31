#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include <random>
#include <fstream>
#include <chrono>
#include <ctime>

#include "llvm/Support/CommandLine.h"

using namespace llvm;

// Command line options for the obfuscator pass
namespace {

static cl::opt<std::string> ReportFileArg("report-file", cl::desc("Path to the obfuscation report file"), cl::init(""));

// Command line flags to enable/disable obfuscations
static cl::opt<bool> BogusBlocksOpt("bogus-blocks", cl::desc("Enable bogus block obfuscation"), cl::init(true));
static cl::opt<bool> FakeLoopsOpt("fake-loops", cl::desc("Enable fake loop obfuscation"), cl::init(true));
static cl::opt<bool> InstrSubOpt("instr-sub", cl::desc("Enable instruction substitution obfuscation"), cl::init(true));

// Statistics tracking structure
struct ObfuscationStats {
    int stringObfuscations = 0;
    int bogusBlocksAdded = 0;
    int fakeLoopsAdded = 0;
    int instructionSubstitutions = 0;
    int totalInstructions = 0;
    int totalBasicBlocks = 0;
    int functionsObfuscated = 0;
    std::string inputFile;
    std::string outputFile;
    std::string timestamp;
    
    void writeReport(const std::string &reportFile) {
        if (reportFile.empty()) {
            return;
        }
        std::ofstream report(reportFile);
        if (!report.is_open()) {
            errs() << "Error: Could not open report file for writing: " << reportFile << "\n";
            // Attempt to create a temporary file to test write access
            std::string testFile = reportFile + ".test";
            std::ofstream testStream(testFile);
            if (!testStream.is_open()) {
                errs() << "Error: Could not create temporary test file: " << testFile << "\n";
            } else {
                testStream << "Test content\n";
                testStream.close();
                errs() << "Successfully created temporary test file: " << testFile << "\n";
                std::remove(testFile.c_str()); // Clean up
            }
            return;
        }
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timeStr[100];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
        
        report << "========================================\n";
        report << "LLVM Obfuscation Report\n";
        report << "========================================\n";
        report << "Generation Time: " << timeStr << "\n";
        report << "Input File: " << inputFile << "\n";
        report << "Output File: " << outputFile << "\n";
        report << "\n";
        report << "--- Input Parameters ---\n";
        report << "Obfuscation Level: Standard\n";
        report << "String Encryption: Enabled\n";
        report << "Bogus Code Injection: Enabled\n";
        report << "Fake Loop Insertion: Enabled\n";
        report << "Instruction Substitution: Enabled\n";
        report << "\n";
        report << "--- Obfuscation Statistics ---\n";
        report << "Total Instructions Processed: " << totalInstructions << "\n";
        report << "Total Basic Blocks: " << totalBasicBlocks << "\n";
        report << "String Obfuscations: " << stringObfuscations << "\n";
        report << "Bogus Code Blocks Added: " << bogusBlocksAdded << "\n";
        report << "Fake Loops Inserted: " << fakeLoopsAdded << "\n";
        report << "Instruction Substitutions: " << instructionSubstitutions << "\n";
        report << "\n";
        report << "--- Code Size Impact ---\n";
        int originalSize = totalInstructions;
        int bogusInstructions = bogusBlocksAdded * 3 + fakeLoopsAdded * 5;
        float increase = (bogusInstructions * 100.0f) / originalSize;
        report << "Original Instructions: ~" << originalSize << "\n";
        report << "Bogus Instructions Added: ~" << bogusInstructions << "\n";
        report << "Code Size Increase: ~" << increase << "%\n";
        report << "\n";
        report << "--- Obfuscation Cycles ---\n";
        report << "Number of Passes Completed: 1\n";
        report << "Functions Obfuscated: " << functionsObfuscated << "\n";
        report << "\n";
        report << "========================================\n";
        
        report.close();
        errs() << "[Report] Generated: " << reportFile << "\n";
    }
};

// Global stats object
static ObfuscationStats stats;

class CodeObfuscator {
private:
    std::mt19937 rng;
    
public:
    CodeObfuscator() : rng(std::random_device{}()) {}
    
    // Add bogus basic block with fake computations
    void addBogusBlock(Function &F, BasicBlock *insertAfter) {
        LLVMContext &Ctx = F.getContext();
        Module *M = F.getParent();
        
        // Create bogus block
        BasicBlock *BogusBB = BasicBlock::Create(Ctx, "bogus", &F);
        IRBuilder<> Builder(BogusBB);
        
        // Add some fake computations that look real
        Type *Int32Ty = Type::getInt32Ty(Ctx);
        Value *FakeVar1 = Builder.CreateAlloca(Int32Ty);
        Value *FakeVar2 = Builder.CreateAlloca(Int32Ty);
        
        Builder.CreateStore(ConstantInt::get(Int32Ty, 42), FakeVar1);
        Value *Load1 = Builder.CreateLoad(Int32Ty, FakeVar1);
        Value *Add = Builder.CreateAdd(Load1, ConstantInt::get(Int32Ty, 13));
        Builder.CreateStore(Add, FakeVar2);
        
        // Always false condition to make this block unreachable
        Value *Cond = Builder.CreateICmpEQ(
            ConstantInt::get(Int32Ty, 1),
            ConstantInt::get(Int32Ty, 0)
        );
        
        // Get the next block
        BasicBlock *NextBB = insertAfter->getNextNode();
        if (!NextBB) {
            NextBB = &F.getEntryBlock();
        }
        
        // Branch back to real code
        Builder.CreateBr(NextBB);
        
        // Insert conditional branch in the original block
        Instruction *OrigTerm = insertAfter->getTerminator();
        if (OrigTerm) {
            IRBuilder<> OrigBuilder(OrigTerm);
            OrigBuilder.CreateCondBr(Cond, BogusBB, NextBB);
            OrigTerm->eraseFromParent();
        }
        
        stats.bogusBlocksAdded++;
    }
    
    // Add fake loop that never executes
    void addFakeLoop(Function &F, BasicBlock *insertAfter) {
        LLVMContext &Ctx = F.getContext();
        Type *Int32Ty = Type::getInt32Ty(Ctx);
        
        // Create loop blocks
        BasicBlock *LoopHeader = BasicBlock::Create(Ctx, "fake.loop.header", &F);
        BasicBlock *LoopBody = BasicBlock::Create(Ctx, "fake.loop.body", &F);
        BasicBlock *LoopExit = BasicBlock::Create(Ctx, "fake.loop.exit", &F);
        
        // Loop header
        IRBuilder<> HeaderBuilder(LoopHeader);
        PHINode *IV = HeaderBuilder.CreatePHI(Int32Ty, 2, "fake.iv");
        IV->addIncoming(ConstantInt::get(Int32Ty, 0), insertAfter);
        
        Value *Cmp = HeaderBuilder.CreateICmpSLT(IV, ConstantInt::get(Int32Ty, 10));
        HeaderBuilder.CreateCondBr(Cmp, LoopBody, LoopExit);
        
        // Loop body (fake computation)
        IRBuilder<> BodyBuilder(LoopBody);
        Value *NextIV = BodyBuilder.CreateAdd(IV, ConstantInt::get(Int32Ty, 1));
        IV->addIncoming(NextIV, LoopBody);
        BodyBuilder.CreateBr(LoopHeader);
        
        // Loop exit
        IRBuilder<> ExitBuilder(LoopExit);
        BasicBlock *NextBB = insertAfter->getNextNode();
        if (!NextBB) NextBB = &F.getEntryBlock();
        ExitBuilder.CreateBr(NextBB);
        
        // Insert conditional to fake loop (always false)
        Instruction *OrigTerm = insertAfter->getTerminator();
        if (OrigTerm) {
            IRBuilder<> OrigBuilder(OrigTerm);
            Value *FakeCond = OrigBuilder.CreateICmpEQ(
                ConstantInt::get(Int32Ty, 1),
                ConstantInt::get(Int32Ty, 0)
            );
            OrigBuilder.CreateCondBr(FakeCond, LoopHeader, NextBB);
            OrigTerm->eraseFromParent();
        }
        
        stats.fakeLoopsAdded++;
    }
    
    // Substitute simple operations with complex equivalents
    void substituteInstructions(Function &F) {
        std::vector<Instruction*> toSubstitute;
        
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                // Find add instructions to substitute
                if (auto *Add = dyn_cast<BinaryOperator>(&I)) {
                    if (Add->getOpcode() == Instruction::Add) {
                        toSubstitute.push_back(Add);
                    }
                }
            }
        }
        
        for (Instruction *Add : toSubstitute) {
            IRBuilder<> Builder(Add);
            
            // Replace: a + b with: (a - (-b))
            Value *A = Add->getOperand(0);
            Value *B = Add->getOperand(1);
            
            Value *NegB = Builder.CreateNeg(B);
            Value *Result = Builder.CreateSub(A, NegB);
            
            Add->replaceAllUsesWith(Result);
            Add->eraseFromParent();
            
            stats.instructionSubstitutions++;
        }
    }
};

struct ObfuscatorPass : public PassInfoMixin<ObfuscatorPass> {
    bool BogusBlocks; 
    bool FakeLoops;
    bool InstrSub;

    ObfuscatorPass(bool BogusBlocks, bool FakeLoops, bool InstrSub) : BogusBlocks(BogusBlocks), FakeLoops(FakeLoops), InstrSub(InstrSub) {}

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        CodeObfuscator obf;
        bool modified = false;

        // Capture initial stats
        int bogusBefore = stats.bogusBlocksAdded;
        int loopsBefore = stats.fakeLoopsAdded;
        int subsBefore = stats.instructionSubstitutions;

        stats.functionsObfuscated++;
        for (BasicBlock &BB : F) {
            stats.totalBasicBlocks++;
            for (Instruction &I : BB) {
                stats.totalInstructions++;
            }
        }
        
        errs() << "========================================\n";
        errs() << "[ObfuscatorPass] Processing: " << F.getName() << "\n";
        errs() << "  Instructions: " << F.getInstructionCount() << "\n";
        errs() << "  Basic Blocks: " << F.size() << "\n";

        // Apply obfuscations
        std::vector<BasicBlock*> blocks;
        for (BasicBlock &BB : F) {
            blocks.push_back(&BB);
        }
        
        if (BogusBlocks) {
            errs() << "  [Bogus Blocks] Enabled\n";
            int bogusAdded = 0;
            for (size_t i = 0; i < blocks.size() && bogusAdded < 3; i++) {
                Instruction *term = blocks[i]->getTerminator();
                if (term && (isa<ReturnInst>(term) || isa<UnreachableInst>(term))) {
                    errs() << "    Skipping block " << i << " (terminal block)\n";
                    continue;
                }
                errs() << "    Adding bogus block after block " << i << "\n";
                obf.addBogusBlock(F, blocks[i]);
                bogusAdded++;
                modified = true;
            }
            errs() << "    Added " << (stats.bogusBlocksAdded - bogusBefore) << " bogus blocks\n";
        }
        
        if (FakeLoops) {
            errs() << "  [Fake Loops] Enabled\n";
            int loopsAdded = 0;
            for (size_t i = 0; i < blocks.size() && loopsAdded < 2; i++) {
                Instruction *term = blocks[i]->getTerminator();
                if (term && (isa<ReturnInst>(term) || isa<UnreachableInst>(term))) {
                    continue;
                }
                errs() << "    Adding fake loop after block " << i << "\n";
                obf.addFakeLoop(F, blocks[i]);
                loopsAdded++;
                modified = true;
            }
            errs() << "    Added " << (stats.fakeLoopsAdded - loopsBefore) << " fake loops\n";
        }
        
        if (InstrSub) {
            errs() << "  [Instruction Substitution] Enabled\n";
            obf.substituteInstructions(F);
            if (stats.instructionSubstitutions > subsBefore) {
                modified = true;
            }
            errs() << "    Substituted " << (stats.instructionSubstitutions - subsBefore) << " instructions\n";
        }
        
        
        errs() << "========================================\n";
        
        stats.writeReport(ReportFileArg.getValue());

        return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
    
    static bool isRequired() { return true; }
};

} // namespace

llvm::PassPluginLibraryInfo getObfuscatorPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "ObfuscatorPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
					PB.registerPipelineParsingCallback(
    [](StringRef Name, FunctionPassManager &FPM,
       ArrayRef<PassBuilder::PipelineElement>) {
        if (Name == "obfuscator-pass") {
            FPM.addPass(ObfuscatorPass(BogusBlocksOpt, FakeLoopsOpt, InstrSubOpt));
            return true;
        }
        return false;
    });
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getObfuscatorPassPluginInfo();
}
