SOFTWARE PREREQUISITES:

Flowgen software requirements:

• Python3 
http://www.python.org/getit/

• PlantUML (it is already provided, NO need to install)
http://plantuml.sourceforge.net/

• LLVM-Clang 3.4 (or superior) from the SVN repository and change Python2 bindings to Python3 bindings.

Follow the instructions on 
http://clang.llvm.org/get_started.html 
in order to
- checkout LLVM
- checkout clang
Then use:
> export CC=you_favorite_C_compiler
> export CXX=your_favorite_C++_compiler
before configuring with the --enable-optimized flag:
> configure --enable-optimized
> make

Then overwrite the python2 binding libraries with the python3 version via simple copy-paste 
https://github.com/kennytm/clang-cindex-python3
in the corresponding directory
user_path_to_llvm/llvm/tools/clang/bindings/python

If not set already, set the environment variables: 
$PYTHONPATH=user_path_to_llvm/llvm/tools/clang/bindings/python/
(the previous path points to the sources of llvm)
$LD_LIBRARY_PATH=user_path_to_llvm/build/Release+Asserts/lib
(the previous path points to the built llvm system. To get the folder Release+Asserts one does need to use the --enable-optimized flag when configuring)


===========
RUNNING THE PROGRAM

There is an example in the directory EXAMPLE, with some C++ code. The makefile is already configured to run Flowgen simply by typing
> make flowdoc
in order to generate the documentation as .html files inside flowdoc/

The //$ comments can be changed in the test C++ code.

