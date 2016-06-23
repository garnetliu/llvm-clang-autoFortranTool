#include "NodeInspect.h"

//-----------formatter functions----------------------------------------------------------------------------------------------------

RecordDeclFormatter::RecordDeclFormatter(RecordDecl *r) {
  recordDecl = r;
};

bool RecordDeclFormatter::isStruct() {
  return structOrUnion == STRUCT;
};

bool RecordDeclFormatter::isUnion() {
  return structOrUnion == UNION;
};

void RecordDeclFormatter::setTagName(string name) {
  tag_name = name;
}

string RecordDeclFormatter::getFortranFields() {
  string fieldsInFortran = "";
  if (!recordDecl->field_empty()) {
    for (auto it = recordDecl->field_begin(); it != recordDecl->field_end(); it++) {
      CToFTypeFormatter tf((*it)->getType(), recordDecl->getASTContext());
      string identifier = (*it)->getNameAsString();

      fieldsInFortran += "\t" + tf.getFortranTypeASString(true) + " :: " + identifier + "\n";
    }
  }
  return fieldsInFortran;
}

string RecordDeclFormatter::getFortranStructASString() {
  // initalize mode here
  setMode();

  string rd_buffer;

  if (mode == ID_ONLY) {
    string identifier = "struct_" + recordDecl->getNameAsString();
    string fieldsInFortran = getFortranFields();
    rd_buffer = "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE\n";
    
  } else if (mode == TAG_ONLY) {
    string identifier = tag_name;
    string fieldsInFortran = getFortranFields();

    rd_buffer = "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE\n";
  } else if (mode == ID_TAG) {
    string identifier = tag_name;
    string fieldsInFortran = getFortranFields();

    rd_buffer = "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE\n";    
  }

  return rd_buffer;

};

void RecordDeclFormatter::setMode() {
  // int ANONYMOUS = 0;
  // int ID_ONLY = 1;
  // int TAG_ONLY = 2;
  // int ID_TAG = 3;

  if (!(recordDecl->getNameAsString()).empty() and !tag_name.empty()) {
    mode = ID_TAG;
  } else if (!(recordDecl->getNameAsString()).empty() and tag_name.empty()) {
    mode = ID_ONLY;
  } else if ((recordDecl->getNameAsString()).empty() and tag_name.empty()) {
    mode = ANONYMOUS;
  } else if ((recordDecl->getNameAsString()).empty() and !tag_name.empty()) {
    mode = TAG_ONLY;
  }
};

CToFTypeFormatter::CToFTypeFormatter(QualType qt, ASTContext &ac): ac(ac) {
  c_qualType = qt;
};

bool CToFTypeFormatter::isSameType(QualType qt2) {
  // for pointer type, only distinguish between the function pointer from other pointers
  if (c_qualType.getTypePtr()->isPointerType() and qt2.getTypePtr()->isPointerType()) {
    if (c_qualType.getTypePtr()->isFunctionPointerType() and qt2.getTypePtr()->isFunctionPointerType()) {
      return true;
    } else if ((!c_qualType.getTypePtr()->isFunctionPointerType()) and (!qt2.getTypePtr()->isFunctionPointerType())) {
      return true;
    } else {
      return false;
    }
  } else if (c_qualType.getTypePtr()->isIntegerType() and qt2.getTypePtr()->isIntegerType()) {
    // consider size_t and int
    return true;
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
  } else if (c_qualType.getTypePtr()->isArrayType()) {
    const ArrayType *at = c_qualType.getTypePtr()->getAsArrayTypeUnsafe ();
    QualType e_qualType = at->getElementType ();
    if (e_qualType.getTypePtr()->isCharType()) {
      if (typeWrapper) {
        f_type = "character(c_char)";
      } else {
        f_type = "c_char";
      }
    } else {
      f_type = "array type (" + e_qualType.getAsString()+")";
    }
  }
  //  else if (c_qualType.getTypePtr()->isAnyCharacterType()) {
  //   if (typeWrapper) {
  //     f_type = "character(c_char)";
  //   } else {
  //     f_type = "c_char";
  //   }
  // }

  else {
    f_type = "type not yet implemented (" + c_qualType.getAsString()+")";
  }
  return f_type;
};


bool CToFTypeFormatter::isAllDigit(const string input) {
  // "123L" "18446744073709551615ULL" "18446744073709551615UL" "1.23"
  return std::all_of(input.begin(), input.end(), ::isdigit);
};

bool CToFTypeFormatter::isString(const string input) {
  if (input[0] == '\"' and input[input.size()-1] =='\"') {
    return true;
  }
  return false;
};

bool CToFTypeFormatter::isCharType(const string input) {
  if (input.find("char") != std::string::npos) {
      return true;
  }
  return false;
};

// beware: #define __sgetc(p) (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))
bool CToFTypeFormatter::isIntType(const string input) {
  if (input.find("int") != std::string::npos and (input.find("long") == std::string::npos)) {
      return true;
  }
  return false;
};

bool CToFTypeFormatter::isShortType(const string input) {
  if (input.find("short") != std::string::npos) {
      return true;
  }
  return false;
};

bool CToFTypeFormatter::isLongType(const string input) {
  if (input.find("long") != std::string::npos and (input.find("int") == std::string::npos)) {
      return true;
  }
  return false;
};

// -----------initializer FunctionDeclFormatter--------------------
FunctionDeclFormatter::FunctionDeclFormatter(FunctionDecl *f, Rewriter &r) : rewriter(r) {
  funcDecl = f;
  returnQType = funcDecl->getReturnType();
  params = funcDecl->parameters();
  isInSystemHeader = rewriter.getSourceMgr().isInSystemHeader(funcDecl->getSourceRange().getBegin());
};

// for inserting types to "USE iso_c_binding, only: <<< c_ptr, c_int>>>""
string FunctionDeclFormatter::getParamsTypesASString() {
  string paramsType;
  QualType prev_qt;
  std::vector<QualType> qts;
  bool first = true;
  // loop through all arguments
  for (auto it = params.begin(); it != params.end(); it++) {
    if (first) {
      prev_qt = (*it)->getOriginalType();
      qts.push_back(prev_qt);
      CToFTypeFormatter tf((*it)->getOriginalType(), funcDecl->getASTContext());
      paramsType = tf.getFortranTypeASString(false);
      first = false;

      // add the return type too
      CToFTypeFormatter rtf(returnQType, funcDecl->getASTContext());
      if (!returnQType.getTypePtr()->isVoidType()) {
        if (rtf.isSameType(prev_qt)) {
          //llvm::outs() << "same type as previous" << (*it)->getOriginalType().getAsString() + "\n";
        } else {
          // check if type is in the vector
          bool add = true;
          for (auto v = qts.begin(); v != qts.end(); v++) {
            if (rtf.isSameType(*v)) {
              add = false;
            }
          }
          if (add) {
            paramsType += (", " + rtf.getFortranTypeASString(false));
          }
          //llvm::outs() << "different type as previous" << (*it)->getOriginalType().getAsString() + "\n";
        }
        prev_qt = returnQType;
        qts.push_back(prev_qt);
      }

    } else {
      CToFTypeFormatter tf((*it)->getOriginalType(), funcDecl->getASTContext());
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

// for inserting variable decls "<<<type(c_ptr), value :: arg_1>>>"
string FunctionDeclFormatter::getParamsDeclASString() { 
  string paramsDecl;
  int index = 1;
  for (auto it = params.begin(); it != params.end(); it++) {
    // if the param name is empty, rename it to arg_index
    string pname = (*it)->getNameAsString();
    if (pname.empty()) {
      pname = "arg_" + to_string(index);
    }
    
    CToFTypeFormatter tf((*it)->getOriginalType(), funcDecl->getASTContext());
    // in some cases parameter doesn't have a name
    paramsDecl += "\t" + tf.getFortranTypeASString(true) + ", value" + " :: " + pname + "\n"; // need to handle the attribute later
    index ++;
  }
  return paramsDecl;
}

// for inserting variable decls "getline(<<<arg_1, arg_2, arg_3>>>)"
string FunctionDeclFormatter::getParamsNamesASString() { 
  string paramsNames;
  int index = 1;
  for (auto it = params.begin(); it != params.end(); it++) {
    if (it == params.begin()) {
    // if the param name is empty, rename it to arg_index
      //uint64_t  getTypeSize (QualType T) const for array!!!
    string pname = (*it)->getNameAsString();
    if (pname.empty()) {
      pname = "arg_" + to_string(index);
    }
      paramsNames += pname;
    } else { // parameters in between
    // if the param name is empty, rename it to arg_index
      //uint64_t  getTypeSize (QualType T) const for array!!!
      //       outs() << " c_qualType size:" << ac.getTypeSize(c_qualType)
      // << "e_qualType size: " << ac.getTypeSize(e_qualType) <<"\n";
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

// return the entire function decl in fortran
string FunctionDeclFormatter::getFortranFunctDeclASString() {
  string fortanFunctDecl;
  if (!isInSystemHeader) {
    string funcType;
    string imports = "USE iso_c_binding, only: " + getParamsTypesASString();
    // check if the return type is void or not
    if (returnQType.getTypePtr()->isVoidType()) {
      funcType = "SUBROUTINE";
    } else {
      CToFTypeFormatter tf(returnQType, funcDecl->getASTContext());
      funcType = tf.getFortranTypeASString(true) + " FUNCTION";
    }

    fortanFunctDecl = funcType + " " + funcDecl->getNameAsString() + "(" + getParamsNamesASString() + ")" + " bind (C)\n";
    fortanFunctDecl += "\t" + imports + "\n";
    fortanFunctDecl += getParamsDeclASString();
    // preserve the function body as comment
    if (funcDecl->hasBody()) {
      Stmt *stmt = funcDecl->getBody();
      clang::SourceManager &sm = rewriter.getSourceMgr();
      // comment out the entire function {!body...}
      string bodyText = Lexer::getSourceText(CharSourceRange::getTokenRange(stmt->getSourceRange()), sm, LangOptions(), 0);
      string commentedBody = "! comment out function body by default \n";
      std::istringstream in(bodyText);
      for (std::string line; std::getline(in, line);) {
        commentedBody += "! " + line + "\n";
      }
      fortanFunctDecl += commentedBody;

    }
    fortanFunctDecl += "END " + funcDecl->getNameAsString() + "\n";   
  }

  return fortanFunctDecl;
};

// -----------initializer MacroFormatter--------------------
MacroFormatter::MacroFormatter(const Token MacroNameTok, const MacroDirective *md, CompilerInstance &ci) : md(md) { //, ci(ci) {
    const MacroInfo *mi = md->getMacroInfo();
    SourceManager& SM = ci.getSourceManager();

    // define macro properties
    isObjectOrFunction = mi->isObjectLike();
    isInSystemHeader = SM.isInSystemHeader(mi->getDefinitionLoc());

    // source text
    macroName = Lexer::getSourceText(CharSourceRange::getTokenRange(MacroNameTok.getLocation(), MacroNameTok.getEndLoc()), SM, LangOptions(), 0);
    string macroDef = Lexer::getSourceText(CharSourceRange::getTokenRange(mi->getDefinitionLoc(), mi->getDefinitionEndLoc()), SM, LangOptions(), 0);
    
    // strangely there might be a "(" follows the macroName for function macros, remove it if there is
    if (macroName.back() == '(') {
      //outs() << "unwanted parenthesis found, remove it \n";
      macroName.erase(macroName.size()-1);
    }

    // get value for object macro
    bool frontSpace = true;
    for (size_t i = macroName.size(); i < macroDef.size(); i++) {
      if (macroDef[i] != ' ') {
        frontSpace = false;
        macroVal += macroDef[i];
      } else if (frontSpace == false) {
        macroVal += macroDef[i];
      }
    }
}

bool MacroFormatter::isObjectLike() {
  return isObjectOrFunction;
};
bool MacroFormatter::isFunctionLike() {
  return !isObjectOrFunction;
};

// return the entire macro in fortran
string MacroFormatter::getFortranMacroASString() {
  string fortranMacro;
  if (!isInSystemHeader) {
    // handle object first
    if (isObjectLike()) {
      // analyze type
      if (!macroVal.empty()) {
        if (CToFTypeFormatter::isAllDigit(macroVal)) {
          fortranMacro = "integer(c_int), parameter, public :: "+ macroName + " = " + macroVal + "\n";
        } else if (CToFTypeFormatter::isString(macroVal)) {
          fortranMacro = "CHARACTER("+ to_string(macroVal.size()-2)+"), parameter, public :: "+ macroName + " = " + macroVal + "\n";
        } else if (CToFTypeFormatter::isLongType(macroVal)) {
          // format type def
        } else if (CToFTypeFormatter::isShortType(macroVal)) {
          // format type def
        } else if (CToFTypeFormatter::isIntType(macroVal)) {
          // format type def
        } else {
          outs() << "type not supported yet\n";
        }
    } else { // macroVal.empty(), make the object a bool positive
      fortranMacro = "integer(c_int), parameter, public :: "+ macroName  + " = 1 \n";
    }


    } else {
      outs() << "function is not supported yet\n";
    }    
  }


  return fortranMacro;
};



//-----------AST visit functions----------------------------------------------------------------------------------------------------

bool TraverseNodeVisitor::TraverseDecl(Decl *d) {
  if (isa<TranslationUnitDecl> (d)) {
    // tranlastion unit decl is the top node of all AST, ignore the inner structure of tud for now
    RecursiveASTVisitor<TraverseNodeVisitor>::TraverseDecl(d);

  } else if (isa<FunctionDecl> (d)) {
    FunctionDeclFormatter fdf(cast<FunctionDecl> (d), TheRewriter);
    if (!fdf.isInSystemHeader) {
      llvm::outs() << fdf.getFortranFunctDeclASString() << "\n";      
    }
  } else if (isa<TypedefDecl> (d)) {
    TypedefDecl *tdd = cast<TypedefDecl> (d);
    if (tdd->getSourceRange().getBegin().isValid()) {
      if (TheRewriter.getSourceMgr().isInSystemHeader(tdd->getSourceRange().getBegin())) {
        // type defs in header, skip for now
      } else {
        // type defs to be tranlated
        outs() << "TypedefDecl not isInSystemHeader\n";
      }      
    } else { 
        // location not valid: top dummy type defs, skip for now
    }

  } else if (isa<RecordDecl> (d)) {
    // struct
    RecordDeclFormatter rdf(cast<RecordDecl> (d));

    // // NOT WORKING!
    // if (recordDecl->isAnonymousStructOrUnion()) {
    //   llvm::outs() << "is Anonymous\n"; 
    // } else {
    //   llvm::outs() << "is not Anonymous\n"; 
    // }

    // assume struct, not considering union
    llvm::outs() << rdf.getFortranStructASString();

  } else if (isa<VarDecl> (d)) {
    VarDecl *varDecl = cast<VarDecl> (d);

    if (varDecl->getType().getTypePtr()->isStructureType()) {
      // structure type
      RecordDecl *rd = varDecl->getType().getTypePtr()->getAsStructureType()->getDecl();
      RecordDeclFormatter rdf(rd);
      rdf.setTagName(varDecl->getNameAsString());

      llvm::outs() << rdf.getFortranStructASString();
    } else {
      llvm::outs() << "not structure type\n";
      llvm::outs() << "found VarDecl " << varDecl->getNameAsString() 
      << " type: " << varDecl->getType().getAsString() << "\n";
    }

  } else if (isa<EnumDecl> (d)) {
    EnumDecl *enumDecl = cast<EnumDecl> (d);
    llvm::outs() << "found EnumDecl " << enumDecl->getNameAsString()+ "\n";
  } else {

    llvm::outs() << "found other type of declaration \n";
    d->dump();
  }
    // comment out because function declaration doesn't need to be traversed.
    // RecursiveASTVisitor<TraverseNodeVisitor>::TraverseDecl(d); // Forward to base class

    return true; // Return false to stop the AST analyzing
};


bool TraverseNodeVisitor::TraverseStmt(Stmt *x) {
  llvm::outs() << "found statement \n";
  x->dump();
  RecursiveASTVisitor<TraverseNodeVisitor>::TraverseStmt(x);
  return true;
};
bool TraverseNodeVisitor::TraverseType(QualType x) {
  llvm::outs() << "found type " << x.getAsString() << "\n";
  x->dump();
  RecursiveASTVisitor<TraverseNodeVisitor>::TraverseType(x);
  return true;
};

//-----------PP Callbacks functions----------------------------------------------------------------------------------------------------

class TraverseMacros : public PPCallbacks {
  CompilerInstance &ci;
  // SourceManager& SM;// = ci.getSourceManager();
  // Preprocessor &pp; // = ci.getPreprocessor();
  // int Indent;
  // llvm::formatted_raw_ostream FOuts;
public:

  explicit TraverseMacros(CompilerInstance &ci)
  : ci(ci) {}//, SM(ci.getSourceManager()), pp(ci.getPreprocessor()), 
  //Indent(0), FOuts(llvm::outs()) {}

  // void FileChanged(SourceLocation loc, FileChangeReason Reason, SrcMgr::CharacteristicKind, FileID) {
  //   SourceManager& SM = ci.getSourceManager();
  //   if (Reason != EnterFile && Reason != ExitFile)
  //     return;
  //   if (const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(loc))) {
  //     if (Reason == EnterFile) {
  //       FOuts << "Include Tree:";
  //       FOuts.PadToColumn(13 + Indent * 2);
  //       FOuts << FE->getName() << "\n";
  //       Indent++;
  //     } else if (Reason == ExitFile) {
  //       Indent--;
  //     }
  //   }
  // };

  // conditional macros
  void  If (SourceLocation Loc, SourceRange ConditionRange, ConditionValueKind ConditionValue) {};
  void  Elif (SourceLocation Loc, SourceRange ConditionRange, ConditionValueKind ConditionValue, SourceLocation IfLoc) {};
  void  Ifdef (SourceLocation Loc, const Token &MacroNameTok, const MacroDefinition &MD) {};
  void  Ifndef (SourceLocation Loc, const Token &MacroNameTok, const MacroDefinition &MD) {};
  void  Else (SourceLocation Loc, SourceLocation IfLoc) {};


  void MacroExpands (const Token &MacroNameTok, const MacroDefinition &MD, SourceRange Range, const MacroArgs *Args) {
    // #define X 3
    // #if X == 1
    // int a;
    // #else
    // int b;
    // #endif
  };

  void MacroDefined (const Token &MacroNameTok, const MacroDirective *MD) {
    MacroFormatter mf(MacroNameTok, MD, ci);
    outs() << mf.getFortranMacroASString();
  }

};

//-----------the main program----------------------------------------------------------------------------------------------------

class TraverseNodeConsumer : public clang::ASTConsumer {
public:
  TraverseNodeConsumer(Rewriter &R) : Visitor(R) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
  // Traversing the translation unit decl via a RecursiveASTVisitor
  // will visit all nodes in the AST.

    // initalize interface
    string beginSourceInterface = "INTERFACE\n";
    llvm::outs() << beginSourceInterface;

    Visitor.TraverseDecl(Context.getTranslationUnitDecl());

    // end interface
    string endSourceInterface = "END INTERFACE\n";
    llvm::outs() << endSourceInterface;
  }

private:
// A RecursiveASTVisitor implementation.
  TraverseNodeVisitor Visitor;
};

class TraverseNodeAction : public clang::ASTFrontendAction {
public:
  TraverseNodeAction() {}

  // // for macros inspection
  bool BeginSourceFileAction(CompilerInstance &ci, StringRef Filename) override
  {
    // get module name
    fullPathFileName = Filename;
    size_t slashes = Filename.find_last_of("/\\");
    string filename = Filename.substr(slashes+1);
    size_t dot = filename.find('.');
    string moduleName = filename.substr(0, dot);

    // initalize Module and imports
    string beginSourceModule;
    beginSourceModule = "MODULE " + moduleName + "\n";
    beginSourceModule += "USE, INTRINSIC :: iso_c_binding\n";
    beginSourceModule += "implicit none\n";
    outs() << beginSourceModule;


    // std::unique_ptr<Find_Includes> find_includes_callback(new Find_Includes());
    // Preprocessor &pp = ci.getPreprocessor();
    // pp.addPPCallbacks(std::move(find_includes_callback));
    Preprocessor &pp = ci.getPreprocessor();
    //pp.addPPCallbacks(llvm::make_unique<TraverseMacros>(ci.getSourceManager()));
    pp.addPPCallbacks(llvm::make_unique<TraverseMacros>(ci));

    return true;
  }

  void EndSourceFileAction() override {
    //Now emit the rewritten buffer.
    //TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
    
    size_t slashes = fullPathFileName.find_last_of("/\\");
    string filename = fullPathFileName.substr(slashes+1);
    size_t dot = filename.find('.');
    string moduleName = filename.substr(0, dot);

    string endSourceModle;
    endSourceModle = "END MODULE " + moduleName + "\n";
    outs() << endSourceModle;
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    TheRewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
    return llvm::make_unique<TraverseNodeConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
  string fullPathFileName;
};

int main(int argc, const char **argv) {
  if (argc > 1) {
    CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<TraverseNodeAction>().get());
  }
  //  else if (argc == 3) {
  //   clang::tooling::runToolOnCode(new TraverseNodeAction, argv[1]);
  // } else {
  //   llvm::outs() 
  //   << "USAGE: ~/clang-llvm/build/bin/node-inspect <PATH> OR "
  //   << "~/clang-llvm/build/bin/node-inspect <CODE> inline";
  // }
};