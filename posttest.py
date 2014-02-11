#!/usr/bin/env python
import sys
import os
import glob
import csv


#### main program

infile_str=os.path.splitext(os.path.basename(sys.argv[1]))[0]
print (infile_str)


htmloffline_str=''
htmloffline_str+="""<html><head><title>docs</title></head>"""+'\n'+"""<body><hr>"""
 

       

reader = csv.reader(open('flowdoc/aux_files/'+infile_str+'.flowdb', "rt", encoding="utf8"), delimiter='\t')
for row in reader:
  usr_key=''.join(e for e in row[0] if e.isalnum())
  for i in range(int(row[1])+1):
    print(range(1))
    zoomID=''
    zoom_str=''
    if i==1:
      zoomID='1'
      zoom_str='  -- zoom 1'    
    htmloffline_str+="<p>"+row[2]+zoom_str+"</p>"+"""<a name="#"""+usr_key+zoomID+""""></a>"""
    if os.path.exists('flowdoc/aux_files/'+usr_key+zoomID+'.cmapx'):
        htmloffline_str+= """<img src="aux_files/"""+usr_key+zoomID+""".png" """    
        map_str=open('flowdoc/aux_files/'+usr_key+zoomID+'.cmapx').read()
        htmloffline_str+=""" USEMAP="#"""+usr_key+zoomID+'_map'
        htmloffline_str+=""""/> """   
        htmloffline_str+=map_str+"""<hr>
        """
    else:
        htmloffline_str+= """<img src="aux_files/"""+usr_key+zoomID+'.png'    
        htmloffline_str+=""" "> 
        <hr>"""
  htmloffline_str+="<hr><hr>"    

htmloffline_str+="""</body></html>"""
        

writefunc = open('flowdoc/'+infile_str+'.html',"w")
writefunc.write(htmloffline_str)
writefunc.close()


