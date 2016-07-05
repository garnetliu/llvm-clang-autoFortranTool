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
  } else if (c_qualType.getTypePtr()->isBooleanType()) {
    if (typeWrapper) {
      f_type = "LOGICAL(C_BOOL)";
    } else {
      f_type = "C_BOOL";
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
    // ARRAY
  } else if (c_qualType.getTypePtr()->isArrayType()) {
    const ArrayType *at = c_qualType.getTypePtr()->getAsArrayTypeUnsafe ();
    // recursively get element type
    QualType e_qualType = at->getElementType ();
    CToFTypeFormatter etf(e_qualType, ac);

    f_type = etf.getFortranTypeASString(typeWrapper);
  } else {
    f_type = "unrecognized_type(" + c_qualType.getAsString()+")";
  }
  return f_type;
};


bool CToFTypeFormatter::isIntLike(const string input) {
  // "123L" "18446744073709551615ULL" "18446744073709551615UL" 
  if (std::all_of(input.begin(), input.end(), ::isdigit)) {
    return true;
  } else {
    string temp = input;
    size_t doubleF = temp.find_first_of(".eF");
    if (doubleF!=std::string::npos) {
      return false;
    }    
    
    size_t found = temp.find_first_of("01234567890");
    if (found==std::string::npos) {
      return false;
    }

    while (found!=std::string::npos)
    {
      temp.erase(found, found+1);
      found=temp.find_first_of("01234567890");
    }
    
    if (!temp.empty()) {
      found = temp.find_first_of("xUL()-");
      while (found!=std::string::npos)
      {
        temp.erase(found, found+1);
        found=temp.find_first_of("xUL()-");
      }
      return temp.empty();
    } else {
      return false;
    }
  }
};

bool CToFTypeFormatter::isDoubleLike(const string input) {
  // "1.23", 1.18973149535723176502e+4932L
  string temp = input;
  size_t found = temp.find_first_of("01234567890");
  if (found==std::string::npos) {
    return false;
  }
  while (found!=std::string::npos)
  {
    temp.erase(found, found+1);
    found=temp.find_first_of("01234567890");
  }
  // no digit anymore
  if (!temp.empty()) {
    size_t doubleF = temp.find_first_of(".eFUL()+-");
    while (doubleF!=std::string::npos)
    {
      temp.erase(doubleF, doubleF+1);
      doubleF=temp.find_first_of(".eFUL()+-");
    }
    return temp.empty();
  } else {
    return false;
  }
  

  
  if (!temp.empty()) {
    found = temp.find_first_of(".eFL()-");
    while (found!=std::string::npos)
    {
      temp.erase(found, found+1);
      found=temp.find_first_of("xUL()-");
    }
    return temp.empty();
  } else {
    return false;
  }
};

bool CToFTypeFormatter::isString(const string input) {
  if (input[0] == '\"' and input[input.size()-1] =='\"') {
    return true;
  }
  return false;
};

bool CToFTypeFormatter::isType(const string input) {
  // only support int short long char for now
  if (input == "short" or input == "long" or input == "char" or input == "int" or
      input.find(" int") != std::string::npos or 
      input.find(" short") != std::string::npos or
      input.find(" long") != std::string::npos or 
      input.find(" char") != std::string::npos) {
    return true;
  }
  return false;
};

string CToFTypeFormatter::createFortranType(const string macroName, const string macroVal) {
  string ft_buffer;
  string type_id = "typeID_" + macroName ;
  // replace space with underscore 
  size_t found = type_id.find_first_of(" ");
  while (found!=string::npos) {
    type_id[found]='_';
    found=type_id.find_first_of(" ",found+1);
  }

  if (macroName[0] == '_') {
    ft_buffer = "! underscore is invalid character name\n";
    ft_buffer += "!TYPE, BIND(C) :: " + macroName+ "\n";
    if (macroVal.find("char") != std::string::npos) {
      ft_buffer += "!\tCHARACTER(C_CHAR) :: " + type_id + "\n";
    } else if (macroVal.find("long") != std::string::npos) {
      ft_buffer += "!\tINTEGER(C_LONG) :: " + type_id + "\n";
    } else if (macroVal.find("short") != std::string::npos) {
      ft_buffer += "!\tINTEGER(C_SHORT) :: " + type_id + "\n";
    } else {
      ft_buffer += "!\tINTEGER(C_INT) :: " + type_id + "\n";
    }
  } else {
    ft_buffer = "TYPE, BIND(C) :: " + macroName+ "\n";
    if (macroVal.find("char") != std::string::npos) {
      ft_buffer += "\tCHARACTER(C_CHAR) :: " + type_id + "\n";
    } else if (macroVal.find("long") != std::string::npos) {
      ft_buffer += "\tINTEGER(C_LONG) :: " + type_id + "\n";
    } else if (macroVal.find("short") != std::string::npos) {
      ft_buffer += "\tINTEGER(C_SHORT) :: " + type_id + "\n";
    } else {
      ft_buffer += "\tINTEGER(C_INT) :: " + type_id + "\n";
    }
  }
  return ft_buffer;
};

// -----------initializer VarDeclFormatter--------------------
VarDeclFormatter::VarDeclFormatter(VarDecl *v, Rewriter &r) : rewriter(r) {
  varDecl = v;
  isInSystemHeader = rewriter.getSourceMgr().isInSystemHeader(varDecl->getSourceRange().getBegin());
};

string VarDeclFormatter::getInitValueASString() {
  string valString;
  if (varDecl->hasInit() and !isInSystemHeader) {
    if (varDecl->getType().getTypePtr()->isStructureType()) {
      // structure type skip
    } else if (varDecl->getType().getTypePtr()->isCharType()) {
      // single CHAR
      char character = varDecl->evaluateValue ()->getInt().getExtValue ();
      string cString;
      cString += character;
      valString = "\'" + cString + "\'";
    } else if (varDecl->getType().getTypePtr()->isIntegerType()) {
      // INT
      int intValue = varDecl->evaluateValue ()->getInt().getExtValue();
      valString = to_string(intValue);
    } else if (varDecl->getType().getTypePtr()->isRealType()) {
      // REAL
      valString = varDecl->evaluateValue ()->getAsString(varDecl->getASTContext(), varDecl->getType());
    } else if (varDecl->getType().getTypePtr()->isComplexType()) {
      // COMPLEX
      APValue *apVal = varDecl->evaluateValue ();
      if (apVal->isComplexFloat ()) {
        float real = apVal->getComplexFloatReal ().convertToFloat ();
        float imag = apVal->getComplexFloatImag ().convertToFloat ();
        valString = "(" + to_string(real) + "," + to_string(imag) +")";
      } else if (apVal->isComplexInt ()) {
        int real = apVal->getComplexIntReal ().getExtValue ();
        int imag = apVal->getComplexIntImag ().getExtValue ();
        valString = "(" + to_string(real) + "," + to_string(imag) +")";
      } 
    } else if (varDecl->getType().getTypePtr()->isPointerType()) {
      // POINTER --- handled by getFortranPtrDeclASString()
      valString = "!" + varDecl->evaluateValue ()->getAsString(varDecl->getASTContext(), varDecl->getType());
    } else if (varDecl->getType().getTypePtr()->isArrayType()) {
      // ARRAY --- handled by getFortranArrayDeclASString()
      Expr *exp = varDecl->getInit();
      string arrayText = Lexer::getSourceText(CharSourceRange::getTokenRange(exp->getExprLoc (), varDecl->getSourceRange().getEnd()), rewriter.getSourceMgr(), LangOptions(), 0);
      // comment out arrayText
      std::istringstream in(arrayText);
      for (std::string line; std::getline(in, line);) {
        valString += "! " + line + "\n";
      }
      // const ArrayType *at = varDecl->getType().getTypePtr()->getAsArrayTypeUnsafe ();
      // QualType e_qualType = at->getElementType ();
      // if (e_qualType.getTypePtr()->isCharType()) {
      //   Expr *exp = varDecl->getInit();
      //   string arrayText = Lexer::getSourceText(CharSourceRange::getTokenRange(exp->getExprLoc (), varDecl->getSourceRange().getEnd()), rewriter.getSourceMgr(), LangOptions(), 0);
      //   valString = arrayText;
      // } else {
      //   valString = "!" + varDecl->evaluateValue ()->getAsString(varDecl->getASTContext(), varDecl->getType());

          
      //   }

      //   string arrayText = Lexer::getSourceText(CharSourceRange::getTokenRange(exp->getExprLoc (), varDecl->getSourceRange().getEnd()), rewriter.getSourceMgr(), LangOptions(), 0);
      //   size_t found = arrayText.find_first_of("{");
      //   while (found!=string::npos) {
      //     arrayText[found]='(';
      //     arrayText.insert(found+1, "/");
      //     found=arrayText.find_first_of("{",found);
      //   }     
      //   found = arrayText.find_first_of("}");
      //   while (found!=string::npos) {
      //     arrayText[found]='/';
      //     arrayText.insert(found+1, ")");
      //     found=arrayText.find_first_of("}",found);
      //   }
      //   valString = arrayText;
      // }

      
    } else {
      valString = "!" + varDecl->evaluateValue()->getAsString(varDecl->getASTContext(), varDecl->getType());
    }
  }
  return valString;

};

string VarDeclFormatter::getFortranArrayEleASString(InitListExpr *ile) {
  return "";
};

string VarDeclFormatter::getFortranArrayDeclASString() {
  string arrayDecl;
  if (varDecl->getType().getTypePtr()->isArrayType() and !isInSystemHeader) {
    CToFTypeFormatter tf(varDecl->getType(), varDecl->getASTContext());
    if (!varDecl->hasInit()) {
      // only declared, no init
      arrayDecl = tf.getFortranTypeASString(true) + ", public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + "\n";
    } else {
      // has init
      const ArrayType *at = varDecl->getType().getTypePtr()->getAsArrayTypeUnsafe ();
      QualType e_qualType = at->getElementType ();
      if (e_qualType.getTypePtr()->isCharType()) {
        // handle stringliteral case
        Expr *exp = varDecl->getInit();
        string arrayText = Lexer::getSourceText(CharSourceRange::getTokenRange(exp->getExprLoc (), varDecl->getSourceRange().getEnd()), rewriter.getSourceMgr(), LangOptions(), 0);
        arrayDecl = tf.getFortranTypeASString(true) + ", parameter, public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + " = " + arrayText + "\n";
      } else {
        bool evaluatable = false;
        Expr *exp = varDecl->getInit();
        if (isa<InitListExpr> (exp)) {
          InitListExpr *ile = cast<InitListExpr> (exp);
          ArrayRef< Expr * > elements = ile->inits ();
          size_t numOfEle = elements.size();
          outs() << "array size: " << numOfEle << "\n";
          for (auto it = elements.begin (); it != elements.end(); it++) {
                Expr *element = (*it);
                if (isa<InitListExpr> (element)) {
                  // multidimensional array
                  InitListExpr *innerIle = cast<InitListExpr> (element);
                  ArrayRef< Expr * > innerElements = innerIle->inits ();
                  size_t numOfInnerEle = innerElements.size();
                  outs() << "inner array size: " << numOfInnerEle << "\n";
                  for (auto it = innerElements.begin (); it != innerElements.end(); it++) {
                    Expr *innerelement = (*it);
                    if (innerelement->isEvaluatable (varDecl->getASTContext())) {
                      evaluatable = true;
                      clang::Expr::EvalResult r;
                      innerelement->EvaluateAsRValue(r, varDecl->getASTContext());
                      string eleVal = r.Val.getAsString(varDecl->getASTContext(), e_qualType);
                      outs() << "innerelement: " << eleVal << "\n";
                    }
                  }
                } else {
                  if (element->isEvaluatable (varDecl->getASTContext())) {
                    evaluatable = true;
                    clang::Expr::EvalResult r;
                    element->EvaluateAsRValue(r, varDecl->getASTContext());
                    string eleVal = r.Val.getAsString(varDecl->getASTContext(), e_qualType);
                    outs() << "element: " << eleVal << "\n";
                  }
                }
              }
              if (!evaluatable) {
                string arrayText = Lexer::getSourceText(CharSourceRange::getTokenRange(varDecl->getSourceRange()), rewriter.getSourceMgr(), LangOptions(), 0);
                // comment out arrayText
                std::istringstream in(arrayText);
                for (std::string line; std::getline(in, line);) {
                  arrayDecl += "! " + line + "\n";
                }
              } else {
                //INTEGER(C_INT), public :: i
                //INTEGER(C_INT), public :: array(100) = [1, 324, 32423, (0, i=4, 100)]
              }
            }
          }     
      }
  }
  return arrayDecl;
};

string VarDeclFormatter::getFortranPtrDeclASString() {
  return "";
};

string VarDeclFormatter::getFortranVarDeclASString() {
  string vd_buffer;
  if (varDecl->getType().getTypePtr()->isStructureType()) {
    // structure type
    RecordDecl *rd = varDecl->getType().getTypePtr()->getAsStructureType()->getDecl();
    RecordDeclFormatter rdf(rd, rewriter);
    rdf.setTagName(varDecl->getNameAsString());
    vd_buffer = rdf.getFortranStructASString();
  } else if (varDecl->getType().getTypePtr()->isArrayType()) {
      // handle initialized numeric array specifically

      vd_buffer = getFortranArrayDeclASString();
      // Expr *exp = varDecl->getInit();
      // string arrayText = Lexer::getSourceText(CharSourceRange::getTokenRange(exp->getExprLoc (), varDecl->getSourceRange().getEnd()), rewriter.getSourceMgr(), LangOptions(), 0);

      // string value = getInitValueASString();
      // CToFTypeFormatter tf(varDecl->getType(), varDecl->getASTContext());
      // if (value.empty()) {
      //   vd_buffer = tf.getFortranTypeASString(true) + ", public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + "\n";
      // } else if (value[0] == '!') {
      //   vd_buffer = tf.getFortranTypeASString(true) + ", public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + " " + value + "\n";
      // } else {
      //   // array is not empty

      //   // calculate the size of the array
      //   const ArrayType *at = varDecl->getType().getTypePtr()->getAsArrayTypeUnsafe ();
      //   QualType e_qualType = at->getElementType ();
      //   int typeSize = varDecl->getASTContext().getTypeSizeInChars(varDecl->getType()).getQuantity();
      //   int elementSize = varDecl->getASTContext().getTypeSizeInChars(e_qualType).getQuantity();
      //   int array_size = typeSize / elementSize;

      //   // remove  { }  !!!!ONLY for one dimensional array
      //   string array_args = arrayText.substr(1, arrayText.size()-2);

      //   if (array_args.empty()) {
      //     vd_buffer = tf.getFortranTypeASString(true) + ", public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + "\n";
      //   } else {
      //     // calculate the number of elements
      //     int numOfEle = 0;
      //     size_t found = array_args.find_first_of(",");

      //     if (found==string::npos) {
      //       numOfEle = 1;
      //     } else {
      //       while (found!=string::npos) {
      //         numOfEle++;
      //         found=array_args.find_first_of(",",found+1);
      //       }
      //       numOfEle++;            
      //     }

      //     string identifier = varDecl->getNameAsString() + "(" + to_string(array_size) + ")";
      //     string arrayIndex = varDecl->getNameAsString() + "_i";

      //     //INTEGER(C_INT), public :: i
      //     vd_buffer = tf.getFortranTypeASString(true) + ", public :: " + arrayIndex + "\n";
      //     //INTEGER(C_INT), public :: array(100) = [1, 324, 32423, (0, i=4, 100)]
      //     vd_buffer += tf.getFortranTypeASString(true) + ", parameter, public :: " + identifier + " = [" + array_args + ",(0,"+ arrayIndex + "="+ to_string(++numOfEle) +"," + to_string(array_size) + ")]\n";
      //   }

      // }    
  } else if (varDecl->getType().getTypePtr()->isPointerType()) {
    // varDecl->hasInit()

    // dig and find the type that this pointer points to
    QualType pointerType = varDecl->getType();
    QualType pointeeType = pointerType.getTypePtr()->getPointeeType();
    QualType *ptrQT = &pointerType;
    QualType *pteQT = &pointeeType;
    while (pteQT->getTypePtr()->isPointerType()) {
      ptrQT = pteQT;
      QualType temp = ptrQT->getTypePtr()->getPointeeType();
      pteQT = &temp;
    }
    CToFTypeFormatter tf(*pteQT, varDecl->getASTContext());
    

    outs() << "this is a pointer variable declaration\n"
    << "name: " << varDecl->getNameAsString() 
    << " pointee type in fortran " << tf.getFortranTypeASString(true) << "\n";
    if (varDecl->hasInit()) {
      Expr *exp =  varDecl->getInit ();
      if (isa<UnaryOperator> (exp)) {
        UnaryOperator *uop = cast<UnaryOperator> (exp);
        outs() << "is a UnaryOperator\n";
        exp = uop->getSubExpr();
        if (isa<DeclRefExpr> (exp)) {
          outs() << "is a DeclRefExpr\n";
        }
      } else {
        outs() << "not a UnaryOperator\n";
      }
      // string expText = Lexer::getSourceText(CharSourceRange::getTokenRange(exp->getExprLoc (), varDecl->getSourceRange().getEnd()), rewriter.getSourceMgr(), LangOptions(), 0);
      // outs() <<"exp name: " << expText << "\n";
    }
  } else {
    string value = getInitValueASString();
    CToFTypeFormatter tf(varDecl->getType(), varDecl->getASTContext());
    if (value.empty()) {
      vd_buffer = tf.getFortranTypeASString(true) + ", public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + "\n";
    } else if (value[0] == '!') {
      vd_buffer = tf.getFortranTypeASString(true) + ", public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + " " + value + "\n";
    } else {
      vd_buffer = tf.getFortranTypeASString(true) + ", parameter, public :: " + tf.getFortranIdASString(varDecl->getNameAsString()) + " = " + value + "\n";
    }


  }
  return vd_buffer;
};

// -----------initializer Typedef--------------------
TypedefDeclFormater::TypedefDeclFormater(TypedefDecl *t, Rewriter &r) : rewriter(r) {
  typedefDecl = t;
  isLocValid = typedefDecl->getSourceRange().getBegin().isValid();
  if (isLocValid) {
    isInSystemHeader = rewriter.getSourceMgr().isInSystemHeader(typedefDecl->getSourceRange().getBegin());
  } 
  
};

string TypedefDeclFormater::getFortranTypedefDeclASString() {
  string typdedef_buffer;
  if (isLocValid and !isInSystemHeader) {
      if (typedefDecl->getTypeSourceInfo()->getType().getTypePtr()->isStructureType()) {
        // skip it, since it will be translated in recordecl
      } else {
        // other regular type defs
        TypeSourceInfo * typeSourceInfo = typedefDecl->getTypeSourceInfo();
        CToFTypeFormatter tf(typeSourceInfo->getType(), typedefDecl->getASTContext());
        string identifier = typedefDecl->getNameAsString();
        typdedef_buffer = "TYPE, BIND(C) :: " + identifier + "\n";
        typdedef_buffer += "\t"+ tf.getFortranTypeASString(true) + "::" + identifier+"_"+tf.getFortranTypeASString(false) + "\n";
        typdedef_buffer += "END TYPE " + identifier + "\n";
      }
    } 
  return typdedef_buffer;
};

// -----------initializer EnumDeclFormatter--------------------
EnumDeclFormatter::EnumDeclFormatter(EnumDecl *e, Rewriter &r) : rewriter(r) {
  enumDecl = e;
  isInSystemHeader = rewriter.getSourceMgr().isInSystemHeader(enumDecl->getSourceRange().getBegin());
};

string EnumDeclFormatter::getFortranEnumASString() {
  string enum_buffer;
  if (!isInSystemHeader) {
    string enumName = enumDecl->getNameAsString();
    enum_buffer = "ENUM, BIND( C )\n";
    enum_buffer += "\tenumerator :: ";
    for (auto it = enumDecl->enumerator_begin (); it != enumDecl->enumerator_end (); it++) {
      string constName = (*it)->getNameAsString ();
      int constVal = (*it)->getInitVal ().getExtValue ();
      enum_buffer += constName + "=" + to_string(constVal) + ", ";
    }
      // erase the redundant colon
      enum_buffer.erase(enum_buffer.size()-2);
      enum_buffer += "\n";
      if (!enumName.empty()) {
        enum_buffer += "\tenumerator " + enumName+"\n";
      }

      enum_buffer += "END ENUM\n";
  }
  

  return enum_buffer;
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
    string fieldsInFortran = getFortranFields();
    if (fieldsInFortran.empty()) {
      rd_buffer = "! struct without fields may cause warning\n";
    }

    if (mode == ID_ONLY) {
      string identifier = "struct_" + recordDecl->getNameAsString();
      
      rd_buffer += "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";
      
    } else if (mode == TAG_ONLY) {
      string identifier = tag_name;

      rd_buffer += "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";
    } else if (mode == ID_TAG) {
      string identifier = tag_name;

      rd_buffer += "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";    
    } else if (mode == TYPEDEF) {
      string identifier = recordDecl->getTypeForDecl ()->getLocallyUnqualifiedSingleStepDesugaredType().getAsString();
      rd_buffer += "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";
    } else if (mode == ANONYMOUS) {
      string identifier = recordDecl->getTypeForDecl ()->getLocallyUnqualifiedSingleStepDesugaredType().getAsString();
      // replace space with underscore 
      size_t found = identifier.find_first_of(" ");
      while (found!=string::npos) {
        identifier[found]='_';
        found=identifier.find_first_of(" ",found+1);
      }
      rd_buffer += "! ANONYMOUS struct may or may not have a declared name\n";
      string temp_buf = "TYPE, BIND(C) :: " + identifier + "\n" + fieldsInFortran + "END TYPE " + identifier +"\n";
      // comment out temp_buf
      std::istringstream in(temp_buf);
      for (std::string line; std::getline(in, line);) {
        rd_buffer += "! " + line + "\n";
      }
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
  } else if ((recordDecl->getNameAsString()).empty() and !tag_name.empty()) {
    mode = TAG_ONLY;
  } else if ((recordDecl->getNameAsString()).empty() and tag_name.empty()) {
    string identifier = recordDecl->getTypeForDecl ()->getLocallyUnqualifiedSingleStepDesugaredType().getAsString();
    if (identifier.find(" ") != string::npos) {
      mode = ANONYMOUS;
    } else {
      // is a identifier
      mode = TYPEDEF;
    }
    
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
    string paramsString = getParamsTypesASString();
    string imports;
    if (!paramsString.empty()) {
      imports = "USE iso_c_binding, only: " + getParamsTypesASString();
    } else {
      imports = "USE iso_c_binding";
    }
    
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
      string commentedBody;
      std::istringstream in(bodyText);
      for (std::string line; std::getline(in, line);) {
        commentedBody += "! " + line + "\n";
      }
      fortanFunctDecl += commentedBody;

    }
    if (returnQType.getTypePtr()->isVoidType()) {
      fortanFunctDecl += "END SUBROUTINE " + funcDecl->getNameAsString() + "\n";   
    } else {
      fortanFunctDecl += "END FUNCTION " + funcDecl->getNameAsString() + "\n";
    }
    
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
    macroDef = Lexer::getSourceText(CharSourceRange::getTokenRange(mi->getDefinitionLoc(), mi->getDefinitionEndLoc()), SM, LangOptions(), 0);
    
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
  // remove tabs from macroVal
  size_t found = macroVal.find_first_of("\t");
  while (found!=std::string::npos)
  {
    macroVal.erase(found, found+1);
    found=macroVal.find_first_of("\t");
  }

  if (!isInSystemHeader) {
    // handle object first
    if (isObjectLike()) {
      // analyze type
      if (!macroVal.empty()) {
        if (CToFTypeFormatter::isString(macroVal)) {
          if (macroName[0] == '_') {
            fortranMacro = "! underscore is invalid character name\n";
            fortranMacro += "!CHARACTER("+ to_string(macroVal.size()-2)+"), parameter, public :: "+ macroName + " = " + macroVal + "\n";
          } else {
            fortranMacro = "CHARACTER("+ to_string(macroVal.size()-2)+"), parameter, public :: "+ macroName + " = " + macroVal + "\n";
          }
        
        } else if (CToFTypeFormatter::isIntLike(macroVal)) {
          // invalid chars
          if (macroVal.find_first_of("UL") != std::string::npos or macroName[0] == '_') {
            fortranMacro = "!INTEGER(C_INT), parameter, public :: "+ macroName + " = " + macroVal + "\n";
          } else if (macroVal.find("x") != std::string::npos) {
            size_t x = macroVal.find_last_of("x");
            string val = macroVal.substr(x+1);
            fortranMacro = "INTEGER(C_INT), parameter, public :: "+ macroName + " = int(z\'" + val + "\')\n";
          } else {
            fortranMacro = "INTEGER(C_INT), parameter, public :: "+ macroName + " = " + macroVal + "\n";
          }

        } else if (CToFTypeFormatter::isDoubleLike(macroVal)) {
          if (macroVal.find_first_of("FUL") != std::string::npos or macroName[0] == '_') {
            fortranMacro = "!REAL(C_DOUBLE), parameter, public :: "+ macroName + " = " + macroVal + "\n";
          } else {
            fortranMacro = "REAL(C_DOUBLE), parameter, public :: "+ macroName + " = " + macroVal + "\n";
          }
          
        } else if (CToFTypeFormatter::isType(macroVal)) {
          // only support int short long char for now
          fortranMacro = CToFTypeFormatter::createFortranType(macroName, macroVal);

        } else {
          std::istringstream in(macroDef);
          for (std::string line; std::getline(in, line);) {
            fortranMacro += "! " + line + "\n";
          }
        }
    } else { // macroVal.empty(), make the object a bool positive
      if (macroName[0] == '_') {
        fortranMacro = "! underscore is invalid character name\n";
        fortranMacro += "!INTEGER(C_INT), parameter, public :: "+ macroName  + " = 1 \n";
      } else {
        fortranMacro = "INTEGER(C_INT), parameter, public :: "+ macroName  + " = 1 \n";
      }
    }


    } else {
        // function macro
      size_t rParen = macroDef.find(')');
      string functionBody = macroDef.substr(rParen+1, macroDef.size()-1);
      if (macroName[0] == '_') {
        fortranMacro = "! underscore is invalid character name.\n";
        fortranMacro += "!INTERFACE\n";
        if (md->getMacroInfo()->arg_empty()) {
          fortranMacro += "!SUBROUTINE "+ macroName + "() bind (C)\n";
        } else {
          fortranMacro += "!SUBROUTINE "+ macroName + "(";
          for (auto it = md->getMacroInfo()->arg_begin (); it != md->getMacroInfo()->arg_end (); it++) {
            fortranMacro += (*it)->getName();
            fortranMacro += ", ";
          }
          // erase the redundant colon
          fortranMacro.erase(fortranMacro.size()-2);
          fortranMacro += ") bind (C)\n";
        }
        if (!functionBody.empty()) {
          std::istringstream in(functionBody);
          for (std::string line; std::getline(in, line);) {
            fortranMacro += "! " + line + "\n";
          }
        }
        fortranMacro += "!END SUBROUTINE " + macroName + "\n";
        fortranMacro += "!END INTERFACE\n";
      } else {
        fortranMacro = "INTERFACE\n";
        if (md->getMacroInfo()->arg_empty()) {
          fortranMacro += "SUBROUTINE "+ macroName + "() bind (C)\n";
        } else {
          fortranMacro += "SUBROUTINE "+ macroName + "(";
          for (auto it = md->getMacroInfo()->arg_begin (); it != md->getMacroInfo()->arg_end (); it++) {
            string arg = (*it)->getName();
            // remove underscore
            size_t found = arg.find_first_of("_");
            while (found!=std::string::npos)
            {
              arg.erase(found, found+1);
              found=arg.find_first_of("_");
            }
            fortranMacro += arg;
            fortranMacro += ", ";
          }
          // erase the redundant colon
          fortranMacro.erase(fortranMacro.size()-2);
          fortranMacro += ") bind (C)\n";
        }
        if (!functionBody.empty()) {
          std::istringstream in(functionBody);
          for (std::string line; std::getline(in, line);) {
            fortranMacro += "! " + line + "\n";
          }
        }
        fortranMacro += "END SUBROUTINE " + macroName + "\n";
        fortranMacro += "END INTERFACE\n";
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
    llvm::outs() << "INTERFACE\n" 
    << fdf.getFortranFunctDeclASString()
    << "END INTERFACE\n";      
  } else if (isa<TypedefDecl> (d)) {
    TypedefDecl *tdd = cast<TypedefDecl> (d);
    TypedefDeclFormater tdf(tdd, TheRewriter);
    outs() << tdf.getFortranTypedefDeclASString();

  } else if (isa<RecordDecl> (d)) {
    RecordDecl *rd = cast<RecordDecl> (d);
    RecordDeclFormatter rdf(rd, TheRewriter);
    outs() << rdf.getFortranStructASString();


  } else if (isa<VarDecl> (d)) {
    VarDecl *varDecl = cast<VarDecl> (d);
    VarDeclFormatter vdf(varDecl, TheRewriter);
    outs() << vdf.getFortranVarDeclASString();

  } else if (isa<EnumDecl> (d)) {
    EnumDeclFormatter edf(cast<EnumDecl> (d), TheRewriter);
    outs() << edf.getFortranEnumASString();
  } else {

    llvm::outs() << "!found other type of declaration \n";
    d->dump();
    RecursiveASTVisitor<TraverseNodeVisitor>::TraverseDecl(d);
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