//	Language.h : This file defines the Language class, which represents a formal language defined by an alphabet and a set of interpretations (concepts). It provides methods to add symbols, check if a program is well-formed, and evaluate programs based on the defined syntax and semantics. The Language class is designed to be flexible and extensible, allowing for the definition of various types of languages and their associated concepts.
//  Copyright © 2026 Guillermo M. Dávila Andino

#pragma once

#include <iostream>
#include <set>
#include <any>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <valarray>
#include <concepts>
#include <typeinfo>
#include <variant>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <climits> // for UCHAR_MAX
#include <locale>
//#include <string_view>
#include <unordered_map>

inline std::locale currentLocale = std::locale("en_US.utf8");

inline bool isspace(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::space, c);
}
inline bool isblank(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::blank, c);
}
inline bool iscntrl(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::cntrl, c);
}
inline bool isupper(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::upper, c);
}
inline bool islower(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::lower, c);
}
inline bool isalpha(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::alpha, c);
}
inline bool isdigit(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::digit, c);
}
inline bool ispunct(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::punct, c);
}
inline bool isxdigit(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::xdigit, c);
}
inline bool isprint(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::print, c);
}
inline bool isalnum(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::alnum, c);
}
inline bool isgraph(char32_t c) {
	return std::use_facet<std::ctype<char32_t>>(currentLocale).is(std::ctype_base::graph, c);
}

// Overload >> to support u8string
inline std::istream& operator>>(std::istream& is, std::u8string& u8s) {
	std::string temp;
	if (is >> temp) {
		u8s.assign(reinterpret_cast<const char8_t*>(temp.data()), temp.size());
	}
	return is;
}

// Teach cout how to handle u8string
inline std::ostream& operator<<(std::ostream& os, const std::u8string& u8s) {
	// We "re-interpret" the memory as standard char so cout accepts it
	return os << reinterpret_cast<const char*>(u8s.c_str());
}

// Custom getline for std::u8string
inline std::istream& getline(std::istream& is, std::u8string& u8s, char delim = '\n') {
	std::string temp;
	if (std::getline(is, temp, delim)) {
		u8s.assign(reinterpret_cast<const char8_t*>(temp.data()), temp.size());
	}
	return is;
}

inline std::ostream& operator<<(std::ostream& os, const char8_t* s) {
	return os << reinterpret_cast<const char*>(s ? reinterpret_cast<const char8_t*>(s) : u8"");
}

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

template<typename A>
concept Arithmetic = std::is_arithmetic_v<A>;

template<typename P>
concept Primitive = Text<P> || Arithmetic<P> ;

// This template allows us to define arbitrary types as long as we provide the mechanisms to construct it from a string
template<typename D>
concept Defined = std::regular<D> && std::constructible_from<D, std::u8string>;

template <typename V>
concept Value = Primitive<V> || Defined<V> ;

// For the abstract machine, a program is any kind of text.
// With a single char type variable, for example, we can encode up to 2^8 instructions.
// So, we say that instruction represents a program.
template <Value V>
using Program = V;

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


template <Value V>
using Token = std::variant<Medium<V>, Program<V>>;


template <Text V>
inline std::ostream& operator<<(std::ostream& os, const Token<V>& tok) {
	/*if (std::holds_alternative<Program<V>>(tok))
    	return os << static_cast<char>(std::get<Program<V>>(tok));
	if (std::holds_alternative<Medium<V>>(tok))
		return os << reinterpret_cast<const char*>(std::get<Medium<V>>(tok).c_str());
	throw std::runtime_error("Invalid token type for output");*/
	std::visit([&os](const auto& val) {
		if constexpr (requires { val.begin(); })
			for (auto c : val) os << static_cast<char>(c);
		else
			os << static_cast<char>(val);
		}, tok);
	return os;
}

inline Token<char8_t> ToLower (Token<char8_t> text){
	auto& f = std::use_facet<std::ctype<char32_t>>(currentLocale);

	if (std::holds_alternative<Program<char8_t>>(text)){
		char32_t c32 = static_cast<char32_t>(std::get<Program<char8_t>>(text));
		return static_cast<char8_t>(f.tolower(c32));
	}
	if (std::holds_alternative<Medium<char8_t>>(text)){
		Medium<char8_t>& str = std::get<Medium<char8_t>>(text);
		std::transform(str.begin(), str.end(), str.begin(),
			[&f](char8_t c) {
				return static_cast<char8_t>(f.tolower(static_cast<char32_t>(c)));
			});
		return text;
	}
	return {};
}

inline bool str_predicate(bool(*predicate)(char32_t), Token<char8_t> token){
	if (std::holds_alternative<Program<char8_t>>(token)){
		return predicate(static_cast<unsigned char>(std::get<Program<char8_t>>(token))) ;
	}
	if (std::holds_alternative<Medium<char8_t>>(token)){
		const auto& s = std::get<Medium<char8_t>>(token);
		if (s.empty()) return false;
		for (char8_t c : s){
			if (!predicate(static_cast<char32_t>(c)))
				return false;
		}
		return true;
	}
	return false;
}

inline std::set<char8_t> GetCharacterSet(bool (*predicate)(char32_t)) {
    std::set<char8_t> result;
    //for (long int i = 0; i <= 1114111; ++i) {
    for (long int i = 0; i <= UCHAR_MAX; ++i) {
        if (predicate(static_cast<char32_t>(i))) {
            result.insert(static_cast<char8_t>(i));
        }
    }
    return result;
}

// The Language struct represents a formal language defined by an alphabet and a set of interpretations (concepts). It provides methods to add symbols, check if a program is well-formed, and evaluate programs based on the defined syntax and semantics.
template <Value V>
class Language {

public:
	using Alphabet = std::set<Program<V>>;

	// Syntax returns unsigned long long. If zero, the syntax does not match. If non-zero, it indicates how many characters of the program were consumed by the syntax rule.
	using Syntax = std::function<unsigned long long(const Token<V>&)>;
	using Semantic = std::function<std::any (const Token<V>&)>;
	//using Semantic = std::function<Value auto (const Medium<V>&)>;

	//	{ConceptName, Syntax, Semantic}
	using Concept = std::tuple<Token<V>, Syntax, Semantic>;
	using Interpretation = std::vector<Concept>;

	//	[contextName] = Interpretation
	using Contexts = std::unordered_map<Medium<V>, Interpretation>;

	//	{ConceptPtr, Consumed, ContextName}
	using Interpreted = std::tuple<const Concept*, unsigned long long, const Medium<V>>;


	Alphabet A;
	//Interpretation I;
	Contexts C;

	//std::vector<std::set<Medium<char8_t>>> RegisteredNames;
	
	// These are callable names from within the language. Multiple different names can call the same concept.
	//	Sometimes, you need a name to call to a concept from a different context.
	std::unordered_map<Medium<V>, std::vector<std::set<Medium<char8_t>>>> RegisteredNames;


	Language() {

	}
	
	void AddCharacterInterpretations() {
		Medium<V> context = u8"Literal";
		// order matters, so we must be topological about this
		// the first we list here have least precedence, the last have most precedence
		// When listing interpretation, do so from most general to most specialized.
		if constexpr (std::is_same_v<V, char8_t>) {
			InterpretPredicate(iscntrl, u8"control", context);
			InterpretPredicate(isprint, u8"printable", context);
			InterpretPredicate(isgraph, u8"graphic", context);
			InterpretPredicate(isalnum, u8"alphanumeric", context);
			InterpretPredicate(isalpha, u8"alphabetical", context);
			InterpretPredicate(isupper, u8"upper", context);
			InterpretPredicate(islower, u8"lower", context);
			InterpretPredicate(ispunct, u8"punctuation", context);
			InterpretPredicate(isxdigit, u8"hexadecimal", context);
			InterpretPredicate(isdigit, u8"digit", context);
			InterpretPredicate(isspace, u8"space", context);
			InterpretPredicate(isblank, u8"blank", context);
		}

	}

	void AddTypeInterpretations() {
		Medium<V> context = u8"Native Type";
		InterpretType<bool>(context);
		InterpretType<char>(context);
		InterpretType<signed char>(context);
		InterpretType<unsigned char>(context);
		InterpretType<char8_t>(context);
		InterpretType<char16_t>(context);
		InterpretType<char32_t>(context);

		InterpretType<short>(context);
		InterpretType<unsigned short>(context);
		InterpretType<int>(context);
		InterpretType<unsigned int>(context);
		InterpretType<long>(context);
		InterpretType<unsigned long>(context);
		InterpretType<long long>(context);
		InterpretType<unsigned long long>(context);

		InterpretType<float>(context);
		InterpretType<double>(context);
		InterpretType<long double>(context);
	}

	// Helper function to extract the first token from a program, returning it and modifying the original program to remove the extracted token
	Medium<V> Munch(Medium<V>& prog) {
		Medium<V> program{};
		size_t i = 0;
		while (i < prog.size() && isspace((prog[i]))) {
			++i;
		}

		while (i < prog.size() && !isspace((prog[i]))) {
			program += prog[i];
			++i;
		}

		while (i < prog.size() && isspace((prog[i]))) {
			++i;
		}
		prog.erase(0, i);
		return program;
	}

	// A proper Munch.
	std::pair<Medium<V>, unsigned long long> Lunch(const Medium<V>& prog) {
		Medium<V> program{};
		size_t i = 0;
		while (i < prog.size() && isspace((prog[i]))) {
			++i;
		}

		while (i < prog.size() && !isspace((prog[i]))) {
			program += prog[i];
			++i;
		}

		while (i < prog.size() && isspace((prog[i]))) {
			++i;
		}
		return std::make_pair(program, i);
	}

	// A Proper lexer.
	std::vector<std::pair<Medium<V>, unsigned long long>> ChopLine (const Medium<V>& prog) {
		std::pair<Medium<V>, unsigned long long> program{};
		std::vector<std::pair<Medium<V>, unsigned long long>> ProgFile{};
		size_t i = 0;
		//unsigned long long offset = 0;
		while (i<prog.size()){
			while (i < prog.size() && isspace((prog[i]))) {
				++i;
			}

			while (i < prog.size() && !isspace((prog[i]))) {
				if (!ispunct((prog[i]))) {
					if (program.first.empty()) {
						program.second = i; // Set the starting index of the token
					}
					program.first += prog[i];
				}
				else {
					if (program.first.empty()) {
						program.first = prog[i];
						program.second = i; // Set the starting index of the token
						ProgFile.push_back(program);
						program.first.clear();
					}
					else {
						ProgFile.push_back(program);
						program.first.clear();
						program.first = prog[i];
						program.second = i; // Set the starting index of the token
						ProgFile.push_back(program);
						program.first.clear();
					}
				}
				++i;
			}
			if (!program.first.empty()) {
				ProgFile.push_back(program);
				program.first.clear();
			}
		}
		return ProgFile;
	}

	// This function extracts the first bite of a program.
	Program<V> Nibble (Medium<V> & prog){
		Program<V> bite{};
		size_t i = 0;
		while (i < prog.size() && isspace((prog[i]))) {
			++i;
		}
		if (i < prog.size()) {
			bite = prog[i];
			++i;
		}
		while (i < prog.size() && isspace((prog[i]))) {
			++i;
		}
		prog.erase(0, i);
		return bite; 
	}

	// This function copies the first chunk of a program.
	Medium<V> Lick (const Medium<V>& prog){
		Medium<V> program{};
		size_t i = 0;
		while (i < prog.size() && isspace((prog[i]))) {
			++i;
		}

		while (i < prog.size() && !isspace((prog[i]))) {
			program += prog[i];
			++i;
		}
		return program;
	}

	// This function copies the first bite of a program.
	Program<V> Lick_V (const Medium<V> & prog){
		size_t i = 0;
		while (i < prog.size() && isspace(prog[i])) {
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
				if (!A.contains(symbol)) {
					ret = ret && A.insert(symbol).second;
				}
			}
			return ret;
		}
		return false;
	}

	bool AddSymbols (const Alphabet& a){
		bool ret = true;
		for (Program<V> symbol: a){
			if (!A.contains(symbol)) {
				ret = ret && A.insert(symbol).second;
			}
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

	std::vector<Interpreted> is_well_formed(const Token<V>& text) {
		if (is_word(text)) {
			return has_interpretation(text);
		}
		throw std::invalid_argument("text is not well-formed: contains symbols not in the alphabet\n");
		//return { nullptr, 0, {} };
	}

	std::vector<Medium<V>> is_well_formed_context(const Token<V>& text) {
		std::vector<Medium<V>> cntxts{};
		
		if (is_word(text)) {
			std::vector<Interpreted> interpretation =  has_interpretation(text);
			for (const auto& [Concept_Ref, consumed, cntxt] : interpretation) {
				if (std::find(cntxts.begin(), cntxts.end(), cntxt) == cntxts.end()) {
					cntxts.push_back(cntxt);
				}
			}
		}
		return cntxts;
	}
	
	Interpreted is_well_formed(const Token<V>& text, const Medium<V>& cntxt) {
		if (is_word(text)) {
			return has_interpretation(text, cntxt);
		}
		throw std::invalid_argument("text is not well-formed: contains symbols not in the alphabet\n");
		//return { nullptr, 0, {} };
	}
	

	Interpreted has_interpretation(const Token<V>& token, const Medium<V>& cntxt) {
		//std::vector<std::tuple<const Concept*, unsigned long long, const Medium<V>>> interpretations;
		//for (auto Cit = C.rbegin(); Cit != C.rend(); Cit++ ) {
		for (auto& [CntxtName, I]: C){
			if (CntxtName != cntxt) continue;
			//const Interpretation& I = (*Cit).second;
			for (auto it = I.rbegin(); it != I.rend(); it++) {
				unsigned long long consumed = std::get<1>(*it)(token);
				if (consumed > 0) {
					return { &(*it), consumed, CntxtName };

				}
			}
		}
		return { nullptr, 0, {} };
	}
	// This function returns true if its syntax is recognized from within the Concepts
	std::vector<Interpreted> has_interpretation(const Token<V>& token) {
		std::vector<std::tuple<const Concept*, unsigned long long, const Medium<V>>> interpretations;
		//for (auto Cit = C.rbegin(); Cit != C.rend(); Cit++ ) {
		for (auto& [CntxtName, I]: C){
			//const Interpretation& I = (*Cit).second;
			for (auto it = I.rbegin(); it != I.rend(); it++) {
				unsigned long long consumed = std::get<1>(*it)(token);
				if (consumed > 0) {
					interpretations.push_back( { &(*it), consumed, CntxtName });

				}
			}
		}
		return interpretations;
	}

	bool is_registered(const Token<V>& token, const Medium<V> context) {
		if (!RegisteredNames.contains(context)) return false;
		for (const auto& name : RegisteredNames[context]) {
			if (name.contains(std::get<Medium<char8_t>>(token)))
				return true;
		}
		return false;
	}
	std::vector<Medium<V>> is_registered_any(const Token<V>& token) {
		std::vector<Medium<V>> contexts;
		for (const auto& [context, names] : RegisteredNames) {
			for (const auto& name : names) {
				if (name.contains(std::get<Medium<char8_t>>(token))) {
					contexts.push_back(context);
				}
			}
		}
		return contexts;
	}

	std::set<Medium<char8_t>> GetContextsForName(Medium<char8_t>& name) {
		std::set<Medium<char8_t>> contexts;
		for (const auto& [context, names] : RegisteredNames) {
			for (const auto& name_set : names) {
				if (name_set.contains(name)) {
					contexts.insert( context);
				}
			}
		}
		return contexts;
	}

	bool is_concept (const Token<V>& token) {
		for (auto const& [name, I] : C) {
			for (auto it = I.rbegin(); it != I.rend(); it++) {
				if (std::get<0>(*it) == token) return true;

			}
			
		}
		return false;
	}
	
	// Base Interpret method for custom syntax and semantics of strings
	bool Interpret(const Alphabet& a, const Token<V>& t, Syntax syn, Semantic sem, const Medium<V> context) {
		// 1. Find the index or a pointer to the existing context in C
		bool isNew = (C.find(context) == C.end());
		Interpretation& targetInterp = C[context];

		if (isNew) {
			// If it's a new context, register the name
			RegisteredNames[context] = {};
		}

		// 3. Check for duplicates in the actual interpretation vector
		for (const Concept& c : targetInterp) {
			if (std::get<0>(c) == t) {
				if (isNew) {
					RegisteredNames.erase(context); // Clean up the new context if we won't be using it
				}
				throw std::invalid_argument("token already taken\n");
				//return false;
			}
		}

		// 4. Update symbols and check alphabet
		AddSymbols(t);
		AddSymbols(a);
    
		if (is_word(t)) {
			// 5. CRITICAL: Push to the persistent Interpretation vector in C
			targetInterp.push_back(std::make_tuple(t, syn, sem));
			std::set<Medium<V>> n = { std::get<Medium<V>>(t) };
			RegisteredNames[context].push_back(n);
			return true;
		}
		return false;
	}

	bool Interpret(const Alphabet& a, const Token<V>& t, const std::set<Medium<V>>& comms, Syntax syn, Semantic sem, const Medium<V> context) {
		bool s = false;
		for (const auto& n : comms) {
			s = s || (is_registered(n, context));
		}

		if (s) {
			std::cerr << "Can't register names";
			return false;
		}

		s = Interpret(
			a,
			t,
			syn,
			sem,
			context
		);

		if (s) {
			RegisteredNames[context].push_back(comms);
			return true;
		}

		else return false;
	}

	// Interpret method overload for Value-returning functions with no arguments.
	bool Interpret(const Token<V>& t, std::function <std::any ()> f, Medium<V> context) {
		return Interpret(
			std::set<Program<V>>{},
			t,
			[this, t](const Token<V>& prog) { return this->NameSyntax(t, prog); },
			[this, f](const Token<V>& prog) {return this->NullarySemantic(f); },
			context);
	}

	bool InterpretNullaryFunction(const Token<V>& t, const std::set<Medium<V>>& comms, std::function<std::any ()> f, Medium<V> context) {
		return Interpret(
			std::set<Program<V>>{},
			t,
			comms,
			[this, comms](const Token<V>& prog) { return this->MediumFunctionSyntax(prog, comms); },
			[this, f](const Token<V>& prog) { return this->NullarySemantic(f); },
			context
		);
	}

	bool InterpretNullaryVoidFunction(const Token<V>& t, const std::set<Medium<V>>& comms, std::function<void()> f, Medium<V> context) {
		return Interpret(
			std::set<Program<V>>{},
			t,
			comms,
			[this, comms](const Token<V>& prog) { return this->MediumFunctionSyntax(prog, comms); },
			[this, f](const Token<V>& prog) { this->VoidSemantic(f); return std::any{}; },
			context
		);
	}

	// Interpret method overload for Value types, or any object.
	bool Interpret(const Token<V>& t, std::any a, Medium<V> context) {
		return Interpret(
			std::set<Program<V>>{},
			t,
			[this, t](const Token<V>& prog) { return this->NameSyntax(t, prog); },
			[this, a](const Token<V>& prog) {return this->IdentitySemantic(a); },
			context
		);
	}

	// Helper function to interpret a type T by adding its name to the alphabet 
	// and defining its interpretation safely across different character types V.
	template<typename T>
	bool InterpretType(Medium<V> context) {
		const char* raw_name = typeid(T).name();
		std::string narrow_name(raw_name);

		// Convert narrow string to the Medium<V> (u8string/string)
		Medium<V> type_name(narrow_name.begin(), narrow_name.end());

		if (std::get<0>(has_interpretation(type_name, context)) == nullptr) {
			T epsilon {};
			return Interpret(type_name, epsilon, context);
		}
		return false;
	}

	bool InterpretPredicate(bool(*predicate)(char32_t), const Token<V>& name, const Medium<V>& context) {
		return Interpret(
			GetCharacterSet(predicate),
			name,
			[predicate](const Token<V>& prog) -> unsigned long long {
				if (str_predicate(predicate, prog)) {
					if (std::holds_alternative<Program<V>>(prog)) return 1;
					return std::get<Medium<V>>(prog).size();
				}
				return 0;
			},
			[this](const Token<V>& prog) { return this->IdentitySemantic(prog); },
			context
		);
	}

	bool InterpretMediumFunction(const Token<V>& name, const std::set<Medium<V>>& comms, std::function<std::any(const Medium<V>&)> f, Medium<V> context) {
		return Interpret(
			std::set<Program<V>>{}, 
			name, 
			comms,
			[this, comms](const Token<V>& prog) { return this->MediumFunctionSyntax(prog, comms); },
			[this, f](const Token<V>& prog) { return this->MediumFunctionSemantic(prog, f); },
			context
		);
	}

	bool InterpretIntegerArgumentMediumFunction(const Token<V>& name, const std::set<Medium<V>>& comms, std::function<std::any(const Medium<V>&)> f, Medium<V> context) {
		return Interpret(
			std::set<Program<V>>{},
			name,
			comms,
			[this, comms](const Token<V>& prog) { return this->IntegerArgumentSyntax(prog, comms); },
			[this, f](const Token<V>& prog) {return this->MediumFunctionSemantic(prog, f);	},
			context
		);
	}

	bool InterpretIntegerArgumentLongLongFunction(const Token<V>& name, const std::set<Medium<V>>& comms, std::function<std::any(const long long&)> f, Medium<V> context) {
		return Interpret(
			std::set<Program<V>>{},
			name,
			comms,
			[this, comms](const Token<V>& prog) { return this->IntegerArgumentSyntax(prog, comms); },
			[this, f](const Token<V>& prog) {return this->LongLongFunctionSemantic(prog, f);	},
			context
		);
	}

	// Helper function to interpret a token as a Name (i.e., a valid identifier in the language)s
	unsigned long long NameSyntax(const Token<V>& t, const Token<V>& program) {
		if constexpr (Text<V> && std::is_same_v<V, char8_t>) {
			if (str_predicate(isalpha, t) && t == program) {
				if (std::holds_alternative<Program<V>>(t)) {
					return 1; // Single character token has length 1
				}
				else if (std::holds_alternative<Medium<V>>(t)) {
					return std::get<Medium<V>>(t).size();
				}
			}
		}
		return 0;
	}

	// Helper function to interpret a token as a Name and return the result of a provided function if the syntax is valid
	std::any NullarySemantic(std::function<std::any()> f) {
		return f();
	}

	void VoidSemantic(std::function<void()> f) {
		f();
	}	

	// Helper function to interpret a token as a Name and return the provided value if the syntax is valid
	std::any IdentitySemantic(std::any a) {
		return a;
	}

	// The string could also be empty after the name. Use semantics to disambiguate.
	unsigned long long MediumFunctionSyntax(const Token<V>& prog, const std::set<Medium<char8_t>>& comnames) {
		if constexpr (Text<V> && std::is_same_v<V, char8_t>) {
			if (std::holds_alternative<Medium<V>>(prog)) {
				Medium<V> command = Lick(std::get<Medium<V>>(prog));
				if (comnames.contains(std::get<Medium<V>>(ToLower(command)))) {
					return std::get<Medium<V>>(prog).size();
				}
			}
		}
		return 0;

	}

	unsigned long long IntegerArgumentSyntax(const Token<V>& prog, const std::set<Medium<char8_t>>& comnames) {
		if constexpr (Text<V> && std::is_same_v<V, char8_t>) {
			if (std::holds_alternative<Medium<V>>(prog)) {
				Medium<V> program = std::get<Medium<V>>(prog);
				Medium<V> command = Munch(program);
				if (comnames.contains(std::get<Medium<V>>(ToLower(command)))) {
					Medium<V> arg = Munch(program);
					if (IntegerPredicate(arg)) {
						return std::get<Medium<V>>(prog).size() - program.size();
					}
				}
			}
		}
		return 0;
	}

	std::any MediumFunctionSemantic(const Token<V>& prog, std::function<std::any(const Medium<V>&)> f) {
		Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
		//Munch(program);
		return f(program);
	}

	std::any LongLongFunctionSemantic(const Token<char8_t> prog, std::function<std::any(const long long&)> f) {
		Medium<char8_t> program = std::get<Medium<char8_t>>(prog);
		Munch(program); // Remove the command part, leaving only the argument
		return f(std::stoll(std::string(program.begin(), program.end())));
	}

	std::any Evaluate(const Concept& C, const Token<V>& prog) {
		auto [name, syn, sem] = C;
		return sem(prog);
	}


	bool FloatPredicate(const Medium<V>& prog) {
		if (prog.empty()) return false;

		Medium<V> iprog = prog;
		Medium<V> program = Munch(iprog);

		if (!iprog.empty()) return false;

		auto tokens = ChopLine(program);

		if (tokens.empty()) return false;

		if (tokens[0].first == u8"+" || tokens[0].first == u8"-") {
			tokens.erase(tokens.begin());
			if (tokens.empty()) return false;
		}

		if (str_predicate(isdigit, tokens[0].first)) {
			tokens.erase(tokens.begin());
			if (tokens.empty()) return true;
		}

		if (tokens[0].first == u8".") {
			tokens.erase(tokens.begin());
			if (tokens.empty()) return false;
		}

		if (str_predicate(isdigit, tokens[0].first)) {
			tokens.erase(tokens.begin());
			if (tokens.empty()) return true;
		}

		return false;
	}
	
 	bool IntegerPredicate(const Medium<V>& prog) {
		if (prog.empty()) return false;

		Medium<V> iprog = prog;
		Medium<V> program = Munch(iprog);

		if (!iprog.empty()) return false;

		auto tokens = ChopLine(program);

		if (tokens.empty()) return false;

		if (tokens[0].first == u8"+" || tokens[0].first == u8"-") {
			tokens.erase(tokens.begin());
			if (tokens.empty()) return false;
		}

		if (str_predicate(isdigit, tokens[0].first)) {
			tokens.erase(tokens.begin());
			if (tokens.empty()) return true;
		}

		return false;
	}

};

// Character Set operations
inline auto Intersection (std::set<unsigned char>&A, std::set<unsigned char>&B, std::set<unsigned char>&Dest){
	return std::set_intersection(A.begin(), A.end(),
							B.begin(), B.end(),
							std::inserter(Dest, Dest.begin()));
}
inline auto Union (std::set<unsigned char>&A, std::set<unsigned char>&B, std::set<unsigned char>&Dest){
	return std::set_union(A.begin(), A.end(),
							B.begin(), B.end(),
							std::inserter(Dest, Dest.begin()));
}
inline auto Difference (std::set<unsigned char>&A, std::set<unsigned char>&B, std::set<unsigned char>&Dest){
	return std::set_difference(A.begin(), A.end(),
							B.begin(), B.end(),
							std::inserter(Dest, Dest.begin()));
}
inline bool Inclusion (std::set<unsigned char>&A, std::set<unsigned char>&B){
	return std::includes(A.begin(), A.end(),
							B.begin(), B.end());
}

