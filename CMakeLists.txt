set(LLVM_LINK_COMPONENTS support)

add_clang_executable(node-inspect
  NodeInspect.cpp
  )
target_link_libraries(node-inspect
  clangTooling
  clangBasic
  clangASTMatchers
  clangFrontend
  )