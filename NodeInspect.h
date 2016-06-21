// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

// recursive converter
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"

// lexer and writer
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"

// preprocesser
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FormattedStream.h"


#include <stdio.h>
#include <string>
#include <sstream>

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

//------------Formatter class decl----------------------------------------------------------------------------------------------------
class CToFTypeFormatter {
public:
  QualType c_qualType;

  CToFTypeFormatter(QualType qt);
  string getFortranTypeASString(bool typeWrapper);
  bool isSameType(QualType qt2);
};

class RecordDeclFormatter {
public:
  const int ANONYMOUS = 0;
  const int ID_ONLY = 1;
  const int TAG_ONLY = 2;
  const int ID_TAG = 3;
  const int UNION = 0;
  const int STRUCT = 1;

  RecordDecl *recordDecl;
  int mode = ANONYMOUS;
  bool structOrUnion = STRUCT;
  string tag_name;



  // Member functions declarations
  RecordDeclFormatter(RecordDecl *r);
  void setMode(int m);
  void setTagName(string name);
  bool isStruct();
  bool isUnion();
  string getFortranStructASString();
  string getFortranFields();

// private:
  
};



class FunctionDeclFormatter {
public:
  FunctionDecl *funcDecl;

  // Member functions declarations
  FunctionDeclFormatter(FunctionDecl *f, Rewriter &r);
  string getParamsNamesASString();
  string getParamsDeclASString();
  string getFortranFunctDeclASString();
  string getParamsTypesASString();

private:
  QualType returnQType;
  llvm::ArrayRef<ParmVarDecl *> params;
  Rewriter &rewriter;
};


class MacroFormatter {
public:
  const MacroDirective *md;
  string macroName;
  string macroVal;

  MacroFormatter(const Token MacroNameTok, const MacroDirective *m, CompilerInstance &c);
  bool isObjectLike();
  bool isFunctionLike();
  string getFortranMacroASString();

private:
  bool isObjectOrFunction;
  CompilerInstance &ci;
};

//------------Visitor class decl----------------------------------------------------------------------------------------------------

class TraverseNodeVisitor : public RecursiveASTVisitor<TraverseNodeVisitor> {
public:
  TraverseNodeVisitor(Rewriter &R) : TheRewriter(R) {}


  bool TraverseDecl(Decl *d);
  bool TraverseStmt(Stmt *x);
  bool TraverseType(QualType x);

private:
  Rewriter &TheRewriter;
};