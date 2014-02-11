#!/usr/bin/env python
import re
import sys
import clang.cindex
import os
import glob
import csv

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


#looks for an action comment inside the extent of a given node (zoom level modifies the type of action comment)
def lookfor_ActionComment_in_node(nodeIN,zoom):
    #action level 0
    regextextActionComment=    r'^\s*//\$(?!\s+\[)\s+(?P<action>.+)$'
    #action level 1
    regextextActionComment1=   r'^\s*//\$1(?!\s+\[)\s+(?P<action>.+)$'
    #action level 0 and 1
    regextextAnyActionComment1=r'^\s*//\$1?(?!\s+\[)\s+(?P<action>.+)$'
    
    #regex selection according to zoom
    if zoom==0:
      regexToUse=re.compile(regextextActionComment)
    elif zoom==1:
      regexToUse=re.compile(regextextAnyActionComment1)
          
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

#looks up in the database generated at compilation time if a given key (function/method' USR) exists
def read_flowdbs(key):
    for file in glob.glob('flowdoc/aux_files/*.flowdb'):
       reader = csv.reader(open(file, "rt", encoding="utf8"), delimiter='\t')
       for row in reader:
          if key==row[0]:
              temp_file_str=os.path.splitext(os.path.basename(file))[0]
              read_flowdbs.file=temp_file_str
              #read_flowdbs.zoom=row[1]
              #read_flowdbs.displayname=row[2]
              print (infile_str)
              print('\n\nYEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEES\n\n')
              return True
    print('\n\nNOOOOOOOOOEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n\n')
    return False
       
        
htmlonline_str=''
write_htmlonline_firstcall=True
#writes out htmlonline_str adding up all the function/method's strings.
def write_htmlonline(string,outfile_str):
   global htmlonline_str
   global write_htmlonline_firstcall
   
   if write_htmlonline_firstcall: 
     htmlonline_str+="""<html><head><script type="text/javascript" src="jquery.js"></script>
     <script type="text/javascript" src="jquery_plantuml.js"></script>
     <!-- rawdeflate.js is implicity used by jquery_plantuml.js --></head>"""+'\n'+"""<body><hr>"""
     htmlonline_str+="<p>"+outfile_str+"</p>"
     htmlonline_str+= """<img uml=" """+string
     htmlonline_str+= """ "></body></html>"""
     
     write_htmlonline_firstcall=False
   else:
     htmlonline_str =htmlonline_str[:-14]
     htmlonline_str+="<hr><p>"+outfile_str+"</p>"
     htmlonline_str+= """<img uml=" """+string
     htmlonline_str+= """ "></body></html>"""
        
   return

#writes each diagram separately into a plantuml .txt file
def write_txt(string,outfile_str):
   f = open('flowdoc/aux_files/'+outfile_str+".txt","w")
   f.write(string)
   f.close()
   return



   
#finds calls in a source code line that has been annotated with //$ at the end
#the algorithm is based on the fact that call nodes are only associated to the '(' ')' of the source code line 
#several calls in the same line can be displayed. NOTE: calls inside calls are not displayed!
#CXXMemberCallExpr: call to a member method
#CallExpr: call to a function
def find_calls(scan_fileIN,scan_lineIN,scan_column_startIN,scan_column_endIN):
  
  singlelinecallsArrayIN=[]
  callfoundXtimesArray=[]
  singlelinecallsdefArrayIN=[]
    
  for it in range(scan_column_startIN,scan_column_endIN):
     
    loc=clang.cindex.SourceLocation.from_position(tu,scan_fileIN,scan_lineIN,it)
    scan_node=clang.cindex.Cursor.from_location(tu,loc)
    #print('scannode ',scan_node.kind.name)    
    #call found?
    #DECL_REF_EXPR (101) is an expression that refers to some value declaration, such as a function, varible, or enumerator
    referencefound = (scan_node.kind.name=='DECL_REF_EXPR')
    #CALL_EXPR (103) is a call
    callfound = (scan_node.kind.name=='CALL_EXPR')
    
    #DECL_REF_EXPR (101) --> we look for its definition, it should be a VAR_DECL (9) --> we look for its children, there should be a CALL_EXPR (103)
    if referencefound and scan_node.get_definition():
      #print ('reference found')
      if scan_node.get_definition().kind.name == 'VAR_DECL': 
        #print ('scannode.getdefinition is VAR_DECL')       
        for it9 in scan_node.get_definition().get_children():
           #print ('scan_node.get_definition().get_children() ',it9.kind.name)
           if it9.kind.value==103:
              if it9.get_definition() not in singlelinecallsdefArrayIN:
                   singlelinecallsdefArrayIN.append(it9.get_definition())
                      
    elif callfound:
        #print ('call found')
        #call already registered?
        alreadyregistered=False
        for it1 in singlelinecallsArrayIN:
           #call already registered
           if it1 == scan_node:
              #print ('call already registered')
              idx=singlelinecallsArrayIN.index(it1)
              callfoundXtimesArray[idx]+=1
              alreadyregistered=True
              break        
        #call not registered
        if not(alreadyregistered):
           #print ('call not registered')
           #are we inside another call?
           insidecall=False
           for it2 in callfoundXtimesArray:
              #we are inside another call
              if it2==1:
                 #print ('insidecall')
                 insidecall=True
                 break
           #we are not inside another call      
           if not(insidecall):
              #print ('not inside another call')
              #register call              
              singlelinecallsArrayIN.append(scan_node)
              #print ('registered call:', scan_node.kind.name, scan_node.extent, scan_node.displayname.decode("utf-8"), 'USR', scan_node.get_usr())
              #print ('registered call: getreferenced ',scan_node.get_referenced().kind.name, scan_node.get_referenced().displayname, 'USR', scan_node.get_referenced().get_usr())              
              ##print ('registered call: getdefinition ',scan_node.get_definition().kind.name, scan_node.get_definition().displayname, 'USR', scan_node.get_definition().get_usr())
              callfoundXtimesArray.append(1)
        
  for it3 in callfoundXtimesArray:
     if it3>2:
          print ('ERROR, ERROR, ERROR, bad algorithm in find_calls \n')
  #fill in the Array with the call.get_referenced(), rather than the direct calls, to avoid repetitions
  for it4 in singlelinecallsArrayIN:
      if it4.get_referenced():
        if it4.get_referenced() not in singlelinecallsdefArrayIN:
           singlelinecallsdefArrayIN.append(it4.get_referenced())           
      else:
        print ("No reference for the cursor")
   
  return singlelinecallsdefArrayIN  
  

#finds return statements
#BETTER algorithm if I COULD ACCESS parent node from any node?
#ryan gonzalez idea (not implemented): use node attributes to set parent  node.parent=parent_node
def find_returnstmt(nodeIN,zoom):
  returnlineArray=[]
  #two values for type: "True"->return in the flow, "False"-> conditional return inside an action
  returnTypeArray=[]

  def find_returnstmtRE(nodeIN2,zoom):  
    nonlocal returnlineArray
    nonlocal returnTypeArray
    #214: A return statement.
    if nodeIN2.kind.value==214:
        #print ('node',nodeIN2.kind)
        returnlineArray.append(nodeIN2.location.line)
        returnTypeArray.append(True)          
        #print (returnTypeArray)
        return 1
  
    else:    
       # Recurse for children of this node
       returnValue=None
       for c in nodeIN2.get_children():

          #print ('children', c.kind)
          nextlevelReturn=find_returnstmtRE(c,zoom)
          if nextlevelReturn==1:
             #print ('node',nodeIN2.kind)        
             returnValue=1
             if nodeIN2.kind.value==205 and returnTypeArray[-1]==True:
                if not lookfor_ActionComment_in_node(nodeIN2,zoom):
                   returnTypeArray[-1]=False
                   #print (returnTypeArray)                               
             continue
       return returnValue

  find_returnstmtRE(nodeIN,zoom)
  #print (returnlineArray)
  #print (returnTypeArray) 
  return returnlineArray, returnTypeArray 

#finds the first level of if statements inside a given node and returns two arrays
def find_ifstmt(nodeIN):
  ifbeginlineArrayIN=[]
  ifendlineArrayIN=[]
  ifnodeArrayIN=[]
  def find_ifstmtRE(nodeIN2):
    for d in nodeIN2.get_children():
      if d.kind.value==205:
        ifbeginlineArrayIN.append(d.extent.start.line)
        ifendlineArrayIN.append(d.extent.end.line)
        ifnodeArrayIN.append(d)
        return               
      else:        
  	    find_ifstmtRE(d)        
  find_ifstmtRE(nodeIN)
  return ifbeginlineArrayIN,ifendlineArrayIN, ifnodeArrayIN     

#find the else if and else statements of a given if-statement
def find_elsestmt(nodeIN):
  elseifbeginlineArrayIN=[]
  elsebeginlineIN=None
  ifstructurenodeArrayIN=[]  
  #add then() node
  for d in nodeIN.get_children():
     if (d.kind.value==202): 
        ifstructurenodeArrayIN.append(d)
        break  
  
  def find_elsestmtRE(nodeIN2):
    for e in nodeIN2.get_children():
      if e.kind.value==205:
        find_elsestmtRE.node_lastelseifstmt=e
        elseifbeginlineArrayIN.append(e.extent.start.line)
        #add elseif() node
        for f in e.get_children():
           if (f.kind.value==202): 
              ifstructurenodeArrayIN.append(f)
              break              
        find_elsestmtRE(e)
                
  find_elsestmtRE.node_lastelseifstmt=nodeIN
  find_elsestmtRE(nodeIN)  
  
  counter=0
  for f in find_elsestmtRE.node_lastelseifstmt.get_children():
    if f.kind.value==202:
       counter+=1
       if counter==2:
             elsebeginlineIN=f.extent.start.line
             #add else() node
             ifstructurenodeArrayIN.append(f)
  
  return elseifbeginlineArrayIN,elsebeginlineIN, ifstructurenodeArrayIN   


#main process function
def process_find_functions(node):

    #\s --> [ \t\r\f\v] : avoids newlines \n    
    regextextActionComment=r'^\s*//\$(?!\s+\[)\s+(?P<action>.+)$'
    regextextActionComment1=r'^\s*//\$1(?!\s+\[)\s+(?P<action>.+)$'
    regextextAnyActionComment1=r'^\s*//\$1?(?!\s+\[)\s+(?P<action>.+)$'
    regexActionComment = re.compile(regextextActionComment)
    regexActionComment1 = re.compile(regextextActionComment1)
    regexAnyActionCommentZoomArray = [regexActionComment, re.compile(regextextAnyActionComment1)]
    regexContextualComment = re.compile(r'^\s*//\$\s+\[(?P<action>.+)\]\s*$')
    regexHighlightComment = re.compile(r'^\s*(?P<commandline>.+?)\s+//\$\s*(?:$|//.+$)') 
    regexIf = re.compile(r'^\s*if\s*\((?P<condition>.*)\)\s*{\s*(?:$|//.*$)')
    regexIf1line = re.compile(r'^\s*if\s*\((?P<condition>.*)\)\s*{\s*(?:$|//.*$)')
    regexElseIf = re.compile(r'^\s*}\s*else if\s*\((?P<condition>.*)\)\s*{\s*(?:$|//.*$)')

    start_line= node.extent.start.line
    end_line= node.extent.end.line     
    infile_clang=node.location.file
    global infile_str
    infile_str=node.location.file.name.decode("utf-8")
    infile= open(infile_str,'r')            
    #lines enumerated starting from 1
    enum_file=list(enumerate(infile,start=1))      
    infile.close()
    
    #look for comment inside function/method
    comment_inside_method = False
    for j,lineA in enum_file:
      if j in range(start_line,end_line):
        commentB = regexActionComment.match(lineA)
        if commentB:
          comment_inside_method = True
          break
    
    #if ActionComment inside function/method:
    if comment_inside_method == True :            
      print ('Found %s of kind %s [start_line=%s, end_line=%s. At "%s"]' % (
                                            node.spelling, node.kind.name , node.extent.start.line, node.extent.end.line, node.location.file))

      zoom_str_Array=['','1']
      for zoom in range(0,2):
                    
        class_name=''
        if node.kind.name=='CXX_METHOD':           
          class_name=str(node.semantic_parent.spelling.decode("utf8"))+'_'
          #also see node.lexical_parent.spelling
        outfile_str = str(node.get_usr().decode("utf8"))+zoom_str_Array[zoom]
        #remove special characters from outfile_str 
        outfile_str = ''.join(e for e in outfile_str if e.isalnum())
        #outfile= open(outfile_str+'.txt', "w+")  

        #find if statements inside the function     
        ifbeginlineArray, ifendlineArray, ifnodeArray = find_ifstmt(node)        
        
        #variables for conditional statements ('Local' means nested inside another conditional statement)
        elseifbeginlineArray=[]
        elsebeginline=None          
        ifbeginlineLocalArray=[]
        ifendlineLocalArray=[]
        ifnodeLocalArray=[]
        elseifbeginlineLocalArray=[]
        elsebeginlineLocal=None 
        endifWrite = False
        endifLocalWrite = False
        elseifNum = 0
        elseifNumLocal = 0
        IdxIfbeginlineArray = None
        IdxIfbeginlineArrayLocal = None
        
        #find return statements inside the function
        #returnlineArray = find_returnstmt(node,zoom)
        returnlineArray, returnTypeArray=find_returnstmt(node,zoom)
        
        #other variables
        lastcommentlinematched=0
        tab='   '
        indentation_level=0
        last_comment_str=''
        string_notes=''
        string=''
        inside_comment=False
        actioncallsdefArray=[]               
        
        
        def add_note(stringIN):
           nonlocal string_notes
           string_notes+=stringIN+'\n'                   

        def write_last_comment():
           nonlocal string_notes
           nonlocal string
           nonlocal last_comment_str
           nonlocal inside_comment
           nonlocal actioncallsdefArray
           if inside_comment:
              if actioncallsdefArray:
                 last_comment_str=last_comment_str[:-2]+"\\n--------\\n"
                 for it7 in actioncallsdefArray:
                    print('LOOKING IF CALLS EXIST:',it5.kind.name, it5.get_definition(),it5.location)
                    if read_flowdbs(it5.get_usr().decode("utf8")):
                      call_in_filename_str=read_flowdbs.file+'.html'
                      usr_id_str= str(it7.get_usr().decode("utf-8"))
                      usr_id_str = ''.join(e for e in usr_id_str if e.isalnum())
                      classname = ''
                      if it7.kind.name=='CXX_METHOD':
                         classname= str(it7.semantic_parent.spelling.decode("utf-8"))+'::'
                      last_comment_str+=str(it7.result_type.kind.name)+' '+classname+str(it7.displayname.decode("utf-8"))+' -- [['+call_in_filename_str+'#'+usr_id_str+']]'+'\\n'
                      #last_comment_str+=str(it7.result_type.kind.name)+' '+str()+str(it7.displayname.decode("utf-8"))+' -- [[http://www.google.es]]'+'\\n'
                 last_comment_str+=';\n'
              if string_notes != "":
                 last_comment_str+= "note right\n"+string_notes+"end note\n"
              string = string + last_comment_str
              last_comment_str=''              
              inside_comment=False
              string_notes=""
              actioncallsdefArray=[]
           return
        
        string+='@startuml\n\nstart\n'
        
        #main loop over source code lines
        for i, line in enum_file:
          if i in range(start_line,end_line):
              #### 
             
              if i in ifbeginlineArray:
                 #look for comment inside if statement
                 IdxIfbeginlineArray=ifbeginlineArray.index(i)
                 comment_inside_ifstmt = False
                 for h,lineB in enum_file:
                    if h in range(i,ifendlineArray[IdxIfbeginlineArray]):
                      commentB = regexAnyActionCommentZoomArray[zoom].match(lineB)
                      if commentB:
                        comment_inside_ifstmt = True
                        break
                 #if comment inside if statement:
                 if comment_inside_ifstmt == True :
                    #write last comment in string
                    write_last_comment()
                    #write 'if' in string
                    ifstmt=regexIf.match(line)
                    if ifstmt:
                      string+= '\n'+ 'if ('+ifstmt.group('condition')+') then(yes)''\n'
                    else:
                      print ('Error: condition not picked up at line', line) 
                      string+= indentation_level*tab +'\n'+ 'if ('+'Error: unknown condition'+') then(yes)''\n'                     
                    #mark } endif to be written in string
                    endifWrite=True
                    indentation_level+=1
                    #explore substructure: then / else if/ else: elseifbeginlineArray, elsebeginline, ifstructurenodeArray
                    elseifbeginlineArray, elsebeginline, ifstructurenodeArray = find_elsestmt(ifnodeArray[IdxIfbeginlineArray])                  
                    #explore then and update ifbeginlineLocalArray, ifendlineLocalArray, ifnodeLocalArray 
                    ifbeginlineLocalArray, ifendlineLocalArray, ifnodeLocalArray = find_ifstmt(ifstructurenodeArray[0])               

              #if i in elseifbeginlineArray
              elif i in elseifbeginlineArray:
                 elseifNum+=1
                 #write last comment
                 write_last_comment()
                 #write 'else if' in string
                 elseifstmt=regexElseIf.match(line)
                 if elseifstmt:
                   string+=(indentation_level-1)*tab+'else'+'\n'+indentation_level*tab+'if ('+elseifstmt.group('condition')+') then (yes)'+'\n'
                 else:
                   print ('Error: condition not picked up at line', line) 
                   string+=(indentation_level-1)*tab+'else'+'\n'+indentation_level*tab+'if ('+'Error: unknown condition'+') then (yes)'+'\n'                    
                 indentation_level+=1              
                 #explore elseif and update ifbeginlineLocalArray, ifendlineLocalArray, ifnodeLocalArray
                 ifbeginlineLocalArray, ifendlineLocalArray, ifnodeLocalArray = find_ifstmt(ifstructurenodeArray[elseifNum]) 
  
              #if i in elsebeginline
              elif i == elsebeginline:
                 #write last comment
                 write_last_comment()
                 #write 'else' in string
                 string+= (indentation_level-1)*tab+'else'+'\n' 
                 #explore else and update ifbeginlineLocalArray, ifendlineLocalArray, ifnodeLocalArray
                 ifbeginlineLocalArray, ifendlineLocalArray, ifnodeLocalArray = find_ifstmt(ifstructurenodeArray[-1]) 

              #if i is ifendlineArray[IdxIfbeginlineArray] and }'s marked to be written in string:
              elif endifWrite and (i == ifendlineArray[IdxIfbeginlineArray]): 
                   #write last comment
                   write_last_comment()
                   #is the else condition explicitly written? Otherwise write now
                   if elsebeginline==None:
                      string+= (indentation_level-1)*tab+'else'+'\n'
                   #write endif's in string
                   for n in range(elseifNum):
                      string+= (indentation_level-1)*tab+'endif'+'\n'
                      indentation_level-=1
                   string+= (indentation_level-1)*tab +'endif'+'\n'+'\n'
                   indentation_level-=1
                   
                   #reset all variables
                   endifWrite = False
                   elseifNum = 0
                   del elseifbeginlineArray[:]
                   elsebeginline=None
              
              ####
              elif i in ifbeginlineLocalArray:
                 #look for comment inside Local if statement
                 IdxIfbeginlineArrayLocal=ifbeginlineLocalArray.index(i)
                 comment_inside_ifstmt = False
                 for h,lineB in enum_file:
                    if h in range(i,ifendlineLocalArray[IdxIfbeginlineArrayLocal]):
                      commentB = regexAnyActionCommentZoomArray[zoom].match(lineB)
                      if commentB:
                        comment_inside_ifstmt = True
                        break
                 #if comment inside Local if statement:
                 if comment_inside_ifstmt == True :
                    #write last comment
                    write_last_comment()
                    #write 'if' in string
                    ifstmt=regexIf.match(line)
                    if ifstmt:
                      string+= '\n'+ indentation_level*tab + 'if ('+ifstmt.group('condition')+') then(yes)''\n'
                    else:
                      print ('Error: condition not picked up at line', line) 
                      string+= '\n'+ indentation_level*tab + 'if ('+'Error: unknown condition'+') then(yes)''\n'                     
                    #mark } Local endif to be written in string
                    endifLocalWrite=True
                    indentation_level+=1
                    #explore substructure: then / else if/ else: elseifbeginlineLocalArray, elsebeginlineLocal, ifstructurenodeLocalArray
                    elseifbeginlineLocalArray, elsebeginlineLocal, ifstructurenodeLocalArray = find_elsestmt(ifnodeLocalArray[IdxIfbeginlineArrayLocal])                  
               
              #if i in elseifbeginlineLocalArray
              elif i in elseifbeginlineLocalArray:
                 elseifNumLocal+=1
                 #write last comment
                 write_last_comment()
                 #write 'else if' in string
                 elseifstmt=regexElseIf.match(line)
                 if elseifstmt:
                   string+=(indentation_level-1)*tab+'else'+'\n'+indentation_level*tab+'if ('+elseifstmt.group('condition')+') then (yes)'+'\n'
                 else:
                   print ('Error: condition not picked up at line', line) 
                   string+=(indentation_level-1)*tab+'else'+'\n'+indentation_level*tab+'if ('+'Error: unknown condition'+') then (yes)'+'\n'                    
                 indentation_level+=1              
              
              #if i in elsebeginlineLocal
              elif i == elsebeginlineLocal:
                 #write last comment
                 write_last_comment()
                 #write 'else' in string
                 string+= (indentation_level-1)*tab+'else'+'\n' 

              #if i is ifendlineLocalArray[IdxIfbeginlineArrayLocal] and }'s marked to be written in string:
              elif endifLocalWrite and (i == ifendlineLocalArray[IdxIfbeginlineArrayLocal]): 
                   #write last comment
                   write_last_comment()
                   #is the else condition explicitly written? Otherwise write now
                   if elsebeginlineLocal==None:
                      string+= (indentation_level-1)*tab+'else'+'\n' 
                   #write endif's in string
                   for n in range(elseifNumLocal):
                      string+= (indentation_level-1)*tab+'endif'+'\n'
                      indentation_level-=1
                   string+= (indentation_level-1)*tab+'endif'+'\n'+'\n'
                   indentation_level-=1
                   
                   #reset all variables
                   endifLocalWrite = False
                   elseifNumLocal = 0
                   del elseifbeginlineLocalArray[:]
                   elsebeginlineLocal=None              
              
              #elif i in returnlineArray:
              elif i in returnlineArray:
                 if returnTypeArray[returnlineArray.index(i)] == True:
                    #write last comment in string
                    write_last_comment()
                    string+= "\nstop\n"
                 if returnTypeArray[returnlineArray.index(i)] == False:
                    add_note("possible STOP")

                   
              ####  comments
              else:
                anycomment = regexAnyActionCommentZoomArray[zoom].match(line)
                comment_highlight = regexHighlightComment.match(line)
                if anycomment:
                      if lastcommentlinematched == i-1:
                         last_comment_str=last_comment_str[:-2]+' '+anycomment.group('action')+';\n'
                      else:
                         #write last comment
                         write_last_comment()
                         #new comment
                         inside_comment=True
                         comment = regexActionComment.match(line)
                         comment1 = regexActionComment1.match(line)
                         if comment:
                            last_comment_str+= indentation_level*tab +':#yellow:'+comment.group('action')+';\n'
                         elif comment1:
                            last_comment_str+= indentation_level*tab +':'+comment1.group('action')+';\n'
                      lastcommentlinematched = i            

                elif comment_highlight:
                    print ('line',i)

                    scan_column_start=1+comment_highlight.start('commandline')
                    #the end character is -1. There is an offset of +1 with respect to the file
                    scan_column_end=1+comment_highlight.end('commandline')-1
                    scan_file= infile_clang
                    scan_line=i
                    print (scan_file,scan_line,scan_column_start,scan_column_end)
                    singlelinecallsdefArray = find_calls(scan_file,scan_line,scan_column_start,scan_column_end)
                    #for it4 in singlelinecallsdefArray:
                       #print ('singlelinecallsdefArray',it4.displayname.decode("utf-8"))
                    for it5 in singlelinecallsdefArray:
                       if it5 not in actioncallsdefArray:
                          actioncallsdefArray.append(it5)                             
                          
        
        write_last_comment()
        string+= '\n@enduml'
        #print (string)
        
        write_htmlonline(string,outfile_str)
        write_txt(string,outfile_str)

      return  

   

def find_functions(node):

  if node.kind.is_declaration():
     #8 is a function and 21 is c++ class method
    if node.kind.value== 8 or node.kind.value==21:
       #if os.path.dirname(str(node.location.file)) == './src':
         process_find_functions(node)
       #return

  # Recurse for children of this node
  for c in node.get_children():
      #print ('children', c.kind)
      find_functions(c)

#### main program

index = clang.cindex.Index.create()
#tu = index.parse(sys.argv[1])
args=["-c","-x","c++","-Wall","-ansi","-I./include"]
#tu = index.parse("./src/t.cpp",args)
#tu_aux1=index.parse("./include/t.h",args, None, 2)
#tu_aux2=index.parse("./src/t.cpp",args)
tu = index.parse(sys.argv[1],args)
print ('Translation unit:', tu.spelling.decode("utf-8"))
#global variable for the name of the input file. It will be defined later on.
infile_str=''
find_functions(tu.cursor)
#print(os.path.splitext(infile_str)[0])
#print(os.path.basename(infile_str))
#print(os.path.splitext(os.path.basename(infile_str))[0])

#f = open('flowdoc/'+os.path.splitext(os.path.basename(infile_str))[0]+".html","w")
#f.write(htmlonline_str)
#f.close()