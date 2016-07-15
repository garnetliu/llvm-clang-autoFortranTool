set(LLVM_LINK_COMPONENTS support)
add_clang_executable(h2m
  h2m.cpp
  )
target_link_libraries(h2m
  clangTooling
  clangBasic
  clangASTMatchers
  clangFrontend
  )

