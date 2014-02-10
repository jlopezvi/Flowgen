#----------------------------------------------------------------------
# === UPDATE FLOWGEN DOCUMENTATION ===
#----------------------------------------------------------------------

CCSOURCEFILES := $(shell ls src/*.cpp)
SOURCEFLOWDOCS_FROMCCSOURCEFILES := $(shell ls src/*.cpp | sed s/"src\/"/"flowdoc\/"/)
FLOWDOCS:= $(SOURCEFLOWDOCS_FROMCCSOURCEFILES:.cpp=.html) flowdoc/simple_demo_src.html
FLOWDBS:= $(SOURCEFLOWDOCS_FROMCCSOURCEFILES:.cpp=.flowdb) flowdoc/simple_demo_src.flowdb

#$(TMPDIR)/%.txt : src/%.cpp
#	@echo "cc-to-txt: processing $^ to make $@"
#	@echo execute src-to-txt-converter
#	
#flowdoc/%.html : $(TMPDIR)/%.txt
#	@echo "txt-to-html: processing $^ to make $@ (remember to also make pngs)"
#	@echo execute txt-to-html-converter
    
#flowdoc/simple_demo_src.html : simple_demo_src.cpp test.py
#	@echo "a"

all: flowdoc

flowdoc/%.flowdb : src/%.cpp simple_demo_src.cpp pretest.py
	@echo "cpp-to-flowdb: preprocessing"
	python3 pretest.py simple_demo_src.cpp
	python3 pretest.py $^
		
flowdoc/.runphase: $(CCSOURCEFILES) simple_demo_src.cpp test.py
	@echo "cpp-to-graphs: depends on $^"
	python3 test.py simple_demo_src.cpp
	python3 test.py $^
	cd flowdoc && java -jar plantuml.jar *.txt
	cat <<EOF > flowdoc/.runphase
	
flowdoc/%.html : flowdoc/%.flowdb posttest.py flowdoc/.runphase
	@echo "to-html: processing $^ to make $@"
	python3 posttest.py $^
			
flowdoc: simple_demo_src.cpp src/*.cpp $(FLOWDBS) $(FLOWDOCS) Makefile flowdoc/.runphase 
	@echo "Hopla! Finished flowdoc creation. Check flowdocs."
	
