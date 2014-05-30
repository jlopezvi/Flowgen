#!/usr/bin/env python
import re
import sys
import clang.cindex
import os

#clang node types (CursorKind)
#21: CXX_METHOD
#205: if statement
#202: compound statement
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


#looks for an annotated action comment inside the extent of a given node (zoom level modifies the type of action comment)
def lookfor_ActionComment_in_node(nodeIN,zoom):
    #action level 0
    regextextActionComment=    r'^\s*//\$(?!\s+\[)\s+(?P<action>.+)$'
    #action level 1
    regextextActionComment1=   r'^\s*//\$1(?!\s+\[)\s+(?P<action>.+)$'
    #action level 0 and 1
    #regextextAnyActionComment1=r'^\s*//\$1?(?!\s+\[)\s+(?P<action>.+)$'
    
    #regex selection according to zoom
    if zoom==0:
      regexToUse=re.compile(regextextActionComment)
    elif zoom==1:
      regexToUse=re.compile(regextextActionComment1)
          
    infile_str=nodeIN.location.file.name.decode("utf-8")
    infile= open(infile_str,'r')            
    start_line=nodeIN.extent.start.line
    end_line=nodeIN.extent.end.line
    enum_file=list(enumerate(infile,start=1))      
    infile.close()
   
    #loop over source code lines
    for i, line in enum_file:
      if i in range(start_line,end_line):
         if regexToUse.match(line):
             return True   
    return False 



def find_functions(node):

  global writefunc
  if node.kind.is_declaration():
     #8 is a function and 21 is c++ class method
    if node.kind.value== 8 or node.kind.value==21:
       #if os.path.dirname(str(node.location.file)) == './src':
         if lookfor_ActionComment_in_node(node,0):
            zoom_str='0'
            if lookfor_ActionComment_in_node(node,1):
               zoom_str='1'  
            classname = ''
            if node.kind.name=='CXX_METHOD':
               classname= str(node.semantic_parent.spelling.decode("utf-8"))+'::'
            writefunc.write(node.get_usr().decode("utf8")+'\t'+zoom_str+'\t'+str(node.result_type.kind.name)+' '+classname+node.displayname.decode("utf8")+'\n')
       #return

  # Recurse for children of this node
  for c in node.get_children():
      #print ('children', c.kind)
      find_functions(c)

#### main program

index = clang.cindex.Index.create()
args=["-Wall","-ansi","-I./include","-I../include"]
tu = index.parse(sys.argv[1],args)
print ('Translation unit:', tu.spelling.decode("utf-8"))
infile_str=os.path.splitext(os.path.basename(sys.argv[1]))[0]
print (infile_str)
writefunc = open('flowdoc/aux_files/'+infile_str+'.flowdb',"w")
find_functions(tu.cursor)
writefunc.close()