set(LLVM_LINK_COMPONENTS support)
add_clang_executable(fortran-convert
  FortranConvert.cpp
  )
target_link_libraries(fortran-convert
  clangTooling
  clangBasic
  clangASTMatchers
  clangFrontend
  )

