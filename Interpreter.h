//	Interpreter.h : This file defines the Interpreter class, which provides a Read-Eval-Print Loop (REPL
//  Copyright © 2026 Guillermo M. Dávila Andino

#pragma once

#include <print>
#include <iostream>
#include "Language.h"

class Interpreter {
public:
	AbstractMachine* A;

	Medium<char8_t> context = u8"Interpreter";

	std::set<Medium<char8_t>> hp = { u8"help", u8"hp" };

	Medium<char8_t> HelpExpl() {
		return u8"Displays the available concepts in the Abstract Machine's language. {help|hp}";
	}

	Interpreter(AbstractMachine* a) {
		A = a;
		A->language->InterpretNullaryVoidFunction(u8"help", hp, [this]() { return this->Help(); }, HelpExpl(), context );
	}

	Medium<char8_t> CurrentLine;


	void Help() {
		std::print("The following concepts are available:\n");
		for (auto& [cntxtName, I]: A->language->C){
			std::cout << cntxtName << ":\n";
		//for (auto Cit = A->language->C.rbegin(); Cit != A->language->C.rend(); Cit++) {
			//const Language<char8_t>::Interpretation& I = (*Cit).second;
			for (const auto& c : I) {
				std::cout << "    " << std::get<0>(c) << std::endl;
				auto expl = std::get<3>(c);
				if (expl != u8"") {
					std::cout << "        " << expl << std::endl;
				}

			}
		}
	}

	

	void REPL() {
		bool exit = false;

		
		while (!exit) {
			// Read
			std::cout << std::endl << "> ";
			getline(std::cin, CurrentLine);

			// Eval
			if (u8"exit" == std::get<Medium<char8_t>>(ToLower(A->language->Lick(CurrentLine)))) {
				exit = true;
			}
			else {
				// Eval
				A->Run(CurrentLine);

				//A->StateRegister->Unload(u8"temp");

				//auto eval = A->Run(CurrentLine);

				//// Print
				//for (auto [Concept_Ptr, result, prog] : eval) {
				//	auto tok = std::get<0>(Concept_Ptr);

				//	if (result.has_value() && result.type() == typeid(Token<char8_t>)) {
				//		std::cout << "Actual Stored Type: " << result.type().name() << "\n";
				//		// Safe to cast now
				//		std::cout << "Token: " << tok << ", Value: " << std::any_cast<Token<char8_t>>(result) << "\n";
				//	}
				//	else {
				//		// This block handles the case where Evaluation failed or type is wrong
				//		std::cout << "Evaluation failed or returned empty/wrong std::any.\n";
				//		// Add more diagnostics if needed
				//		if (result.has_value()) {
				//			std::cout << "Error: Expected Token<char8_t>, but std::any holds type: " << result.type().name() << "\n";
				//		}
				//		else {
				//			std::cout << "Error: std::any is empty (no matching language rule found).\n";
				//		}
				//	}
				//}


			}

			

		}
	}
};