#include "source1.h"
#include <iostream>

int main()
{       
    int control_flag=0;
    //$ ask user whether to proceed
    std::cin >> control_flag;
    
    if (control_flag==1){
        //$1 call shower
        //pointer to the object VINCIA
        VINCIAPARENT* vinciaOBJ = new VINCIA();
        vinciaOBJ->shower();  //$ 
        //$2 test 
    } else if(0==0){
    }
    return 0;
}