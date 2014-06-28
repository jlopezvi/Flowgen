#!/usr/bin/env python
import re
import sys
import clang.cindex
import os
import filecmp

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


#looks for an action comment inside the extent of a given node (write_zoomlevel <= diagram_zoom)
def lookfor_ActionComment_in_node(nodeIN,diagram_zoom):
    
    def regexActionComment(zoom):
       if zoom==0:
          zoom=''  
       regextextActionComment_zoom=r'^\s*//\$'+str(zoom)+r'(?!\s+\[)\s+(?P<action>.+)$'
       return re.compile(regextextActionComment_zoom)       

    infile_str=nodeIN.location.file.name.decode("utf-8")
    infile= open(infile_str,'r')            
    start_line=nodeIN.extent.start.line
    end_line=nodeIN.extent.end.line
    enum_file=list(enumerate(infile,start=1))      
    infile.close()
    
    #loop over zoom levels, first the lowest
    for it_zoom in range(0,diagram_zoom+1):
       #loop over source code lines
       for i, line in enum_file:
          if i in range(start_line,end_line):
             if regexActionComment(it_zoom).match(line):
                 lookfor_ActionComment_in_node.write_zoomlevel=it_zoom
                 return True   
    lookfor_ActionComment_in_node.write_zoomlevel=None
    return False
    
#looks for an annotated action comment inside the extent of a given node (zoom level modifies the type of action comment)
def lookfor_actionAnnotation_inNode(nodeIN,zoom):

    if zoom==0:
      zoom=''  
    regextextActionComment=r'^\s*//\$'+str(zoom)+r'(?!\s+\[)\s+(?P<action>.+)$'
    regexToUse=re.compile(regextextActionComment)
    
    ##action level 0
    #regextextActionComment=    r'^\s*//\$(?!\s+\[)\s+(?P<action>.+)$'
    ##action level 1
    #regextextActionComment1=   r'^\s*//\$1(?!\s+\[)\s+(?P<action>.+)$'
    ##action level 0 and 1
    ##regextextAnyActionComment1=r'^\s*//\$1?(?!\s+\[)\s+(?P<action>.+)$'

          
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
  
  global writefunc, relevant_folder
  if node.kind.is_declaration():
     #8 is a function and 21 is c++ class method
    if node.kind.value== 8 or node.kind.value==21:
       if os.path.dirname(node.location.file.name.decode("utf8")) == relevant_folder:
         #print(node.location.file.name.decode("utf8"))         
         if lookfor_actionAnnotation_inNode(node,0):
            zoom_str='0'
            if lookfor_actionAnnotation_inNode(node,1):
               zoom_str='1'  
               if lookfor_actionAnnotation_inNode(node,2):
                  zoom_str='2'
            classname = ''
            if node.kind.name=='CXX_METHOD':
               classname= str(node.semantic_parent.spelling.decode("utf-8"))+'::'
            print('Found annotated method/function:', classname+node.displayname.decode("utf8"))
            writefunc.write(node.get_usr().decode("utf8")+'\t'+zoom_str+'\t'+str(node.result_type.kind.name)+' '+classname+node.displayname.decode("utf8")+'\n')
       return

  # Recurse for children of this node
  for c in node.get_children():
      #print ('children', c.kind)
      find_functions(c)

#### main program

index = clang.cindex.Index.create()
args=["-Wall","-ansi"]
if len(sys.argv)>=2:
   args+=sys.argv[2:]
tu = index.parse(sys.argv[1],args)
print ('Translation unit:', tu.spelling.decode("utf-8"))
relevant_folder=os.path.dirname(tu.spelling.decode("utf-8"))
for diagnostic in tu.diagnostics:
  print(diagnostic)
infile_str=os.path.splitext(os.path.basename(sys.argv[1]))[0]
#print (infile_str)
if not os.path.exists('flowdoc/aux_files/'):
   print('CREATING FOLDER flowdoc/aux_files/')
   os.makedirs('flowdoc/aux_files/')
writefunc = open('flowdoc/aux_files/'+infile_str+'.flowdb',"w")
find_functions(tu.cursor)
writefunc.close()

