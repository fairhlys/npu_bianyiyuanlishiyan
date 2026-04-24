#pragma once

#include <any>
#include <memory>
#include <string>
#include <vector>

#include <antlr4-runtime.h>

class MiniCParser : public antlr4::Parser {
public:
	class FuncTypeContext : public antlr4::ParserRuleContext {};
	class FuncDefContext : public antlr4::ParserRuleContext {};
	class AssignStatementContext : public antlr4::ParserRuleContext {};
	class BlockStatementContext : public antlr4::ParserRuleContext {};
	class ExpressionStatementContext : public antlr4::ParserRuleContext {};
	class IfStatementContext : public antlr4::ParserRuleContext {};
	class WhileStatementContext : public antlr4::ParserRuleContext {};
	class ForStatementContext : public antlr4::ParserRuleContext {};
	class BreakStatementContext : public antlr4::ParserRuleContext {};
	class ContinueStatementContext : public antlr4::ParserRuleContext {};
	class VarDefContext : public antlr4::ParserRuleContext {};
	class ConstDeclContext : public antlr4::ParserRuleContext {};
	class ConstDefContext : public antlr4::ParserRuleContext {};
	class ConstInitValContext : public antlr4::ParserRuleContext {};
	class ConstExpContext : public antlr4::ParserRuleContext {};
	class InitValContext : public antlr4::ParserRuleContext {};
	class BasicTypeContext : public antlr4::ParserRuleContext {};
	class FuncFParamContext : public antlr4::ParserRuleContext {};
	class FuncFParamsContext : public antlr4::ParserRuleContext {};
	class FuncRParamsContext : public antlr4::ParserRuleContext {};
	class AssignExprStmtContext : public antlr4::ParserRuleContext {};
	class AssignExprStmtTailContext : public antlr4::ParserRuleContext {};
	class CompileUnitContext : public antlr4::ParserRuleContext {};
	class IdtailContext : public antlr4::ParserRuleContext {};
	class VarDeclListContext : public antlr4::ParserRuleContext {};
	class BlockContext : public antlr4::ParserRuleContext {};
	class BlockItemListContext : public antlr4::ParserRuleContext {};
	class BlockItemContext : public antlr4::ParserRuleContext {};
	class VarDeclContext : public antlr4::ParserRuleContext {};
	class StatementContext : public antlr4::ParserRuleContext {};
	class ReturnStatementContext : public antlr4::ParserRuleContext {};
	class ExprContext : public antlr4::ParserRuleContext {};
	class AddExpContext : public antlr4::ParserRuleContext {};
	class MulExpContext : public antlr4::ParserRuleContext {};
	class UnaryExpContext : public antlr4::ParserRuleContext {};
	class UnaryOpContext : public antlr4::ParserRuleContext {};
	class CondContext : public antlr4::ParserRuleContext {};
	class LOrExpContext : public antlr4::ParserRuleContext {};
	class LAndExpContext : public antlr4::ParserRuleContext {};
	class EqExpContext : public antlr4::ParserRuleContext {};
	class RelExpContext : public antlr4::ParserRuleContext {};
	class EqOpContext : public antlr4::ParserRuleContext {};
	class RelOpContext : public antlr4::ParserRuleContext {};
	class LAndOpContext : public antlr4::ParserRuleContext {};
	class LOrOpContext : public antlr4::ParserRuleContext {};
	class RealParamListContext : public antlr4::ParserRuleContext {};
	class AddOpContext : public antlr4::ParserRuleContext {};
	class MulOpContext : public antlr4::ParserRuleContext {};
	class PrimaryExpContext : public antlr4::ParserRuleContext {};
	class NumberContext : public antlr4::ParserRuleContext {};
	class LValContext : public antlr4::ParserRuleContext {};

	explicit MiniCParser(antlr4::TokenStream * input) : antlr4::Parser(input) {}
	~MiniCParser() override = default;

	FuncDefContext * funcDef() { return createContext<FuncDefContext>(); }
	FuncTypeContext * funcType() { return createContext<FuncTypeContext>(); }
	AssignStatementContext * assignStatement() { return createContext<AssignStatementContext>(); }
	BlockStatementContext * blockStatement() { return createContext<BlockStatementContext>(); }
	ExpressionStatementContext * expressionStatement() { return createContext<ExpressionStatementContext>(); }
	IfStatementContext * ifStatement() { return createContext<IfStatementContext>(); }
	WhileStatementContext * whileStatement() { return createContext<WhileStatementContext>(); }
	ForStatementContext * forStatement() { return createContext<ForStatementContext>(); }
	BreakStatementContext * breakStatement() { return createContext<BreakStatementContext>(); }
	ContinueStatementContext * continueStatement() { return createContext<ContinueStatementContext>(); }
	VarDefContext * varDef() { return createContext<VarDefContext>(); }
	ConstDeclContext * constDecl() { return createContext<ConstDeclContext>(); }
	ConstDefContext * constDef() { return createContext<ConstDefContext>(); }
	ConstInitValContext * constInitVal() { return createContext<ConstInitValContext>(); }
	ConstExpContext * constExp() { return createContext<ConstExpContext>(); }
	InitValContext * initVal() { return createContext<InitValContext>(); }
	BasicTypeContext * basicType() { return createContext<BasicTypeContext>(); }
	FuncFParamContext * funcFParam() { return createContext<FuncFParamContext>(); }
	FuncFParamsContext * funcFParams() { return createContext<FuncFParamsContext>(); }
	FuncRParamsContext * funcRParams() { return createContext<FuncRParamsContext>(); }

	CompileUnitContext * compileUnit() { return createContext<CompileUnitContext>(); }
	IdtailContext * idtail() { return createContext<IdtailContext>(); }
	VarDeclListContext * varDeclList() { return createContext<VarDeclListContext>(); }
	BlockContext * block() { return createContext<BlockContext>(); }
	BlockItemListContext * blockItemList() { return createContext<BlockItemListContext>(); }
	BlockItemContext * blockItem() { return createContext<BlockItemContext>(); }
	VarDeclContext * varDecl() { return createContext<VarDeclContext>(); }
	StatementContext * statement() { return createContext<StatementContext>(); }
	ReturnStatementContext * returnStatement() { return createContext<ReturnStatementContext>(); }
	AssignExprStmtContext * assignExprStmt() { return createContext<AssignExprStmtContext>(); }
	AssignExprStmtTailContext * assignExprStmtTail() { return createContext<AssignExprStmtTailContext>(); }
	ExprContext * expr() { return createContext<ExprContext>(); }
	AddExpContext * addExp() { return createContext<AddExpContext>(); }
	MulExpContext * mulExp() { return createContext<MulExpContext>(); }
	UnaryExpContext * unaryExp() { return createContext<UnaryExpContext>(); }
	UnaryOpContext * unaryOp() { return createContext<UnaryOpContext>(); }
	CondContext * cond() { return createContext<CondContext>(); }
	LOrExpContext * lOrExp() { return createContext<LOrExpContext>(); }
	LAndExpContext * lAndExp() { return createContext<LAndExpContext>(); }
	EqExpContext * eqExp() { return createContext<EqExpContext>(); }
	RelExpContext * relExp() { return createContext<RelExpContext>(); }
	EqOpContext * eqOp() { return createContext<EqOpContext>(); }
	RelOpContext * relOp() { return createContext<RelOpContext>(); }
	LAndOpContext * lAndOp() { return createContext<LAndOpContext>(); }
	LOrOpContext * lOrOp() { return createContext<LOrOpContext>(); }
	RealParamListContext * realParamList() { return createContext<RealParamListContext>(); }
	AddOpContext * addOp() { return createContext<AddOpContext>(); }
	MulOpContext * mulOp() { return createContext<MulOpContext>(); }
	PrimaryExpContext * primaryExp() { return createContext<PrimaryExpContext>(); }
	NumberContext * number() { return createContext<NumberContext>(); }
	LValContext * lVal() { return createContext<LValContext>(); }

	std::string getGrammarFileName() const override { return "MiniC.g4"; }
	const std::vector<std::string> & getRuleNames() const override { return ruleNames; }
	const antlr4::dfa::Vocabulary & getVocabulary() const override { return vocabulary; }
	antlr4::atn::SerializedATNView getSerializedATN() const override { return serializedATN; }

private:
	template <typename ContextT>
	ContextT * createContext()
	{
		return new ContextT();
	}

	static inline const std::vector<std::string> ruleNames {
		"funcDef", "funcType", "assignStatement", "blockStatement", "expressionStatement",
		"ifStatement", "whileStatement", "forStatement", "breakStatement", "continueStatement", "varDef",
		"constDecl", "constDef", "constInitVal", "constExp", "initVal", "basicType", "funcFParam",
		"funcFParams", "funcRParams", "compileUnit", "idtail", "varDeclList", "block", "blockItemList",
		"blockItem", "varDecl", "statement", "returnStatement", "assignExprStmt", "assignExprStmtTail",
		"expr", "addExp", "mulExp", "unaryExp", "unaryOp", "cond", "lOrExp", "lAndExp", "eqExp",
		"relExp", "eqOp", "relOp", "lAndOp", "lOrOp", "realParamList", "addOp", "mulOp",
		"primaryExp", "number", "lVal"
	};

	static inline const antlr4::dfa::Vocabulary vocabulary {};
	static inline const antlr4::atn::SerializedATNView serializedATN {};
};
