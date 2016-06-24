#include "NodeInspect.h"

//-----------formatter functions----------------------------------------------------------------------------------------------------

// -----------initializer RecordDeclFormatter--------------------
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
  } else {
    return c_qualType == qt2;
  }
};

string CToFTypeFormatter::getFortranIdASString(string raw_id) {
  if (c_qualType.getTypePtr()->isArrayType()) {
    const ArrayType *at = c_qualType.getTypePtr()->getAsArrayTypeUnsafe ();
    QualType e_qualType = at->getElementType ();
    int typeSize = ac.getTypeSizeInChars(c_qualType).getQuantity();
    int elementSize = ac.getTypeSizeInChars(e_qualType).getQuantity();
    int numOfEle = typeSize / elementSize;
    string arr_suffix = "(" + to_string(numOfEle) + ")";
    raw_id += arr_suffix;
  }
  return raw_id;
};

string CToFTypeFormatter::getFortranTypeASString(bool typeWrapper) {
  string f_type;

      // char
  if (c_qualType.getTypePtr()->isCharType()) {
    if (typeWrapper) {
      f_type = "CHARACTER(C_CHAR)";
    } else {
      f_type = "C_CHAR";
    }
    // INT
  } else if (c_qualType.getTypePtr()->isIntegerType()) {
    //int typeSize = ac.getTypeSizeInChars(c_qualType).getQuantity();
    // diff int type may have same size so use stirng matching for now
     if (c_qualType.getAsString()== "size_t") {
      // size_t
      if (typeWrapper) {
        f_type = "INTEGER(C_SIZE_T)";
      } else {
        f_type = "C_SIZE_T";
      }
    } else if (c_qualType.getAsString()== "unsigned char" or c_qualType.getAsString()== "signed char") {
      // signed/unsigned char
      if (typeWrapper) {
        f_type = "INTEGER(C_SIGNED_CHAR)";
      } else {
        f_type = "C_SIGNED_CHAR";
      }        
    } else if (c_qualType.getAsString().find("short") != std::string::npos) {
      // short
      if (typeWrapper) {
        f_type = "INTEGER(C_SHORT)";
      } else {
        f_type = "C_SHORT";
      }  
    } else if (c_qualType.getAsString().find("long") != std::string::npos) {
      // long or long long, assume long
      if (typeWrapper) {
        f_type = "INTEGER(C_LONG)";
      } else {
        f_type = "C_LONG";
      }      
    } else {
      // other int so just assume int
      if (typeWrapper) {
        f_type = "INTEGER(C_INT)";
      } else {
        f_type = "C_INT";
      }      
    }

    // REAL
  } else if (c_qualType.getTypePtr()->isRealType()) {

    if (c_qualType.getAsString().find("long") != std::string::npos) {
      // long double
      if (typeWrapper) {
        f_type = "REAL(C_LONG_DOUBLE)";
      } else {
        f_type = "C_LONG_DOUBLE";
      } 
    } else if (c_qualType.getAsString()== "float") {
      // float
      if (typeWrapper) {
        f_type = "REAL(C_FLOAT)";
      } else {
        f_type = "C_FLOAT";
      }
    } else if (c_qualType.getAsString()== "__float128") {
      // __float128
      if (typeWrapper) {
        f_type = "REAL(C_FLOAT128)";
      } else {
        f_type = "C_FLOAT128";
      }
    } else {
      // should be double
      if (typeWrapper) {
        f_type = "REAL(C_DOUBLE)";
      } else {
        f_type = "C_DOUBLE";
      }      
    }
    // COMPLEX
  } else if (c_qualType.getTypePtr()->isComplexType ()) {
    if (c_qualType.getAsString().find("float") != std::string::npos) {
      //  float _Complex 
      if (typeWrapper) {
        f_type = "COMPLEX(C_FLOAT_COMPLEX)";
      } else {
        f_type = "C_FLOAT_COMPLEX";
      }        
    } else if (c_qualType.getAsString().find("long") != std::string::npos) {
       //  long double _Complex 
      if (typeWrapper) {
        f_type = "COMPLEX(C_LONG_DOUBLE_COMPLEX)";
      } else {
        f_type = "C_LONG_DOUBLE_COMPLEX";
      }
    } else {
      // assume double _Complex 
      if (typeWrapper) {
        f_type = "COMPLEX(C_DOUBLE_COMPLEX)";
      } else {
        f_type = "C_DOUBLE_COMPLEX";
      }
    }
    // POINTER
  } else if (c_qualType.getTypePtr()->isPointerType ()) {
    if (c_qualType.getTypePtr()->isFunctionPointerType()){
      if (typeWrapper) {
        f_type = "TYPE(C_FUNPTR)";
      } else {
        f_type = "C_FUNPTR";
      }
    } else {
      if (typeWrapper) {
        f_type = "TYPE(C_PTR)";
      } else {
        f_type = "C_PTR";
      }
    }
    // ARRAY
  } else if (c_qualType.getTypePtr()->isArrayType()) {
    const ArrayType *at = c_qualType.getTypePtr()->getAsArrayTypeUnsafe ();
    // recursively get element type
    QualType e_qualType = at->getElementType ();
    CToFTypeFormatter etf(e_qualType, ac);

    f_type = etf.getFortranTypeASString(typeWrapper);
  } else if (c_qualType.getTypePtr()->isStructureType()) {
    // struct type
    f_type = c_qualType.getAsString();
    // replace space with underscore 
    size_t found = f_type.find_first_of(" ");
    while (found!=string::npos) {
      f_type[found]='_';
      found=f_type.find_first_of(" ",found+1);
    }
    if (typeWrapper) {
      f_type = "TYPE(" + f_type + ")";
    } 
  } else {
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

// -----------initializer RecordDeclFormatter--------------------

RecordDeclFormatter::RecordDeclFormatter(RecordDecl* rd, Rewriter &r) : rewriter(r) {
  recordDecl = rd;
  isInSystemHeader = rewriter.getSourceMgr().isInSystemHeader(recordDecl->getSourceRange().getBegin());
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
      string identifier = tf.getFortranIdASString((*it)->getNameAsString());

      fieldsInFortran += "\t" + tf.getFortranTypeASString(true) + " :: " + identifier + "\n";
    }
  }
  return fieldsInFortran;
}

string RecordDeclFormatter::getFortranStructASString() {
  // initalize mode here
  setMode();

  string rd_buffer;
  if (!isInSystemHeader) {
    if (mode == ID_ONLY) {
      string identifier = "struct_" + recordDecl->getNameAsString();
      string fieldsInFortran = getFortranFields();
      rd_buffer = "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";
      
    } else if (mode == TAG_ONLY) {
      string identifier = tag_name;
      string fieldsInFortran = getFortranFields();

      rd_buffer = "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";
    } else if (mode == ID_TAG) {
      string identifier = tag_name;
      string fieldsInFortran = getFortranFields();

      rd_buffer = "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";    
    }

    
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
      string commentedBody = "! function body \n";
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
        if (CToFTypeFormatter::isString(macroVal)) {
          fortranMacro = "CHARACTER("+ to_string(macroVal.size()-2)+"), parameter, public :: "+ macroName + " = " + macroVal + "\n";
        } else if (CToFTypeFormatter::isAllDigit(macroVal)) {
          fortranMacro = "integer(c_int), parameter, public :: "+ macroName + " = " + macroVal + "\n";
        } else if (CToFTypeFormatter::isLongType(macroVal)) {
          // format type def
        } else if (CToFTypeFormatter::isShortType(macroVal)) {
          // format type def
        } else if (CToFTypeFormatter::isIntType(macroVal)) {
          // format type def
        } else {
          outs() << "!macro type not supported yet\n";
        }
    } else { // macroVal.empty(), make the object a bool positive
      fortranMacro = "integer(c_int), parameter, public :: "+ macroName  + " = 1 \n";
    }


    } else {
        // function macro
      if (md->getMacroInfo()->arg_empty()) {
        outs() << "args are empty\n";
      } else {
        for (auto it = md->getMacroInfo()->arg_begin (); it != md->getMacroInfo()->arg_end (); it++) {
          outs() << " getName () " << (*it)->getName();
        }
        for (auto it = md->getMacroInfo()->tokens_begin (); it != md->getMacroInfo()->tokens_end (); it++) {
          outs() << " token: " << (*it).getName();
        }
      }    
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

      llvm::outs() << "INTERFACE\n" 
      << fdf.getFortranFunctDeclASString() << "\n"
      << "END INTERFACE\n";      
    }
  } else if (isa<TypedefDecl> (d)) {
    TypedefDecl *tdd = cast<TypedefDecl> (d);
    if (tdd->getSourceRange().getBegin().isValid()) {
      if (TheRewriter.getSourceMgr().isInSystemHeader(tdd->getSourceRange().getBegin())) {
        // type defs in header, skip for now
      } else {
        // type defs to be tranlated
        string sourceText = Lexer::getSourceText(CharSourceRange::getTokenRange(tdd->getSourceRange()), TheRewriter.getSourceMgr(), LangOptions(), 0);
        string typeDefStr = "! type def content \n";
        std::istringstream in(sourceText);
        for (std::string line; std::getline(in, line);) {
          typeDefStr += "! " + line + "\n";
        }
        outs() << typeDefStr;
      }      
    } else { 
        // location not valid: top dummy type defs at top of the file, skip for now
    }

  } else if (isa<RecordDecl> (d)) {
    // struct
    RecordDeclFormatter rdf(cast<RecordDecl> (d), TheRewriter);
    // recordDecl->isAnonymousStructOrUnion() NOT WORKING!

    // assume struct, not considering union
    llvm::outs() << rdf.getFortranStructASString();

  } else if (isa<VarDecl> (d)) {
    VarDecl *varDecl = cast<VarDecl> (d);

    if (varDecl->getType().getTypePtr()->isStructureType()) {
      // structure type
      RecordDecl *rd = varDecl->getType().getTypePtr()->getAsStructureType()->getDecl();
      RecordDeclFormatter rdf(rd, TheRewriter);
      rdf.setTagName(varDecl->getNameAsString());

      llvm::outs() << rdf.getFortranStructASString();
    } else {
      if (TheRewriter.getSourceMgr().isInSystemHeader(varDecl->getSourceRange().getBegin())) {
        // should skip  "variable decl in system header\n";
      } else {
        llvm::outs() << "not structure type\n";
        llvm::outs() << "found VarDecl " << varDecl->getNameAsString() 
        << " type: " << varDecl->getType().getAsString() << "\n";        
      }

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

  // // conditional macros
  // void  If (SourceLocation Loc, SourceRange ConditionRange, ConditionValueKind ConditionValue) {};
  // void  Elif (SourceLocation Loc, SourceRange ConditionRange, ConditionValueKind ConditionValue, SourceLocation IfLoc) {};
  // void  Ifdef (SourceLocation Loc, const Token &MacroNameTok, const MacroDefinition &MD) {};
  // void  Ifndef (SourceLocation Loc, const Token &MacroNameTok, const MacroDefinition &MD) {};
  // void  Else (SourceLocation Loc, SourceLocation IfLoc) {};


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

    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
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