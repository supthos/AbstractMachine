// AbstractMachine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Language.h"
#include <print>
#include <format>

int main()
{

    std::cout << "This is Abstract Machine!\n";
    std::cout << "Copyright © 2026 Guillermo M. Dávila Andino\n";
    std::cout << "All rights reserved.\n";

    //AbstractMachine AM1;


    //Substrate<char8_t> a;

    AbstractMachine a;
    
    auto [tok, result] = a.language.Evaluate(u8"5628");
    
    if (result.has_value() && result.type() == typeid(Token<char8_t>)) {
        std::cout << "Actual Stored Type: " << result.type().name() << "\n";
        // Safe to cast now
        std::cout << "Token: " << tok << ", Value: " << std::any_cast<Token<char8_t>>(result) << "\n";
    }
    else {
        // This block handles the case where Evaluation failed or type is wrong
        std::cout << "Evaluation failed or returned empty/wrong std::any.\n";
        // Add more diagnostics if needed
        if (result.has_value()) {
            std::cout << "Error: Expected Token<char8_t>, but std::any holds type: " << result.type().name() << "\n";
        }
        else {
            std::cout << "Error: std::any is empty (no matching language rule found).\n";
        }
    }

    ProgramFile<char8_t> pf;
    pf.push_back(u8"start 16");
    pf.push_back(u8"write 1");
    pf.push_back(u8"left");
    pf.push_back(u8"write 1");
    pf.push_back(u8"left");
    pf.push_back(u8"write 1");
    pf.push_back(u8"end");


    try {
        a.LoadAndRun(pf);
    } catch (const std::exception& e) {
        std::cerr << "Aborted with error: " << e.what() << std::endl;
    }

}

