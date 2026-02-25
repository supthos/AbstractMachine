#pragma once

#include <iostream>
#include <set>
#include <any>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <functional>

#include <cmath>
#include <valarray>
#include <concepts>
#include <typeinfo>
#include <variant>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <climits> // for UCHAR_MAX
#include <codecvt>
#include <locale>
#include <charconv>
#include <string_view>
#include <unordered_map>



template<typename T>
concept String =
std::same_as<T, std::string> ||
std::same_as<T, std::wstring> ||
std::same_as<T, std::u8string> ||
std::same_as<T, std::u16string> ||
std::same_as<T, std::u32string>;

template<typename T>
concept Char =
std::same_as<T, char> || // type for character representation which can be most efficiently processed on the target system 
std::same_as<T, wchar_t> ||  // defective inconsistencies across platforms
std::same_as<T, signed char> ||  // type for signed character representation
std::same_as<T, unsigned char> || // type for unsigned character representation. Also used to inspect object representations (raw memory).
std::same_as<T, char8_t> ||  // type for UTF-8 character representation
std::same_as<T, char16_t> || //type for UTF-16 character representation
std::same_as<T, char32_t>; 	// type for UTF-32 character representation

// This concept holds all the representations of textual types.
template <typename T>
concept Text = Char<T> || String<T>;

// For the abstract machine, a program is any kind of text.
// With a single char type variable, for example, we can encode up to 2^8 instructions.
// So, we say that instruction represents a program.
template <Text T>
using Program = T;


template<typename A>
concept Arithmetic = std::is_arithmetic_v<A>;

template<typename P>
concept Primitive = Text<P> || Arithmetic<P> ;

// This template allows us to define arbitrary types as long as we provide the mechanisms to construct it from a string
template<typename D>
concept Defined = std::regular<D> && std::constructible_from<D, std::u8string>;

template <typename V>
concept Value = Primitive<V> || Defined<V> ;

template<typename V>
concept HasInner = Value<V> && requires{ typename V::inner_type; };

// General Case (No inner_type)
template<Value V>
struct MediumHelper {
	using type = std::conditional_t<Char<V>,
		std::basic_string<V>,
		std::conditional_t<Arithmetic<V>, std::valarray<V>, std::vector<V>>>;
};

// Specialization for types with inner_type (like Symbol)
template<Value V>
	requires HasInner<V>
struct MediumHelper<V> {
	using type = std::conditional_t< Arithmetic<typename V::inner_type>,
		std::valarray<typename V::inner_type>,
		std::vector<typename V::inner_type>
	>;
};

template <Value V>
using Medium = typename MediumHelper<V>::type;

// We represent a program file as, essentially, a vector of strings.
// These objects are inherently 3-dimensional. They have volume.
template <Value V>
using ProgramFile = std::vector<Medium<V>>;

// The Symbol struct represents a symbol in the language, which can hold a value of an arithmetic type and can be constructed from a program (text) representation. It also provides an implicit conversion to the underlying value type and to the program representation.
// It unifies the handling of conversion of arithmetic types to text. I guess it is a form of serialization.
template <Arithmetic A, Text T>
struct Symbol {
	A value;
	using inner_type = A;
	Symbol(const A& val) : value(val) {}

	// ... (operator== and default constructor are correct) ...
	Symbol() : value(A{}) {}
	bool operator==(const Symbol& other) const noexcept {
		return value == other.value;
	}
	bool operator==(const A& other) const noexcept {
		return value == other;
	}

	// 2. Input Requirement: V(Program) constructor (Corrected)
	Symbol(const Program<T>& p) {
		if (p.empty()) {
			value = A{};
			return;
		}

		if constexpr (std::is_same_v<A, bool>) {
			// FIX: Use robust string comparison
			std::u8string lower_p;
			std::transform(p.begin(), p.end(), std::back_inserter(lower_p), ::tolower);
			value = (lower_p == u8"true" || lower_p == u8"1");
		}
		// else if constexpr (std::is_integral_v<A>) {
		// 	// FIX: Add try/catch and explicit cast
		// 	try {
		// 		value = static_cast<A>(std::stoll(p));
		// 	}
		// 	catch (const std::exception&) {
		// 		value = A{}; // Fallback to zero
		// 	}
		// }
		// else if constexpr (std::is_floating_point_v<A>) {
		// 	// FIX: Add try/catch and explicit cast
		// 	try {
		// 		value = static_cast<A>(std::stod(p));
		// 	}
		// 	catch (const std::exception&) {
		// 		value = A{}; // Fallback to zero
		// 	}
		// }
		// else {
		// 	value = A{};
		// }
		else if constexpr (std::is_arithmetic_v<A>) {
			// std::from_chars requires a raw char range
			const char* first = reinterpret_cast<const char*>(p.data());
			const char* last = first + p.size();
			
			auto [ptr, ec] = std::from_chars(first, last, value);

			// If parsing fails (e.g., "abc" for an int), fall back to default
			if (ec != std::errc{}) {
				value = A{}; 
			}
		}
		else {
			value = A{};
		}
	}

	// 3. Output Requirement: Implicit conversion operator
	/*operator Program<T>() const {
		return static_cast<T>(std::to_string(value));
	}*/
	// operator Program<T>() const {
	// 	std::u8string temp = std::to_string(value);
	// 	if constexpr (std::is_same_v<T, std::u8string>) {
	// 		return temp;
	// 	}
	// 	// If T is another string type, proper conversion (e.g., std::wstring(temp.begin(), temp.end())) is required here.
	// 	// If T is a Char type, the conversion is likely nonsensical.
	// 	return static_cast<T>(temp); // Still problematic but illustrates the intent.
	// }
	operator Program<T>() const {
		if constexpr (std::is_same_v<A, bool>) {
			// Explicitly return string representations for boolean types
			if constexpr (String<T>) {
				return value ? T(u8"true") : T(u8"false");
			} else {
				return value ? static_cast<T>('1') : static_cast<T>('0');
			}
		} 
		else if constexpr (std::is_arithmetic_v<A>) {
			char buffer[64]; 
			auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
			
			if (ec == std::errc{}) {
				std::u8string temp(reinterpret_cast<const char8_t*>(buffer), ptr - buffer);
				
				// Branch 1: T is a full string type
				if constexpr (String<T>) {
					return T(temp.begin(), temp.end());
				}
				// Branch 2: T is a single character type
				else if constexpr (Char<T>) {
					if (temp.size() == 1) {
						return static_cast<T>(temp[0]);
					}
					// If it's a multi-digit number being forced into one char,
					// we interpret the numeric value itself as the character code.
					return static_cast<T>(value);
				}
			}
		}
		return Program<T>{}; 
	}
	operator A() const {
		return value;
	}
};

template <Value V>
using Token = std::variant<Medium<V>, Program<V>>;


template <Text V>
std::ostream& operator<<(std::ostream& os, const Token<V>& tok) {
	if (std::holds_alternative<Program<V>>(tok))
    	return os << reinterpret_cast<const char*>(std::get<Program<V>>(tok));
	if (std::holds_alternative<Medium<V>>(tok))
		return os << reinterpret_cast<const char*>(std::get<Medium<V>>(tok).c_str());
	throw std::runtime_error("Invalid token type for output");
}

Token<char8_t> ToLower (Token<char8_t> text){
	if (std::holds_alternative<Program<char8_t>>(text)){
		return static_cast<char8_t>(std::tolower(static_cast<unsigned char>(std::get<Program<char8_t>>(text))));
	}
	if (std::holds_alternative<Medium<char8_t>>(text)){
		Medium<char8_t>& str = std::get<Medium<char8_t>>(text);
		std::transform(str.begin(), str.end(), str.begin(),
			[](char8_t c){ return static_cast<char8_t>(std::tolower(static_cast<unsigned char>(c))); });
		return text;
	}
	return text;
}


bool str_predicate(int(*predicate)(int), Token<char8_t> token){
	if (std::holds_alternative<Program<char8_t>>(token)){
		return predicate(static_cast<unsigned char>(std::get<Program<char8_t>>(token))) ;
	}
	if (std::holds_alternative<Medium<char8_t>>(token)){
		for (char8_t c : std::get<Medium<char8_t>>(token) ){
			if (!predicate(static_cast<unsigned char>(c)))
				return false;
		}
		return true;
	}
	return false;
}

std::set<char8_t> GetCharacterSet(int (*predicate)(int)) {
    std::set<char8_t> result;
    for (int i = 0; i <= UCHAR_MAX; ++i) {
        if (predicate(static_cast<unsigned char>(i))) {
            result.insert(static_cast<char8_t>(i));
        }
    }
    return result;
}

// The Language struct represents a formal language defined by an alphabet and a set of interpretations (concepts). It provides methods to add symbols, check if a program is well-formed, and evaluate programs based on the defined syntax and semantics.
template <Value V>
class Language {
	using Alphabet = std::set<Program<V>>;

	
	using Syntax = std::function<bool(const Token<V>&)>;
	using Semantic = std::function<std::any (const Token<V>&)>;
	//using Semantic = std::function<Value auto (const Medium<V>&)>;
	using Concept = std::tuple<Token<V>, Syntax, Semantic>;
	using Interpretation = std::vector<Concept>;

	public:
	Alphabet A;
	Interpretation I;

	Language() {

		// order matters, so we must be topological about this
		// the first we list here have least precedence, the last have most precedence
		// When listing interpretation, do so from most general to most specialized.
		InterpretPredicate(std::iscntrl, u8"control");
		InterpretPredicate(std::isprint, u8"printable");
		InterpretPredicate(std::isgraph, u8"graphic");
		InterpretPredicate(std::isalnum, u8"alphanumeric");
		InterpretPredicate(std::isalpha, u8"alphabetical");
		InterpretPredicate(std::isupper, u8"upper");
		InterpretPredicate(std::islower, u8"lower");
		InterpretPredicate(std::ispunct, u8"punctuation");
		InterpretPredicate(std::isxdigit, u8"hexadecimal");
		InterpretPredicate(std::isdigit, u8"digit");
		InterpretPredicate(std::isspace, u8"space");
		InterpretPredicate(std::isblank, u8"blank");

	}

		// Helper function to extract the first token from a program, returning it and modifying the original program to remove the extracted token
	Medium<V> Munch(Medium<V>& prog) {
		Medium<V> program{};
		size_t i = 0;
		while (i < prog.size() && std::isspace(static_cast<unsigned char>(prog[i]))) {
			++i;
		}

		while (i < prog.size() && !std::isspace(static_cast<unsigned char>(prog[i]))) {
			program += prog[i];
			++i;
		}

		while (i < prog.size() && std::isspace(static_cast<unsigned char>(prog[i]))) {
			++i;
		}
		prog.erase(0, i);
		return program;
	}


	// This function extracts the first bite of a program.
	Program<V> Nibble (Medium<V> & prog){
		Program<V> bite;
		size_t i = 0;
		while (i < prog.size() && std::isspace(static_cast<unsigned char>(prog[i]))) {
			++i;
		}
		bite = prog[i];
		while (i < prog.size() && std::isspace(static_cast<unsigned char>(prog[i]))) {
			++i;
		}
		prog.erase(0, i);
		return bite; 
	}

	// This function copies the first chunk of a program.
	Medium<V> Lick (const Medium<V>& prog){
		Medium<V> program{};
		size_t i = 0;
		while (i < prog.size() && std::isspace(static_cast<unsigned char>(prog[i]))) {
			++i;
		}

		while (i < prog.size() && !std::isspace(static_cast<unsigned char>(prog[i]))) {
			program += prog[i];
			++i;
		}
		return program;
	}

	// This function copies the first bite of a program.
	Program<V> Lick_V (const Medium<V> & prog){
		size_t i = 0;
		while (i < prog.size() && std::isspace(static_cast<unsigned char>(prog[i]))) {
			++i;
		}
		return prog[i]; 
	}

	// Helper function to split a program into tokens based on whitespace, using Munch to extract tokens iteratively
	ProgramFile<V> Chunkify(Medium<V> & prog) {
		ProgramFile<V> file;
		while (!prog.empty()) {
			Medium<V> token = Munch(prog);

			// Safety check: if Munch returns a non-empty token, add it.
			if (!token.empty()) {
				file.push_back(token);
			}
		}
		return file;
	}

	ProgramFile<V> Chunkify(const Medium<V> & prog){
		Medium <V> buffer = prog;
		return Chunkify(buffer);
	}


	bool AddSymbols (const Token<V>& token){
		if (std::holds_alternative<Program<V>>(token))
			return A.insert(std::get<Program<V>>(token)).second;
		bool ret = true;
		if (std::holds_alternative<Medium<V>>(token)){
			for (const Program<V>& symbol : std::get<Medium<V>>(token)) {
				ret = ret && A.insert(symbol).second;
			}
			return ret;
		}
		return false;
	}

	bool AddSymbols (const Alphabet& a){
		bool ret = true;
		for (Program<V> symbol: a){
			ret = ret && A.insert(symbol).second;
		}
		return ret;
	}


		// Helper function to check if a program is a valid word (i.e., all symbols are in the alphabet)
	bool is_word(const Token<V>& token) const {
		if (std::holds_alternative<Program<V>>(token)){
			if (!A.contains(std::get<Program<V>>(token))) {
				return false;
			}
			return true;
		}
		if (std::holds_alternative<Medium<V>>(token)){
			for (const V& symbol : std::get<Medium<V>>(token)) {
				if (!A.contains(symbol)) {
					return false;
				}
			}
			return true;
		}
		return false;
	}

	bool is_well_formed(const Token<V>& text) {
		return is_word(text) && has_interpretation(text) ;
	}
	
	// This function returns true if its syntax is recognized from within the Concepts
	bool has_interpretation(const Token<V>& token) {
		for (const Concept& c : I) {
			if (std::get<1>(c)(token) == true) return true;
		}
		return false;
	}
	
	// Base Interpret method for custom syntax and semantics of strings
	bool Interpret(const Alphabet& a, const Token<V>& t, Syntax syn, Semantic sem) {
		for (const Concept& c : I) {
			if (std::get<0>(c) == t) {
				throw std::invalid_argument("token already taken\n");
				return false;
			}
		}
		AddSymbols(t);
		AddSymbols(a);
		if (is_word(t)) {
			I.push_back(std::make_tuple(t,syn,sem));
			return true;
		}
		return false;
	}

	// Interpret method overload for Value-returning functions with no arguments.
	bool Interpret(const Token<V>& t, std::function <std::any ()> f) {
		return Interpret(
			std::set<Program<V>>{},
			t,
			[this, t](const Token<V>& prog) { return this->NameSyntax(t, prog); },
			[this, f](const Token<V>& prog) {return this->NullarySemantic(f); });
	}
	// Interpret method overload for Value types.
	bool Interpret(const Token<V>& t, std::any a) {
		return Interpret(
			std::set<Program<V>>{},
			t,
			[this, t](const Token<V>& prog) { return this->NameSyntax(t, prog); },
			[this, a](const Token<V>& prog) {return this->IdentitySemantic(a); }
		);
	}

		// Helper function to interpret a type T by adding its name to the alphabet and defining its interpretation
	// template<typename T>
	// void InterpretType() {
	// 	const std::u8string type_name = typeid(T).name();
	// 	if (!is_well_formed(type_name)) {
	// 		T epsilon {};
	// 		Interpret(type_name, epsilon);

	// 	}
	// }

	// Helper function to interpret a type T by adding its name to the alphabet 
	// and defining its interpretation safely across different character types V.
	template<typename T>
	bool InterpretType() {
		const char* raw_name = typeid(T).name();
		std::string narrow_name(raw_name);

		Token<V> type_token;
		if constexpr (String<Program<V>>) {
			type_token = Program<V>(narrow_name);
		} else if constexpr (Char<Program<V>>) {
			return false;	
		}

		if (!has_interpretation(type_token)) {
			T epsilon {};
			return Interpret(type_token, epsilon);
		}
		return false;
	}

	// void InterpretType() {
	// 	// 1. Use C++26 reflection to get the type's name at compile-time
	// 	// std::meta::name_of returns a string_view of the type's identifier
	// 	constexpr auto type_name_view = std::meta::name_of(^T);
		
	// 	// 2. Convert to your language's specific Program<V> type
	// 	Token<V> type_token;
		
	// 	if constexpr (String<Program<V>>) {
	// 		// Safe conversion from string_view to u8string/string/etc.
	// 		type_token = Program<V>(type_name_view.begin(), type_name_view.end());
	// 	} 
	// 	else if constexpr (Char<Program<V>>) {
	// 		// Fallback for single-char tokens
	// 		type_token = static_cast<Program<V>>(type_name_view[0]);
	// 	}

	// 	// 3. Register with the Abstract Machine's alphabet
	// 	if (!has_interpretation(type_token)) {
	// 		T epsilon {};
	// 		Interpret(type_token, std::any(epsilon));
	// 	}
	// }

	void InterpretPredicate(int(*predicate)(int), const Token<V>& name) {
		Interpret(
			GetCharacterSet(predicate), 
			name, 
			[predicate](const Token<V>& prog) { return str_predicate(predicate, prog); },
			[this](const Token<V>& prog) { return this->IdentitySemantic(prog); }
		);
	}

	void InterpretMediumFunction(const Token<V>& name, const std::set<Medium<V>>& comms, std::function<std::any(const Medium<V>&)> f) {
		 Interpret(
			std::set<Program<V>>{}, 
			name, 
			[this, comms](const Token<V>& prog) { return this->MediumFunctionSyntax(prog, comms); },
			[this, f](const Token<V>& prog) { return this->MediumFunctionSemantic(prog, f); }
		);
	}

	// FunctionName: Args... -> Ret = { } 
	// Helper function to interpret a function with a specific syntax and semantics based on its return type and argument types
	template <Value Ret, Value...Args>
	bool InterpretFunction(const Token<V>& name, const Token<V>& prog, std::function<Ret(Args...)>) {
		size_t Arguments = sizeof...(Args);
		using ArgumentTypes = std::tuple<Args...>;

		// Interpret each argument type
		(InterpretType<Args>(), ...);
		// Interpret return type
		InterpretType<Ret>();

		ProgramFile<char8_t> Program = Chunkify(prog);

		unsigned ProgramCounter = 1;
		if ( (Program[0] == name) && (Arguments == Program.size() - 1)) {
			while (ProgramCounter < Program.size()) {
				// Check if the current token has an interpretation
				if (has_interpretation(Program[ProgramCounter])) {
					auto interp = Evaluate(Program[ProgramCounter]);
					if (has_interpretation(interp.first)) {
						auto type = Evaluate(interp.first);
						// Check if the type of the interpreted token matches the expected argument type
						if (type.first == interp.first)
							ProgramCounter++;
						else return false;
						
					}
				}
			}
			return true;
		}
		else return false;
	}

	// Helper function to interpret a token as a Name (i.e., a valid identifier in the language)
	bool NameSyntax(const Token<V>& t, const Token<V>& program) {
		if (str_predicate(std::isalpha, t) && t == program){
			return true;
		}
		return false;
	}

	// Helper function to interpret a token as a Name and return the result of a provided function if the syntax is valid
	std::any NullarySemantic(std::function<std::any()> f) {
		return f();
	}

	// Helper function to interpret a token as a Name and return the provided value if the syntax is valid
	std::any IdentitySemantic(std::any a) {
		return a;
	}

	// The string could also be empty after the name. Use semantics to disambiguate.
	bool MediumFunctionSyntax(const Token<V>& prog, const std::set<Medium<char8_t>>& comnames) {
		if (std::holds_alternative<Medium<V>>(prog)) {
			Medium<V> command = Lick(std::get<Medium<char8_t>>(prog));
			if ( comnames.contains(std::get<Medium<char8_t>>(ToLower(command)))){
				ProgramFile<char8_t> file = Chunkify(std::get<Medium<char8_t>>(prog));
				return true;
			}
		}
		return false;
	}

	std::any MediumFunctionSemantic(const Token<V>& prog, std::function<std::any(const Medium<V>&)> f) {
		Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
		Munch(program);
		return f(program);
	}


	// Evaluate a program by checking each interpretation's syntax and returning the corresponding semantic value if a match is found
	// Returns a pair. First is the name or the type of the object evaluated. Second is the result of the evaluation.

	std::pair<Token<V>, std::any> Evaluate(const Token<V>& prog) {
		for (const Concept& r : I) {
			if (std::get<1>(r)(prog) == true)
				return std::make_pair(std::get<0>(r), std::get<2>(r)(prog));
		}
		return std::make_pair(Token<V>{}, std::any{}); // Token{} instead of "Error" for type safety
	}

};

struct Resource {
    Language<char8_t> language;
    std::any resource;
};



class States : public Resource {

	/*enum class Symbols: char {
		SE, LD, UL, CL
	};*/
	//ProgramFile<char8_t> File; // Holds the states of the machine as a program file. 

	States() {
		//resource = File;
		//resource = { File {} };
		//Name = "States";
		//language.A.insert("STATE"); // STATE
		//language.A.insert("state");
		//language.A.insert("SE");
		//language.A.insert("LOAD"); // LOAD
		//language.I.push_back()

		//language.A.insert("load");
		//language.A.insert("LD");
		//language.A.insert("UNLOAD"); // UNLOAD
		//language.A.insert("unload");
		//language.A.insert("UD");
		//language.A.insert("CALL"); // CONDITIONAL CALL
		//language.A.insert("call");
		//language.A.insert("CL");
		//language.I.
		language.InterpretMediumFunction(u8"load", ld, [this](const Medium<char8_t>& p) { return this->Load(p); });
		language.InterpretMediumFunction(u8"unload", ud, [this](const Medium<char8_t>& p) { return this->Unload(p); });

	}

	std::set<Medium<char8_t>> ld = {u8"load", u8"ld"};
	std::set<Medium<char8_t>> ud = {u8"unload", u8"ud"};
	std::set<Medium<char8_t>> cl = {u8"call", u8"cl"};
	std::set<Medium<char8_t>> se = {u8"state", u8"se"};
	std::set<Medium<char8_t>> at = {u8"accept", u8"at"};
	std::set<Medium<char8_t>> ne = {u8"name", u8"ne"};
	

	/*void Transition(Word<Program> P) {
		if (P.Formula == "state" 
			|| P.Formula == "State" 
			|| P.Formula == "STATE") {
			language.I.push_back();
		}
	}*/
	enum StateKind : int { 
		ER = -1, // Error state
		NL = 0, // Normal state
		AG = 1, // Accepting state
	};


	std::unordered_map<unsigned long long, Token<char8_t>> states; // Maps state numbers to their corresponding tokens (programs).
	std::hash<Token<char8_t>> hasher; // Maps tokens (programs) to their corresponding state numbers for quick lookup.

	// state 0 is the starting state by default 
	unsigned long long state = 0; // current state register
	//unsigned icount = 0; // instruction counter
	std::vector<unsigned long long> instnum{}; //Instruction number stack
	std::vector<unsigned long long> previous{}; //Previous
	std::set<unsigned long long> accepting{};

	unsigned long long State() const { return state; }

	std::pair<StateKind,unsigned long long> Load(const Token<char8_t>& program) {
		Medium<char8_t> prog = std::get<Medium<char8_t>>(program);
		StateKind kind = StateKind::NL;
		Medium<char8_t> name;
		unsigned long long new_state;

		if (at.contains(std::get<Medium<char8_t>>(ToLower(language.Lick(prog))))) {
			kind = StateKind::AG;
			language.Munch(prog); // Remove "accept"
		}

		if (!prog.empty()){
			if(ne.contains(std::get<Medium<char8_t>>(ToLower(language.Lick(prog))))) {
				language.Munch(prog); // Remove "name"	
				if (!prog.empty()){
					name = language.Munch(prog);
					if (!name.empty() && str_predicate(std::isalpha, name)) {
						new_state = hasher(name);
					} 
				}
			}
		}
		else {
			return std::make_pair(StateKind::ER, 0); // Invalid name
		}

		if (!prog.empty()) {
			new_state = hasher(prog);
		}
		else {
			return std::make_pair(StateKind::ER, 0); // Invalid name
		}
		
		states[new_state] = program;
		if (kind == StateKind::AG) {
			accepting.insert(new_state);
		}
		return std::make_pair(kind, new_state);
	}

	unsigned long long Unload(Medium<char8_t> program) {
		Medium<char8_t> prog = program;
		language.Munch(prog); // Remove the command (e.g., "unload") to get the state identifier
		unsigned long long s ;

		if (prog.empty()) {
			s = state;
		}
		else {
			prog = language.Munch(prog); // Get the next token which should be the state identifier
			if (!prog.empty()) {
				if( str_predicate(std::isalpha, prog)) {
					s = hasher(prog);
				}
				else if( str_predicate(std::isdigit, prog)) {
					s = std::stoull(std::string(prog.begin(), prog.end()));
				}
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
};




// The Value V of the substrate is the type of the symbols on the tape. It can be any type that satisfies the Value concept, which includes primitive types (like char, int, bool) and user-defined types that can be constructed from a string representation. 
// The programs that are read are written in ProgramFile<char8_t>. The same as the AbstractMachine's language. The Substrate is a resource of the AbstractMachine, and it provides the basic operations that the machine can perform on its tape. 
template <Value V>
class Substrate: public Resource {
public:

	Substrate() {
		order = 16;
		head = 0;
		Tape = MakeTape(order);

		/*language.Interpret(Digits, "digits", &Substrate<char>::DigitsSyntax, &Substrate<char>::DigitsSemantic);
		language.Interpret({}, "read", &Substrate<char>::ReadSyntax, &Substrate<char>::ReadSemantic);
		language.Interpret({}, "write", &Substrate<char>::writeSyntax, &Substrate<char>::writeSemantic);
	*/	//language.Interpret("head", std::make_pair(headSyntax, headConcept));
		

		//language.Interpret(
		//	std::set<char>{}, // empty alphabet for these tokens
		//	"read",
		//	[this](const Medium<char>& prog) { return this->ReadSyntax(prog); },
		//	[this](const Medium<char>& prog) { return this->ReadSemantic(prog); }
		//);
		language.Interpret(u8"read", Read());
		language.Interpret(u8"head", Head());
		language.Interpret(u8"left", Left());
		language.Interpret(u8"right", Right());

		language.Interpret(
			std::set<char8_t>{},
			u8"write",
			[this](const Token<char8_t>& prog) { return this->WriteSyntax(prog); },
			[this](const Token<char8_t>& prog) { return this->WriteSemantic(prog); }
		);

	}

	// std::any writeSemantic(const Medium<char8_t>& program) {
	// 	if (writeSyntax(program)) {
	// 		unsigned i = 6;
	// 		for (; (program.begin() + i) != program.end(); i++) {
	// 			char element = program[i];
	// 			Write(V(element));
	// 			Right();
	// 		}
	// 		return i - 6;
	// 	}
	// 	return NULL;
	// }

	// std::any WriteSemantic(const Token<char8_t>& prog) {
		
	// 	if (WriteSyntax(prog) == true) {
	// 		Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
	// 		language.Munch(program);
			
	// 		if constexpr (std::is_same_v<V, std::u8string>) {
	// 			return Write(V(program));
	// 		}
	// 		Medium<char8_t> val= language.Munch(program);
	// 		if ((program.empty() || program == u8"") && val.size() == sizeof(V)) {
	// 			return Write(V(val));
	// 		}

	// 	}
	// 	return std::any{};
	// }

	std::any WriteSemantic(const Token<char8_t>& prog) {
    if (WriteSyntax(prog) == true) {
        Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
        language.Munch(program); // Remove "write" command
        Medium<char8_t> valStr = language.Munch(program); // Get the data to write

        // Case 1: The Tape stores full Strings
        if constexpr (String<V>) {
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
            } catch (...) { return std::any{}; }
        }
        // Case 3: The Tape stores Numbers (int, long long, etc.)
        else if constexpr (Arithmetic<V>) {
			// Convert u8string to a char range for from_chars
			const char* first = reinterpret_cast<const char*>(valStr.data());
			const char* last = first + valStr.size();
			V numericVal;

			auto [ptr, ec] = std::from_chars(first, last, numericVal);

			if (ec == std::errc{}) {
				return Write(numericVal);
			} else {
				if (ec == std::errc::invalid_argument)
					std::cout << "This is not a number.\n";
				else if (ec == std::errc::result_out_of_range)
					std::cout << "This number is larger than an int.\n";
				// Handle error: result was out of range or not a number
				return std::any{ptr, ec}; // Return parsing result for debugging
			}
		}
    }
    return std::any{};
}


	// bool writeSyntax(const Medium<char8_t>& program) {
	// 	Medium<char8_t> prefix(program.begin(), program.begin() + 5);
	// 	if (prefix == u8"write "
	// 		&& program.size() > 6 ) {
	// 		return true;
	// 	}
	// 	else return false;
	// }

	bool WriteSyntax(const Token<char8_t>& prog) {
		if (std::holds_alternative<Program<char8_t>>(prog))
			return false;
		if (std::holds_alternative<Medium<char8_t>>(prog)){
			Medium<char8_t> command = language.Lick(std::get<Medium<char8_t>>(prog));
			std::set<std::u8string> comnames = { u8"write", u8"we"};
			if ( comnames.contains(std::get<Medium<char8_t>>(ToLower(command))) && (std::get<Medium<char8_t>>(prog)).size() > (command).size())
				return true;
		}
		return false;
	}
	
	//bool headSyntax(Medium<char>& prog) {
	//	if (prog.size() == 1 && prog[0] == HD) return true;
	//	else return false;
	//}

	/*std::any headConcept(Medium<char>& prog) {
		if(headSyntax(prog))
		return Head();
	}*/

	//friend class AbstractMachine;
	 Medium<V> MakeTape(const unsigned long long& k) {
		if (k >= (sizeof(unsigned long long) * 8)) throw std::overflow_error("Tape order too large");
		std::size_t size = std::size_t(1ULL << k);
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
		else {
			return Medium<V>(size, V{});
		}
	}

	Medium<V> Tape;

	long long head;
	unsigned long long order;

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
		return false;
	}

	long long Head() const { return head; }
	
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
	void NewTape(unsigned long long n) {
		Tape = MakeTape(n);
		//zero = Tape.size() / 2;
		order = n;
	}

	bool MoreTape() {
		if (order + 1 >= (sizeof(unsigned long long) * 8)) {
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
		unsigned newOrder = static_cast<unsigned>(std::ceil(std::log2(static_cast<double>(newSize))));
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

// Character Set operations
auto Intersection (std::set<unsigned char>A, std::set<unsigned char>B, std::set<unsigned char>Dest){
	return std::set_intersection(A.begin(), A.end(),
							B.begin(), B.end(),
							std::inserter(Dest, Dest.begin()));
}
auto Union (std::set<unsigned char>A, std::set<unsigned char>B, std::set<unsigned char>Dest){
	return std::set_union(A.begin(), A.end(),
							B.begin(), B.end(),
							std::inserter(Dest, Dest.begin()));
}
auto Difference (std::set<unsigned char>A, std::set<unsigned char>B, std::set<unsigned char>Dest){
	return std::set_difference(A.begin(), A.end(),
							B.begin(), B.end(),
							std::inserter(Dest, Dest.begin()));
}
bool Inclusion (std::set<unsigned char>A, std::set<unsigned char>B){
	return std::includes(A.begin(), A.end(),
							B.begin(), B.end());
}


// Here, we define Abstract Machine to use a language over char8_t
// but we could easily refactor to any Value V.
//template <Value V>
class AbstractMachine {
public:
	friend class States;
	Language<char8_t> language;
    std::vector<Resource> Resources;
	
	
	AbstractMachine() {


		language.InterpretMediumFunction(u8"run", RunComms, [this](const Medium<char8_t>& prog) { return this->Run(prog); });



		//AddResource("type", Types);
	}

	~AbstractMachine() {};
	

	//std::vector <const std::type_info*> types;
	//std::vector <Medium<unsigned char>> types;

	std::set<Medium<char8_t>> RunComms = { u8"run", u8"rn"};

	std::any Run(const Medium<char8_t> prog) {
		if (language.is_well_formed(prog)) {
			return language.Evaluate(prog).second;
		}
		// fallback for default interpretation
		// because if you invoke a resource first, it's done through the machine language.
		for (Resource& res : Resources) {
			if (res.language.is_well_formed(prog)) {
				return res.language.Evaluate(prog).second;
			}
		}
		return {};
	}

	bool ResNameSyntax(const Token<char8_t>& name, const Token<char8_t>& prog, Resource res) {
		if (std::holds_alternative<Program<char8_t>>(name) || std::holds_alternative<Program<char8_t>>(prog))
			return false;
		
		if (std::holds_alternative<Medium<char8_t>>(name) && std::holds_alternative<Medium<char8_t>>(prog)){
			unsigned i = 0;
			Medium<char8_t> program = language.Lick(std::get<Medium<char8_t>>(prog));

			if (str_predicate(std::isalpha, program) == true && program == std::get<Medium<char8_t>>(name)) {
				if (res.language.is_well_formed(prog)) {
					return true;
				}
			}
			return false;
		}
		return false;
	}

	std::any ResNameSemantic(const Token<char8_t>& prog, Resource res) {
		Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
		language.Munch(program);
		return res.language.Evaluate(program);
	}

	void AddResource(const Medium<char8_t>& name, Resource res) {
		if (language.is_well_formed(name) == false) {
		//if (language.is_well_formed(typeid(res).name()) == false) {
			language.Interpret(
				GetCharacterSet(std::isalpha),
				name,
				[this, name, res](const Token<char8_t>& prog) { return this->ResNameSyntax(name, prog, res); },
				[this, res](const Token<char8_t>& prog) { return this->ResNameSemantic(prog, res); }
			);
			Resources.push_back(res);
		}
		
	}

	
};


//template<Text T>
//using File = std::conditional_t<
//    String<T>,
//    std::vector<T>,
//    std::basic_string<T> 
//>;
