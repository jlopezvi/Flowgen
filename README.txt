FLOWGEN SOFTWARE PREREQUISITES:

• Python3 
http://www.python.org/getit/

• PlantUML (it is already provided, NO need to install)
http://plantuml.sourceforge.net/

• LLVM-Clang 3.4 (or superior) from the SVN repository and change Python2 bindings to Python3 bindings.


[FOR MAC]

Follow the instructions on 
http://clang.llvm.org/get_started.html 
in order to
- checkout LLVM
- checkout clang
Then (optionally) use, before configuring:
> export CC=you_favorite_C_compiler
> export CXX=your_favorite_C++_compiler
[ Explanation: Clang needs to be compiled with a external compiler, and in this way the user has control over the compiler that will be used. ]
Now configure the package with the --enable-optimized flag:
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


[FOR WINDOWS]
...


===========
RUNNING FLOWGEN

There is an example in the directory EXAMPLE, with some C++ code. The makefile is configured to either compile the program, by typing
> make a.out
or to run Flowgen and generate the documentation, by typing
> make flowdoc
The documentation is generated as .html files inside flowdoc/

Note: type ‘make’ to do both actions at the same time.
Note: it may be necessary to adjust the variables FLOWGEN_DIR and CXX to run the makefile.

The //$ annotations and the code can be changed in the test C++ code to experiment with Flowgen.

