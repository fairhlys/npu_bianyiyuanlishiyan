#pragma once

#include <any>

#include <antlr4-runtime.h>

#include "MiniCBaseVisitor.h"

class Antlr4CSTVisitor : public MiniCBaseVisitor {
public:
	std::any visitFuncDef(MiniCParser::FuncDefContext *) override { return std::any(); }
	std::any visitFuncType(MiniCParser::FuncTypeContext *) override { return std::any(); }
	std::any visitAssignStatement(MiniCParser::AssignStatementContext *) override { return std::any(); }
	std::any visitBlockStatement(MiniCParser::BlockStatementContext *) override { return std::any(); }
	std::any visitExpressionStatement(MiniCParser::ExpressionStatementContext *) override { return std::any(); }
	std::any visitIfStatement(MiniCParser::IfStatementContext *) override { return std::any(); }
	std::any visitWhileStatement(MiniCParser::WhileStatementContext *) override { return std::any(); }
	std::any visitForStatement(MiniCParser::ForStatementContext *) override { return std::any(); }
	std::any visitBreakStatement(MiniCParser::BreakStatementContext *) override { return std::any(); }
	std::any visitContinueStatement(MiniCParser::ContinueStatementContext *) override { return std::any(); }
	std::any visitVarDef(MiniCParser::VarDefContext *) override { return std::any(); }
	std::any visitConstDecl(MiniCParser::ConstDeclContext *) override { return std::any(); }
	std::any visitConstDef(MiniCParser::ConstDefContext *) override { return std::any(); }
	std::any visitConstInitVal(MiniCParser::ConstInitValContext *) override { return std::any(); }
	std::any visitConstExp(MiniCParser::ConstExpContext *) override { return std::any(); }
	std::any visitInitVal(MiniCParser::InitValContext *) override { return std::any(); }
	std::any visitBasicType(MiniCParser::BasicTypeContext *) override { return std::any(); }
	std::any visitFuncFParam(MiniCParser::FuncFParamContext *) override { return std::any(); }
	std::any visitFuncFParams(MiniCParser::FuncFParamsContext *) override { return std::any(); }
	std::any visitFuncRParams(MiniCParser::FuncRParamsContext *) override { return std::any(); }
	std::any visitCompileUnit(MiniCParser::CompileUnitContext *) override { return std::any(); }
	std::any visitIdtail(MiniCParser::IdtailContext *) override { return std::any(); }
	std::any visitVarDeclList(MiniCParser::VarDeclListContext *) override { return std::any(); }
	std::any visitBlock(MiniCParser::BlockContext *) override { return std::any(); }
	std::any visitBlockItemList(MiniCParser::BlockItemListContext *) override { return std::any(); }
	std::any visitBlockItem(MiniCParser::BlockItemContext *) override { return std::any(); }
	std::any visitVarDecl(MiniCParser::VarDeclContext *) override { return std::any(); }
	std::any visitStatement(MiniCParser::StatementContext *) override { return std::any(); }
	std::any visitReturnStatement(MiniCParser::ReturnStatementContext *) override { return std::any(); }
	std::any visitAssignExprStmt(MiniCParser::AssignExprStmtContext *) override { return std::any(); }
	std::any visitAssignExprStmtTail(MiniCParser::AssignExprStmtTailContext *) override { return std::any(); }
	std::any visitExpr(MiniCParser::ExprContext *) override { return std::any(); }
	std::any visitAddExp(MiniCParser::AddExpContext *) override { return std::any(); }
	std::any visitMulExp(MiniCParser::MulExpContext *) override { return std::any(); }
	std::any visitUnaryExp(MiniCParser::UnaryExpContext *) override { return std::any(); }
	std::any visitUnaryOp(MiniCParser::UnaryOpContext *) override { return std::any(); }
	std::any visitCond(MiniCParser::CondContext *) override { return std::any(); }
	std::any visitLOrExp(MiniCParser::LOrExpContext *) override { return std::any(); }
	std::any visitLAndExp(MiniCParser::LAndExpContext *) override { return std::any(); }
	std::any visitEqExp(MiniCParser::EqExpContext *) override { return std::any(); }
	std::any visitRelExp(MiniCParser::RelExpContext *) override { return std::any(); }
	std::any visitEqOp(MiniCParser::EqOpContext *) override { return std::any(); }
	std::any visitRelOp(MiniCParser::RelOpContext *) override { return std::any(); }
	std::any visitLAndOp(MiniCParser::LAndOpContext *) override { return std::any(); }
	std::any visitLOrOp(MiniCParser::LOrOpContext *) override { return std::any(); }
	std::any visitRealParamList(MiniCParser::RealParamListContext *) override { return std::any(); }
	std::any visitAddOp(MiniCParser::AddOpContext *) override { return std::any(); }
	std::any visitMulOp(MiniCParser::MulOpContext *) override { return std::any(); }
	std::any visitPrimaryExp(MiniCParser::PrimaryExpContext *) override { return std::any(); }
	std::any visitNumber(MiniCParser::NumberContext *) override { return std::any(); }
	std::any visitLVal(MiniCParser::LValContext *) override { return std::any(); }
};
