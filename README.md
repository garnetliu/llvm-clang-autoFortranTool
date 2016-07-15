# llvm-clang-autoFortranTool

Tool Building Instruction

Tool name: h2m (header to module)

Reference: http://clang.llvm.org/docs/LibASTMatchersTutorial.html


STEP0: Installation: Obtaining Clang

As Clang is part of the LLVM project, you’ll need to download LLVM’s source code first. Both Clang and LLVM are maintained as Subversion repositories, but we’ll be accessing them through the git mirror. For further information, see the getting started guide.

mkdir ~/clang-llvm && cd ~/clang-llvm git clone 
http://llvm.org/git/llvm.git cd llvm/tools git clone 
http://llvm.org/git/clang.git cd clang/tools git clone 
http://llvm.org/git/clang-tools-extra.git extra

Next you need to obtain the CMake build system and Ninja build tool. You may already have CMake installed, but current binary versions of CMake aren’t built with Ninja support.

cd ~/clang-llvm 
git clone https://github.com/martine/ninja.git 
cd ninja git checkout release 
./bootstrap.py 
sudo cp ninja /usr/bin/  
cd ~/clang-llvm 
git clone git://cmake.org/stage/cmake.git 
cd cmake git checkout next 
./bootstrap 
make 
sudo make install

Okay. Now we’ll build Clang!

cd ~/clang-llvm 
mkdir build && cd build 
cmake -G Ninja ../llvm -DLLVM_BUILD_TESTS=ON  # Enable tests; default is off. 
ninja 
ninja check       # Test LLVM only. 
ninja clang-test  # Test Clang only. 
ninja install

Finally, we want to set Clang as its own compiler.

cd ~/clang-llvm/build 
ccmake ../llvm

The second command will bring up a GUI for configuring Clang. You need to set the entry for CMAKE_CXX_COMPILER. 
Press 't' to turn on advanced mode. Scroll down to CMAKE_CXX_COMPILER, and set it to /usr/bin/clang++, or wherever you installed it. Press 'c' to configure, then 'g' to generate CMake’s files.
Finally, run ninja one last time, and you’re done.

ninja 


STEP1: Create ClangTool
1.	Create directory for the tool

cd ~/clang-llvm/llvm/tools/clang
mkdir tools/extra/h2m

2.	Tell cmake it exists (in the  tools/extra directory)

echo 'add_subdirectory(h2m)' >> tools/extra/CMakeLists.txt


3.	Configure cmakelists.ext in the tools/extra/h2m directory

vim tools/extra/h2m/CMakeLists.txt

set(LLVM_LINK_COMPONENTS support)
add_clang_executable(h2m
  H2m.cpp
  )
target_link_libraries(h2m
  clangTooling
  clangBasic
  clangASTMatchers
  clangFrontend
  )

4.	Add the tool files to the directory: 

tools/extra/h2m/h2m.h
tools/extra/h2m/h2m.cpp

5.	Compile the tool

cd ~/clang-llvm/build

ninja

6.	Test the tool

~/clang-llvm/build/bin/h2m /usr/include/pthread.h –-
Note the two dashes after we specify the source file. The additional options for the compiler are passed after the dashes rather than loading them from a compilation database - there just aren’t any options needed right now.

STEP2: Recompile ClangTool
1. Modify h2m file

tools/extra/h2m/h2m.h
tools/extra/h2m/h2m.cpp

2. Compile the tool

cd ~/clang-llvm/build

ninja

