INSTALLATION INSTRUCTIONS:

Flowgen software requirements:

• LLVM-Clang 3.4 (or superior) + Python3 bindings 
http://clang.llvm.org/get_started.html 
https://github.com/kennytm/clang-cindex-python3
Check that these two environment variables are set up correctly after clang installation (they are needed in order to use clang bindings to python):
$PYTHONPATH
$LD_LIBRARY_PATH

• Python3 
http://www.python.org/getit/

• PlantUML (this is already provided inside the directory flowdoc/, NO need to install)
http://plantuml.sourceforge.net/


Running the program:
The sample C++ code is already configured with a makefile.
The user only has to type
> make flowdoc
in order to generate the documentation as .html files inside flowdoc/

The //$ comments can be changed in the test C++ code.

