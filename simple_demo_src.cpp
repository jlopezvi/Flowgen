#include "t.h"
#include <iostream>

class Person {
};


class Room {
public:
    void add_person(Person person)
    {
        //$ do important stuff
        
        //$1 do stuff
    }

private:
    Person* people_in_room;
};

void function_a(){
// nothing to do
}

int function_c(){
//$ test visit function c
return 1;
}



int main()
{
    //$ action
    //$ number 1 
    
    std::cout<<"test\n";
    Person* p = new Person();
    
    
    function_a(); 

    
    int testReference = function_c();
    
    int var = testReference + 1; 
    
    VINCIA* vinciaOBJ = new VINCIA();
    vinciaOBJ->shower();  //$   


    

    //$ [null]
    return 0;

}