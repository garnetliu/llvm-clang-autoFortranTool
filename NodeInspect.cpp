// node format modules
#include "nodeFormatter.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");


// -----------START----------------------------------------------------------------------------------------------------

class TraverseNodeVisitor : public RecursiveASTVisitor<TraverseNodeVisitor> {
public:
    bool TraverseDecl(Decl *d) {
        if (isa<TranslationUnitDecl> (d)) {
          // tranlastion unit decl is the top node of all AST, ignore the inner structure of tud for now
          llvm::outs() << "this is a TranslationUnitDecl\n";
        } else if (isa<FunctionDecl> (d)) {
          // create formatter
          FunctionDeclFormatter fdf(cast<FunctionDecl> (d));
          // llvm::ArrayRef<ParmVarDecl *> params = fdf.funcDecl->parameters();
          // type checking
          // for (auto it = params.begin(); it != params.end(); it++) {
          //   // if ((*it)->getOriginalType().getTypePtr()->isPointerType()) {
          //   //   llvm::outs() << "PTR TYPE\n";
          //   //   if ((*it)->getOriginalType().getTypePtr()->isFunctionPointerType()) {
          //   //     llvm::outs() << "FUNC * TYPE\n";
          //   //   }
          //   //   if ((*it)->getOriginalType().getTypePtr()->getPointeeType().getTypePtr()->isIntegerType()) {
          //   //     llvm::outs() << "INT * TYPE\n";
          //   //   }
          //   llvm::outs() << (*it)->getOriginalType().getAsString();

          //   }
          

          // // report findings
          // llvm::outs()
          // << "this is a FunctionDecl: " << funcDecl->getNameAsString() 
          // << " with return type: " << returnQType.getAsString()
          // << " parameters: ";
          // for (it = params.begin(); it != params.end(); it++) {
          //   // ParmVarDecl *parmvardecl = *it; //iterators work like pointers to container elements.
          //   llvm::outs() << "param name: " << (*it)->getNameAsString() << " type: " << (*it)->getOriginalType().getAsString() << " ";
          //  }
          // llvm::outs() << "\n";

          // -------------------------------dump Fortran-------------------------------

          llvm::outs() << fdf.getFortranFunctDeclASString()
          << "\n";

          // char * c_type;
          // llvm::outs() 
          // << "SUBROUTINE " << funcDecl->getNameAsString()
          // << "(";
          // for (it = params.begin(); it != params.end(); it++) {
          //   // doesn't support multiple arg syntax yet
          //   llvm::outs() << (*it)->getNameAsString();
          // }
          // llvm::outs() << ") bind (C)\n";
          // for (it = params.begin(); it != params.end(); it++) {
          //   // doesn't support multiple arg syntax yet
          //   if ((*it)->getOriginalType().getTypePtr()->isPointerType()) {
          //     if ((*it)->getOriginalType().getTypePtr()->getPointeeType().getTypePtr()->isIntegerType()) {
          //       c_type = "c_int";
          //     }
          //   }
          // }
          // llvm::outs() << "\tUSE iso_c_binding, only: "<< c_type <<"\n";
          // llvm::outs() << "\tinteger("<< c_type <<"), VALUE, INTENT(IN) :: ";
          // for (it = params.begin(); it != params.end(); it++) {
          //   // doesn't support multiple arg syntax yet
          //   llvm::outs() << (*it)->getNameAsString();
          // }
          // llvm::outs() << "\n";
          // llvm::outs() << "END SUBROUTINE " << funcDecl->getNameAsString();
          // llvm::outs() << "\n";
          // -------------------------------dump Fortran-------------------------------


        } else if (isa<TypedefDecl> (d)) {
          //TypedefDecl *tdd = cast<TypedefDecl> (d);
          llvm::outs() << "found TypedefDecl \n";
        } else if (isa<RecordDecl> (d)) {
          RecordDecl *recordDecl = cast<RecordDecl> (d);
          // struct name: recordDecl->getNameAsString()
          if (!recordDecl->field_empty()) {
            for (auto it = recordDecl->field_begin(); it != recordDecl->field_end(); it++) {
              llvm::outs() << "identifier:" <<(*it)->getNameAsString() 
              << " type: " << (*it)->getType().getAsString()<< "\n";
            }
          }
          
          llvm::outs() << "found RecordDecl " << recordDecl->getNameAsString() + "\n";
        } else if (isa<VarDecl> (d)) {
          VarDecl *varDecl = cast<VarDecl> (d);
          // name: myType
          // type: struct MyType (or a loc identifier)
          llvm::outs() << "found VarDecl " << varDecl->getNameAsString() 
          << " type: " << varDecl->getType().getAsString() << "\n";
        } else if (isa<EnumDecl> (d)) {
          EnumDecl *enumDecl = cast<EnumDecl> (d);
          llvm::outs() << "found EnumDecl " << enumDecl->getNameAsString()+ "\n";
        } else {
          llvm::outs() << "found other declaration \n";
        }

        RecursiveASTVisitor<TraverseNodeVisitor>::TraverseDecl(d); // Forward to base class
        return true; // Return false to stop the AST analyzing
    }


    bool TraverseStmt(Stmt *x) {
        llvm::outs() << "found statement \n";
        x->dump();
        RecursiveASTVisitor<TraverseNodeVisitor>::TraverseStmt(x);
        return true;
    }
    bool TraverseType(QualType x) {
        llvm::outs() << "found type " << x.getAsString() << "\n";
        x->dump();
        RecursiveASTVisitor<TraverseNodeVisitor>::TraverseType(x);
        return true;
    }
};

class TraverseNodeConsumer : public clang::ASTConsumer {
public:
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    // Traversing the translation unit decl via a RecursiveASTVisitor
    // will visit all nodes in the AST.
    llvm::outs() << "start traversing first declaration \n";
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    llvm::outs() << "finished traversing last declaration \n";
  }
private:
  // A RecursiveASTVisitor implementation.
  TraverseNodeVisitor Visitor;
};

class TraverseNodeAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {

    return std::unique_ptr<clang::ASTConsumer>(new TraverseNodeConsumer);
  }
};

int main(int argc, const char **argv) {
  if (argc == 2) {
    CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
    ClangTool Tool(OptionsParser.getCompilations(),
                  OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<TraverseNodeAction>().get());
  } else if (argc == 3) {
    clang::tooling::runToolOnCode(new TraverseNodeAction, argv[1]);
  } else {
    llvm::outs() 
    << "USAGE: ~/clang-llvm/build/bin/node-inspect <PATH> OR"
    << "~/clang-llvm/build/bin/node-inspect <CODE> inline";
  }
}