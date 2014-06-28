#----------------------------------------------------------------------
# === UPDATE FLOWGEN DOCUMENTATION ===
#----------------------------------------------------------------------

#default compiler (to compile the program, Flowgen ignores this: it does always use Clang via the Python bindings)
CXX=g++
#Flowgen folder
FLOWGEN_DIR=../

CCSOURCEFILES := $(shell ls src/*.cpp)
FLOWHTML_FROMCCSOURCEFILES := $(shell ls src/*.cpp | sed s/"src\/"/"flowdoc\/"/)
FLOWDBS_FROMCCSOURCEFILES := $(shell ls src/*.cpp | sed s/"src\/"/"flowdoc\/aux_files\/"/)
FLOWHTML:= $(FLOWHTML_FROMCCSOURCEFILES:.cpp=.html) flowdoc/simple_demo_src.html
FLOWDBS:= $(FLOWDBS_FROMCCSOURCEFILES:.cpp=.flowdb) flowdoc/aux_files/simple_demo_src.flowdb

#$(TMPDIR)/%.txt : src/%.cpp
#	@echo "cc-to-txt: processing $^ to make $@"
#	@echo execute src-to-txt-converter
#	
#flowdoc/%.html : $(TMPDIR)/%.txt
#	@echo "txt-to-html: processing $^ to make $@ (remember to also make pngs)"
#	@echo execute txt-to-html-converter
    
#flowdoc/simple_demo_src.html : simple_demo_src.cpp makeflows.py
#	@echo "a"

all: flowdoc a.out

#compiles the sample program	
a.out: simple_demo_src.cpp src/source1.cpp Makefile
	$(CXX) simple_demo_src.cpp src/source1.cpp -I./include

#generates the documentation			
flowdoc: simple_demo_src.cpp src/*.cpp $(FLOWDBS) $(FLOWHTML) Makefile flowdoc/aux_files/runphase 
	@echo "Hopla! Finished flowdoc creation. Check flowdocs."
	
flowdoc/aux_files/%.flowdb : src/%.cpp simple_demo_src.cpp $(FLOWGEN_DIR)build_db.py
	@echo "cpp-to-flowdb: preprocessing"
	python3 $(FLOWGEN_DIR)build_db.py simple_demo_src.cpp -I./include
	python3 $(FLOWGEN_DIR)build_db.py src/source1.cpp -I./include
		
flowdoc/aux_files/runphase: $(CCSOURCEFILES) simple_demo_src.cpp $(FLOWGEN_DIR)makeflows.py
	@echo "cpp-to-graphs: depends on $^"
	python3 $(FLOWGEN_DIR)makeflows.py simple_demo_src.cpp -I./include
	python3 $(FLOWGEN_DIR)makeflows.py src/source1.cpp -I./include
	java -jar $(FLOWGEN_DIR)plantuml.jar flowdoc/aux_files/*.txt
	cat <<EOF > flowdoc/aux_files/runphase
	
flowdoc/%.html : flowdoc/aux_files/%.flowdb $(FLOWGEN_DIR)makehtml.py flowdoc/aux_files/runphase
	@echo "to-html: processing $^ to make $@"
	python3 $(FLOWGEN_DIR)makehtml.py simple_demo_src.cpp
	python3 $(FLOWGEN_DIR)makehtml.py src/source1.cpp

