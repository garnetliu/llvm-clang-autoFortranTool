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


#include <stdio.h>
#include <string>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace std;

class FunctionDeclFormatter {
public:
	FunctionDecl *funcDecl;

	// Member functions declarations
	FunctionDeclFormatter(FunctionDecl *funcDecl);
	string getParamsNamesASString();

private:
	QualType returnQType;
	llvm::ArrayRef<ParmVarDecl *> params;
};

// member function definitions
FunctionDeclFormatter::FunctionDeclFormatter(FunctionDecl *f) {
	funcDecl = f;
	returnQType = funcDecl->getReturnType();
	params = funcDecl->parameters();
};

string FunctionDeclFormatter::getParamsNamesASString() {
	//llvm::ArrayRef<ParmVarDecl *>::iterator it;
	string paramsNames;
	for (auto it = params.begin(); it != params.end(); it++) {
	  if (it == params.begin()) {
	    paramsNames += (*it)->getNameAsString();
	  } else { // parameters in between
	    paramsNames += ", " + (*it)->getNameAsString();
	  }
	}
	return paramsNames;
};
