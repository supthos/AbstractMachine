//  AbstractMachine.h : This file defines the AbstractMachine class, which represents a Turing-complete computational model. The AbstractMachine has a tape (modeled by the Substrate resource) and a state register (modeled by the States resource). It can interpret and execute programs written in its language, which is defined by the Language class. The AbstractMachine can be extended with additional resources and commands as needed.
//	Copyright © 2026 Guillermo M. Dávila Andino

#pragma once

#include "Language.h"


class Resource {
public:
	virtual ~Resource() = default;
	std::shared_ptr<Language<char8_t>> language;
	Medium<char8_t> name{};
	//Language<char8_t> language;
	//std::any resource;
};


class States : public Resource {
public:
	// command names
	std::set<Medium<char8_t>> ld = { u8"load", u8"ld" };
	std::set<Medium<char8_t>> ud = { u8"unload", u8"ud" };
	std::set<Medium<char8_t>> se = { u8"state", u8"se" };
	std::set<Medium<char8_t>> at = { u8"accept", u8"at" };

	// identifiers or parameters
	std::set<Medium<char8_t>> ne = { u8"name", u8"ne" };
	std::set<Medium<char8_t>> ag = { u8"accepting", u8"ag" };
	std::set<Medium<char8_t>> st = { u8"start", u8"st" };

	States(std::shared_ptr<Language<char8_t>> lang) {
		name = u8"States";

		language = lang;

		//language->AddCharacterInterpretations();
		language->InterpretIntegerArgumentMediumFunction(u8"load", ld, [this](const Medium<char8_t>& p) { return this->Load(p); }, name);
		language->InterpretIntegerArgumentMediumFunction(u8"unload", ud, [this](const Medium<char8_t>& p) { return this->Unload(p); }, name);


		language->InterpretMediumFunction(u8"accepting", ag, [this](const Medium<char8_t>& p) { return this->AcceptingSemantic(p); }, name);
		language->InterpretNullaryFunction(u8"state", se, [this]() { return this->State(); }, name);
	}


	enum StateKind : int {
		ER = -1, // Error state
		NL = 0, // Normal state
		AG = 1, // Accepting state
	};


	std::unordered_map<unsigned long long, Token<char8_t>> states; // Maps state numbers to their corresponding tokens (programs).
	std::hash<Token<char8_t>> hasher; // Maps tokens (programs) to their corresponding state numbers for quick lookup.

	// state 0 is the starting state by default 
	unsigned long long state = 0; // current state register
	unsigned long long icount = 0; // instruction counter within program
	std::vector<unsigned long long> instnum{}; //Instruction number stack
	std::vector<unsigned long long> previous{}; //Previous state stack for backtracking
	std::set<unsigned long long> accepting{};

	unsigned long long State() const { return state; }

	unsigned long long Instruction() const { return icount; }
	unsigned long long PreviousInstruction() const { return instnum.back(); }
	unsigned long long PreviousState() const { return previous.back(); }

	// return true if in accepting state
	bool Accepting() const { return accepting.contains(state); }
	bool Accepting(unsigned long st) const { return  accepting.contains(st); }

	// Load returns a pair of the state kind and the new state number. 
	std::pair<StateKind, unsigned long long> Load(const Token<char8_t>& program) {
		Medium<char8_t> prog = std::get<Medium<char8_t>>(program);
		StateKind kind = StateKind::NL;
		Medium<char8_t> name;
		unsigned long long new_state;

		language->Munch(prog); // Remove "load"

		if (at.contains(std::get<Medium<char8_t>>(ToLower(language->Lick(prog))))) {
			kind = StateKind::AG;
			language->Munch(prog); // Remove "accept"
		}

		if (!prog.empty()) {
			if (ne.contains(std::get<Medium<char8_t>>(ToLower(language->Lick(prog))))) {
				language->Munch(prog); // Remove "name"	
				if (!prog.empty()) {
					name = language->Munch(prog);
					if (!name.empty() && str_predicate(isalpha, name)) {
						new_state = hasher(name);
					}
					else {
						return std::make_pair(StateKind::ER, 0ULL); // ← add this
					}
				}
			}
			else if (st.contains(std::get<Medium<char8_t>>(ToLower(language->Lick(prog))))) {
				new_state = 0;
				//language->Munch(prog); // Remove "start"
				if (!prog.empty()) {
					states[new_state] = prog; // Store the remaining program as the state representation
				}
				else {
					states[new_state] = Medium<char8_t>{}; // Empty state representation
				}
				if (kind == StateKind::AG) {
					Accept(new_state);
				}
				return std::make_pair(kind, new_state);
			}
			else {
				new_state = hasher(prog);
			}
		}
		else {
			return std::make_pair(StateKind::ER, 0); // Invalid name
		}

		states[new_state] = prog;
		if (kind == StateKind::AG) {
			Accept(new_state);
		}
		return std::make_pair(kind, new_state);
	}

	unsigned long long Unload(Medium<char8_t> program) {
		Medium<char8_t> prog = program;
		language->Munch(prog); // Remove the command (e.g., "unload") to get the state identifier
		unsigned long long s;

		if (prog.empty()) {
			s = state;
		}
		else {
			prog = language->Munch(prog); // Get the next token which should be the state identifier
			if (!prog.empty()) {
				if (str_predicate(isalpha, prog)) {
					s = hasher(prog);
				}
				else if (str_predicate(isdigit, prog)) {
					//s = std::stoull(std::string(prog.begin(), prog.end()));
					std::string str(reinterpret_cast<const char*>(prog.data()), prog.size());
					s = std::stoull(str);
				}
				else return 0; // Invalid state identifier
			}
			else return 0; // Invalid state identifier
		}
		if (states.contains(s)) {
			states.erase(s);
			if (accepting.contains(s))
				accepting.erase(s);
			if (state == s) {
				state = previous.empty() ? 0 : previous.back();
				if (!previous.empty()) {
					previous.pop_back();
				}
			}
			return state;
		}
		return 0; // Invalid state
	}




	std::any AcceptingSemantic(Medium<char8_t> program) {
		Medium<char8_t> prog = program;
		language->Munch(prog); // Remove the command (e.g., "accepting") to get the state identifier
		if (!prog.empty()) {
			prog = language->Munch(prog); // Get the next token which should be the state identifier
			if (!prog.empty()) {
				if (str_predicate(isalpha, prog)) {
					unsigned long s = hasher(prog);
					return Accepting(s);
				}
				else if (str_predicate(isdigit, prog)) {
					//unsigned long s = std::stoull(std::string(prog.begin(), prog.end()));
					std::string str(reinterpret_cast<const char*>(prog.data()), prog.size());
					unsigned long s = std::stoull(str);
					return Accepting(s);
				}
			}
		}
		return Accepting();
	}

	// Mark a state as accepting
	bool Accept(unsigned long st) {
		if (states.contains(st)) {
			accepting.insert(st);
			return true;
		}
		else { return false; }
	}
};




// The Value V of the substrate is the type of the symbols on the tape. It can be any type that satisfies the Value concept, which includes primitive types (like char, int, bool) and user-defined types that can be constructed from a string representation. 
// The programs that are read are written in ProgramFile<char8_t>. The same as the AbstractMachine's language. The Substrate is a resource of the AbstractMachine, and it provides the basic operations that the machine can perform on its tape. 
template <Value V>
class Substrate : public Resource {
public:

	std::set<Medium<char8_t>> readcomms = { u8"read", u8"rd" };
	std::set<Medium<char8_t>> headcomms = { u8"head", u8"hd" };
	std::set<Medium<char8_t>> leftcomms = { u8"left", u8"lt" };
	std::set<Medium<char8_t>> rightcomms = { u8"right", u8"rt" };
	std::set<Medium<char8_t>> writecomms = { u8"write", u8"we" };
	std::set<Medium<char8_t>> gotocomms = { u8"goto", u8"go" };
	std::set<Medium<char8_t>> shrinkcomms = { u8"shrink", u8"sk" };
	std::set<Medium<char8_t>> movecomms = { u8"move", u8"me" };


	Medium<V> Tape;

	long long head;
	unsigned char order;


	Substrate(std::shared_ptr<Language<char8_t>> lang) {
		language = lang;

		name = u8"Tape";

		order = 16;
		head = 0;
		Tape = MakeTape(order);

		language->InterpretNullaryFunction(u8"read", readcomms, [this]() { return Read(); }, name);
		language->InterpretNullaryFunction(u8"head", headcomms, [this]() { return Head(); }, name);
		language->InterpretNullaryFunction(u8"left", leftcomms, [this]() { return Left(); }, name);
		language->InterpretNullaryFunction(u8"right", rightcomms, [this]() { return Right(); }, name);

		language->InterpretNullaryVoidFunction(u8"shrink", shrinkcomms, [this]() { Shrink(); }, name);

		language->Interpret(
			std::set<char8_t>{},
			u8"write",
			writecomms,
			[this](const Token<char8_t>& prog) { return this->WriteSyntax(prog); },
			[this](const Token<char8_t>& prog) { return this->WriteSemantic(prog); },
			name
		);

		language->InterpretIntegerArgumentLongLongFunction(u8"goto", gotocomms, [this](const long long& prog) { return this->GoTo(prog); }, name);
		language->InterpretIntegerArgumentLongLongFunction(u8"move", movecomms, [this](const long long& prog) { return this->Move(prog); }, name);
	}

	long long Head() const { return head; }

	std::any WriteSemantic(const Token<char8_t>& prog) {
		Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
		language->Munch(program); // Remove "write" command
		Medium<char8_t> valStr = language->Munch(program); // Get the data to write

		// Case 0: Tape stores bool values
		if constexpr (std::is_same_v<V, bool>) {
			valStr = std::get<Medium<char8_t>>(ToLower(valStr)); // Canonicalize input for boolean parsing
			if (valStr == u8"true" || valStr == u8"1") {
				return Write(true);
			}
			else if (valStr == u8"false" || valStr == u8"0") {
				return Write(false);
			}
			else {
				std::cout << "Invalid boolean value.\n";
				return std::any{};
			}
		}
		// Case 1: The Tape stores full Strings
		else if constexpr (String<V>) {
			// Program<V> is V, which is a string type.
			// We can pass valStr (Medium<char8_t>) directly or cast it.
			return Write(V(valStr));
		}
		// Case 2: The Tape stores single Characters
		else if constexpr (Char<V>) {
			if (valStr.size() == 1) {
				return Write(static_cast<V>(valStr[0]));
			}
			// Fallback for numeric codes (e.g., "65" -> 'A')
			try {
				std::string s(reinterpret_cast<const char*>(valStr.data()), valStr.size());
				return Write(static_cast<V>(std::stoi(s)));
			}
			catch (...) { return std::any{}; }
		}
		// Case 3: The Tape stores Numbers (int, long long, etc.)
		else if constexpr (Arithmetic<V>) {
			// Convert u8string to a char range for from_chars
			const char* first = reinterpret_cast<const char*>(valStr.data());
			const char* last = first + valStr.size();
			V numericVal = 0;

			auto [ptr, ec] = std::from_chars(first, last, numericVal);

			if (ec == std::errc{}) {
				return Write(numericVal);
			}
			else {
				if (ec == std::errc::invalid_argument)
					std::cout << "This is not a number.\n";
				else if (ec == std::errc::result_out_of_range)
					std::cout << "This number is larger than an int.\n";
				// Handle error: result was out of range or not a number
				return std::any{ ptr, ec }; // Return parsing result for debugging
			}
		}

		return std::any{};
	}

	unsigned long long WriteSyntax(const Token<char8_t>& prog) {
		if (!std::holds_alternative<Medium<char8_t>>(prog)) return 0;

		const Medium<char8_t>& medium = std::get<Medium<char8_t>>(prog);
		auto [commandToken, cmdConsumed] = language->Lunch(medium);

		// command must be a write command and there must be data after it
		if (!writecomms.contains(std::get<Medium<char8_t>>(ToLower(commandToken))) ) {
			//if (medium.size() <= cmdConsumed) //throw std::invalid_argument("No value provided to write\n");
				return 0;
		}

		// remaining buffer after the command
		Medium<char8_t> remaining(medium.begin() + static_cast<std::ptrdiff_t>(cmdConsumed), medium.end());
		auto [valueToken, valConsumed] = language->Lunch(remaining);

		if (valueToken.empty()) throw std::invalid_argument("No value provided to write\n");

		bool convertible = false;

		// --- Check convertibility to V without performing the write ---
		if constexpr (std::is_same_v<V, bool>) {
			// Use ToLower to canonicalize boolean strings
			Token<char8_t> vt = valueToken;
			auto lowered = ToLower(vt);
			auto& lowerMedium = std::get<Medium<char8_t>>(lowered);
			std::u8string lower_s(lowerMedium.begin(), lowerMedium.end());
			if (lower_s == u8"true" || lower_s == u8"1" || lower_s == u8"false" || lower_s == u8"0")
				convertible = true;
		}
		else if constexpr (String<V>) {
			// any token can be treated as a string-like V
			convertible = true;
		}
		else if constexpr (Char<V>) {
			// single character token OK
			if (valueToken.size() == 1) convertible = true;
			else {
				// numeric-code fallback, parse as signed integer then range-check for V
				std::string s;
				s.reserve(valueToken.size());
				for (char8_t c : valueToken) s.push_back(static_cast<char>(c));

				long long tmp = 0;
				auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), tmp);
				if (ec == std::errc{}) {
					using Target = std::remove_cv_t<V>;
					if constexpr (std::is_signed_v<Target>) {
						long long tmin = static_cast<long long>(std::numeric_limits<Target>::min());
						long long tmax = static_cast<long long>(std::numeric_limits<Target>::max());
						if (tmp >= tmin && tmp <= tmax) convertible = true;
					}
					else {
						if (tmp >= 0) {
							unsigned long long utmp = static_cast<unsigned long long>(tmp);
							unsigned long long umax = static_cast<unsigned long long>(std::numeric_limits<Target>::max());
							if (utmp <= umax) convertible = true;
						}
					}
				}
			}
		}
		else if constexpr (Arithmetic<V>) {
			// try parsing into numeric V
			std::string s;
			s.reserve(valueToken.size());
			for (char8_t c : valueToken) s.push_back(static_cast<char>(c));
			V numericVal = 0;
			auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), numericVal);
			if (ec == std::errc{}) convertible = true;
		}
		else if constexpr (Defined<V>) {
			// try constructing V from the token
			try {
				std::u8string s(valueToken.begin(), valueToken.end());
				(void)V(s);
				convertible = true;
			}
			catch (...) {
				convertible = false;
			}
		}

		// return total consumed length (command + value) when convertible, else 0
		return convertible ? (cmdConsumed + valConsumed) : 0;
	}

	//friend class AbstractMachine;
	Medium<V> MakeTape(const unsigned char& k) {
		//if (k >= (sizeof(unsigned) * 8)) throw std::overflow_error("Tape order too large");
		if (k >= 64) throw std::overflow_error("Tape order too large");
		std::size_t size = std::size_t(1) << k;
		if constexpr (requires { typename V::inner_type; }) {
			using Inner = typename V::inner_type;
			if constexpr (std::is_arithmetic_v<Inner>) {
				if constexpr (std::is_same_v<Medium<V>, std::valarray<Inner>>) {
					return Medium<V>(Inner{}, size); // valarray(value, n) accepted
				}
				else {
					return Medium<V>(size, Inner{}); // vector(size, init)
				}
			}
			else {
				return Medium<V>(size, Inner{});
			}
		}
		else if constexpr (Text<V>) {
			return Medium<V>(size, V{});
		}
		else if constexpr (Arithmetic<V>) {
			return Medium<V>(V{}, size);
		}

		else {
			return Medium<V>(size);
		}
	}



	V Read() {
		std::int64_t zero = static_cast<std::int64_t>(Tape.size()) / 2;
		std::int64_t idx = head + zero;
		if (idx < 0 || static_cast<std::size_t>(idx) >= Tape.size()) {
			MoreTape();
			zero = static_cast<std::int64_t>(Tape.size()) / 2;
			idx = head + zero;
		}
		if constexpr (requires { typename V::inner_type; }) {
			return V(Tape[static_cast<std::size_t>(idx)]);
		}
		else {
			return Tape[static_cast<std::size_t>(idx)];
		}
	}

	bool Write(const Program<V>& a) {
		std::int64_t zero = static_cast<std::int64_t>(Tape.size()) / 2;
		std::int64_t idx = head + zero;
		if (idx < 0 || static_cast<std::size_t>(idx) >= Tape.size()) {
			MoreTape();
			zero = static_cast<std::int64_t>(Tape.size()) / 2;
			idx = head + zero;
		}
		if constexpr (requires { typename V::inner_type; }) {
			Tape[static_cast<std::size_t>(idx)] = a.value;
			return true;
		}
		else {
			Tape[static_cast<std::size_t>(idx)] = a;
			return true;
		}
		//return false;
	}


	bool Left() {
		if (--head < -(1LL << (order - 1))) {
			if (MoreTape() == false)
				return false;
		}
		if (head <= std::numeric_limits<long long>::max() && head >= std::numeric_limits<long long>::min())
			return true;
		else
			return false;
	}

	bool Right() {
		long long zero = 1LL << (order - 1);
		if (++head >= zero) {
			if (MoreTape() == false)
				return false;
		}
		if (head <= std::numeric_limits<long long>::max() && head >= std::numeric_limits<long long>::min())
			return true;
		else
			return false;
	}



	bool Move(const long long& c) {
		long long zero = 1LL << (order - 1);
		while ((zero + c + head) >= static_cast<long long>(Tape.size())) {
			if (MoreTape() == false)
				return false;
		}
		head += c;
		if (head <= std::numeric_limits<long long>::max() && head >= std::numeric_limits<long long>::min())
			return true;
		else
			return false;
	}

	bool GoTo(const long long& s) {
		long long zero = 1LL << (order - 1);
		while (zero + s >= static_cast<long long>(Tape.size())) {
			if (MoreTape() == false)
				return false;
		}
		head = s;
		if (head <= std::numeric_limits<long long>::max() && head >= std::numeric_limits<long long>::min())
			return true;
		else
			return false;
	}
	void NewTape(unsigned char n) {
		Tape = MakeTape(n);
		//zero = Tape.size() / 2;
		order = n;
	}

	bool MoreTape() {
		//if (order + 1 >= (sizeof(unsigned char) * 8)) { // max order = 255 for unsigned char
		if (order + 1 >= 64) { // max order = 63 for unsigned long long, which is the largest we can use for indexing the tape
			std::cerr << "Max tape order reached\n";
			return false;
		}
		std::size_t oldSize = Tape.size();
		std::size_t newSize = oldSize * 2;
		Medium<V> VTape = MakeTape(order + 1); // makes newSize
		std::size_t oldZero = oldSize / 2;
		std::size_t newZero = newSize / 2;
		for (std::size_t i = 0; i < oldSize; ++i) {
			VTape[i + newZero - oldZero] = Tape[i];
		}
		Tape = std::move(VTape);
		++order;
		return true;
	}

	void Shrink() {
		long long zero = 1LL << (order - 1);
		long long minIndex = static_cast<long long>(Tape.size()) - 1;
		long long maxIndex = -1;

		// Find the bounds of non-default values
		for (long long i = 0, n = static_cast<long long>(Tape.size()); i < n; ++i) {
			V val;
			if constexpr (requires { typename V::inner_type; }) {
				val = V(Tape[i]);
			}
			else {
				val = Tape[i];
			}
			if (!(val == V{})) {
				minIndex = std::min(minIndex, i);
				maxIndex = std::max(maxIndex, i);
			}
		}

		// If no non-default values found, reset to minimal tape
		if (maxIndex < minIndex) {
			NewTape(1);
			head = 0;
			return;
		}

		long long newSize = maxIndex - minIndex + 1;
		unsigned char newOrder = static_cast<unsigned>(std::ceil(std::log2(static_cast<double>(newSize))));
		Medium<V> newTape = MakeTape(newOrder);
		long long newZero = 1LL << (newOrder - 1);

		for (long long i = minIndex; i <= maxIndex; ++i) {
			newTape[i - minIndex + newZero] = Tape[i];
		}

		head = head - minIndex;
		Tape = std::move(newTape);
		order = newOrder;
	}

};


// Here, we define Abstract Machine to use a language over char8_t
// but we could easily refactor to any Value V.
//template <Value V>
class AbstractMachine {
public:
	// friend class States;
	//Language<char8_t> language;
	std::shared_ptr<Language<char8_t>> language;
	std::vector<std::unique_ptr<Resource>> Resources;

	//std::vector<Token<char8_t>> ResourceRegistry;

	Substrate<bool>* Tape;
	States* StateRegister;

	std::set<Medium<char8_t>> RunComms = { u8"run", u8"rn" };
	std::set<Medium<char8_t>> TapeComms = { u8"tape", u8"te" };
	std::set<Medium<char8_t>> StateComms = { u8"state", u8"se" };

	//std::set<Medium<char8_t>> sm = { u8"system", u8"sm" };
	std::set<Medium<char8_t>> ng = { u8"nothing", u8"ng" };
	std::set<Medium<char8_t>> st = { u8"start", u8"st" };
	std::set<Medium<char8_t>> cl = { u8"call", u8"cl" };
	std::set<Medium<char8_t>> ed = { u8"end", u8"ed" };
	std::set<Medium<char8_t>> rt = { u8"reset", u8"re" };
	std::set<Medium<char8_t>> bh = { u8"branch", u8"bh" };

	Medium<char8_t> name = u8"Abstract Machine";

	typedef bool TapeSymbol;

	void Initialize() {
		language = std::make_shared<Language<char8_t>>();
		language->AddCharacterInterpretations();
		language->AddTypeInterpretations();

		language->InterpretMediumFunction(u8"run", RunComms, [this](const Medium<char8_t>& prog) { return this->Run(prog); }, name);

		/*language->InterpretMediumFunction(u8"system", sm, [this](const Medium<char8_t>& p) {
			std::string command(p.begin(), p.end());
			this->System(command);
			return std::any{};},
			name
		);*/

		language->InterpretNullaryVoidFunction(u8"nothing", ng, [this]() { this->Nothing(); }, name);

		language->Interpret(
			std::set<Program<char8_t>>{},
			u8"start",
			st,
			[this](const Token<char8_t>& prog) { return this->StartSyntax(prog); },
			[this](const Token<char8_t>& prog) { return this->StartSemantic(std::get<Medium<char8_t>>(prog)); },
			name
		);

		language->InterpretNullaryVoidFunction(u8"end", ed, [this]() { this->End(); }, name);

		language->Interpret(
			std::set<Program<char8_t>>{},
			u8"call",
			cl,
			[this](const Token<char8_t>& prog) { return this->CallSyntax(prog); },
			[this](const Token<char8_t>& prog) { return this->CallSemantic(std::get<Medium<char8_t>>(prog)); },
			name
		);

		language->InterpretNullaryVoidFunction(u8"reset", rt, [this]() { this->Reset(); }, name);

		language->Interpret(
			std::set<Program<char8_t>>{},
			u8"branch",
			bh,
			[this](const Token<char8_t>& prog) { return this->BranchSyntax<TapeSymbol>(prog); },
			[this](const Token<char8_t>& prog) { return this->BranchSemantic<TapeSymbol>(prog); },
			name
		);

		AddResource(std::make_unique<Substrate<bool>>(language));
		AddResource(std::make_unique<States>(language));

		Tape = static_cast<Substrate<TapeSymbol>*>(Resources[0].get());
		StateRegister = static_cast<States*>(Resources[1].get());
	}

	AbstractMachine() {
		Initialize();
		Start();
	}

	AbstractMachine(const unsigned long& tape_order) {
		Initialize();
		Start(tape_order);
	}

	AbstractMachine(const Token<char8_t>& program) : AbstractMachine() {
		LoadAndRun(program);
	}

	AbstractMachine(const ProgramFile<char8_t>& file) : AbstractMachine() {
		LoadAndRun(file);
	}

	AbstractMachine(const unsigned long& tape_order, const Token<char8_t>& program) : AbstractMachine(tape_order) {
		LoadAndRun(program);
	}

	AbstractMachine(const unsigned long& tape_order, const ProgramFile<char8_t>& file) : AbstractMachine(tape_order) {
		LoadAndRun(file);
	}

	virtual ~AbstractMachine() = default;

	/*void System(std::string command) {
		system(command.c_str());
	}*/

	bool is_resource(const Medium<char8_t>& prog) const {
		if (language->is_registered(prog, u8"Resource")) {
			auto [Concept_Ptr, consumed, cntxt] = language->is_well_formed(prog, u8"Resource");
			if (std::get<Medium<char8_t>>(std::get<0>(*Concept_Ptr)) == prog) {
				return true;
			}
		}
		return false;
	}

	ProgramFile<char8_t> ChopLine2(Medium<char8_t> prog) const {
		ProgramFile<char8_t> pf;
		Medium<char8_t> line;
		Medium<char8_t> inst;
		Medium<char8_t> program = prog;
		bool registered = false;
		while (!program.empty()){
			inst = language->Munch(program);

			if (std::get<Medium<char8_t>>(ToLower(inst)) == u8"load" || std::get<Medium<char8_t>>(ToLower(inst)) == u8"ld") {
				pf.push_back(prog);
				break;
			}

			if (!language->is_registered_any(inst).empty()) {
				line = inst;
				while (!program.empty()) {
					inst = language->Munch(program);
					registered = !language->is_registered_any(inst).empty();
					if (!registered && !inst.empty()) {
						line = line + u8" " + inst;
					}
					else {
						registered = false;
						pf.push_back(line);
						line.clear();
						if (!inst.empty())
							line = inst;
					}
				}
				if (!line.empty()) {
					pf.push_back(line);
				}
			}
			else {
				pf.push_back(inst);
			}
		}
		return pf;
	}

	std::vector<std::tuple<Token<char8_t>, std::any, unsigned long long>> Run(const Medium<char8_t>& prog) {
		//unsigned long long retval = unsigned long long(true);

		//StateRegister->icount = 0;

		std::vector<std::tuple<Token<char8_t>, std::any, unsigned long long>> results;
		Medium<char8_t> program{};

		ProgramFile<char8_t> pf = ChopLine2(prog);
		for (auto& line : pf) {

			auto contexts = language->is_well_formed_context(line);

			unsigned i = 0;

			if (contexts.empty()) {
				std::cerr << "No interpretation found for: "
					<< reinterpret_cast<const char*>(line.c_str()) << "\n";
				continue;
			}
			else if (contexts.size() > 1) {
				for (auto it = contexts.begin(); it != contexts.end(); it++) {
					if ((*it) == u8"Literal") {
						it = contexts.erase(it);
						break;
					}
				}
				if (contexts.size() > 1) {
					std::cout << "Choose a non-trivial Context for interpretation.\n";
					for (const auto&  ctx : contexts) {
						std::cout << i++ << ": " << ctx << "\n";
					}
					std::cout << "# ";
					std::cin >> i;
				}
			}

			auto [Concept_Ptr, consumed, context] = language->has_interpretation(line, contexts[i]);

			if (consumed > 0 && Concept_Ptr != nullptr) {
				program = Medium<char8_t>(line.begin(), line.begin() + consumed);

				results.push_back(std::make_tuple(std::get<0>(*Concept_Ptr), language->Evaluate(*Concept_Ptr, program), consumed));
			}

			if (consumed > 0 && consumed < line.size()) {
				program = Medium<char8_t>(line.begin() + consumed, line.end());
				auto subresults = Run(program);
				results.insert(results.end(), subresults.begin(), subresults.end());
			}
			consumed = 0;
			for (const auto& [concepts, value, length] : results) {
				consumed += length;
			}
			if (consumed < line.size()) {
				program = Medium<char8_t>(line.begin() + consumed, line.end());
				if (!str_predicate(isspace, program)) {
					throw std::invalid_argument("Unconsumed input remaining after evaluation\n");
					return {};
				}
			}
		}
		return results;
	}

	void AddResource(std::unique_ptr<Resource> res) {
		if (language->is_word(res->name) && !language->is_registered(res->name, u8"Resource")) {
			Resources.push_back(std::move(res));
			auto resPtr = Resources.back().get();

			Medium<char8_t> t = resPtr->name;

			language->Interpret(t, *resPtr, u8"Resource");

		}
	}

	void RemoveResource(const Medium<char8_t>& name) {


		if (is_resource(name)) {

			for (auto it = Resources.begin(); it != Resources.end(); it++) {
				if ((*it)->name == name) {
					//Res = std::move(*it);
					Resources.erase(it);
					break;
				}
			}
			for (auto it = language->RegisteredNames[u8"Resource"].begin(); it != language->RegisteredNames[u8"Resource"].end(); it++) {
				if ((*it).contains(name)) {
					language->RegisteredNames[u8"Resource"].erase(it);
					break;
				}
			}

			language->C.erase(name);

			if (name == u8"Tape") {
				Tape = nullptr;
			}
			else if (name == u8"StateRegister") {
				StateRegister = nullptr;
			}

		}

	}

	void Start() {
		if (!is_resource(u8"Tape")) {
			AddResource(std::make_unique<Substrate<bool>>(language));
			Tape = static_cast<Substrate<TapeSymbol>*>(Resources.back().get());
		}
		if (!is_resource(u8"States")) {
			AddResource(std::make_unique<States>(language));
			StateRegister = static_cast<States*>(Resources.back().get());
		}

		Tape->head = StateRegister->state = StateRegister->icount = 0;

		Tape->NewTape(Tape->order);

		StateRegister->states.clear();
		StateRegister->instnum.clear();
		StateRegister->previous.clear();

		StateRegister->Load(u8"load name null nothing");
	}
	void Start(unsigned long n) {
		if (!is_resource(u8"Tape")) {
			AddResource(std::make_unique<Substrate<bool>>(language));
			Tape = static_cast<Substrate<TapeSymbol>*>(Resources.back().get());
		}
		if (!is_resource(u8"States")) {
			AddResource(std::make_unique<States>(language));
			StateRegister = static_cast<States*>(Resources.back().get());
		}

		Tape->head = StateRegister->state = StateRegister->icount = 0;

		Tape->NewTape(n);

		StateRegister->states.clear();
		StateRegister->instnum.clear();
		StateRegister->previous.clear();
		StateRegister->Load(u8"load name null nothing");
	}

	unsigned long long StartSyntax(const Token<char8_t>& program) {
		if (!std::holds_alternative<Medium<char8_t>>(program)) return 0;
		Medium<char8_t> prog = std::get<Medium<char8_t>>(program);
		if (st.contains(language->Munch(prog))) {

			if (!prog.empty()) {
				Medium<char8_t> arg = language->Munch(prog);

				if (str_predicate(isdigit, arg)) {
					return std::get<Medium<char8_t>>(program).size() - prog.size();
				}
			}
			else {
				return std::get<Medium<char8_t>>(program).size();
			}
		}
		else return 0;
	}

	std::any StartSemantic(Medium<char8_t> program) {
		Medium<char8_t> prog = program;
		language->Munch(prog); // Remove "start" command
		if (!prog.empty()) {
			prog = language->Munch(prog); // Get the next token which should be the tape order
			if (!prog.empty()) {
				if (str_predicate(isdigit, prog)) {
					//unsigned long n = std::stoull(std::string(prog.begin(), prog.end()));
					std::string str(reinterpret_cast<const char*>(prog.data()), prog.size());
					unsigned long n = std::stoull(str);
					Start(n);
				}
			}
		}
		else Start();
		return {};
	}

	void Nothing() {}

	void End() {
		long long zero = 1LL << (Tape->order - 1);
		std::string mess = "";
		std::string lt = "", rt = "";
		long long block = Tape->head / 64;
		for (unsigned i = 0; i < 64; i++) {
			if (Tape->Tape[(zero / 64 + block - 1) * 64 + i] == false) lt += "0";
			else lt += "1";
			if (Tape->Tape[(zero / 64 + block + 1) * 64 - i - 1] == false) rt += "0";
			else rt += "1";
		}

		if (Tape->head >= 0) {
			for (int i = 0; i < 63 - ((Tape->head) % 64); i++) {
				mess += " ";
			}
			mess += "_";
		}
		else if (Tape->head < 0) {
			for (int i = -1; i > -64 - ((Tape->head + 1) % 64); i--) {
				mess += " ";
			}
			mess += "^";
		}


		std::cout << "State of the machine: \n"
			<< lt << std::endl
			<< mess << std::endl
			<< rt << std::endl;
		std::cout << "Head: " << Tape->head << std::endl;
		std::cout << "State: " << StateRegister->state << std::endl;
		std::cout << "Count: " << StateRegister->icount << std::endl;
		std::cout << "Trail: ";

		for (auto a : StateRegister->previous) {
			std::cout << a << " ";
		}
		std::cout << "\n"
			<< "Route: ";
		for (auto a : StateRegister->instnum) {
			std::cout << a << " ";
		}
		std::cout << "\n"
			<< "States: " << StateRegister->states.size() << std::endl;
	}

	std::any Call(unsigned long long state) {
		std::any retval = false;
		if (StateRegister->states.contains(state)) {
			StateRegister->previous.push_back(StateRegister->state);
			StateRegister->instnum.push_back(StateRegister->icount);
			StateRegister->state = state;

			retval = Run(std::get<Medium<char8_t>>(StateRegister->states[state]));

			StateRegister->state = StateRegister->previous.back();
			StateRegister->previous.pop_back();
			StateRegister->icount = StateRegister->instnum.back();
			StateRegister->instnum.pop_back();
		}
		else std::cerr << "State not in memory";
		return retval;
	}

	unsigned long long CallSyntax(const Token<char8_t>& program) {
		if (!std::holds_alternative<Medium<char8_t>>(program)) return 0;
		Medium<char8_t> prog = std::get<Medium<char8_t>>(program);
		if (cl.contains(language->Munch(prog)) && !prog.empty()) {
			Medium<char8_t> state = language->Munch(prog);
			if (!state.empty()) {
				if (str_predicate(isalpha, state) || str_predicate(isdigit, state)) {
					return std::get<Medium<char8_t>>(program).size() - prog.size();
				}
			}
		}
		return 0;
	}

	std::any CallSemantic(const Medium<char8_t>& program) {
		Medium<char8_t> prog = program;
		language->Munch(prog);
		if (!prog.empty()) {
			prog = language->Munch(prog);
			if (!prog.empty()) {
				if (str_predicate(isalpha, prog)) {
					return Call(StateRegister->hasher(prog));
				}
				else if (str_predicate(isdigit, prog)) {
					std::string str(reinterpret_cast<const char*>(prog.data()), prog.size());
					return Call(std::stoull(str));
					//return Call(std::stoull(std::string(prog.begin(), prog.end())));
				}
			}
		}
		return false;
	}

	// In AbstractMachine, before BranchSyntax
	template<Value V>
	static V ParseValue(const Medium<char8_t>& token) {
		if constexpr (Defined<V>) {
			// Defined types must be constructible from u8string (per concept)
			return V(token);
		}
		else if constexpr (std::same_as<V, bool>) {
			return token == u8"true" || token == u8"1";
		}
		else if constexpr (std::is_integral_v<V>) {
			std::string s(reinterpret_cast<const char*>(token.data()), token.size());
			return static_cast<V>(std::stoll(s));
		}
		else if constexpr (std::is_floating_point_v<V>) {
			std::string s(reinterpret_cast<const char*>(token.data()), token.size());
			return static_cast<V>(std::stod(s));
		}
		else if constexpr (Char<V>) {
			return token.empty() ? V{} : static_cast<V>(token[0]);
		}
		else {
			return V{};
		}
	}

	template<Value V>
	unsigned long long BranchSyntax(const Token<char8_t>& prog) {
		if (!std::holds_alternative<Medium<char8_t>>(prog)) return 0;
		Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
		if (bh.contains(std::get<Medium<char8_t>>(ToLower(language->Munch(program))))) {
			if (!program.empty()) {
				language->Munch(program);
				if (!program.empty() ) {
					Medium<char8_t> truebranch = language->Munch(program);
					if (!program.empty() && (str_predicate(isalpha, truebranch) || str_predicate(isdigit, truebranch))) {
						Medium<char8_t> falsebranch = language->Munch(program);
						if ((str_predicate(isalpha, falsebranch) || str_predicate(isdigit, falsebranch))) {
							return std::get<Medium<char8_t>>(prog).size() - program.size() ;
						}
					}
					
				}
			}
		}
		return 0;
	}

	template<Value V>
	std::any BranchSemantic(const Token<char8_t>& program) {
		Medium<char8_t> prog = std::get<Medium<char8_t>>(program);
		language->Munch(prog);
		if (!prog.empty()) {
			V cond = ParseValue<V>(language->Munch(prog));
			bool conditionMet = false;

			if (cond == Tape->Read()) conditionMet = true;
			else conditionMet = false;

			Medium<char8_t> truebranch = language->Munch(prog);
			Medium<char8_t> falsebranch = language->Munch(prog);

			if (conditionMet) {
				return CallSemantic(u8"call " + truebranch);
			}
			else {
				return CallSemantic(u8"call " + falsebranch);
			}

		}
		return false;
	}

	std::any LoadAndRun(Token<char8_t> program) {
		unsigned long ld = StateRegister->Load(program).second;
		if (StateRegister->states.contains(ld)) {
			return Run((std::get<Medium<char8_t>>(StateRegister->states[ld])));
		}
		else return false;
	}
	std::any LoadAndRun(ProgramFile<char8_t> file) {
		std::vector<unsigned long> StateStack;
		std::vector<std::any> results;
		for (Medium<char8_t> line : file) {
			StateStack.push_back(StateRegister->Load(line).second);
		}
		for (unsigned long st : StateStack) {
			if (StateRegister->states.contains(st)) {
				results.push_back(Run((std::get<Medium<char8_t>>(StateRegister->states[st]))));
			}
			else {
				results.push_back(false);
			}
		}
		return results;
	}

	void Reset() {
		Start();
		End();
	}
};



