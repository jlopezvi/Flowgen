	echo "cpp-to-flowdb: preprocessing"
	python %FLOWGEN_DIR%build_db.py simple_demo_src.cpp -I./include
	IF %ERRORLEVEL% NEQ 0 GOTO Err1
	python %FLOWGEN_DIR%build_db.py src/source1.cpp -I./include
	IF %ERRORLEVEL% NEQ 0 GOTO Err1
		
	echo "cpp-to-graphs:"
	python %FLOWGEN_DIR%makeflows.py simple_demo_src.cpp -I./include
	IF %ERRORLEVEL% NEQ 0 GOTO Err2
	python %FLOWGEN_DIR%makeflows.py src/source1.cpp -I./include
	IF %ERRORLEVEL% NEQ 0 GOTO Err2
	java -jar %FLOWGEN_DIR%plantuml.jar flowdoc/aux_files/*.txt
	IF %ERRORLEVEL% NEQ 0 GOTO Err2
	
	echo "to-html: processing"
	python %FLOWGEN_DIR%makehtml.py simple_demo_src.cpp
	IF %ERRORLEVEL% NEQ 0 GOTO Err3
	python %FLOWGEN_DIR%makehtml.py src/source1.cpp
	IF %ERRORLEVEL% NEQ 0 GOTO Err3
			
	echo "Hopla! Finished flowdoc creation. Check flowdocs."
	goto END

:Err1
	echo "Preprocessing error"
	goto END

:Err2
	echo "Runphase error"
	goto END

:Err3
	echo "To-html processing error"

:END