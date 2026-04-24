#pragma once

#include <any>

#include <antlr4-runtime.h>

#include "MiniCParser.h"

class MiniCVisitor : public antlr4::tree::ParseTreeVisitor {
public:
	virtual std::any visitFuncDef(MiniCParser::FuncDefContext *) { return std::any(); }
	virtual std::any visitFuncType(MiniCParser::FuncTypeContext *) { return std::any(); }
	virtual std::any visitAssignStatement(MiniCParser::AssignStatementContext *) { return std::any(); }
	virtual std::any visitBlockStatement(MiniCParser::BlockStatementContext *) { return std::any(); }
	virtual std::any visitExpressionStatement(MiniCParser::ExpressionStatementContext *) { return std::any(); }
	virtual std::any visitIfStatement(MiniCParser::IfStatementContext *) { return std::any(); }
	virtual std::any visitWhileStatement(MiniCParser::WhileStatementContext *) { return std::any(); }
	virtual std::any visitForStatement(MiniCParser::ForStatementContext *) { return std::any(); }
	virtual std::any visitBreakStatement(MiniCParser::BreakStatementContext *) { return std::any(); }
	virtual std::any visitContinueStatement(MiniCParser::ContinueStatementContext *) { return std::any(); }
	virtual std::any visitVarDef(MiniCParser::VarDefContext *) { return std::any(); }
	virtual std::any visitConstDecl(MiniCParser::ConstDeclContext *) { return std::any(); }
	virtual std::any visitConstDef(MiniCParser::ConstDefContext *) { return std::any(); }
	virtual std::any visitConstInitVal(MiniCParser::ConstInitValContext *) { return std::any(); }
	virtual std::any visitConstExp(MiniCParser::ConstExpContext *) { return std::any(); }
	virtual std::any visitInitVal(MiniCParser::InitValContext *) { return std::any(); }
	virtual std::any visitBasicType(MiniCParser::BasicTypeContext *) { return std::any(); }
	virtual std::any visitFuncFParam(MiniCParser::FuncFParamContext *) { return std::any(); }
	virtual std::any visitFuncFParams(MiniCParser::FuncFParamsContext *) { return std::any(); }
	virtual std::any visitFuncRParams(MiniCParser::FuncRParamsContext *) { return std::any(); }
	virtual std::any visitCompileUnit(MiniCParser::CompileUnitContext *) { return std::any(); }
	virtual std::any visitIdtail(MiniCParser::IdtailContext *) { return std::any(); }
	virtual std::any visitVarDeclList(MiniCParser::VarDeclListContext *) { return std::any(); }
	virtual std::any visitBlock(MiniCParser::BlockContext *) { return std::any(); }
	virtual std::any visitBlockItemList(MiniCParser::BlockItemListContext *) { return std::any(); }
	virtual std::any visitBlockItem(MiniCParser::BlockItemContext *) { return std::any(); }
	virtual std::any visitVarDecl(MiniCParser::VarDeclContext *) { return std::any(); }
	virtual std::any visitStatement(MiniCParser::StatementContext *) { return std::any(); }
	virtual std::any visitReturnStatement(MiniCParser::ReturnStatementContext *) { return std::any(); }
	virtual std::any visitAssignExprStmt(MiniCParser::AssignExprStmtContext *) { return std::any(); }
	virtual std::any visitAssignExprStmtTail(MiniCParser::AssignExprStmtTailContext *) { return std::any(); }
	virtual std::any visitExpr(MiniCParser::ExprContext *) { return std::any(); }
	virtual std::any visitAddExp(MiniCParser::AddExpContext *) { return std::any(); }
	virtual std::any visitMulExp(MiniCParser::MulExpContext *) { return std::any(); }
	virtual std::any visitUnaryExp(MiniCParser::UnaryExpContext *) { return std::any(); }
	virtual std::any visitUnaryOp(MiniCParser::UnaryOpContext *) { return std::any(); }
	virtual std::any visitCond(MiniCParser::CondContext *) { return std::any(); }
	virtual std::any visitLOrExp(MiniCParser::LOrExpContext *) { return std::any(); }
	virtual std::any visitLAndExp(MiniCParser::LAndExpContext *) { return std::any(); }
	virtual std::any visitEqExp(MiniCParser::EqExpContext *) { return std::any(); }
	virtual std::any visitRelExp(MiniCParser::RelExpContext *) { return std::any(); }
	virtual std::any visitEqOp(MiniCParser::EqOpContext *) { return std::any(); }
	virtual std::any visitRelOp(MiniCParser::RelOpContext *) { return std::any(); }
	virtual std::any visitLAndOp(MiniCParser::LAndOpContext *) { return std::any(); }
	virtual std::any visitLOrOp(MiniCParser::LOrOpContext *) { return std::any(); }
	virtual std::any visitRealParamList(MiniCParser::RealParamListContext *) { return std::any(); }
	virtual std::any visitAddOp(MiniCParser::AddOpContext *) { return std::any(); }
	virtual std::any visitMulOp(MiniCParser::MulOpContext *) { return std::any(); }
	virtual std::any visitPrimaryExp(MiniCParser::PrimaryExpContext *) { return std::any(); }
	virtual std::any visitNumber(MiniCParser::NumberContext *) { return std::any(); }
	virtual std::any visitLVal(MiniCParser::LValContext *) { return std::any(); }
};
