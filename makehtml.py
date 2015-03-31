#!/usr/bin/env python
#### writes out HTML files. Uses .flowdbs information and .cmapx information (for hyperlinks).
#### HMTL files point to PNG files with the diagrams

import sys
import os
import glob
import csv




infile_str=os.path.splitext(os.path.basename(sys.argv[1]))[0]
print (infile_str)


htmloffline_str=''
htmloffline_str+="""<html>
    <head>
      <title>docs</title>
      <script src="htmlCSSandJS/tabcontent.js" type="text/javascript"></script>
      <link href="htmlCSSandJS/template1/tabcontent.css" rel="stylesheet" type="text/css" />
    </head>
    
    <body>"""
 
       
#read corresponding .flowdb file (with the same filename as the argument of the call of makehtml.py)
reader = csv.reader(open('flowdoc/aux_files/'+infile_str+'.flowdb', "rt", encoding="utf8"), delimiter='\t')
#loop over annotated functions / methods
for row in reader:
  usr_key=''.join(e for e in row[0] if e.isalnum())
  
  htmloffline_str+="""
    <hr><hr><hr>"""
  htmloffline_str+="<p>"+row[2]+"</p>"+"""<a name="#"""+usr_key+""""></a>"""
  #if zoom level is 0
  if int(row[1])==0:
      zoomID=''  
      #if hyperlinks exist in the diagrams include them
      if os.path.exists('flowdoc/aux_files/'+usr_key+zoomID+'.cmapx'):
          htmloffline_str+= """
          <img src="aux_files/"""+usr_key+zoomID+""".png" """    
          map_str=open('flowdoc/aux_files/'+usr_key+zoomID+'.cmapx').read()
          htmloffline_str+=""" USEMAP="#"""+usr_key+zoomID+'_map'
          htmloffline_str+=""""/> """   
          htmloffline_str+=map_str+'\n'
      else:
          htmloffline_str+= """
          <img src="aux_files/"""+usr_key+zoomID+'.png'    
          htmloffline_str+=""" "> """
  #else, loop over zoom levels and put them into tabs
  else:
    htmloffline_str+="""
      <ul class="tabs" data-persist="true">"""
    for i in range(int(row[1])+1):
      zoomID=''
      if i==1:
        zoomID='1'
      if i==2:
        zoomID='2'
      htmloffline_str+="""
        <li><a href="#view"""+zoomID+'_'+usr_key+""" ">zoom"""+zoomID+"""</a></li>"""
    htmloffline_str+="""
      </ul>"""
    
    htmloffline_str+= """
      <div class="tabcontents"> """
    for i in range(int(row[1])+1):
      zoomID=''
      if i==1:
        zoomID='1'
      if i==2:
        zoomID='2'
      htmloffline_str+="""
        <div id="view"""+zoomID+'_'+usr_key+""" ">"""
      #if hyperlinks exist in the diagrams include them
      if os.path.exists('flowdoc/aux_files/'+usr_key+zoomID+'.cmapx'):
          htmloffline_str+= """<img src="aux_files/"""+usr_key+zoomID+""".png" """    
          map_str=open('flowdoc/aux_files/'+usr_key+zoomID+'.cmapx').read()
          htmloffline_str+=""" USEMAP="#"""+usr_key+zoomID+'_map'
          htmloffline_str+=""""/> """   
          htmloffline_str+=map_str+'\n'
      else:
          htmloffline_str+= """<img src="aux_files/"""+usr_key+zoomID+'.png'    
          htmloffline_str+=""" "> """
      htmloffline_str+="</div>"
    htmloffline_str+="""
      </div>"""
    

htmloffline_str+="""
    </body>
</html>"""
        
#write string into HTML file
writefunc = open('flowdoc/'+infile_str+'.html',"w")
writefunc.write(htmloffline_str)
writefunc.close()


