// AbstractMachine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "AbstractMachine.h"
#include "Interpreter.h"
#include <print>


int main()
{

    std::print( "This is Abstract Machine!\n");
    std::print( "Copyright © 2026 Guillermo M. Dávila Andino\n");
    std::print( "All rights reserved.\n");

    AbstractMachine a;

    Interpreter I(&a);

    I.REPL();

}

