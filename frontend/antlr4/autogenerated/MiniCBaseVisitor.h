#pragma once

#include <any>

#include "MiniCVisitor.h"

class MiniCBaseVisitor : public MiniCVisitor {
public:
	std::any visitFuncDef(MiniCParser::FuncDefContext * ctx) override { return visitChildren(ctx); }
	std::any visitFuncType(MiniCParser::FuncTypeContext * ctx) override { return visitChildren(ctx); }
	std::any visitAssignStatement(MiniCParser::AssignStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitBlockStatement(MiniCParser::BlockStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitExpressionStatement(MiniCParser::ExpressionStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitIfStatement(MiniCParser::IfStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitWhileStatement(MiniCParser::WhileStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitForStatement(MiniCParser::ForStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitBreakStatement(MiniCParser::BreakStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitContinueStatement(MiniCParser::ContinueStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitVarDef(MiniCParser::VarDefContext * ctx) override { return visitChildren(ctx); }
	std::any visitConstDecl(MiniCParser::ConstDeclContext * ctx) override { return visitChildren(ctx); }
	std::any visitConstDef(MiniCParser::ConstDefContext * ctx) override { return visitChildren(ctx); }
	std::any visitConstInitVal(MiniCParser::ConstInitValContext * ctx) override { return visitChildren(ctx); }
	std::any visitConstExp(MiniCParser::ConstExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitInitVal(MiniCParser::InitValContext * ctx) override { return visitChildren(ctx); }
	std::any visitBasicType(MiniCParser::BasicTypeContext * ctx) override { return visitChildren(ctx); }
	std::any visitFuncFParam(MiniCParser::FuncFParamContext * ctx) override { return visitChildren(ctx); }
	std::any visitFuncFParams(MiniCParser::FuncFParamsContext * ctx) override { return visitChildren(ctx); }
	std::any visitFuncRParams(MiniCParser::FuncRParamsContext * ctx) override { return visitChildren(ctx); }
	std::any visitCompileUnit(MiniCParser::CompileUnitContext * ctx) override { return visitChildren(ctx); }
	std::any visitIdtail(MiniCParser::IdtailContext * ctx) override { return visitChildren(ctx); }
	std::any visitVarDeclList(MiniCParser::VarDeclListContext * ctx) override { return visitChildren(ctx); }
	std::any visitBlock(MiniCParser::BlockContext * ctx) override { return visitChildren(ctx); }
	std::any visitBlockItemList(MiniCParser::BlockItemListContext * ctx) override { return visitChildren(ctx); }
	std::any visitBlockItem(MiniCParser::BlockItemContext * ctx) override { return visitChildren(ctx); }
	std::any visitVarDecl(MiniCParser::VarDeclContext * ctx) override { return visitChildren(ctx); }
	std::any visitStatement(MiniCParser::StatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitReturnStatement(MiniCParser::ReturnStatementContext * ctx) override { return visitChildren(ctx); }
	std::any visitAssignExprStmt(MiniCParser::AssignExprStmtContext * ctx) override { return visitChildren(ctx); }
	std::any visitAssignExprStmtTail(MiniCParser::AssignExprStmtTailContext * ctx) override { return visitChildren(ctx); }
	std::any visitExpr(MiniCParser::ExprContext * ctx) override { return visitChildren(ctx); }
	std::any visitAddExp(MiniCParser::AddExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitMulExp(MiniCParser::MulExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitUnaryExp(MiniCParser::UnaryExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitUnaryOp(MiniCParser::UnaryOpContext * ctx) override { return visitChildren(ctx); }
	std::any visitCond(MiniCParser::CondContext * ctx) override { return visitChildren(ctx); }
	std::any visitLOrExp(MiniCParser::LOrExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitLAndExp(MiniCParser::LAndExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitEqExp(MiniCParser::EqExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitRelExp(MiniCParser::RelExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitEqOp(MiniCParser::EqOpContext * ctx) override { return visitChildren(ctx); }
	std::any visitRelOp(MiniCParser::RelOpContext * ctx) override { return visitChildren(ctx); }
	std::any visitLAndOp(MiniCParser::LAndOpContext * ctx) override { return visitChildren(ctx); }
	std::any visitLOrOp(MiniCParser::LOrOpContext * ctx) override { return visitChildren(ctx); }
	std::any visitRealParamList(MiniCParser::RealParamListContext * ctx) override { return visitChildren(ctx); }
	std::any visitAddOp(MiniCParser::AddOpContext * ctx) override { return visitChildren(ctx); }
	std::any visitMulOp(MiniCParser::MulOpContext * ctx) override { return visitChildren(ctx); }
	std::any visitPrimaryExp(MiniCParser::PrimaryExpContext * ctx) override { return visitChildren(ctx); }
	std::any visitNumber(MiniCParser::NumberContext * ctx) override { return visitChildren(ctx); }
	std::any visitLVal(MiniCParser::LValContext * ctx) override { return visitChildren(ctx); }
};
