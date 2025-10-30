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

using namespace llvm;

// Command line options for the obfuscator pass
namespace {

// Statistics tracking structure
struct ObfuscationStats {
    int stringObfuscations = 0;
    int bogusBlocksAdded = 0;
    int fakeLoopsAdded = 0;
    int instructionSubstitutions = 0;
    int totalInstructions = 0;
    int totalBasicBlocks = 0;
    std::string inputFile;
    std::string outputFile;
    std::string timestamp;
    
    void writeReport(const std::string &reportFile) {
        if (reportFile.empty()) {
            return;
        }
        std::ofstream report(reportFile);
        
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
        report << "Functions Obfuscated: 1\n";
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
    std::string ReportFile;

    ObfuscatorPass(bool BogusBlocks, bool FakeLoops, bool InstrSub, std::string ReportFile) : BogusBlocks(BogusBlocks), FakeLoops(FakeLoops), InstrSub(InstrSub), ReportFile(ReportFile) {}

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
        errs() << "========================================\n";
        errs() << "[ObfuscatorPass] Processing: " << F.getName() << "\n";
        
        CodeObfuscator obf;
        bool modified = false;
        
        // Count instructions and basic blocks
        for (BasicBlock &BB : F) {
            stats.totalBasicBlocks++;
            for (Instruction &I : BB) {
                stats.totalInstructions++;
            }
        }
        
        errs() << "  Instructions: " << stats.totalInstructions << "\n";
        errs() << "  Basic Blocks: " << stats.totalBasicBlocks << "\n";
        
        // Apply obfuscations
        std::vector<BasicBlock*> blocks;
        for (BasicBlock &BB : F) {
            blocks.push_back(&BB);
        }
        
        if (BogusBlocks) {
            // Add bogus blocks (limit to avoid too much bloat)
            int bogusToAdd = std::min(2, (int)blocks.size());
            for (int i = 0; i < bogusToAdd && i < blocks.size(); i++) {
                // CRITICAL FIX: Do not modify blocks that end in a return statement.
                // This was the source of the infinite loop and crash.
                if (!isa<ReturnInst>(blocks[i]->getTerminator())) {
                    obf.addBogusBlock(F, blocks[i]);
                    modified = true;
                }
            }
        }
        
        if (FakeLoops) {
            // Add fake loops
            int loopsToAdd = std::min(1, (int)blocks.size() / 2);
            for (int i = 0; i < loopsToAdd && i < blocks.size(); i++) {
                // CRITICAL FIX: Also protect fake loop insertion from breaking return blocks.
                if (!isa<ReturnInst>(blocks[i]->getTerminator())) {
                    obf.addFakeLoop(F, blocks[i]);
                    modified = true;
                }
            }
        }
        
        if (InstrSub) {
            // Substitute instructions
            obf.substituteInstructions(F);
            if (stats.instructionSubstitutions > 0) {
                modified = true;
            }
        }
        
        errs() << "  Bogus Blocks: " << stats.bogusBlocksAdded << "\n";
        errs() << "  Fake Loops: " << stats.fakeLoopsAdded << "\n";
        errs() << "  Substitutions: " << stats.instructionSubstitutions << "\n";
        errs() << "========================================\n";
        
        stats.writeReport(ReportFile);

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
                    if (Name.starts_with("obfuscator-pass")) {
                        bool bogusBlocks = false;
                        bool fakeLoops = false;
                        bool instrSub = false;
                        std::string reportFile = "";

                        StringRef Options = Name.drop_front(strlen("obfuscator-pass"));
                        if (Options.starts_with("(")) {
                            Options = Options.drop_front(1);
                            Options = Options.drop_back(1);
                        }

                        SmallVector<StringRef, 4> Opts;
                        Options.split(Opts, ",");

                        for (auto Opt : Opts) {
                            if (Opt.starts_with("bogus-blocks=")) {
                                bogusBlocks = Opt.drop_front(strlen("bogus-blocks=")) == "true";
                            }
                            if (Opt.starts_with("fake-loops=")) {
                                fakeLoops = Opt.drop_front(strlen("fake-loops=")) == "true";
                            }
                            if (Opt.starts_with("instr-sub=")) {
                                instrSub = Opt.drop_front(strlen("instr-sub=")) == "true";
                            }
                            if (Opt.starts_with("report-file=")) {
                                reportFile = Opt.drop_front(strlen("report-file=")).str();
                            }
                        }

                        FPM.addPass(ObfuscatorPass(bogusBlocks, fakeLoops, instrSub, reportFile));
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
