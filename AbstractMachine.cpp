// AbstractMachine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Language.h"
#include <print>
#include <format>

//class AbstractMachine {
//public:
//    void Start() {}
//    void Quit() {}
//    int Load() { return 0; }
//    int Run() { return 0; }
//    std::any P;
//    std::map<std::string, std::any> SymTable;
//};


int main()
{

    std::cout << "Hello World!\n";

    //AbstractMachine AM1;


    Substrate<char8_t> a;
    auto [tok, result] = a.language.Evaluate(u8"5628");
    if (result.has_value() && result.type() == typeid(unsigned long long)) {
        std::cout << "Actual Stored Type: " << result.type().name() << "\n";
        // Safe to cast now
        std::cout << "Token: " << tok << ", Value: " << std::any_cast<unsigned long long>(result) << "\n";
    }
    else {
        // This block handles the case where Evaluation failed or type is wrong
        std::cout << "Evaluation failed or returned empty/wrong std::any.\n";
        // Add more diagnostics if needed
        if (result.has_value()) {
            std::cout << "Error: Expected unsigned long long, but std::any holds type: " << result.type().name() << "\n";
        }
        else {
            std::cout << "Error: std::any is empty (no matching language rule found).\n";
        }
    }

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
