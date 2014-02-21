function menusep(txt)
{
    document.write('<h5>');
    document.write(txt);
    document.write('</h5>');

}


function menu(activeitemdesordered)
{
var order=new Array();
order[0]=0;
order[1]=1;
order[2]=2;
order[3]=3;
order[4]=4;
order[5]=5;
order[6]=6;
order[7]=7;
order[8]=8;
order[9]=9;
order[10]=10;
order[11]=11;
order[12]=12;
order[13]=13;
order[14]=14;
order[15]=15;
order[16]=16;

var menuitems=new Array();
menuitems[0]="About";
menuitems[1]="Actions";
menuitems[2]="Actions_zooming";
menuitems[3]="Actions_modifiers";
menuitems[4]="Classes";
menuitems[5]="Functions/Methods";
menuitems[6]="Func/Meth_description";
menuitems[7]="Func/Meth_returns";
menuitems[8]="Calls";
menuitems[9]="Main calls";
menuitems[10]="IF Statements";
menuitems[11]="IF Statements_Zooming";
menuitems[12]="FOR Statements";
menuitems[13]="FORStatements_Continue";
menuitems[14]="FORStatements_Break";
menuitems[15]="Hyperlinks to Code";
menuitems[16]="To Do Comments";


var menuitemslinks=new Array();
menuitemslinks[0]="index.html";
menuitemslinks[1]="actions.html";
menuitemslinks[2]="actions_zooming.html";
menuitemslinks[3]="actions_modifiers.html";
menuitemslinks[4]="classes.html";
menuitemslinks[5]="functions_methods.html";
menuitemslinks[6]="functions_methods_description.html";
menuitemslinks[7]="functions_methods_returns.html";
menuitemslinks[8]="calls.html";
menuitemslinks[9]="calls_main.html";
menuitemslinks[10]="IFstatements.html";
menuitemslinks[11]="IFstatements_zooming.html";
menuitemslinks[12]="FORstatements.html";
menuitemslinks[13]="FORstatements_continue.html";
menuitemslinks[14]="FORstatements_break.html";
menuitemslinks[15]="codereferences.html";
menuitemslinks[16]="toDoComments.html";

for (var i=0;i<menuitems.length;i++)
{
 if (i==0) menusep("About");
 if (i==1) menusep("Actions");
 if (i==4) menusep("Classes");
 if (i==5) menusep("Functions/Methods");
 if (i==8) menusep("Calls");
 if (i==10) menusep("Flow Bifurcations - IF")
 if (i==12) menusep("Flow Bifurcations - LOOPS")
 if (i==15) menusep("Others")
  
 if (i==order[activeitemdesordered])
 { menuentry(menuitems[i],".");}
 else
 {menuentry(menuitems[i],menuitemslinks[i]);}
}

}

          
          
function menuentry(txt,link)
{
  if (link == ".") {
    document.write('<ul>');
    document.write('<li class="none">');
    document.write('<strong>');
    document.write(txt); 
    document.write('</strong>');
    document.write('</li>');
    document.write('</ul>');
  } else {
    document.write('<ul>');
    document.write('<li class="none">');
    document.write('<strong>');
    document.write('  <a href="') ; document.write(link) ; document.write('">');
    document.write(txt);
    document.write('</a>');  
    document.write('</strong>');
    document.write('</li>');
    document.write('</ul>');
  }

}

                                        
function newsentry(date,link,href,txt)
{
document.write('<table bgcolor="#ffffff" border=0 cellpadding=0 cellspacing="0">');
document.write('<tr><td>');
document.write(date);document.write(': ');
if (link != " ") { 
 document.write(' <a href="');
 document.write(href);
 document.write('">');
 document.write(link);
 document.write('</a>');
}
document.write('<span class=newstxt>');
document.write(txt);
document.write('</span>');
document.write('</td></tr>');
document.write('<tr bgcolor="#ffffff" height="10"><td></td></tr>');
document.write('<tr bgcolor="#ffffff" height="1"><td></td></tr>');
document.write('<tr bgcolor="#dddddd" height="1"><td></td></tr>');
document.write('<tr bgcolor="#a0a0a0" height="1"><td></td></tr>');
document.write('<tr bgcolor="#dddddd" height="1"><td></td></tr>');
document.write('<tr bgcolor="#ffffff" height="6"><td></td></tr>');
document.write('</table>');
}

function header(txt,txtR) {
document.write('<table cellspacing="0" cellpadding="0" border="0" width="100%" bgcolor="#ffffff">');
document.write('<tr><td width="40%" valign="top"><a href="http://www.cern.ch"><img src="http://www.cern.ch/skands/img/CERNLogo.gif" width="420" height="76" border="0" alt="CERN" vspace="5" hspace="10"></a></td><td width="60%" valign="top">');
document.write('<table width="100%" cellspacing="0" cellpadding="20" border="0" bgcolor="#eeeeee">');
document.write(' <tr><td valign="top"><span class="subheader">');
document.write(' Peter Z. Skands </br>');
document.write('Staff Scientist (LD), ');
document.write('    <a href="http://wwwth.cern.ch">Theoretical');
document.write('      Physics</a><br>');
document.write('      4-2-050 (Case 01600), 1211 Geneva 23, CH</br></span>');
document.write('</td>');
document.write('<td valign="top">');
document.write('<span class="h4">');
document.write(' Phone: +41 22 767 2447<br>Fax: +41 22 767 3850<br>mail: peter.skands.AT.cern.ch</br>');
document.write('</span></td></tr>');
document.write('</table>');
document.write('</td></tr><tr height="1"><td colspan="2" bgcolor="#000000"></td></tr>');
document.write('</td></tr><tr height="20"><td colspan="2" bgcolor="#ffffff"></td></tr>');
document.write('</table>');
}
function headerold(txt,txtR) {
document.write('<table cellspacing="0" cellpadding="0" border="0" width="100%" bgcolor="#dddddd">');
document.write('<tr height="1"><td colspan="5" bgcolor="#ffffff"></td></tr>');
document.write('<tr height="1"><td colspan="5" bgcolor="#eeeeee"></td></tr>');
document.write('<tr height="1"><td colspan="5" bgcolor="#ffffff"></td></tr>');
document.write('<tr height="56"><td width="15"></td><td><span class="header">');
document.write(txt);
document.write('</span></td>');
document.write('<td align="right">');
if (txtR != '') {
 document.write(txtR);
}
document.write('<td width="19"></td></tr>');
document.write('<tr height="1"><td colspan="5" bgcolor="#ffffff"></td></tr>');
document.write('<tr height="1"><td colspan="5" bgcolor="#dddddd"></td></tr>');
document.write('<tr height="1"><td colspan="5" bgcolor="#ffffff"></td></tr>');
document.write('<tr height="1"><td colspan="5" bgcolor="#eeeeee"></td></tr>');
document.write('<tr height="19"><td colspan="5" bgcolor="#ffffff"></td></tr>');
document.write('</table>');
}

function subheader(txt) {
document.write('<table cellspacing="0" cellpadding="3" border="0" width="100%">');
document.write('<tr><td width="0"></td><td><span class="subheader">');
document.write(txt);
document.write('</span></td></tr>');
document.write('</table>');
}

function top()
{
document.write('<table cellspacing="0" cellpadding="0" border="0" width="100%">');
document.write('<tbody>');
document.write(' <tr>');
document.write('  <td height="20" valign="bottom" align="left">');
document.write('   <img src="img/gold-bullet.gif" alt=" * ">');
document.write('   <a href="http://www.fnal.gov/pub/today/">Fermilab Today</a> &nbsp; &nbsp;');
document.write('   <span align="left"><img src="img/gold-bullet.gif" alt=" * ">');
document.write('   <a href="http://theory.fnal.gov/seminars/seminars.html">Fermilab Theory Seminars</a></span>');
document.write('   &nbsp; &nbsp; <span align="left"><img src="img/gold-bullet.gif" alt=" * ">');
document.write('   <a href="http://theory.fnal.gov/jetp/">Wine&Cheese Seminars</a></span>');
document.write('  </td>');
document.write('  <td width="15"></td>');
document.write(' </tr>');
document.write('</tbody>');
document.write('</table>');
}

function address()
{
document.write('<br>');
document.write(' <span class="smallblack">');
document.write('Peter Z. Skands<br>');
document.write('Associate Scientist<br>');
document.write('Theoretical Physics<br>');
document.write('Fermilab MS106<br>');
document.write('PO Box 500<br>');
document.write('Batavia IL-60510<br>');
document.write('USA<br><br>');
document.write('Home (USA):<br>');
document.write('29W473 Blackthorn Ln (<a href="http://maps.google.com/maps?q=29W+473+Blackthorn+Lane,+Warrenville,+IL&ie=utf-8&oe=utf-8&rls=org.mozilla:en-GB:official&client=firefox-a&um=1&sa=N&tab=wl">map</a>)<br>');
document.write('Warrenville IL-60555<br>');
document.write('USA<br><br>');
document.write('Home (France):<br>');
document.write('610 Rue des Moraines<br>');
document.write('F-01170 Chevry (Veraz)<br>');
document.write('France<br><br>');
document.write('Tel (FNAL):<br> &nbsp; (+1) 630 840-2270<br>');
document.write('Fax (FNAL):<br> &nbsp; (+1) 630 840-5435<br>');
document.write('Cell (USA):<br> &nbsp; (+1) 630 456-1962<br>');
document.write('</span><br><br>');
}

function menusepOld(txt)
{
    document.write('<tr bgcolor="#dcdcdc" height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr bgcolor="#dcdcdc" height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr bgcolor="#dcdcdc" height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr height="1">');
    document.write('<td width="15"></td><td>');
    document.write('</td></tr>');
    document.write('<tr bgcolor="#dcdcdc" height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr bgcolor="#dcdcdc" height="1">');
    document.write('<td width="15"></td><td></td></tr>');
    document.write('<tr height="1" bgcolor="#444444">');
    document.write('<td width="15"></td><td>');
    if (txt != " ") {
      document.write('<span class="boldorange">');
      document.write(txt);
      document.write('</span>');
    }
    document.write('</td></tr>');
    document.write('<tr height="19">');
    document.write('<td width="15"></td><td></td></tr>');
}
