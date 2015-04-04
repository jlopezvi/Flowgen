#!/usr/bin/env python
import re
import sys
import clang.cindex
import os
import glob
import csv

# colors:  http://www.color-hex.com/color/6699cc#    chosen (#84add6, #b2cce5, #e0eaf4)

##RESTRICTIONS:
# - parallel actions do not work so far.
# - only one nested if-statement inside another one.
# - loops cannot be nested.

#clang node types (CursorKind)
#8: FUNCTION_DECL
#21: CXX_METHOD
#205: IF_STMT  an if statement
#202: COMPOUND_STMT  a compound statement
#212: continue statement.
#213: A break statement.
#214: A return statement.
#207: A while statement.
#208: A do statement.
#209: A for statement.
#103: CALL_EXPR An expression that calls a function or method.
#101: DECL_REF_EXPR An expression that refers to some value declaration, such as a function, varible, or enumerator.  CursorKind.DECL_REF_EXPR = CursorKind(101)

#clang node properties
#node.displayname: more info than .spelling
#node.get_definition(): returns the defining node.
#node.location.line, node.location.column, node.location.filename
#node.get_usr() Return the Unified Symbol Resultion (USR) for the entity referenced by the given cursor (or None).
#A Unified Symbol Resolution (USR) is a string that identifies a particular entity (function, class, variable, etc.) within a program. USRs can be compared across translation units
#node.get_referenced() Return the referenced object of a call
#additional feature for Cursors (nodes) that has to be added to clang python bindings
def get_referenced(self):
    return clang.cindex.conf.lib.clang_getCursorReferenced(self)


clang.cindex.Cursor.get_referenced = get_referenced


#looks for an action comment inside the extent of a given node (write_zoomlevel <= diagram_zoom)
#stops at the lowest zoomlevel
def lookfor_lowestZoomactionAnnotation_inNode(nodeIN, diagram_zoom):
    def regexActionComment(zoom):
        if zoom == 0:
            zoom = ''
        regextextActionComment_zoom = r'^\s*//\$' + str(zoom) + r'(?!\s+\[)\s+(?P<action>.+)$'
        return re.compile(regextextActionComment_zoom)

    infile_str = nodeIN.location.file.name.decode("utf-8")
    infile = open(infile_str, 'r')
    start_line = nodeIN.extent.start.line
    end_line = nodeIN.extent.end.line
    enum_file = list(enumerate(infile, start=1))
    infile.close()

    #loop over zoom levels, first the lowest
    for it_zoom in range(0, diagram_zoom + 1):
        #loop over source code lines
        for i, line in enum_file:
            if i in range(start_line, end_line):
                if regexActionComment(it_zoom).match(line):
                    lookfor_lowestZoomactionAnnotation_inNode.write_zoomlevel = it_zoom
                    return True
    lookfor_lowestZoomactionAnnotation_inNode.write_zoomlevel = None
    return False


#looks up in the database generated at compilation time if a given key (function/method' USR) exists
def read_flowdbs(key):
    for file in glob.glob('flowdoc/aux_files/*.flowdb'):
        reader = csv.reader(open(file, "rt", encoding="utf8"), delimiter='\t')
        for row in reader:
            if key == row[0]:
                temp_file_str = os.path.splitext(os.path.basename(file))[0]
                read_flowdbs.file = temp_file_str
                ##read_flowdbs.zoom=row[1]
                ##read_flowdbs.displayname=row[2]
                #print (infile_str)
                #print('\n\nYEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEES\n\n')
                return True
    #print('\n\nNOOOOOOOOOEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n\n')
    return False


def read_single_flowdb(key, file):
    reader = csv.reader(open(file, "rt", encoding="utf8"), delimiter='\t')
    for row in reader:
        if key == row[0]:
            read_single_flowdb.max_diagram_zoomlevel = int(row[1])
            ##read_flowdbs.displayname=row[2]
            return True
    return False


htmlonline_str = ''
write_htmlonline_firstcall = True
#writes out htmlonline_str adding up all the function/method's strings.
def write_htmlonline(string, outfile_str):
    global htmlonline_str
    global write_htmlonline_firstcall

    if write_htmlonline_firstcall:
        htmlonline_str += """<html><head><script type="text/javascript" src="jquery.js"></script>
     <script type="text/javascript" src="jquery_plantuml.js"></script>
     <!-- rawdeflate.js is implicity used by jquery_plantuml.js --></head>""" + '\n' + """<body><hr>"""
        htmlonline_str += "<p>" + outfile_str + "</p>"
        htmlonline_str += """<img uml=" """ + string
        htmlonline_str += """ "></body></html>"""

        write_htmlonline_firstcall = False
    else:
        htmlonline_str = htmlonline_str[:-14]
        htmlonline_str += "<hr><p>" + outfile_str + "</p>"
        htmlonline_str += """<img uml=" """ + string
        htmlonline_str += """ "></body></html>"""

    return


#writes each diagram separately into a plantuml .txt file
def write_txt(string, outfile_str):
    f = open('flowdoc/aux_files/' + outfile_str + ".txt", "w")
    f.write(string)
    f.close()
    return


#finds calls in a source code line that has been annotated with //$ at the end
#call nodes are only associated to the characters '(' ',' ')' of the source code line 
#there can also be variables whose definition involves a call. These kind of calls are also picked up.
#several different calls in the same line can be identified. 
#TO DO: identify calls inside other calls!
#CXXMemberCallExpr: call to a member method
#CallExpr: call to a function
def find_calls(scan_fileIN, scan_lineIN, scan_column_startIN, scan_column_endIN):
    singlelinecallsdefArrayIN = []

    for it in range(scan_column_startIN, scan_column_endIN):

        loc = clang.cindex.SourceLocation.from_position(tu, scan_fileIN, scan_lineIN, it)
        scan_node = clang.cindex.Cursor.from_location(tu, loc)
        #print('position',it,'scannode ',scan_node.kind.name)
        #call or variable found?
        #DECL_REF_EXPR (101) is an expression that refers to some value declaration, such as a function, variable, or enumerator (use node.get_referenced())
        referencefound = (scan_node.kind.name == 'DECL_REF_EXPR')
        #CALL_EXPR (103) is a call
        callfound = (scan_node.kind.name == 'CALL_EXPR')

        #DECL_REF_EXPR (101) --> we look for its definition (in the same source file!), it should be a VAR_DECL (9) --> we look for its children, there should be a CALL_EXPR (103)
        if referencefound and scan_node.get_definition():
            #print ('reference found')
            if scan_node.get_definition().kind.name == 'VAR_DECL':
                #print ('scannode.getdefinition is VAR_DECL')
                for it9 in scan_node.get_definition().get_children():
                    #print ('scan_node.get_definition().get_children() ',it9.kind.name)
                    if it9.kind.value == 103:
                        if it9.get_definition() not in singlelinecallsdefArrayIN:
                            singlelinecallsdefArrayIN.append(it9.get_definition())

        #CALL_EXPR (103) is a call  --> it is better to use the node_call.get_referenced() but this may not be defined (we throw a WARNING in this case)
        #Note: apparently node_call.get_definition() is not defined.
        elif callfound:
            #print ('call found')
            if scan_node not in singlelinecallsdefArrayIN:
                if scan_node.get_referenced():
                    singlelinecallsdefArrayIN.append(scan_node.get_referenced())
                    #print ('registered call: getreferenced ',scan_node.get_referenced().kind.name, scan_node.get_referenced().displayname, 'USR', scan_node.get_referenced().get_usr())
                    #print ('registered call: getdefinition ',scan_node.get_definition().kind.name, scan_node.get_definition().displayname, 'USR')
                else:
                    print('WARNING ', scan_node.spelling, ": No get_referenced for the cursor")
                    singlelinecallsdefArrayIN.append(scan_node)
                    #print ('registered call:', scan_node.kind.name, scan_node.extent, scan_node.displayname.decode("utf-8"), 'USR', scan_node.get_usr())

    return singlelinecallsdefArrayIN


#finds return statements
#BETTER algorithm if I COULD ACCESS parent node from any node?
#ryan gonzalez idea (not implemented): use node attributes to set parent  node.parent=parent_node
def find_returnstmt(nodeIN, zoom):
    returnlineArray = []
    #two values for type: "True"->return in the flow, "False"-> conditional return inside an action
    returnTypeArray = []

    def find_returnstmtRE(nodeIN2, zoom):
        nonlocal returnlineArray
        nonlocal returnTypeArray
        #214: A return statement.
        if nodeIN2.kind.value == 214:
            #print ('node',nodeIN2.kind)
            returnlineArray.append(nodeIN2.location.line)
            returnTypeArray.append(True)
            #print (returnTypeArray)
            return 1

        else:
            # Recurse for children of this node
            returnValue = None
            for c in nodeIN2.get_children():

                #print ('children', c.kind)
                nextlevelReturn = find_returnstmtRE(c, zoom)
                if nextlevelReturn == 1:
                    #print ('node',nodeIN2.kind)
                    returnValue = 1
                    if nodeIN2.kind.value == 205 and returnTypeArray[-1] == True:
                        if not lookfor_lowestZoomactionAnnotation_inNode(nodeIN2, zoom):
                            returnTypeArray[-1] = False
                            #print (returnTypeArray)
                    continue
            return returnValue

    find_returnstmtRE(nodeIN, zoom)
    #print (returnlineArray)
    #print (returnTypeArray)
    return returnlineArray, returnTypeArray


#finds the first level of if-statements inside a given node and returns three arrays
def find_ifstmt(nodeIN):
    ifbeginlineArrayIN = []
    ifendlineArrayIN = []
    ifnodeArrayIN = []

    def find_ifstmtRE(nodeIN2):
        for d in nodeIN2.get_children():
            if d.kind.value == 205:
                ifbeginlineArrayIN.append(d.extent.start.line)
                ifendlineArrayIN.append(d.extent.end.line)
                ifnodeArrayIN.append(d)
            else:
                find_ifstmtRE(d)

    find_ifstmtRE(nodeIN)
    return ifbeginlineArrayIN, ifendlineArrayIN, ifnodeArrayIN


#finds the then{} else if{} and else{} statements of a given if-statement
#elseifbeginlineArrayIN: array with the else-if begin lines
#elsebeginlineIN: array with the else begin line (if existing)
#ifstructurenodeArrayIN: array with the compound statement nodes of then, else-if, and else (if existing)
#ifstructureelseifnodeArrayIN: array with the else-if nodes
def find_elsestmt(nodeIN):
    elseifbeginlineArrayIN = []
    elsebeginlineIN = None
    ifstructurenodeArrayIN = []
    ifstructureelseifnodeArrayIN = []
    #add then{} node
    for d in nodeIN.get_children():
        if (d.kind.name == 'COMPOUND_STMT'):
            ifstructurenodeArrayIN.append(d)
            break

    def find_elsestmtRE(nodeIN2):
        for e in nodeIN2.get_children():
            if e.kind.name == 'IF_STMT':
                #add elseif() node
                ifstructureelseifnodeArrayIN.append(e)
                find_elsestmtRE.node_lastelseifstmt = e
                elseifbeginlineArrayIN.append(e.extent.start.line)
                #add compound statement corresponding to elseif() node
                for f in e.get_children():
                    if (f.kind.name == 'COMPOUND_STMT'):
                        ifstructurenodeArrayIN.append(f)
                        break
                find_elsestmtRE(e)

    find_elsestmtRE.node_lastelseifstmt = nodeIN
    find_elsestmtRE(nodeIN)

    counter = 0
    for f in find_elsestmtRE.node_lastelseifstmt.get_children():
        if f.kind.name == 'COMPOUND_STMT':
            counter += 1
            if counter == 2:
                elsebeginlineIN = f.extent.start.line
                #add else() node
                ifstructurenodeArrayIN.append(f)

    return elseifbeginlineArrayIN, elsebeginlineIN, ifstructurenodeArrayIN, ifstructureelseifnodeArrayIN

# 207: A while statement.
# 208: A do statement.
# 209: A for statement.
# finds the first level of loop-statements inside a given node and returns three arrays
def find_loopstmt(nodeIN):
    loopbeginlineArrayIN = []
    loopendlineArrayIN = []
    loopnodeArrayIN = []
    looptypeArrayIN = []
    def find_loopstmtRE(nodeIN2):
        for d in nodeIN2.get_children():
            if 207 <= d.kind.value <= 209:
                loopbeginlineArrayIN.append(d.extent.start.line)
                loopendlineArrayIN.append(d.extent.end.line)
                loopnodeArrayIN.append(d)
                looptypeArrayIN.append(d.kind.value)
            else:
                find_loopstmtRE(d)
    find_loopstmtRE(nodeIN)
    return loopbeginlineArrayIN, loopendlineArrayIN, loopnodeArrayIN, looptypeArrayIN


# Main process function.
# The maximum zoom level is already known
def process_find_functions(node, MAX_diagram_zoomlevel):
    # \s --> [ \t\r\f\v] : avoids newlines \n
    # (?! ): negative lookahead
    # ()?: optional group
    regextextActionComment = r'^\s*//\$(?!\s+\[)(\s+(?P<tag><\w+>))?\s+(?P<action>.+)$'
    regextextActionComment1 = r'^\s*//\$1(?!\s+\[)\s+(?P<action>.+)$'
    regextextAnyActionComment1 = r'^\s*//\$1?(?!\s+\[)\s+(?P<action>.+)$'
    regextextAnyActionComment = r'^\s*//\$(?P<zoomlevel>[0-9])?(?!\s+\[)\s+(?P<action>.+)$'
    regexActionComment = re.compile(regextextActionComment)
    regexActionComment1 = re.compile(regextextActionComment1)
    regexAnyActionCommentZoomArray = [regexActionComment, re.compile(regextextAnyActionComment1)]
    #anycomment_previousline = regexAnyActionCommentZoomArray[zoom].match(enum_file[i-1-1][1])
    def regexActionComment(zoom):
        if zoom == 0:
            zoom = ''
        regextextActionComment_zoom = r'^\s*//\$' + str(zoom) + r'(?!\s+\[)\s+(?P<action>.+)$'
        return re.compile(regextextActionComment_zoom)

    regexContextualComment = re.compile(r'^\s*//\$\s+\[(?P<condition>.+)\]\s*$')
    regexHighlightComment = re.compile(r'^\s*(?P<commandline>.+?)\s+//\$\s*(?:$|//.+$)')
    #regexIf = re.compile(r'^\s*if\s*\((?P<condition>.*)\)\s*{?\s*(?:$|//.*$)')
    #regexElseIf = re.compile(r'^\s*}?\s*else if\s*\((?P<condition>.*)\)\s*{\s*(?:$|//.*$)')
    #this only works in a one line
    #regexIf1line = re.compile(r'^\s*if\s*\((?P<condition>.*)\)\s*{\s*(?:$|//.*$)')

    start_line = node.extent.start.line
    end_line = node.extent.end.line
    infile_clang = node.location.file
    global infile_str
    infile_str = node.location.file.name.decode("utf-8")
    infile = open(infile_str, 'r')
    #lines enumerated starting from 1
    enum_file = list(enumerate(infile, start=1))
    infile.close()

    ##look for comment inside function/method
    #comment_inside_method = False
    #if lookfor_lowestZoomactionAnnotation_inNode(node,0):
    #   comment_inside_method = True

    ##if ActionComment inside function/method:
    #if comment_inside_method == True :            
    print('Processing %s of kind %s [start_line=%s, end_line=%s. At "%s"]' % (
        node.spelling.decode("utf-8"), node.kind.name, node.extent.start.line, node.extent.end.line,
        node.location.file))

    # TO DO: zoom loop generates all possible zoom levels. Instead, only relevant zoom for each diagram should be generated.
    zoom_str_Array = ['', '1', '2']
    for diagram_zoomlevel in range(0, MAX_diagram_zoomlevel + 1):

        class_name = ''
        if node.kind.name == 'CXX_METHOD':
            class_name = str(node.semantic_parent.spelling.decode("utf8")) + '_'
            #also see node.lexical_parent.spelling
        outfile_str = str(node.get_usr().decode("utf8")) + zoom_str_Array[diagram_zoomlevel]
        #remove special characters from outfile_str 
        outfile_str = ''.join(e for e in outfile_str if e.isalnum())
        #outfile= open(outfile_str+'.txt', "w+")  

        # find if statements inside the function
        ifbeginlineArray, ifendlineArray, ifnodeArray = find_ifstmt(node)
        # print (ifbeginlineArray, ifendlineArray, ifnodeArray)

        # find loop statements inside the function
        loopbeginlineArray, loopendlineArray, loopnodeArray, looptypeArray = find_loopstmt(node)
        # print (loopbeginlineArray, loopendlineArray, loopnodeArray)

        #variables for conditional statements ('Nested' means nested inside another conditional statement)
        elseifbeginlineArray = []
        elsebeginline = None
        ifstructurenodeArray = []
        ifbeginlineNestedArray = []
        ifendlineNestedArray = []
        ifnodeNestedArray = []
        ifstructureelseifnodeArray = []
        elseifbeginlineNestedArray = []
        elsebeginlineNested = None
        ifstructurenodeNestedArray = []
        ifstructureelseifnodeNestedArray = []
        endifWrite = False
        endifNestedWrite = False
        elseifNum = 0
        elseifNumNested = 0
        IdxIfbeginlineArray = None
        IdxIfbeginlineArrayNested = None
        #write_zoomlevel_beforeifstmt=None
        ifstmt_write_zoomlevel = None
        ifstmtNested_write_zoomlevel = None

        #variables for loop statements
        endloopWrite = False
        IdxLoopbeginlineArray = None
        loopstmt_write_zoomlevel = None
        loopdescription_flag=False

        #find return statements inside the function
        returnlineArray, returnTypeArray = find_returnstmt(node, diagram_zoomlevel)

        #other variables
        #TO DO: use depthlevel
        depthlevel = 0
        #flagparallelactions=(flag TRUE/FALSE,depthlevel)
        #TO DO: change array for another more transparent structure, like an object with attributes
        flagparallelactions = [False, 0]
        lastcommentlinematched = [0, 0, 0]
        tab = '   '
        indentation_level = 0
        last_comment_str = ["", "", ""]
        string_notes = ["", "", ""]
        string = ''
        string_tmp = ["", "", ""]
        inside_comment_flag = [False, False, False]
        actioncallsdefArray = []
        write_zoomlevel = None


        def increase_depthlevel():
            nonlocal depthlevel
            depthlevel += 1
            write_strings(write_zoomlevel)
            return

        def decrease_depthlevel():
            nonlocal flagparallelactions, depthlevel, string, indentation_level
            depthlevel -= 1
            write_strings(write_zoomlevel)
            ##if activated parallelflag
            #if flagparallelactions[0]==True and depthlevel==flagparallelactions[1]:
            #   string+= indentation_level*tab+'end fork\n'
            #   flagparallelactions[0]=False
            #   flagparallelactions[1]=None
            return


        def add_note(stringIN):
            nonlocal string_notes
            string_notes[write_zoomlevel] += stringIN + '\n'
            return

            #taken from http://stackoverflow.com/questions/2657693/insert-a-newline-character-every-64-characters-using-python

        #def insert_newlines(string, every=75):
        #    lines = []
        #    for i in range(0, len(string), every):
        #       lines.append(string[i:i+every])
        #    return '\n'.join(lines)     

        def color(zoomlevel_IN):
            if zoomlevel_IN == 0:
                return '#84add6'
            elif zoomlevel_IN == 1:
                return '#b2cce5'
            elif zoomlevel_IN == 2:
                return '#e0eaf4'


        def write_strings(write_zoomlevelMIN):
            nonlocal string, string_tmp, diagram_zoomlevel
            write_zoomlevelMAX = -100  #initialize variable to absurd value
            #write_zoomlevelMIN: the MIN zoomlevel annotations that will be written. Specified as an entry to the function.
            #write_zoomlevelMAX: the MAX zoomlevel annotations that will be written. Found out inside this function.
            #diagram_zoomlevel: the diagram zoomlevel. write_zoomlevelMAX is lower or equal.

            def write_string_container(write_zoomlevelIN):
                nonlocal string_tmp, last_comment_str, inside_comment_flag

                string_tmp[write_zoomlevelIN] += indentation_level * tab + 'partition ' + color(
                    write_zoomlevelIN) + ' "' + last_comment_str[write_zoomlevelIN] + '" {\n' + string_tmp[
                                                     write_zoomlevelIN + 1] + indentation_level * tab + '}\n'
                last_comment_str[write_zoomlevelIN] = ""
                inside_comment_flag[write_zoomlevelIN] = False
                string_tmp[write_zoomlevelIN + 1] = ""
                return

            def write_string_normal(write_zoomlevelIN):
                nonlocal string_notes
                nonlocal string_tmp
                nonlocal last_comment_str
                nonlocal inside_comment_flag
                nonlocal actioncallsdefArray
                if inside_comment_flag[write_zoomlevelIN]:
                    #write action comment
                    last_comment_str[write_zoomlevelIN] = indentation_level * tab + ':' + color(
                        write_zoomlevelIN) + ':' + last_comment_str[write_zoomlevelIN] + ';\n'
                    #write extra if there are calls
                    if actioncallsdefArray:
                        last_comment_str[write_zoomlevelIN] = last_comment_str[write_zoomlevelIN][:-2] + "\n----"
                        for it7 in actioncallsdefArray:
                            usr_id_str = str(it7.get_usr().decode("utf-8"))
                            usr_id_str = ''.join(e for e in usr_id_str if e.isalnum())
                            classname = ''
                            if it7.kind.name == 'CXX_METHOD':
                                classname = str(it7.semantic_parent.spelling.decode("utf-8")) + '::'
                            if read_flowdbs(it7.get_usr().decode("utf8")):
                                call_in_filename_str = read_flowdbs.file + '.html'
                                last_comment_str[write_zoomlevelIN] += '\n' + str(
                                    it7.result_type.kind.name) + ' ' + classname + str(it7.displayname.decode(
                                    "utf-8")) + ' -- [[' + call_in_filename_str + '#' + usr_id_str + ' link]]'
                            else:
                                last_comment_str[write_zoomlevelIN] += '\n' + str(
                                    it7.result_type.kind.name) + ' ' + classname + str(it7.displayname.decode("utf-8"))

                                #last_comment_str+=str(it7.result_type.kind.name)+' '+str()+str(it7.displayname.decode("utf-8"))+' -- [[http://www.google.es]]'+'\\n'
                        last_comment_str[write_zoomlevelIN] += ';\n'
                    #write extra if there are notes
                    if string_notes[write_zoomlevelIN] != "":
                        last_comment_str[write_zoomlevelIN] += "note right\n" + string_notes[
                            write_zoomlevelIN] + "end note\n"
                        string_notes[write_zoomlevelIN] = ""
                    #write in temporal string
                    string_tmp[write_zoomlevelIN] += last_comment_str[write_zoomlevelIN]
                    last_comment_str[write_zoomlevelIN] = ''
                    #reinitialize flags
                    inside_comment_flag[write_zoomlevelIN] = False
                    actioncallsdefArray = []
                return


            #reverse loop to find write_zoomlevelMAX and call write_string_normal(write_zoomlevelMAX) if necessary
            for zoom_it in range(diagram_zoomlevel, write_zoomlevelMIN - 1, -1):
                #annotation exists at this level and is not written in temporal string yet
                if inside_comment_flag[zoom_it]:
                    write_zoomlevelMAX = zoom_it
                    write_string_normal(write_zoomlevelMAX)
                    break
                #the temporal string exists at this level
                elif string_tmp[zoom_it] != "":
                    write_zoomlevelMAX = zoom_it
                    break

            #reverse loop from ( write_zoomlevelMAX - 1 ) to write_zoomlevelMIN, where write_string_container() is called
            for zoom_it2 in range(write_zoomlevelMAX - 1, write_zoomlevelMIN - 1, -1):
                write_string_container(zoom_it2)


            #if zoomlevelMIN=0 write temporal string to main string
            if write_zoomlevelMIN == 0:
                string += string_tmp[0]
                string_tmp[0] = ''

            return

            ##write last action annotations for current zoom level and all possible higher ones in their corresponding temporal string
            #for zoom_it in range(write_zoomlevelMIN, diagram_zoomlevel+1):
            #   write_string_normal(zoom_it)
            ##write temporal strings of higher level zooms in the current zoomlevel temporal string
            #for zoom_it2 in range(write_zoomlevelMIN+1,diagram_zoomlevel+1):
            #   string_tmp[write_zoomlevelMIN]+=string_tmp[zoom_it2]
            #   string_tmp[zoom_it2]=''


        # Functions for the if statements.
        # TO DO: reuse parent-if-statement functions as nested-if-statement functions

        def ifbeginlineArray_method():
            nonlocal elseifbeginlineArray, elsebeginline, ifstructurenodeArray, ifstructureelseifnodeArray
            nonlocal ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray
            nonlocal string_tmp, indentation_level, depthlevel
            nonlocal endifWrite, IdxIfbeginlineArray, write_zoomlevel, ifstmt_write_zoomlevel
            # look for comment inside if statement
            IdxIfbeginlineArray = ifbeginlineArray.index(i)
            node = ifnodeArray[IdxIfbeginlineArray]
            #if comment inside if statement:
            if lookfor_lowestZoomactionAnnotation_inNode(node, diagram_zoomlevel):
                #adjust zoomlevel
                ifstmt_write_zoomlevel = lookfor_lowestZoomactionAnnotation_inNode.write_zoomlevel
                #write_zoomlevel_beforeifstmt=write_zoomlevel
                write_zoomlevel = ifstmt_write_zoomlevel
                #increase depthlevel
                increase_depthlevel()
                #write 'if' in string
                description = regexContextualComment.match(enum_file[i - 1 - 1][1])
                if description:
                    string_tmp[write_zoomlevel] += '\n' + indentation_level * tab + 'if (' + description.group(
                        'condition') + ') then(yes)''\n'
                else:
                    string_condition = ' '.join(
                        t.spelling.decode("utf-8") for t in list(node.get_children())[0].get_tokens())
                    string_condition = string_condition[:-1]
                    string_tmp[
                        write_zoomlevel] += '\n' + indentation_level * tab + 'if (' + string_condition + ' ?) then(yes)''\n'
                #mark } endif to be written in string
                endifWrite = True
                indentation_level += 1
                #explore substructure: then / else if/ else: elseifbeginlineArray, elsebeginline, ifstructurenodeArray, ifstructureelseifnodeArray
                elseifbeginlineArray, elsebeginline, ifstructurenodeArray, ifstructureelseifnodeArray = find_elsestmt(
                    ifnodeArray[IdxIfbeginlineArray])
                #explore then and update ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray
                ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray = find_ifstmt(ifstructurenodeArray[0])
            return

        def elseifbeginlineArray_method():
            nonlocal ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray
            nonlocal elseifNum, string_tmp, indentation_level, write_zoomlevel
            write_zoomlevel = ifstmt_write_zoomlevel
            decrease_depthlevel()
            increase_depthlevel()
            elseifNum += 1
            node = ifstructureelseifnodeArray[elseifNum - 1]
            #write 'else if' in string
            description = regexContextualComment.match(enum_file[i - 1 - 1][1])
            if description:
                string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'elseif (' + description.group(
                    'condition') + ') then (yes)' + '\n'
            else:
                string_condition = ' '.join(
                    t.spelling.decode("utf-8") for t in list(node.get_children())[0].get_tokens())
                string_condition = string_condition[:-1]
                string_tmp[write_zoomlevel] += (
                                                   indentation_level - 1) * tab + 'elseif (' + string_condition + ' ?) then (yes)' + '\n'
                #explore elseif and update ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray
            ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray = find_ifstmt(
                ifstructurenodeArray[elseifNum])
            return

        def elsebeginline_method():
            nonlocal ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray
            nonlocal string_tmp, indentation_level, write_zoomlevel
            write_zoomlevel = ifstmt_write_zoomlevel
            decrease_depthlevel()
            increase_depthlevel()
            #write 'else' in string
            string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'else(no)' + '\n'
            #explore else and update ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray
            ifbeginlineNestedArray, ifendlineNestedArray, ifnodeNestedArray = find_ifstmt(ifstructurenodeArray[-1])
            return

        def ifendlineArray_method():
            nonlocal string_tmp, indentation_level, depthlevel
            nonlocal endifWrite, elsebeginline, elseifNum, ifstmt_write_zoomlevel, write_zoomlevel
            write_zoomlevel = ifstmt_write_zoomlevel
            decrease_depthlevel()
            #is the else condition explicitly written? Otherwise write now
            if elsebeginline == None:
                string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'else(no)' + '\n'
            #write endif's in string
            string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'endif' + '\n' + '\n'
            indentation_level -= 1

            #reset all variables
            depthlevel -= 1
            endifWrite = False
            elseifNum = 0
            del elseifbeginlineArray[:]
            elsebeginline = None
            ifstmt_write_zoomlevel = None
            #write_zoomlevel=write_zoomlevel_beforeifstmt
            #write_zoomlevel_before_ifstmt=None
            return

        ##
        def ifbeginlineNestedArray_method():
            nonlocal IdxIfbeginlineArrayNested, string_tmp, indentation_level, depthlevel, endifNestedWrite
            nonlocal elseifbeginlineNestedArray, elsebeginlineNested, ifstructurenodeNestedArray, ifstructureelseifnodeNestedArray, ifstmtNested_write_zoomlevel, write_zoomlevel
            #look for comment inside Nested if statement
            IdxIfbeginlineArrayNested = ifbeginlineNestedArray.index(i)
            node = ifnodeArray[IdxIfbeginlineArrayNested]
            #if comment inside if statement:
            if lookfor_lowestZoomactionAnnotation_inNode(node, diagram_zoomlevel):
                #adjust zoomlevel
                ifstmtNested_write_zoomlevel = lookfor_lowestZoomactionAnnotation_inNode.write_zoomlevel
                write_zoomlevel = ifstmtNested_write_zoomlevel
                #increase depthlevel
                increase_depthlevel()
                #write 'if' in string
                description = regexContextualComment.match(enum_file[i - 1 - 1][1])
                if description:
                    string_tmp[write_zoomlevel] += '\n' + indentation_level * tab + 'if (' + description.group(
                        'condition') + ') then(yes)''\n'
                else:
                    string_condition = ' '.join(
                        t.spelling.decode("utf-8") for t in list(node.get_children())[0].get_tokens())
                    string_condition = string_condition[:-1]
                    string_tmp[
                        write_zoomlevel] += '\n' + indentation_level * tab + 'if (' + string_condition + ' ?) then(yes)''\n'
                #mark } Nested endif to be written in string
                endifNestedWrite = True
                indentation_level += 1
                #explore substructure: then / else if/ else: elseifbeginlineNestedArray, elsebeginlineNested, ifstructurenodeNestedArray
                elseifbeginlineNestedArray, elsebeginlineNested, ifstructurenodeNestedArray, ifstructureelseifnodeNestedArray = find_elsestmt(
                    ifnodeNestedArray[IdxIfbeginlineArrayNested])
            return

        def elseifbeginlineNestedArray_method():
            nonlocal string, indentation_level, elseifNumNested, write_zoomlevel
            elseifNumNested += 1
            node = ifstructureelseifnodeNestedArray[elseifNumNested - 1]
            write_zoomlevel = ifstmtNested_write_zoomlevel
            decrease_depthlevel()
            increase_depthlevel()
            #write 'else if' in string
            description = regexContextualComment.match(enum_file[i - 1 - 1][1])
            if description:
                string_tmp[write_zoomlevel] += (
                                                   indentation_level - 1) * tab + 'else(no)' + '\n' + indentation_level * tab + 'if (' + description.group(
                    'condition') + ') then (yes)' + '\n'
            else:
                string_condition = ' '.join(
                    t.spelling.decode("utf-8") for t in list(node.get_children())[0].get_tokens())
                string_condition = string_condition[:-1]
                string_tmp[write_zoomlevel] += (
                                                   indentation_level - 1) * tab + 'else(no)' + '\n' + indentation_level * tab + 'if (' + string_condition + ' ?) then (yes)' + '\n'
            indentation_level += 1
            return

        def elsebeginlineNested_method():
            nonlocal string_tmp, indentation_level, write_zoomlevel
            write_zoomlevel = ifstmtNested_write_zoomlevel
            decrease_depthlevel()
            increase_depthlevel()
            #write 'else' in string
            string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'else(no)' + '\n'
            return

        def ifendlineNestedArray_method():
            nonlocal string_tmp, indentation_level, depthlevel
            nonlocal endifNestedWrite, elsebeginlineNested, elseifNumNested, ifstmtNested_write_zoomlevel, write_zoomlevel
            write_zoomlevel = ifstmtNested_write_zoomlevel
            decrease_depthlevel()
            #is the else condition explicitly written? Otherwise write now
            if elsebeginlineNested == None:
                string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'else(no)' + '\n'
                #write endif's in string
            for n in range(elseifNumNested):
                string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'endif' + '\n'
                indentation_level -= 1
            string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'endif' + '\n' + '\n'
            indentation_level -= 1

            #reset all variables
            depthlevel -= 1
            endifNestedWrite = False
            elseifNumNested = 0
            del elseifbeginlineNestedArray[:]
            elsebeginlineNested = None
            ifstmtNested_write_zoomlevel = None
            return


        # Functions for the loop statements.
        def loopbeginlineArray_method():
            nonlocal string_tmp, indentation_level, depthlevel
            nonlocal endloopWrite, IdxLoopbeginlineArray, write_zoomlevel, loopstmt_write_zoomlevel, loopdescription_flag
            IdxLoopbeginlineArray = loopbeginlineArray.index(i)
            node = loopnodeArray[IdxLoopbeginlineArray]
            # if comment inside loop statement with the adequate zoom level:
            if lookfor_lowestZoomactionAnnotation_inNode(node, diagram_zoomlevel):
                # adjust zoomlevels and depthlevel
                loopstmt_write_zoomlevel = lookfor_lowestZoomactionAnnotation_inNode.write_zoomlevel
                write_zoomlevel = loopstmt_write_zoomlevel
                increase_depthlevel()
                # write 'loop' in string
                description = regexContextualComment.match(enum_file[i - 1 - 1][1])
                if description:
                    string_tmp[write_zoomlevel] += '\n' + indentation_level * tab + 'while (' + description.group(
                        'condition') + ')''\n'
                    loopdescription_flag=True
                else:
                    # depends on the loop type
                    # 207: A while statement.
                    if looptypeArray[IdxLoopbeginlineArray] == 207:
                        string_condition = ' '.join(
                            t.spelling.decode("utf-8") for t in list(node.get_children())[0].get_tokens())[:-1]
                        string_tmp[
                            write_zoomlevel] += '\n' + indentation_level * tab + 'while (' + string_condition + '? )''\n'
                    # 208: A do statement.
                    elif looptypeArray[IdxLoopbeginlineArray] == 208:
                        string_tmp[write_zoomlevel] += '\n' + indentation_level * tab + 'repeat''\n'
                    # 209: A for statement.
                    elif looptypeArray[IdxLoopbeginlineArray] == 209:
                        #the '0','1','2' children of the node contain the spellings of the three elements of the FOR loop. 
                        #We have to call the command get_tokens, which produces an iterator over all tokens and then join them into the same string.
                        #However, for the '0' and '2' children, we don't want the last token. We have first to convert the iterator into a list and then use [:-1]
                        string_condition = 'FOR ('+' '.join(
                            t.spelling.decode("utf-8") for t in list(list(node.get_children())[0].get_tokens())[:-1])+' '+' '.join(
                            t.spelling.decode("utf-8") for t in list(node.get_children())[1].get_tokens())+' '+' '.join(
                            t.spelling.decode("utf-8") for t in list(list(node.get_children())[2].get_tokens())[:-1])+' )'
                        string_tmp[
                            write_zoomlevel] += '\n' + indentation_level * tab + 'while (' + string_condition + ')''\n'
                # mark } endloop to be written in string
                endloopWrite = True
                indentation_level += 1
            return

        def loopendlineArray_method():
            nonlocal string_tmp, indentation_level, depthlevel, IdxLoopbeginlineArray
            nonlocal endloopWrite, loopstmt_write_zoomlevel, write_zoomlevel, loopdescription_flag
            write_zoomlevel = loopstmt_write_zoomlevel
            decrease_depthlevel()
            #write 'loop end' in string; it depends on the loop type
            # 207: A while statement.
            if loopdescription_flag:
                string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'endwhile' + '\n' + '\n'
            else:
                if looptypeArray[IdxLoopbeginlineArray]==207:
                    string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'endwhile' + '\n' + '\n'
                # 208: A do statement.
                elif looptypeArray[IdxLoopbeginlineArray]==208:
                    pass
                    node = loopnodeArray[IdxLoopbeginlineArray]
                    string_condition = ' '.join(
                            t.spelling.decode("utf-8") for t in list(node.get_children())[1].get_tokens())[:-1]
                    string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'repeat while ('+ string_condition+ '? )''\n' + '\n'
                # 209: A for statement.
                elif looptypeArray[IdxLoopbeginlineArray]==209:
                    string_tmp[write_zoomlevel] += (indentation_level - 1) * tab + 'endwhile' + '\n' + '\n'
            indentation_level -= 1
            #reset all variables
            depthlevel -= 1
            endloopWrite = False
            loopstmt_write_zoomlevel = None
            loopdescription_flag=False
            return


        string += '@startuml\n\nstart\n skinparam activityBackgroundColor #white \n'

        #main loop over source code lines
        #TO DO: optimization
        for i, line in enum_file:
            if i in range(start_line, end_line):

                #look for an annotated action and set zoomlevel if found
                for zoom_it2 in range(0, diagram_zoomlevel + 1):
                    anyactionannotation = regexActionComment(zoom_it2).match(line)
                    if anyactionannotation:
                        write_zoomlevel = zoom_it2
                        break
                #look for highlight annotation
                comment_highlight = regexHighlightComment.match(line)
                #actions
                if anyactionannotation:
                    #this line continues a previous multi-line action annotation
                    if lastcommentlinematched[write_zoomlevel] == i - 1:
                        last_comment_str[write_zoomlevel] += '\\n' + anyactionannotation.group('action')
                    #first line of action annotation
                    else:
                        write_strings(write_zoomlevel)
                        #new comment at the given zoom level
                        inside_comment_flag[write_zoomlevel] = True
                        ##if <parallel>
                        ##TO DO combine parallel and if statements. paralell inside parallel
                        #if comment.group('tag'):
                        #   if comment.group('tag')=="<parallel>":
                        #      #if begin of parallel actions:
                        #      if flagparallelactions[0]==False:
                        #         string+= indentation_level*tab+'fork\n'
                        #         flagparallelactions[0]=True
                        #         flagparallelactions[1]=depthlevel
                        #      #else
                        #      else:
                        #         if depthlevel==flagparallelactions[1]:
                        #            string+= indentation_level*tab+'fork again\n'
                        ##if not <parallel> but activated parallelflag
                        #else:
                        #   if flagparallelactions[0]==True and depthlevel==flagparallelactions[1]:
                        #      string+= indentation_level*tab+'end fork\n'
                        #      flagparallelactions[0]=False
                        #      flagparallelactions[1]=None
                        #add line to current action annotation
                        last_comment_str[write_zoomlevel] += anyactionannotation.group('action')

                    lastcommentlinematched[write_zoomlevel] = i

                else:

                    #  calls,...
                    if comment_highlight:
                        scan_column_start = 1 + comment_highlight.start('commandline')
                        #the end character is -1. There is an offset of +1 with respect to the file
                        scan_column_end = 1 + comment_highlight.end('commandline') - 1
                        scan_file = infile_clang
                        scan_line = i
                        print('LOOKING FOR CALLS AT: ', scan_file, scan_line, scan_column_start, scan_column_end)
                        singlelinecallsdefArray = find_calls(scan_file, scan_line, scan_column_start, scan_column_end)
                        #for it4 in singlelinecallsdefArray:
                        #print ('singlelinecallsdefArray',it4.displayname.decode("utf-8"))
                        for it5 in singlelinecallsdefArray:
                            if it5 not in actioncallsdefArray:
                                actioncallsdefArray.append(it5)

                    #### ...,OR if statements,...
                    elif i in ifbeginlineArray:
                        ifbeginlineArray_method()
                    #if i in elseifbeginlineArray
                    elif i in elseifbeginlineArray:
                        elseifbeginlineArray_method()
                        #if i in elsebeginline
                    elif i == elsebeginline:
                        elsebeginline_method()
                    #if i is ifendlineArray[IdxIfbeginlineArray] and } is marked to be written in string:
                    elif endifWrite and (i == ifendlineArray[IdxIfbeginlineArray]):
                        ifendlineArray_method()
                        #### Nested if statements
                    elif i in ifbeginlineNestedArray:
                        ifbeginlineNestedArray_method()
                        #if i in elseifbeginlineNestedArray
                    elif i in elseifbeginlineNestedArray:
                        elseifbeginlineNestedArray_method()
                        #if i in elsebeginlineNested
                    elif i == elsebeginlineNested:
                        elsebeginlineNested_method()
                    #if i is ifendlineNestedArray[IdxIfbeginlineArrayNested] and } is marked to be written in string:
                    elif endifNestedWrite and (i == ifendlineNestedArray[IdxIfbeginlineArrayNested]):
                        ifendlineNestedArray_method()

                    #### ...,OR loops,...
                    elif i in loopbeginlineArray:
                        loopbeginlineArray_method()
                    # if i is loopendlineArray[IdxLoopbeginlineArray] and } is marked to be written in string:
                    elif endloopWrite and (i == loopendlineArray[IdxLoopbeginlineArray]):
                        loopendlineArray_method()

                    # ...,OR return statements):
                    elif i in returnlineArray:
                        #print('RETURN:',i,line)
                        if returnTypeArray[returnlineArray.index(i)] == True:
                            #if pending flags, finish them
                            #write_zoomlevel=0
                            #print('write_zoomlevel',write_zoomlevel)
                            #decrease_depthlevel()
                            #print('write_zoomlevel2',write_zoomlevel)
                            write_strings(write_zoomlevel)
                            string_tmp[write_zoomlevel] += "\nstop\n"
                        if returnTypeArray[returnlineArray.index(i)] == False:
                            #print('possible stop', i, line)
                            add_note("possible STOP")

        write_strings(0)
        string += '\n@enduml'
        #print (string)

        write_htmlonline(string, outfile_str)
        write_txt(string, outfile_str)

    return


#finds the functions to process. TO DO: It should be updated after build_db.py and method lookfor_lowestZoomactionAnnotation_inNode(nodeIN,zoom) have been included
def find_functions(node):
    global relevant_folder
    if node.kind.is_declaration():
        if node.kind.name == 'CXX_METHOD' or node.kind.name == 'FUNCTION_DECL':
            if os.path.dirname(node.location.file.name.decode("utf8")) == relevant_folder:
                #is it in database?
                keyIN = node.get_usr().decode("utf8")
                fileIN = 'flowdoc/aux_files/' + \
                         os.path.splitext(os.path.basename(node.location.file.name.decode("utf8")))[0] + '.flowdb'
                if read_single_flowdb(keyIN, fileIN):
                    process_find_functions(node, read_single_flowdb.max_diagram_zoomlevel)
                    return

    # Recurse for children of this node
    for c in node.get_children():
        #print ('children', c.kind)
        find_functions(c)

#### main program

index = clang.cindex.Index.create()
# possible arguments to invoke index.parse:
# "-stdlib=libc++", to use libc++
# "-v" to et information on the include paths
# some arguments are hard-coded...
args = ["-c", "-x", "c++", "-Wall", "-ansi"]
# ...others arguments are taken from the command line invocation
if len(sys.argv) >= 2:
    args += sys.argv[2:]
# In 'cindex.py': def parse(self, path, args=None, unsaved_files=None, options = 0)
tu = index.parse(sys.argv[1], args)
print('Translation unit:', tu.spelling.decode("utf-8"))
relevant_folder = os.path.dirname(tu.spelling.decode("utf-8"))
for diagnostic in tu.diagnostics:
    print(diagnostic)
#global variable for the name of the input file. It will be defined later on.
infile_str = ''
find_functions(tu.cursor)