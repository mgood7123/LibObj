//==============================================================================
// FILE:
//    HelloWorld.cpp
//
// DESCRIPTION:
//    Counts the number of C++ record declarations in the input translation
//    unit. The results are printed on a file-by-file basis (i.e. for each
//    included header file separately).
//
//    Internally, this implementation leverages llvm::StringMap to map file
//    names to the corresponding #count of declarations.
//
// USAGE:
//   clang -cc1 -load <BUILD_DIR>/lib/libHelloWorld.dylib '\'
//    -plugin hello-world test/HelloWorld-basic.cpp
//
// License: The Unlicense
//==============================================================================
#include <cassert>
#include <memory>
#include <string>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ExternalASTSource.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/FileManager.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Sema/CodeCompleteConsumer.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaConsumer.h"
#include "clang/Sema/TemplateInstCallback.h"

#include "llvm/ADT/StringMap.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TimeProfiler.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

#define OUT llvm::outs()

struct SCOPED_FUNC_LOGGER {
        const char * id;
        SCOPED_FUNC_LOGGER(const char * id) : id(id) {
            OUT << "ENTER FUNCTION : " << id << "\n";
        }

        ~SCOPED_FUNC_LOGGER() {
            OUT << "EXIT FUNCTION : " << id << "\n\n";
        }
};

#define LOG_FUNC                                                               \
    SCOPED_FUNC_LOGGER SCOPED_FUNCTION_LLVM_CLANG_LOGGER =                     \
        SCOPED_FUNC_LOGGER(__PRETTY_FUNCTION__);

class REWRITE_CONSUMER : public clang::ASTConsumer {
        clang::FileID * f_id;
        clang::Rewriter * f_rewrite;
        bool * f_error;

    public:
        REWRITE_CONSUMER(clang::FileID * f_id, clang::Rewriter * f_rewrite,
                         bool * f_error) :
            f_id(f_id),
            f_rewrite(f_rewrite), f_error(f_error) {
            LOG_FUNC
        }

        void HandleTranslationUnit(clang::ASTContext & context) override {
            if (*f_error) {
                return;
            }
            OUT << "\n\n";
            LOG_FUNC
            Decl * decl = context.getTranslationUnitDecl();
            decl->dumpColor();
            OUT << "\n\n";
        }
};

class REWRITER : public clang::PluginASTAction {
        clang::CompilerInstance * this_compiler_;

        std::string our_filename;
        clang::FileID our_file_id;
        clang::Rewriter file_rewriter;
        bool file_rewriter_error = false;

    public:
        std::unique_ptr<clang::ASTConsumer>
        CreateASTConsumer(clang::CompilerInstance & this_compiler,
                          llvm::StringRef input_file) override {
            LOG_FUNC

            auto & SourceManager {this_compiler.getSourceManager()};
            auto & LangOpts {this_compiler.getLangOpts()};

            this_compiler_ = &this_compiler;

            our_filename = "tmp_file.cpp";

            file_rewriter.setSourceMgr(SourceManager, LangOpts);

            auto f =
                SourceManager.getFileManager().getOptionalFileRef(input_file);

            if (f.hasValue()) {
                OUT << "\nobtained file id for input file: "
                    << input_file.data() << "\n\n";
                our_file_id = SourceManager.translateFile(f.getValue());
                return std::make_unique<REWRITE_CONSUMER>(
                    &our_file_id, &file_rewriter, &file_rewriter_error);
            } else {
                OUT << "\ncould not obtain file id for input file: "
                    << input_file.data() << "\n\n";
                file_rewriter_error = true;
                return std::make_unique<REWRITE_CONSUMER>(nullptr, nullptr,
                                                          &file_rewriter_error);
            }
        }

        bool PrepareToExecuteAction(CompilerInstance & CI) override {
            LOG_FUNC
            return clang::PluginASTAction::PrepareToExecuteAction(CI);
        }

        bool BeginInvocation(CompilerInstance & CI) override {
            LOG_FUNC
            return clang::PluginASTAction::BeginInvocation(CI);
        }

        virtual bool BeginSourceFileAction(CompilerInstance & CI) override {
            LOG_FUNC
            return clang::PluginASTAction::BeginSourceFileAction(CI);
        }

        void ExecuteAction() override {
            LOG_FUNC
            // https://github.com/llvm/llvm-project/blob/release/13.x/clang/lib/Frontend/FrontendAction.cpp#L1039

            // clang::PluginASTAction::ExecuteAction();
            CompilerInstance & CI = getCompilerInstance();
            if (!CI.hasPreprocessor())
                return;

            // FIXME: Move the truncation aspect of this into Sema, we delayed
            // this till here so the source manager would be initialized.
            if (hasCodeCompletionSupport()
                && !CI.getFrontendOpts().CodeCompletionAt.FileName.empty()) {
                OUT << "create code completion consumer\n";
                CI.createCodeCompletionConsumer();
                OUT << "created code completion consumer\n";
            }

            // Use a code completion consumer?
            CodeCompleteConsumer * CompletionConsumer = nullptr;
            if (CI.hasCodeCompletionConsumer())
                CompletionConsumer = &CI.getCodeCompletionConsumer();

            if (!CI.hasSema()) {
                OUT << "create sema\n";
                CI.createSema(getTranslationUnitKind(), CompletionConsumer);
                OUT << "created sema\n";
            }

            OUT << "parse ast\n";

            // https://github.com/llvm/llvm-project/blob/release/13.x/clang/lib/Parse/ParseAST.cpp#L95

            customParseAST(CI.getSema(), CI.getFrontendOpts().ShowStats,
                           CI.getFrontendOpts().SkipFunctionBodies);
            OUT << "parsed ast\n";
        }

        /// ParseAST - Parse the entire file specified, notifying the
        /// ASTConsumer as the file is parsed.  This inserts the parsed decls
        /// into the translation unit held by Ctx.
        ///
        void customParseAST(Preprocessor & PP, ASTConsumer * Consumer,
                            ASTContext & Ctx, bool PrintStats,
                            TranslationUnitKind TUKind,
                            CodeCompleteConsumer * CompletionConsumer,
                            bool SkipFunctionBodies) {
            LOG_FUNC

            std::unique_ptr<Sema> S(
                new Sema(PP, Ctx, *Consumer, TUKind, CompletionConsumer));

            // Recover resources if we crash before exiting this method.
            llvm::CrashRecoveryContextCleanupRegistrar<Sema> CleanupSema(
                S.get());

            customParseAST(*S.get(), PrintStats, SkipFunctionBodies);
        }

        /// Resets LLVM's pretty stack state so that stack traces are printed
        /// correctly when there are nested CrashRecoveryContexts and the inner
        /// one recovers from a crash.
        class ResetStackCleanup :
            public llvm::CrashRecoveryContextCleanupBase<ResetStackCleanup,
                                                         const void> {
            public:
                ResetStackCleanup(llvm::CrashRecoveryContext * Context,
                                  const void * Top) :
                    llvm::CrashRecoveryContextCleanupBase<ResetStackCleanup,
                                                          const void>(Context,
                                                                      Top) {
                    LOG_FUNC
                }
                void recoverResources() override {
                    LOG_FUNC
                    llvm::RestorePrettyStackState(resource);
                }
        };

        /// If a crash happens while the parser is active, an entry is printed
        /// for it.
        class PrettyStackTraceParserEntry : public llvm::PrettyStackTraceEntry {
                const LIBOBJ_clang::Parser & P;

            public:
                PrettyStackTraceParserEntry(const LIBOBJ_clang::Parser & p) :
                    P(p) {
                    LOG_FUNC
                }
                void print(raw_ostream & OS) const override {
                    LOG_FUNC
                    const Token & Tok = P.getCurToken();
                    if (Tok.is(tok::eof)) {
                        OS << "<eof> parser at end of file\n";
                        return;
                    }

                    if (Tok.getLocation().isInvalid()) {
                        OS << "<unknown> parser at unknown location\n";
                        return;
                    }

                    const Preprocessor & PP = P.getPreprocessor();
                    Tok.getLocation().print(OS, PP.getSourceManager());
                    if (Tok.isAnnotation()) {
                        OS << ": at annotation token\n";
                    } else {
                        // Do the equivalent of PP.getSpelling(Tok) except for
                        // the parts that would allocate memory.
                        bool Invalid = false;
                        const SourceManager & SM =
                            P.getPreprocessor().getSourceManager();
                        unsigned Length = Tok.getLength();
                        const char * Spelling =
                            SM.getCharacterData(Tok.getLocation(), &Invalid);
                        if (Invalid) {
                            OS << ": unknown current parser token\n";
                            return;
                        }
                        OS << ": current parser token '"
                           << StringRef(Spelling, Length) << "'\n";
                    }
                }
        };

        void customParseAST(Sema & S, bool PrintStats,
                            bool SkipFunctionBodies) {
            LOG_FUNC
            // Collect global stats on Decls/Stmts (until we have a module
            // streamer).
            if (PrintStats) {
                Decl::EnableStatistics();
                Stmt::EnableStatistics();
            }

            // Also turn on collection of stats inside of the Sema object.
            bool OldCollectStats = PrintStats;
            std::swap(OldCollectStats, S.CollectStats);

            // Initialize the template instantiation observer chain.
            // FIXME: See note on "finalize" below.
            initialize(S.TemplateInstCallbacks, S);

            ASTConsumer * Consumer = &S.getASTConsumer();

            std::unique_ptr<LIBOBJ_clang::Parser> ParseOP(
                new LIBOBJ_clang::Parser(S.getPreprocessor(), S,
                                         SkipFunctionBodies));
            LIBOBJ_clang::Parser & P = *ParseOP.get();

            llvm::CrashRecoveryContextCleanupRegistrar<const void,
                                                       ResetStackCleanup>
                CleanupPrettyStack(llvm::SavePrettyStackState());
            PrettyStackTraceParserEntry CrashInfo(P);

            // Recover resources if we crash before exiting this method.
            llvm::CrashRecoveryContextCleanupRegistrar<LIBOBJ_clang::Parser>
                CleanupParser(ParseOP.get());

            OUT << "call S.getPreprocessor().EnterMainSourceFile()"
                << "\n";
            S.getPreprocessor().EnterMainSourceFile();
            OUT << "called S.getPreprocessor().EnterMainSourceFile()"
                << "\n";
            ExternalASTSource * External =
                S.getASTContext().getExternalSource();
            if (External) {
                OUT << "call External->StartTranslationUnit(Consumer);"
                    << "\n";
                External->StartTranslationUnit(Consumer);
                OUT << "called External->StartTranslationUnit(Consumer);"
                    << "\n";
            }

            // If a PCH through header is specified that does not have an
            // include in the source, or a PCH is being created with #pragma
            // hdrstop with nothing after the pragma, there won't be any tokens
            // or a Lexer.
            bool HaveLexer = S.getPreprocessor().getCurrentLexer();

            if (HaveLexer) {
                OUT << "call lexer"
                    << "\n";
                llvm::TimeTraceScope TimeScope("Frontend");
                P.Initialize();
                Parser::DeclGroupPtrTy ADecl;
                OUT << "call P.ParseFirstTopLevelDecl(ADecl)"
                    << "\n";
                bool AtEOF = P.ParseFirstTopLevelDecl(ADecl);
                OUT << "called P.ParseFirstTopLevelDecl(ADecl)"
                    << "\n";
                OUT << "while true begin\n";
                while (true) {
                    if (!AtEOF) {
                        OUT << "parsed something, AtEOF: "
                            << (AtEOF ? "true" : "false") << "\n";
                        // If we got a null return and something *was* parsed,
                        // ignore it.  This is due to a top-level semicolon, an
                        // action override, or a parse error skipping something.
                        if (ADecl && !Consumer->HandleTopLevelDecl(ADecl.get()))
                            return;
                        OUT << "call P.ParseTopLevelDecl(ADecl)"
                            << "\n";
                        AtEOF = P.ParseTopLevelDecl(ADecl);
                        OUT << "called P.ParseTopLevelDecl(ADecl)"
                            << "\n";
                        continue;
                    }
                    OUT << "EOF\n";
                    break;
                }
                OUT << "while true end"
                    << "\n";
                OUT << "called lexer"
                    << "\n";
            }

            // Process any TopLevelDecls generated by #pragma weak.
            for (Decl * D : S.WeakTopLevelDecls())
                Consumer->HandleTopLevelDecl(DeclGroupRef(D));

            Consumer->HandleTranslationUnit(S.getASTContext());

            // Finalize the template instantiation observer chain.
            // FIXME: This (and init.) should be done in the Sema class, but
            // because Sema does not have a reliable "Finalize" function (it has
            // a destructor, but it is not guaranteed to be called
            // ("-disable-free")). So, do the initialization above and do the
            // finalization here:
            finalize(S.TemplateInstCallbacks, S);

            std::swap(OldCollectStats, S.CollectStats);
            if (PrintStats) {
                llvm::errs() << "\nSTATISTICS:\n";
                if (HaveLexer)
                    P.getActions().PrintStats();
                S.getASTContext().PrintStats();
                Decl::PrintStats();
                Stmt::PrintStats();
                Consumer->PrintStats();
            }
        }

        void EndSourceFileAction() override {
            LOG_FUNC

            OUT << "file_rewriter_error = "
                << (file_rewriter_error ? "true" : "false") << "\n";

            if (file_rewriter_error)
                return;

            auto FileRewriteBuffer {
                file_rewriter.getRewriteBufferFor(our_file_id)};

            if (FileRewriteBuffer == nullptr) {
                OUT << "FileRewriteBuffer = nullptr\n";
                return;
            }

            rewrite_compile(this_compiler_, our_filename,
                            FileRewriteBuffer->begin(),
                            FileRewriteBuffer->end());
        }

        template <typename IT>
        void rewrite_compile(clang::CompilerInstance * CI,
                             std::string const & FileName, IT FileBegin,
                             IT FileEnd) {

            LOG_FUNC

            auto & CodeGenOpts {CI->getCodeGenOpts()};
            auto & Target {CI->getTarget()};
            auto & Diagnostics {CI->getDiagnostics()};

            // create new compiler instance
            auto CInvNew {std::make_shared<clang::CompilerInvocation>()};

            bool CInvNewCreated {clang::CompilerInvocation::CreateFromArgs(
                *CInvNew, CodeGenOpts.CommandLineArgs, Diagnostics)};

            assert(CInvNewCreated);

            clang::CompilerInstance CINew;
            CINew.setInvocation(CInvNew);
            CINew.setTarget(&Target);
            CINew.createDiagnostics();

            // create rewrite buffer
            std::string FileContent {FileBegin, FileEnd};
            auto FileMemoryBuffer {
                llvm::MemoryBuffer::getMemBufferCopy(FileContent)};

            // create "virtual" input file
            auto & PreprocessorOpts {CINew.getPreprocessorOpts()};
            PreprocessorOpts.addRemappedFile(FileName, FileMemoryBuffer.get());

            // generate code
            clang::EmitObjAction action;
            CINew.ExecuteAction(action);

            // clean up rewrite buffer
            FileMemoryBuffer.release();
        }

        bool ParseArgs(const CompilerInstance & CI,
                       const std::vector<std::string> & args) override {
            LOG_FUNC
            return true;
        }

        PluginASTAction::ActionType getActionType() override {
            LOG_FUNC
            return ReplaceAction;
        }
};

//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------
static FrontendPluginRegistry::Add<REWRITER>
    X(/*Name=*/"LibObjClangPlugin",
      /*Description=*/"The LibObj plugin");
