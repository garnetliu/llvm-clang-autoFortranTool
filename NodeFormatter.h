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
#include "clang/Frontend/CompilerInstance.h"

// lexer
#include "clang/Lex/Lexer.h"



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
	string getFortranTypeASString(bool typeWrapper);
	bool isSameType(QualType qt2);
};

CToFTypeFormatter::CToFTypeFormatter(QualType qt) {
	c_qualType = qt;
};

bool CToFTypeFormatter::isSameType(QualType qt2) {
	// for pointer type, only distinguish between the function pointer and other pointers
	if (c_qualType.getTypePtr()->isPointerType() and qt2.getTypePtr()->isPointerType()) {
		if (c_qualType.getTypePtr()->isFunctionPointerType() and qt2.getTypePtr()->isFunctionPointerType()) {
			return true;
		} else if ((!c_qualType.getTypePtr()->isFunctionPointerType()) and (!qt2.getTypePtr()->isFunctionPointerType())) {
			return true;
		} else {
			return false;
		}
	} else {
		return c_qualType == qt2;
	}
};

string CToFTypeFormatter::getFortranTypeASString(bool typeWrapper) {
	string f_type;
	// support int, c_ptr
	// int -> ineteger(c_int), VALUE
	if (c_qualType.getTypePtr()->isIntegerType()) {
		if (typeWrapper) {
			f_type = "integer(c_int)";
		} else {
			f_type = "c_int";
		}
	} else if (c_qualType.getTypePtr()->isRealType()) {
		if (typeWrapper) {
			f_type = "real(c_double)";
		} else {
			f_type = "c_double";
		}
	} else if (c_qualType.getTypePtr()->isPointerType ()) {
		if (c_qualType.getTypePtr()->isFunctionPointerType()){
			if (typeWrapper) {
				f_type = "type(c_funptr)";
			} else {
				f_type = "c_funptr";
			}
		} else {
			if (typeWrapper) {
				f_type = "type(c_ptr)";
			} else {
				f_type = "c_ptr";
			}
		}
	} else {
		f_type = "not yet implemented";
	}
	return f_type;
};

class RecordDeclFormatter {
public:
	RecordDecl *recordDecl;

	// Member functions declarations
	RecordDeclFormatter(RecordDecl *r);

// private:
// 	llvm::ArrayRef<ParmVarDecl *> params;
};

RecordDeclFormatter::RecordDeclFormatter(RecordDecl *r) {
	recordDecl = r;
};




class FunctionDeclFormatter {
public:
	FunctionDecl *funcDecl;

	// Member functions declarations
	FunctionDeclFormatter(FunctionDecl *f);
	string getParamsNamesASString();
	string getParamsDeclASString();
	string getFortranFunctDeclASString();
	string getParamsTypesASString();

private:
	QualType returnQType;
	llvm::ArrayRef<ParmVarDecl *> params;
	//SourceManager &srcMgr;
};

// member function definitions
FunctionDeclFormatter::FunctionDeclFormatter(FunctionDecl *f) {
	funcDecl = f;
	returnQType = funcDecl->getReturnType();
	params = funcDecl->parameters();
	//srcMgr = sm;
};

string FunctionDeclFormatter::getParamsTypesASString() {
	string paramsType;
	QualType prev_qt;
	std::vector<QualType> qts;
	bool first = true;
	for (auto it = params.begin(); it != params.end(); it++) {
		if (first) {
			prev_qt = (*it)->getOriginalType();
			qts.push_back(prev_qt);
			CToFTypeFormatter tf((*it)->getOriginalType());
			paramsType = tf.getFortranTypeASString(false);
			first = false;
			//llvm::outs() << "first arg " << (*it)->getOriginalType().getAsString() + "\n";
		} else {
			CToFTypeFormatter tf((*it)->getOriginalType());
			if (tf.isSameType(prev_qt)) {
				//llvm::outs() << "same type as previous" << (*it)->getOriginalType().getAsString() + "\n";
			} else {
				// check if type is in the vector
				bool add = true;
				for (auto v = qts.begin(); v != qts.end(); v++) {
					if (tf.isSameType(*v)) {
						add = false;
					}
				}
				if (add) {
					paramsType += (", " + tf.getFortranTypeASString(false));
				}
				//llvm::outs() << "different type as previous" << (*it)->getOriginalType().getAsString() + "\n";
			}
			prev_qt = (*it)->getOriginalType();
			qts.push_back(prev_qt);
		}
	}
	return paramsType;
}

string FunctionDeclFormatter::getParamsDeclASString() { 
	string paramsDecl;
	int index = 0;
	for (auto it = params.begin(); it != params.end(); it++) {
		// if the param name is empty, rename it to arg_index
		string pname = (*it)->getNameAsString();
		if (pname.empty()) {
			pname = "arg_" + to_string(index);
		}
		
		CToFTypeFormatter tf((*it)->getOriginalType());
		// in some cases parameter doesn't have a name
		paramsDecl += "\t" + tf.getFortranTypeASString(true) + ", value" + " :: " + pname + "\n"; // need to handle the attribute later
		index ++;
	}
	return paramsDecl;
}

string FunctionDeclFormatter::getParamsNamesASString() { 
	string paramsNames;
	int index = 0;
	for (auto it = params.begin(); it != params.end(); it++) {
	  if (it == params.begin()) {
		// if the param name is empty, rename it to arg_index
		string pname = (*it)->getNameAsString();
		if (pname.empty()) {
			pname = "arg_" + to_string(index);
		}
	    paramsNames += pname;
	  } else { // parameters in between
		// if the param name is empty, rename it to arg_index
		string pname = (*it)->getNameAsString();
		if (pname.empty()) {
			pname = "arg_" + to_string(index);
		}
	    paramsNames += ", " + pname; 
	  }
	  index ++;
	}
	return paramsNames;
};

string FunctionDeclFormatter::getFortranFunctDeclASString() {
	string fortanFunctDecl;
	string funcType;
	string imports = "USE iso_c_binding, only: " + getParamsTypesASString();
	// check if the return type is void or not
	if (returnQType.getTypePtr()->isVoidType()) {
		funcType = "SUBROUTINE";
	} else {
		CToFTypeFormatter tf(returnQType);
		funcType = tf.getFortranTypeASString(true) + " FUNCTION";
	}

	fortanFunctDecl = funcType + " " + funcDecl->getNameAsString() + "(" + getParamsNamesASString() + ")" + " bind (C)\n";
	fortanFunctDecl += "\t" + imports + "\n";
	fortanFunctDecl += getParamsDeclASString();
	// // preserve the function body as comment
	// if (funcDecl->hasBody()) {
	// 	Stmt *stmt = funcDecl->getBody();
	// 	clang::SourceManager &sm = ;
	// 	llvm::outs() << "reached here\n";
	// 	string text = Lexer::getSourceText(CharSourceRange::getTokenRange(stmt->getSourceRange()), *sm, LangOptions(), 0);
	//     // if (text.at(text.size()-1) == ',') {
	//     //     text = Lexer::getSourceText(CharSourceRange::getCharRange(stmt->getSourceRange()), sm, LangOptions(), 0);
	//     // }
	//     fortanFunctDecl += text;

	// }
	fortanFunctDecl += "END " + funcType + " " + funcDecl->getNameAsString() + "\n";


	return fortanFunctDecl;
};
