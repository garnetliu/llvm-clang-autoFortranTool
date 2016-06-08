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



class CToFTypeFormatter {
public:
	QualType c_qualType;

	CToFTypeFormatter(QualType qt);
	string getFortranTypeASString();
};

CToFTypeFormatter::CToFTypeFormatter(QualType qt) {
	c_qualType = qt;
};

string CToFTypeFormatter::getFortranTypeASString() {
	string f_type;
	// support int, c_ptr
	// int -> ineteger(c_int), VALUE
	if (c_qualType.getTypePtr()->isIntegerType()) {
		f_type = "integer(kind=c_int)";
	} else if (c_qualType.getTypePtr()->isPointerType ()) {
		f_type = "type(c_ptr)";
	}

	// // int * -> integer(c_int)
	// else if (c_qualType.getTypePtr()->getPointeeType().getTypePtr()->isIntegerType()) {
	// 	f_type = "integer(c_int)";
	// }

	else {

	}

	return f_type;
}

class FunctionDeclFormatter {
public:
	FunctionDecl *funcDecl;

	// Member functions declarations
	FunctionDeclFormatter(FunctionDecl *funcDecl);
	string getParamsNamesASString();
	string getParamsDeclASString();
	string getFortranFunctDeclASString();

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

string FunctionDeclFormatter::getParamsDeclASString() {
	string paramsDecl;
	for (auto it = params.begin(); it != params.end(); it++) {
		CToFTypeFormatter tf((*it)->getOriginalType());
		//type handler
		paramsDecl += "\t" + tf.getFortranTypeASString() + ", VALUE, intent(inout)" + " :: " + (*it)->getNameAsString() + "\n"; // need to handle the attribute later
	}
	return paramsDecl;
}

string FunctionDeclFormatter::getParamsNamesASString() {
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

string FunctionDeclFormatter::getFortranFunctDeclASString() {
	string fortanFunctDecl;
	string funcType;
	string imports = "USE iso_c_binding";
	if (returnQType.getTypePtr()->isVoidType()) {
		funcType = "SUBROUTINE";
	} else {
		CToFTypeFormatter tf(returnQType);
		funcType = tf.getFortranTypeASString() + " FUNCTION";
	}

	fortanFunctDecl = funcType + " " + funcDecl->getNameAsString() + "(" + getParamsNamesASString() + ")" + " bind (C)\n";
	fortanFunctDecl += "\t" + imports + "\n";
	fortanFunctDecl += getParamsDeclASString();
	fortanFunctDecl += "END " + funcType + " " + funcDecl->getNameAsString();


	return fortanFunctDecl;
};
