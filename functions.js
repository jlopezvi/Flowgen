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
order[1]=3;
order[2]=4;
order[3]=5;
order[4]=6;
order[5]=7;
order[6]=8;
order[7]=9;
order[8]=10;
order[9]=11;
order[10]=12;
order[11]=13;
order[12]=15;
order[13]=18;
order[14]=19;
order[15]=20;
order[16]=21;
order[17]=2;
order[18]=1;
order[19]=14
order[20]=16
order[21]=17

var menuitems=new Array();
menuitems[0]="About";
menuitems[1]="Simple_example";
menuitems[2]="Extended_example";
menuitems[3]="Actions";
menuitems[4]="Actions_zooming";
menuitems[5]="Actions_modifiers";
menuitems[6]="Classes";
menuitems[7]="Functions/Methods";
menuitems[8]="Func/Meth_description";
menuitems[9]="Func/Meth_returns";
menuitems[10]="Calls";
menuitems[11]="Main calls";
menuitems[12]="IF Statements";
menuitems[13]="IF Statements_Zooming";
menuitems[14]="Switch-Case Statements";
menuitems[15]="FOR Statements";
menuitems[16]="WHILE Statements";
menuitems[17]="DO-WHILE Statements";
menuitems[18]="Continue Statements";
menuitems[19]="Break Statements";
menuitems[20]="Hyperlinks to Code";
menuitems[21]="To Do Comments";


var menuitemslinks=new Array();
menuitemslinks[0]="index.html";
menuitemslinks[1]="simple_example.html";
menuitemslinks[2]="extended_example.html";
menuitemslinks[3]="actions.html";
menuitemslinks[4]="actions_zooming.html";
menuitemslinks[5]="actions_modifiers.html";
menuitemslinks[6]="classes.html";
menuitemslinks[7]="functions_methods.html";
menuitemslinks[8]="functions_methods_description.html";
menuitemslinks[9]="functions_methods_returns.html";
menuitemslinks[10]="calls.html";
menuitemslinks[11]="calls_main.html";
menuitemslinks[12]="IFstatements.html";
menuitemslinks[13]="IFstatements_zooming.html";
menuitemslinks[14]="SWITCHstatements.html";
menuitemslinks[15]="FORstatements.html";
menuitemslinks[16]="WHILEstatements.html";
menuitemslinks[17]="DOWHILEstatements.html";
menuitemslinks[18]="CONTINUEstatements.html";
menuitemslinks[19]="BREAKstatements.html";
menuitemslinks[20]="codereferences.html";
menuitemslinks[21]="toDoComments.html";


for (var i=0;i<menuitems.length;i++)
{
 if (i==0) menusep("About");
 if (i==3) menusep("Actions");
 if (i==6) menusep("Classes");
 if (i==7) menusep("Functions/Methods");
 if (i==10) menusep("Calls");
 if (i==12) menusep("Flow Bifurcations - IF, Switch")
 if (i==15) menusep("Flow Bifurcations - LOOPS")
 if (i==20) menusep("Others")
  
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

