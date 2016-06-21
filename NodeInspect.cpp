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
      CToFTypeFormatter tf((*it)->getType());
      string identifier = (*it)->getNameAsString();

      fieldsInFortran += "\t" + tf.getFortranTypeASString(true) + " :: " + identifier + "\n";

      // llvm::outs() << "identifier:" <<(*it)->getNameAsString() 
      // << " type: " << (*it)->getType().getAsString()<< "\n";
    }
  }
  return fieldsInFortran;
}

string RecordDeclFormatter::getFortranStructASString() {
  // struct name: recordDecl->getNameAsString()
  // type,bind(c): (*it)->getNameAsString() if empty chang to (*it)->getType().getAsString()
  // type(c_type) :: var1, var2...
  // end type

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

void RecordDeclFormatter::setMode(int m) {
  // int ANONYMOUS = 0;
  // int ID_ONLY = 1;
  // int TAG_ONLY = 2;
  // int ID_TAG = 3;

  if (mode == ID_ONLY and !tag_name.empty()) {
    mode = ID_TAG;
  } else if (mode == TAG_ONLY and m == ID_ONLY) {
    mode = ID_TAG;
  } else {
    mode = m;
  }
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


bool CToFTypeFormatter::isNumeric(const string input) {
  // "123L" "18446744073709551615ULL" "18446744073709551615UL" "1.23"
  return std::all_of(input.begin(), input.end(), ::isdigit);
};



FunctionDeclFormatter::FunctionDeclFormatter(FunctionDecl *f, Rewriter &r) : rewriter(r) {
  funcDecl = f;
  returnQType = funcDecl->getReturnType();
  params = funcDecl->parameters();
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

      // add the return type too
      CToFTypeFormatter rtf(returnQType);
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
  int index = 1;
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
  int index = 1;
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
  fortanFunctDecl += "END " + funcType + " " + funcDecl->getNameAsString() + "\n";


  return fortanFunctDecl;
};


MacroFormatter::MacroFormatter(const Token MacroNameTok, const MacroDirective *md, CompilerInstance &ci) : md(md), ci(ci) {
    const MacroInfo *mi = md->getMacroInfo();
    SourceManager& SM = ci.getSourceManager();

    // source text
    macroName = Lexer::getSourceText(CharSourceRange::getTokenRange(MacroNameTok.getLocation(), MacroNameTok.getEndLoc()), SM, LangOptions(), 0);
    string macroDef = Lexer::getSourceText(CharSourceRange::getTokenRange(mi->getDefinitionLoc(), mi->getDefinitionEndLoc()), SM, LangOptions(), 0);
    
    // there might be a "(" follows the macroName for function macros, remove it
    if (macroName.back() == '(') {
      outs() << "unwanted parenthesis found, remove it \n";
      macroName.erase(macroName.size()-1);
    }

    if (mi->isFunctionLike()) {
      isObjectOrFunction = false;
    } else if (mi->isObjectLike()) {
      isObjectOrFunction = true;

      bool frontSpace = true;
      for (size_t i = macroName.size(); i < macroDef.size(); i++) {
        if (macroDef[i] != ' ') {
          frontSpace = false;
          macroVal += macroDef[i];
        } else if (frontSpace == false) {
          macroVal += macroDef[i];
        }
      }

    // if (CToFTypeFormatter::isNumeric(macroVal)) {
    //   outs() << "value is numeric\n";
    // } else {
    //   outs() << "value is not numerics\n";
    // }

    }

}

bool MacroFormatter::isObjectLike() {
  return isObjectOrFunction;
};
bool MacroFormatter::isFunctionLike() {
  return !isObjectOrFunction;
};

string MacroFormatter::getFortranMacroASString() {
  string fortranMacro;

  // handle object first
  if (isObjectLike()) {
    // analyze type
    if (!macroVal.empty()) {
      if (CToFTypeFormatter::isNumeric(macroVal)) {
        fortranMacro = "integer( c_int), parameter, public :: "+ macroName + " = " + macroVal + "\n";
      }
    } else {
      fortranMacro = "<undecleared type>, parameter, public :: "+ macroName + "\n";
    }
    

  } else {
    outs() << "function is not supported yet\n";
  }

  return fortranMacro;
};



//-----------AST visit functions----------------------------------------------------------------------------------------------------

bool TraverseNodeVisitor::TraverseDecl(Decl *d) {
  if (isa<TranslationUnitDecl> (d)) {
    // tranlastion unit decl is the top node of all AST, ignore the inner structure of tud for now
    llvm::outs() << "this is a TranslationUnitDecl\n";
    RecursiveASTVisitor<TraverseNodeVisitor>::TraverseDecl(d);
  } else if (isa<FunctionDecl> (d)) {
        // create formatter
    FunctionDeclFormatter fdf(cast<FunctionDecl> (d), TheRewriter);
    // -------------------------------dump Fortran-------------------------------
    llvm::outs() << fdf.getFortranFunctDeclASString()
    << "\n";

  } else if (isa<TypedefDecl> (d)) {
    //TypedefDecl *tdd = cast<TypedefDecl> (d);
    llvm::outs() << "found TypedefDecl \n";
  } else if (isa<RecordDecl> (d)) {
    // struct
    RecordDeclFormatter rdf(cast<RecordDecl> (d));
    RecordDecl *recordDecl = rdf.recordDecl;

    // check if there is an identifier

    // // NOT WORKING!
    // if (recordDecl->isAnonymousStructOrUnion()) {
    //   llvm::outs() << "is Anonymous\n"; 
    // } else {
    //   llvm::outs() << "is not Anonymous\n"; 
    // }
    if (!(recordDecl->getNameAsString()).empty()) {
      rdf.setMode(rdf.ID_ONLY);
    } 
    // assume struct, not considering union

    // dump the fortran code
    if (rdf.mode != rdf.ANONYMOUS) {
      // struct has a identifier 
      llvm::outs() << rdf.getFortranStructASString();
    }

    //RecursiveASTVisitor<TraverseNodeVisitor>::TraverseDecl(d);
  } else if (isa<VarDecl> (d)) {
    VarDecl *varDecl = cast<VarDecl> (d);

    if (varDecl->getType().getTypePtr()->isStructureType()) {
      // structure type
      RecordDecl *rd = varDecl->getType().getTypePtr()->getAsStructureType()->getDecl();
      RecordDeclFormatter rdf(rd);

      if (!(rdf.recordDecl->getNameAsString()).empty()) {
        rdf.setMode(rdf.ID_TAG);
      }
      rdf.setTagName(varDecl->getNameAsString());

      llvm::outs() << rdf.getFortranStructASString();

    } else {
      llvm::outs() << "not structure type\n";
    }

    llvm::outs() << "found VarDecl " << varDecl->getNameAsString() 
    << " type: " << varDecl->getType().getAsString() << "\n";

  } else if (isa<EnumDecl> (d)) {
    EnumDecl *enumDecl = cast<EnumDecl> (d);
    llvm::outs() << "found EnumDecl " << enumDecl->getNameAsString()+ "\n";
  } else {
    llvm::outs() << "found other declaration \n";
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

// class Find_Includes : public PPCallbacks
// {
// public:
//   bool has_include;

//   void InclusionDirective(
//     SourceLocation hash_loc,
//     const Token &include_token,
//     StringRef file_name,
//     bool is_angled,
//     CharSourceRange filename_range,
//     const FileEntry *file,
//     StringRef search_path,
//     StringRef relative_path,
//     const clang::Module *imported) {
//     // do something with the include
//     has_include = true;
//     //const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(hash_loc));
//     outs() << "found includes " << include_token.getLiteralData() << "\n";

//   }
// };


class TraverseMacros : public PPCallbacks {
  CompilerInstance &ci;
  // SourceManager& SM;// = ci.getSourceManager();
  // Preprocessor &pp; // = ci.getPreprocessor();
  int Indent;
  llvm::formatted_raw_ostream FOuts;
public:

  explicit TraverseMacros(CompilerInstance &ci)
  : ci(ci), //SM(ci.getSourceManager()), pp(ci.getPreprocessor()), 
  Indent(0), FOuts(llvm::outs()) {}

  void InclusionDirective(
    SourceLocation hash_loc,
    const Token &include_token,
    StringRef file_name,
    bool is_angled,
    CharSourceRange filename_range,
    const FileEntry *file,
    StringRef search_path,
    StringRef relative_path,
    const clang::Module *imported) {
    // do something with the include
    //bool has_include = true;
    //const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(hash_loc));
    outs() << "found includes " << include_token.getLiteralData() << "\n";

  };

  void FileChanged(SourceLocation loc, FileChangeReason Reason, SrcMgr::CharacteristicKind, FileID) {
    SourceManager& SM = ci.getSourceManager();
    if (Reason != EnterFile && Reason != ExitFile)
      return;
    if (const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(loc))) {
      if (Reason == EnterFile) {
        FOuts << "Include Tree:";
        FOuts.PadToColumn(13 + Indent * 2);
        FOuts << FE->getName() << "\n";
        Indent++;
      } else if (Reason == ExitFile) {
        Indent--;
      }
    }
  };

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
  TraverseNodeAction() {}

  // // for macros inspection
  bool BeginSourceFileAction(CompilerInstance &ci, StringRef) override
  {
    // std::unique_ptr<Find_Includes> find_includes_callback(new Find_Includes());
    // Preprocessor &pp = ci.getPreprocessor();
    // pp.addPPCallbacks(std::move(find_includes_callback));

    Preprocessor &pp = ci.getPreprocessor();
    //pp.addPPCallbacks(llvm::make_unique<TraverseMacros>(ci.getSourceManager()));
    pp.addPPCallbacks(llvm::make_unique<TraverseMacros>(ci));

    return true;
  }

//   void EndSourceFileAction()
//   {
//     CompilerInstance &ci = getCompilerInstance();
//     Preprocessor &pp = ci.getPreprocessor();
//     Find_Includes *find_includes_callback = dynamic_cast<Find_Includes *>(pp.getPPCallbacks());

//     // do whatever you want with the callback now
//     if (find_includes_callback->has_include)
//       llvm::outs() << "Found at least one include\n";
//   }

  void EndSourceFileAction() override {
    //Now emit the rewritten buffer.
    //TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
  }

  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    TheRewriter.setSourceMgr(Compiler.getSourceManager(), Compiler.getLangOpts());
    return llvm::make_unique<TraverseNodeConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
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