#pragma once

#include <antlr4-runtime.h>

class MiniCLexer : public antlr4::Lexer {
public:
	explicit MiniCLexer(antlr4::CharStream * input) : antlr4::Lexer(input) {}
	~MiniCLexer() override = default;

	std::string getGrammarFileName() const override { return "MiniC.g4"; }
	const std::vector<std::string> & getRuleNames() const override { return ruleNames; }
	const antlr4::dfa::Vocabulary & getVocabulary() const override { return vocabulary; }
	antlr4::atn::SerializedATNView getSerializedATN() const override { return serializedATN; }

private:
	static inline const std::vector<std::string> ruleNames {};
	static inline const antlr4::dfa::Vocabulary vocabulary {};
	static inline const antlr4::atn::SerializedATNView serializedATN {};
};
