///
/// @file IRGenerator.cpp
/// @brief AST遍历产生线性IR的源文件
/// @author zenglj (zenglj@live.com)
/// @version 1.1
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// <tr><td>2024-11-23 <td>1.1     <td>zenglj  <td>表达式版增强
/// </table>
///
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "AST.h"
#include "Common.h"
#include "Function.h"
#include "IRCode.h"
#include "IRGenerator.h"
#include "Module.h"
#include "Types/ArrayType.h"
#include "Types/PointerType.h"
#include "EntryInstruction.h"
#include "LabelInstruction.h"
#include "ExitInstruction.h"
#include "FuncCallInstruction.h"
#include "BinaryInstruction.h"
#include "MoveInstruction.h"
#include "GotoInstruction.h"
#include "CondBrInstruction.h"
#include "MemVariable.h"

namespace {
	int32_t calc_array_stride_elements(const std::vector<int32_t> & dims, size_t index)
	{
		int64_t stride = 1;
		for (size_t pos = index + 1; pos < dims.size(); ++pos) {
			int32_t dim = dims[pos];
			if (dim <= 0) {
				continue;
			}
			stride *= dim;
		}
		return (int32_t) stride;
	}

	const ArrayType * get_access_array_type(Type * type)
	{
		if (!type) {
			return nullptr;
		}

		if (type->isArrayType()) {
			return dynamic_cast<ArrayType *>(type);
		}

		if (type->isPointerType()) {
			auto * pointerType = dynamic_cast<PointerType *>(type);
			if (!pointerType) {
				return nullptr;
			}

			return dynamic_cast<const ArrayType *>(pointerType->getPointeeType());
		}

		return nullptr;
	}

	Type * get_parameter_decay_type(const ArrayType * arrayType)
	{
		if (!arrayType) {
			return nullptr;
		}

		return const_cast<PointerType *>(PointerType::get(const_cast<Type *>(arrayType->getElementType())));
	}

} // namespace

namespace {
	IRInstOperator ast_cmp_to_ir(ast_operator_type type)
	{
		switch (type) {
			case ast_operator_type::AST_OP_EQ:
				return IRInstOperator::IRINST_OP_ICMP_EQ;
			case ast_operator_type::AST_OP_NE:
				return IRInstOperator::IRINST_OP_ICMP_NE;
			case ast_operator_type::AST_OP_LT:
				return IRInstOperator::IRINST_OP_ICMP_LT;
			case ast_operator_type::AST_OP_LE:
				return IRInstOperator::IRINST_OP_ICMP_LE;
			case ast_operator_type::AST_OP_GT:
				return IRInstOperator::IRINST_OP_ICMP_GT;
			case ast_operator_type::AST_OP_GE:
				return IRInstOperator::IRINST_OP_ICMP_GE;
			default:
				return IRInstOperator::IRINST_OP_MAX;
		}
	}

	bool ends_with_terminator(const InterCode & code)
	{
		auto & insts = const_cast<InterCode &>(code).getInsts();
		if (insts.empty()) {
			return false;
		}

		switch (insts.back()->getOp()) {
			case IRInstOperator::IRINST_OP_GOTO:
			case IRInstOperator::IRINST_OP_BR:
			case IRInstOperator::IRINST_OP_EXIT:
				return true;
			default:
				return false;
		}
	}

	bool eval_static_const_expr(ast_node * node, int32_t & outVal)
	{
		if (!node) {
			return false;
		}

		switch (node->node_type) {
			case ast_operator_type::AST_OP_LEAF_LITERAL_UINT:
				outVal = (int32_t) node->integer_val;
				return true;
			case ast_operator_type::AST_OP_ADD:
			case ast_operator_type::AST_OP_SUB:
			case ast_operator_type::AST_OP_MUL:
			case ast_operator_type::AST_OP_DIV:
			case ast_operator_type::AST_OP_MOD:
			case ast_operator_type::AST_OP_LT:
			case ast_operator_type::AST_OP_LE:
			case ast_operator_type::AST_OP_GT:
			case ast_operator_type::AST_OP_GE:
			case ast_operator_type::AST_OP_EQ:
			case ast_operator_type::AST_OP_NE:
			case ast_operator_type::AST_OP_LAND:
			case ast_operator_type::AST_OP_LOR: {
				if (node->sons.size() != 2) {
					return false;
				}
				int32_t lhs = 0, rhs = 0;
				if (!eval_static_const_expr(node->sons[0], lhs) || !eval_static_const_expr(node->sons[1], rhs)) {
					return false;
				}

				switch (node->node_type) {
					case ast_operator_type::AST_OP_ADD:
						outVal = lhs + rhs;
						break;
					case ast_operator_type::AST_OP_SUB:
						outVal = lhs - rhs;
						break;
					case ast_operator_type::AST_OP_MUL:
						outVal = lhs * rhs;
						break;
					case ast_operator_type::AST_OP_DIV:
						if (rhs == 0) {
							return false;
						}
						outVal = lhs / rhs;
						break;
					case ast_operator_type::AST_OP_MOD:
						if (rhs == 0) {
							return false;
						}
						outVal = lhs % rhs;
						break;
					case ast_operator_type::AST_OP_LT:
						outVal = lhs < rhs;
						break;
					case ast_operator_type::AST_OP_LE:
						outVal = lhs <= rhs;
						break;
					case ast_operator_type::AST_OP_GT:
						outVal = lhs > rhs;
						break;
					case ast_operator_type::AST_OP_GE:
						outVal = lhs >= rhs;
						break;
					case ast_operator_type::AST_OP_EQ:
						outVal = lhs == rhs;
						break;
					case ast_operator_type::AST_OP_NE:
						outVal = lhs != rhs;
						break;
					case ast_operator_type::AST_OP_LAND:
						outVal = (lhs != 0) && (rhs != 0);
						break;
					case ast_operator_type::AST_OP_LOR:
						outVal = (lhs != 0) || (rhs != 0);
						break;
					default:
						return false;
				}
				return true;
			}
			case ast_operator_type::AST_OP_LNOT: {
				if (node->sons.size() != 1) {
					return false;
				}
				int32_t v = 0;
				if (!eval_static_const_expr(node->sons[0], v)) {
					return false;
				}
				outVal = (v == 0);
				return true;
			}
			default:
				return false;
		}
	}

} // namespace

/// @brief 构造函数
/// @param _root AST的根
/// @param _module 符号表
IRGenerator::IRGenerator(ast_node * _root, Module * _module) : root(_root), module(_module)
{
	/* 叶子节点 */
	ast2ir_handlers[ast_operator_type::AST_OP_LEAF_LITERAL_UINT] = &IRGenerator::ir_leaf_node_uint;
	ast2ir_handlers[ast_operator_type::AST_OP_LEAF_VAR_ID] = &IRGenerator::ir_leaf_node_var_id;
	ast2ir_handlers[ast_operator_type::AST_OP_LEAF_TYPE] = &IRGenerator::ir_leaf_node_type;

	/* 表达式运算， 加减 */
	ast2ir_handlers[ast_operator_type::AST_OP_SUB] = &IRGenerator::ir_sub;
	ast2ir_handlers[ast_operator_type::AST_OP_ADD] = &IRGenerator::ir_add;
	ast2ir_handlers[ast_operator_type::AST_OP_MUL] = &IRGenerator::ir_mul;
	ast2ir_handlers[ast_operator_type::AST_OP_DIV] = &IRGenerator::ir_div;
	ast2ir_handlers[ast_operator_type::AST_OP_MOD] = &IRGenerator::ir_mod;
	ast2ir_handlers[ast_operator_type::AST_OP_LT] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_LE] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_GT] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_GE] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_EQ] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_NE] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_LAND] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_LOR] = &IRGenerator::ir_bool_expr;
	ast2ir_handlers[ast_operator_type::AST_OP_LNOT] = &IRGenerator::ir_bool_expr;

	/* 语句 */
	ast2ir_handlers[ast_operator_type::AST_OP_ASSIGN] = &IRGenerator::ir_assign;
	ast2ir_handlers[ast_operator_type::AST_OP_RETURN] = &IRGenerator::ir_return;
	ast2ir_handlers[ast_operator_type::AST_OP_IF] = &IRGenerator::ir_if;
	ast2ir_handlers[ast_operator_type::AST_OP_WHILE] = &IRGenerator::ir_while;
	ast2ir_handlers[ast_operator_type::AST_OP_FOR] = &IRGenerator::ir_for;
	ast2ir_handlers[ast_operator_type::AST_OP_BREAK] = &IRGenerator::ir_break;
	ast2ir_handlers[ast_operator_type::AST_OP_CONTINUE] = &IRGenerator::ir_continue;

	/* 函数调用 */
	ast2ir_handlers[ast_operator_type::AST_OP_FUNC_CALL] = &IRGenerator::ir_function_call;

	/* 函数定义 */
	ast2ir_handlers[ast_operator_type::AST_OP_FUNC_DEF] = &IRGenerator::ir_function_define;
	ast2ir_handlers[ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS] = &IRGenerator::ir_function_formal_params;

	/* 变量定义语句 */
	ast2ir_handlers[ast_operator_type::AST_OP_DECL_STMT] = &IRGenerator::ir_declare_statment;
	ast2ir_handlers[ast_operator_type::AST_OP_VAR_DECL] = &IRGenerator::ir_variable_declare;
	ast2ir_handlers[ast_operator_type::AST_OP_ARRAY_ACCESS] = &IRGenerator::ir_array_access;

	/* 语句块 */
	ast2ir_handlers[ast_operator_type::AST_OP_BLOCK] = &IRGenerator::ir_block;

	/* 编译单元 */
	ast2ir_handlers[ast_operator_type::AST_OP_COMPILE_UNIT] = &IRGenerator::ir_compile_unit;
}

/// @brief 遍历抽象语法树产生线性IR，保存到IRCode中
/// @param root 抽象语法树
/// @param IRCode 线性IR
/// @return true: 成功 false: 失败
bool IRGenerator::run()
{
	ast_node * node;

	// 从根节点进行遍历
	node = ir_visit_ast_node(root);

	return node != nullptr;
}

/// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
/// @param node AST节点
/// @return 成功返回node节点，否则返回nullptr
ast_node * IRGenerator::ir_visit_ast_node(ast_node * node)
{
	// 空节点
	if (nullptr == node) {
		return nullptr;
	}

	bool result;

	std::unordered_map<ast_operator_type, ast2ir_handler_t>::const_iterator pIter;
	pIter = ast2ir_handlers.find(node->node_type);
	if (pIter == ast2ir_handlers.end()) {
		// 没有找到，则说明当前不支持
		result = (this->ir_default)(node);
	} else {
		result = (this->*(pIter->second))(node);
	}

	if (!result) {
		node = nullptr;
	}

	return node;
}

/// @brief 未知节点类型的节点处理
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_default(ast_node * node)
{
	// 未知的节点
	printf("Unkown node(%d)\n", (int) node->node_type);
	return true;
}

/// @brief 编译单元AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_compile_unit(ast_node * node)
{
	module->setCurrentFunction(nullptr);

	for (auto son: node->sons) {

		// 遍历编译单元，要么是函数定义，要么是语句
		ast_node * son_node = ir_visit_ast_node(son);
		if (!son_node) {
			// TODO 自行追加语义错误处理
			return false;
		}
	}

	// 全局变量初始化：统一在main入口注入
	auto initList = module->consumeGlobalInits();
	if (!initList.empty()) {
		Function * mainFunc = module->findFunction("main");
		if (!mainFunc) {
			minic_log(LOG_ERROR, "存在全局初始化但未找到main函数");
			return false;
		}

		auto & insts = mainFunc->getInterCode().getInsts();
		size_t insertPos = 0;
		if (!insts.empty() && insts[0]->getOp() == IRInstOperator::IRINST_OP_ENTRY) {
			insertPos = 1;
		}

		std::vector<Instruction *> initInsts;
		initInsts.reserve(initList.size());
		for (auto & item: initList) {
			GlobalVariable * gv = item.first;
			ConstInt * initVal = module->newConstInt(item.second);
			initInsts.push_back(new MoveInstruction(mainFunc, gv, initVal));
		}

		insts.insert(insts.begin() + (long long) insertPos, initInsts.begin(), initInsts.end());
	}

	return true;
}

/// @brief 函数定义AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_define(ast_node * node)
{
	bool result;

	// 创建一个函数，用于当前函数处理
	if (module->getCurrentFunction()) {
		// 函数中嵌套定义函数，这是不允许的，错误退出
		// TODO 自行追加语义错误处理
		return false;
	}

	// 函数定义的AST包含四个孩子
	// 第一个孩子：函数返回类型
	// 第二个孩子：函数名字
	// 第三个孩子：形参列表
	// 第四个孩子：函数体即block
	ast_node * type_node = node->sons[0];
	ast_node * name_node = node->sons[1];
	ast_node * param_node = node->sons[2];
	ast_node * block_node = node->sons[3];

	std::vector<FormalParam *> formalParams;
	for (auto paramAst: param_node->sons) {
		formalParams.push_back(new FormalParam(paramAst->type, paramAst->name));
	}

	// 创建一个新的函数定义
	Function * newFunc = module->newFunction(name_node->name, type_node->type, formalParams);
	if (!newFunc) {
		// 新定义的函数已经存在，则失败返回。
		// TODO 自行追加语义错误处理
		return false;
	}

	// 当前函数设置有效，变更为当前的函数
	module->setCurrentFunction(newFunc);

	// 进入函数的作用域
	module->enterScope();

	// 获取函数的IR代码列表，用于后面追加指令用，注意这里用的是引用传值
	InterCode & irCode = newFunc->getInterCode();

	// 这里也可增加一个函数入口Label指令，便于后续基本块划分

	// 创建并加入Entry入口指令
	irCode.addInst(new EntryInstruction(newFunc));

	// 创建出口指令并不加入出口指令，等函数内的指令处理完毕后加入出口指令
	LabelInstruction * exitLabelInst = new LabelInstruction(newFunc);

	// 函数出口指令保存到函数信息中，因为在语义分析函数体时return语句需要跳转到函数尾部，需要这个label指令
	newFunc->setExitLabel(exitLabelInst);

	// 遍历形参，没有IR指令，不需要追加
	result = ir_function_formal_params(param_node);
	if (!result) {
		// 形参解析失败
		// TODO 自行追加语义错误处理
		return false;
	}
	node->blockInsts.addInst(param_node->blockInsts);

	// 新建一个Value，用于保存函数的返回值，如果没有返回值可不用申请
	LocalVariable * retValue = nullptr;
	if (!type_node->type->isVoidType()) {

		// 保存函数返回值变量到函数信息中，在return语句翻译时需要设置值到这个变量中
		retValue = static_cast<LocalVariable *>(module->newVarValue(type_node->type));

		// 统一将返回值初始化为0，避免函数末尾没有显式return时产生随机返回值
		ConstInt * zeroVal = module->newConstInt(0);
		node->blockInsts.addInst(new MoveInstruction(newFunc, retValue, zeroVal));
	}
	newFunc->setReturnValue(retValue);

	// 这里最好设置返回值变量的初值为0，以便在没有返回值时能够返回0

	// 函数内已经进入作用域，内部不再需要做变量的作用域管理
	block_node->needScope = false;

	// 遍历block
	result = ir_block(block_node);
	if (!result) {
		// block解析失败
		// TODO 自行追加语义错误处理
		return false;
	}

	// IR指令追加到当前的节点中
	node->blockInsts.addInst(block_node->blockInsts);

	// 此时，所有指令都加入到当前函数中，也就是node->blockInsts

	// node节点的指令移动到函数的IR指令列表中
	irCode.addInst(node->blockInsts);

	// 添加函数出口Label指令，主要用于return语句跳转到这里进行函数的退出
	irCode.addInst(exitLabelInst);

	// 函数出口指令
	irCode.addInst(new ExitInstruction(newFunc, retValue));

	// 恢复成外部函数
	module->setCurrentFunction(nullptr);

	// 退出函数的作用域
	module->leaveScope();

	return true;
}

/// @brief 形式参数AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_formal_params(ast_node * node)
{
	Function * currentFunc = module->getCurrentFunction();
	if (!currentFunc) {
		return false;
	}

	auto & params = currentFunc->getParams();
	if (params.size() != node->sons.size()) {
		minic_log(LOG_ERROR, "函数形参个数不匹配");
		return false;
	}

	for (size_t index = 0; index < params.size(); ++index) {
		FormalParam * formalParam = params[index];
		ast_node * paramNode = node->sons[index];

		// 形参对应两个值：一个是入口处接收实参的形参值，一个是函数体内使用的局部变量
		Value * localVar = module->newVarValue(formalParam->getType(), formalParam->getName());
		if (!localVar) {
			return false;
		}

		// 函数入口把形参值复制到局部变量，后续函数体只使用局部变量
		node->blockInsts.addInst(new MoveInstruction(currentFunc, localVar, formalParam));

		// 形参节点本身不再直接进入作用域，避免和局部变量混用
		(void) paramNode;
	}

	return true;
}

/// @brief 语句块（含函数体）AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_block(ast_node * node)
{
	// 进入作用域
	if (node->needScope) {
		module->enterScope();
	}

	std::vector<ast_node *>::iterator pIter;
	for (pIter = node->sons.begin(); pIter != node->sons.end(); ++pIter) {
		if (*pIter == nullptr) {
			// 空语句(;)在语句块中按no-op处理
			continue;
		}

		// 遍历Block的每个语句，进行显示或者运算
		ast_node * temp = ir_visit_ast_node(*pIter);
		if (!temp) {
			return false;
		}

		node->blockInsts.addInst(temp->blockInsts);
	}

	// 离开作用域
	if (node->needScope) {
		module->leaveScope();
	}

	return true;
}

/// @brief return节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_return(ast_node * node)
{
	ast_node * right = nullptr;
	Function * currentFunc = module->getCurrentFunction();
	if (!currentFunc) {
		return false;
	}

	Type * retType = currentFunc->getReturnType();

	// return语句可能没有没有表达式，也可能有，因此这里必须进行区分判断
	if (!node->sons.empty()) {

		ast_node * son_node = node->sons[0];

		// 返回的表达式的指令保存在right节点中
		right = ir_visit_ast_node(son_node);
		if (!right) {

			// 某个变量没有定值
			return false;
		}

		if (retType->isVoidType()) {
			minic_log(LOG_ERROR, "void函数中return不能带返回值");
			return false;
		}
	} else {
		if (!retType->isVoidType()) {
			minic_log(LOG_ERROR, "非void函数中return必须带返回值");
			return false;
		}
	}

	// 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

	// 返回值存在时则移动指令到node中
	if (right) {

		// 创建临时变量保存IR的值，以及线性IR指令
		node->blockInsts.addInst(right->blockInsts);

		// 返回值赋值到函数返回值变量上，然后跳转到函数的尾部
		node->blockInsts.addInst(new MoveInstruction(currentFunc, currentFunc->getReturnValue(), right->val));

		node->val = right->val;
	} else {
		// 没有返回值
		node->val = nullptr;
	}

	// 跳转到函数的尾部出口指令上
	node->blockInsts.addInst(new GotoInstruction(currentFunc, currentFunc->getExitLabel()));

	return true;
}

/// @brief 类型叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_type(ast_node * node)
{
	// 不需要做什么，直接从节点中获取即可。

	return true;
}

/// @brief 无符号整数字面量叶子节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_uint(ast_node * node)
{
	ConstInt * val;

	// 新建一个整数常量Value
	val = module->newConstInt((int32_t) node->integer_val);

	node->val = val;

	return true;
}

/// @brief 标识符叶子节点翻译成线性中间IR，变量声明的不走这个语句
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_leaf_node_var_id(ast_node * node)
{
	Value * val;

	// 查找ID型Value
	// 变量，则需要在符号表中查找对应的值

	val = module->findVarValue(node->name);

	node->val = val;

	return true;
}

/// @brief 数组下标访问AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_array_access(ast_node * node)
{
	if (node->sons.size() < 2) {
		return false;
	}

	ast_node * id_node = node->sons[0];
	ast_node * index_node = node->sons[1];

	ast_node * base = ir_visit_ast_node(id_node);
	if (!base || !base->val) {
		return false;
	}

	Value * baseVal = base->val;
	Type * baseType = baseVal->getType();
	const ArrayType * arrayType = get_access_array_type(baseType);
	if (!arrayType) {
		minic_log(LOG_ERROR, "数组访问对象不是数组类型");
		return false;
	}

	const auto & dims = arrayType->getDims();
	if (dims.empty()) {
		return false;
	}

	node->blockInsts.addInst(base->blockInsts);

	// 逐个下标计算线性偏移
	if (index_node->sons.size() > dims.size()) {
		minic_log(LOG_ERROR, "数组下标维度过多");
		return false;
	}

	Value * linearIndexVal = nullptr;

	for (size_t index = 0; index < index_node->sons.size(); ++index) {
		ast_node * subscript = ir_visit_ast_node(index_node->sons[index]);
		if (!subscript || !subscript->val) {
			return false;
		}

		node->blockInsts.addInst(subscript->blockInsts);

		Value * termVal = subscript->val;
		int32_t strideElements = calc_array_stride_elements(dims, index);
		if (strideElements != 1) {
			ConstInt * strideVal = module->newConstInt(strideElements);
			BinaryInstruction * scaledIndex = new BinaryInstruction(
				module->getCurrentFunction(),
				IRInstOperator::IRINST_OP_MUL_I,
				subscript->val,
				strideVal,
				IntegerType::getTypeInt());
			node->blockInsts.addInst(scaledIndex);
			termVal = scaledIndex;
		}

		if (!linearIndexVal) {
			linearIndexVal = termVal;
		} else {
			BinaryInstruction * mergedIndex = new BinaryInstruction(
				module->getCurrentFunction(),
				IRInstOperator::IRINST_OP_ADD_I,
				linearIndexVal,
				termVal,
				IntegerType::getTypeInt());
			node->blockInsts.addInst(mergedIndex);
			linearIndexVal = mergedIndex;
		}
	}

	if (!linearIndexVal) {
		linearIndexVal = module->newConstInt(0);
	}

	ConstInt * elementSizeVal = module->newConstInt(arrayType->getElementType()->getSize());
	BinaryInstruction * byteOffset = new BinaryInstruction(
		module->getCurrentFunction(),
		IRInstOperator::IRINST_OP_MUL_I,
		linearIndexVal,
		elementSizeVal,
		IntegerType::getTypeInt());
	node->blockInsts.addInst(byteOffset);

	Type * decayType = get_parameter_decay_type(arrayType);
	BinaryInstruction * addressVal = new BinaryInstruction(
		module->getCurrentFunction(), IRInstOperator::IRINST_OP_ADD_I, baseVal, byteOffset, decayType);
	node->blockInsts.addInst(addressVal);

	if (index_node->sons.size() < dims.size()) {
		node->val = addressVal;
		return true;
	}

	// 检查是否为左值（赋值的左操作数）
	bool isAssignLValue = node->parent && node->parent->node_type == ast_operator_type::AST_OP_ASSIGN &&
						  !node->parent->sons.empty() && node->parent->sons[0] == node;
	
	// 无论左值还是右值，都创建一个 MemVariable 来表示内存中的值
	MemVariable * elemVar =
		module->getCurrentFunction()->newMemVariable((Type *) PointerType::get(IntegerType::getTypeInt()));
	if (!elemVar) {
		return false;
	}
	elemVar->setMemoryAddr(addressVal, 0);
	// 同步生成文本IR中的地址赋值，保证解释执行时该内存变量地址已定义
	node->blockInsts.addInst(new MoveInstruction(module->getCurrentFunction(), elemVar, addressVal));
	
	if (isAssignLValue) {
		// 左值：直接返回 MemVariable，用于存储
		node->val = elemVar;
	} else {
		// 右值：创建一个临时变量来接收 load 的值
		Value * loadedVal = module->newVarValue(IntegerType::getTypeInt(), "");
		if (!loadedVal) {
			return false;
		}
		// 从 MemVariable load 到临时变量
		node->blockInsts.addInst(new MoveInstruction(module->getCurrentFunction(), loadedVal, elemVar));
		node->val = loadedVal;
	}
	return true;
}

/// @brief 整数加法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_add(ast_node * node)
{
	ast_node * src1_node = node->sons[0];
	ast_node * src2_node = node->sons[1];

	// 加法节点，左结合，先计算左节点，后计算右节点

	// 加法的左边操作数
	ast_node * left = ir_visit_ast_node(src1_node);
	if (!left) {
		// 某个变量没有定值
		return false;
	}

	// 加法的右边操作数
	ast_node * right = ir_visit_ast_node(src2_node);
	if (!right) {
		// 某个变量没有定值
		return false;
	}

	// 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

	BinaryInstruction * addInst = new BinaryInstruction(
		module->getCurrentFunction(),
		IRInstOperator::IRINST_OP_ADD_I,
		left->val,
		right->val,
		IntegerType::getTypeInt());

	// 创建临时变量保存IR的值，以及线性IR指令
	node->blockInsts.addInst(left->blockInsts);
	node->blockInsts.addInst(right->blockInsts);
	node->blockInsts.addInst(addInst);

	node->val = addInst;

	return true;
}

/// @brief 整数减法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_sub(ast_node * node)
{
	ast_node * src1_node = node->sons[0];
	ast_node * src2_node = node->sons[1];

	// 加法节点，左结合，先计算左节点，后计算右节点

	// 加法的左边操作数
	ast_node * left = ir_visit_ast_node(src1_node);
	if (!left) {
		// 某个变量没有定值
		return false;
	}

	// 加法的右边操作数
	ast_node * right = ir_visit_ast_node(src2_node);
	if (!right) {
		// 某个变量没有定值
		return false;
	}

	// 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

	BinaryInstruction * subInst = new BinaryInstruction(
		module->getCurrentFunction(),
		IRInstOperator::IRINST_OP_SUB_I,
		left->val,
		right->val,
		IntegerType::getTypeInt());

	// 创建临时变量保存IR的值，以及线性IR指令
	node->blockInsts.addInst(left->blockInsts);
	node->blockInsts.addInst(right->blockInsts);
	node->blockInsts.addInst(subInst);

	node->val = subInst;

	return true;
}

/// @brief 整数乘法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_mul(ast_node * node)
{
	ast_node * src1_node = node->sons[0];
	ast_node * src2_node = node->sons[1];

	ast_node * left = ir_visit_ast_node(src1_node);
	if (!left) {
		return false;
	}

	ast_node * right = ir_visit_ast_node(src2_node);
	if (!right) {
		return false;
	}

	BinaryInstruction * mulInst = new BinaryInstruction(
		module->getCurrentFunction(),
		IRInstOperator::IRINST_OP_MUL_I,
		left->val,
		right->val,
		IntegerType::getTypeInt());

	node->blockInsts.addInst(left->blockInsts);
	node->blockInsts.addInst(right->blockInsts);
	node->blockInsts.addInst(mulInst);

	node->val = mulInst;

	return true;
}

/// @brief 整数除法AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_div(ast_node * node)
{
	ast_node * src1_node = node->sons[0];
	ast_node * src2_node = node->sons[1];

	ast_node * left = ir_visit_ast_node(src1_node);
	if (!left) {
		return false;
	}

	ast_node * right = ir_visit_ast_node(src2_node);
	if (!right) {
		return false;
	}

	BinaryInstruction * divInst = new BinaryInstruction(
		module->getCurrentFunction(),
		IRInstOperator::IRINST_OP_DIV_I,
		left->val,
		right->val,
		IntegerType::getTypeInt());

	node->blockInsts.addInst(left->blockInsts);
	node->blockInsts.addInst(right->blockInsts);
	node->blockInsts.addInst(divInst);

	node->val = divInst;

	return true;
}

/// @brief 整数求余AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_mod(ast_node * node)
{
	ast_node * src1_node = node->sons[0];
	ast_node * src2_node = node->sons[1];

	ast_node * left = ir_visit_ast_node(src1_node);
	if (!left) {
		return false;
	}

	ast_node * right = ir_visit_ast_node(src2_node);
	if (!right) {
		return false;
	}

	BinaryInstruction * modInst = new BinaryInstruction(
		module->getCurrentFunction(),
		IRInstOperator::IRINST_OP_MOD_I,
		left->val,
		right->val,
		IntegerType::getTypeInt());

	node->blockInsts.addInst(left->blockInsts);
	node->blockInsts.addInst(right->blockInsts);
	node->blockInsts.addInst(modInst);

	node->val = modInst;

	return true;
}

/// @brief 按短路语义发射条件分支
/// @param expr 条件表达式AST
/// @param trueTarget 条件为真时跳转目标
/// @param falseTarget 条件为假时跳转目标
/// @param outInsts 输出的IR序列
/// @return 翻译是否成功
bool IRGenerator::emitCondBranch(
	ast_node * expr, LabelInstruction * trueTarget, LabelInstruction * falseTarget, InterCode & outInsts)
{
	if (expr == nullptr) {
		return false;
	}

	Function * currentFunc = module->getCurrentFunction();

	switch (expr->node_type) {
		case ast_operator_type::AST_OP_LOR: {
			// a || b: a为真直接true，否则计算b
			LabelInstruction * rhsLabel = new LabelInstruction(currentFunc);
			if (!emitCondBranch(expr->sons[0], trueTarget, rhsLabel, outInsts)) {
				return false;
			}
			outInsts.addInst(rhsLabel);
			return emitCondBranch(expr->sons[1], trueTarget, falseTarget, outInsts);
		}

		case ast_operator_type::AST_OP_LAND: {
			// a && b: a为假直接false，否则计算b
			LabelInstruction * rhsLabel = new LabelInstruction(currentFunc);
			if (!emitCondBranch(expr->sons[0], rhsLabel, falseTarget, outInsts)) {
				return false;
			}
			outInsts.addInst(rhsLabel);
			return emitCondBranch(expr->sons[1], trueTarget, falseTarget, outInsts);
		}

		case ast_operator_type::AST_OP_LNOT:
			// !a: 交换真假出口
			return emitCondBranch(expr->sons[0], falseTarget, trueTarget, outInsts);

			// This is a comment to indicate the change
			// The following lines are part of the emitCondBranch function
			// The goal is to remove the temporary variable cmpVal
		case ast_operator_type::AST_OP_EQ:
		case ast_operator_type::AST_OP_NE:
		case ast_operator_type::AST_OP_LT:
		case ast_operator_type::AST_OP_LE:
		case ast_operator_type::AST_OP_GT:
		case ast_operator_type::AST_OP_GE: {
			ast_node * left = ir_visit_ast_node(expr->sons[0]);
			if (!left) {
				return false;
			}
			ast_node * right = ir_visit_ast_node(expr->sons[1]);
			if (!right) {
				return false;
			}

			outInsts.addInst(left->blockInsts);
			outInsts.addInst(right->blockInsts);
			IRInstOperator cmpOp = ast_cmp_to_ir(expr->node_type);
			if (cmpOp == IRInstOperator::IRINST_OP_MAX) {
				return false;
			}

			BinaryInstruction * cmpInst =
				new BinaryInstruction(currentFunc, cmpOp, left->val, right->val, IntegerType::getTypeBool());
			outInsts.addInst(cmpInst);
			outInsts.addInst(new CondBrInstruction(currentFunc, cmpInst, trueTarget, falseTarget));
			return true;
		}

		default: {
			// 其他表达式：按“非零为真”
			ast_node * condNode = ir_visit_ast_node(expr);
			if (!condNode) {
				return false;
			}

			outInsts.addInst(condNode->blockInsts);
			ConstInt * zero = module->newConstInt(0);
			BinaryInstruction * cmpInst = new BinaryInstruction(
				currentFunc, IRInstOperator::IRINST_OP_ICMP_NE, condNode->val, zero, IntegerType::getTypeBool());
			outInsts.addInst(cmpInst);
			outInsts.addInst(new CondBrInstruction(currentFunc, cmpInst, trueTarget, falseTarget));
			return true;
		}
	}
}

/// @brief 逻辑/关系表达式翻译成布尔值(0/1)
/// @param node AST节点
/// @return 翻译是否成功
bool IRGenerator::ir_bool_expr(ast_node * node)
{
	Function * currentFunc = module->getCurrentFunction();
	LabelInstruction * trueLabel = new LabelInstruction(currentFunc);
	LabelInstruction * falseLabel = new LabelInstruction(currentFunc);
	LabelInstruction * endLabel = new LabelInstruction(currentFunc);

	Value * boolVal = module->newVarValue(IntegerType::getTypeInt(), "");
	if (!boolVal) {
		return false;
	}

	ConstInt * zero = module->newConstInt(0);
	ConstInt * one = module->newConstInt(1);

	// 默认赋0
	node->blockInsts.addInst(new MoveInstruction(currentFunc, boolVal, zero));

	// 条件分支
	if (!emitCondBranch(node, trueLabel, falseLabel, node->blockInsts)) {
		return false;
	}

	// true分支：赋1
	node->blockInsts.addInst(trueLabel);
	node->blockInsts.addInst(new MoveInstruction(currentFunc, boolVal, one));
	node->blockInsts.addInst(new GotoInstruction(currentFunc, endLabel));

	// false分支：保持0
	node->blockInsts.addInst(falseLabel);
	node->blockInsts.addInst(new GotoInstruction(currentFunc, endLabel));

	// 汇合
	node->blockInsts.addInst(endLabel);
	node->val = boolVal;

	return true;
}

/// @brief if/if-else 节点翻译成线性IR
/// @param node AST节点
/// @return 翻译是否成功
bool IRGenerator::ir_if(ast_node * node)
{
	Function * currentFunc = module->getCurrentFunction();

	if (node->sons.empty()) {
		return false;
	}

	ast_node * condNode = node->sons[0];
	ast_node * thenNode = node->sons.size() >= 2 ? node->sons[1] : nullptr;
	ast_node * elseNode = node->sons.size() >= 3 ? node->sons[2] : nullptr;

	LabelInstruction * trueLabel = new LabelInstruction(currentFunc);
	LabelInstruction * falseLabel = new LabelInstruction(currentFunc);
	LabelInstruction * endLabel = new LabelInstruction(currentFunc);

	if (!emitCondBranch(condNode, trueLabel, falseLabel, node->blockInsts)) {
		return false;
	}

	node->blockInsts.addInst(trueLabel);
	bool thenEndsWithTerminator = false;
	if (thenNode) {
		ast_node * thenIR = ir_visit_ast_node(thenNode);
		if (!thenIR) {
			return false;
		}
		thenEndsWithTerminator = ends_with_terminator(thenIR->blockInsts);
		node->blockInsts.addInst(thenIR->blockInsts);
	}
	if (!thenEndsWithTerminator) {
		node->blockInsts.addInst(new GotoInstruction(currentFunc, endLabel));
	}

	node->blockInsts.addInst(falseLabel);
	if (elseNode) {
		ast_node * elseIR = ir_visit_ast_node(elseNode);
		if (!elseIR) {
			return false;
		}
		bool elseEndsWithTerminator = ends_with_terminator(elseIR->blockInsts);
		node->blockInsts.addInst(elseIR->blockInsts);
		if (!elseEndsWithTerminator) {
			node->blockInsts.addInst(new GotoInstruction(currentFunc, endLabel));
		}
	}

	node->blockInsts.addInst(endLabel);
	return true;
}

/// @brief while 节点翻译成线性IR
/// @param node AST节点
/// @return 翻译是否成功
bool IRGenerator::ir_while(ast_node * node)
{
	Function * currentFunc = module->getCurrentFunction();

	if (node->sons.empty()) {
		return false;
	}

	ast_node * condNode = node->sons[0];
	ast_node * bodyNode = node->sons.size() >= 2 ? node->sons[1] : nullptr;

	LabelInstruction * condLabel = new LabelInstruction(currentFunc);
	LabelInstruction * bodyLabel = new LabelInstruction(currentFunc);
	LabelInstruction * endLabel = new LabelInstruction(currentFunc);

	// 先跳条件块
	node->blockInsts.addInst(new GotoInstruction(currentFunc, condLabel));

	// 条件块
	node->blockInsts.addInst(condLabel);
	if (!emitCondBranch(condNode, bodyLabel, endLabel, node->blockInsts)) {
		return false;
	}

	// 循环体
	node->blockInsts.addInst(bodyLabel);
	continueTargetStack.push_back(condLabel);
	breakTargetStack.push_back(endLabel);

	bool bodyEndsWithTerminator = false;
	if (bodyNode) {
		ast_node * bodyIR = ir_visit_ast_node(bodyNode);
		if (!bodyIR) {
			continueTargetStack.pop_back();
			breakTargetStack.pop_back();
			return false;
		}
		bodyEndsWithTerminator = ends_with_terminator(bodyIR->blockInsts);
		node->blockInsts.addInst(bodyIR->blockInsts);
	}

	continueTargetStack.pop_back();
	breakTargetStack.pop_back();

	// 回到条件判断
	if (!bodyEndsWithTerminator) {
		node->blockInsts.addInst(new GotoInstruction(currentFunc, condLabel));
	}

	// 退出点
	node->blockInsts.addInst(endLabel);
	return true;
}

/// @brief for 节点翻译成线性IR
/// @param node AST节点
/// @return 翻译是否成功
bool IRGenerator::ir_for(ast_node * node)
{
	if (node->sons.empty()) {
		return false;
	}

	auto isMissing = [](ast_node * child) {
		return child == nullptr || child->node_type == ast_operator_type::AST_OP_MAX;
	};

	Function * currentFunc = module->getCurrentFunction();
	ast_node * initNode = node->sons.size() >= 1 ? node->sons[0] : nullptr;
	ast_node * condNode = node->sons.size() >= 2 ? node->sons[1] : nullptr;
	ast_node * stepNode = node->sons.size() >= 3 ? node->sons[2] : nullptr;
	ast_node * bodyNode = node->sons.size() >= 4 ? node->sons[3] : nullptr;

	LabelInstruction * condLabel = new LabelInstruction(currentFunc);
	LabelInstruction * bodyLabel = new LabelInstruction(currentFunc);
	LabelInstruction * stepLabel = new LabelInstruction(currentFunc);
	LabelInstruction * endLabel = new LabelInstruction(currentFunc);

	module->enterScope();

	if (!isMissing(initNode)) {
		ast_node * initIR = ir_visit_ast_node(initNode);
		if (!initIR) {
			module->leaveScope();
			return false;
		}
		node->blockInsts.addInst(initIR->blockInsts);
	}

	node->blockInsts.addInst(new GotoInstruction(currentFunc, condLabel));

	// 条件块
	node->blockInsts.addInst(condLabel);
	if (isMissing(condNode)) {
		node->blockInsts.addInst(new GotoInstruction(currentFunc, bodyLabel));
	} else {
		if (!emitCondBranch(condNode, bodyLabel, endLabel, node->blockInsts)) {
			module->leaveScope();
			return false;
		}
	}

	// 循环体
	node->blockInsts.addInst(bodyLabel);
	continueTargetStack.push_back(isMissing(stepNode) ? condLabel : stepLabel);
	breakTargetStack.push_back(endLabel);

	bool bodyEndsWithTerminator = false;
	if (!isMissing(bodyNode)) {
		ast_node * bodyIR = ir_visit_ast_node(bodyNode);
		if (!bodyIR) {
			continueTargetStack.pop_back();
			breakTargetStack.pop_back();
			module->leaveScope();
			return false;
		}
		bodyEndsWithTerminator = ends_with_terminator(bodyIR->blockInsts);
		node->blockInsts.addInst(bodyIR->blockInsts);
	}

	continueTargetStack.pop_back();
	breakTargetStack.pop_back();

	if (!bodyEndsWithTerminator) {
		node->blockInsts.addInst(new GotoInstruction(currentFunc, isMissing(stepNode) ? condLabel : stepLabel));
	}

	// 步进块
	if (!isMissing(stepNode)) {
		node->blockInsts.addInst(stepLabel);
		ast_node * stepIR = ir_visit_ast_node(stepNode);
		if (!stepIR) {
			module->leaveScope();
			return false;
		}
		bool stepEndsWithTerminator = ends_with_terminator(stepIR->blockInsts);
		node->blockInsts.addInst(stepIR->blockInsts);
		if (!stepEndsWithTerminator) {
			node->blockInsts.addInst(new GotoInstruction(currentFunc, condLabel));
		}
	}

	node->blockInsts.addInst(endLabel);

	module->leaveScope();
	return true;
}

/// @brief break 节点翻译成线性IR
/// @param node AST节点
/// @return 翻译是否成功
bool IRGenerator::ir_break(ast_node * node)
{
	Function * currentFunc = module->getCurrentFunction();
	if (breakTargetStack.empty()) {
		minic_log(LOG_ERROR, "break语句未处于循环体内");
		return false;
	}

	node->blockInsts.addInst(new GotoInstruction(currentFunc, breakTargetStack.back()));
	return true;
}

/// @brief continue 节点翻译成线性IR
/// @param node AST节点
/// @return 翻译是否成功
bool IRGenerator::ir_continue(ast_node * node)
{
	Function * currentFunc = module->getCurrentFunction();
	if (continueTargetStack.empty()) {
		minic_log(LOG_ERROR, "continue语句未处于循环体内");
		return false;
	}

	node->blockInsts.addInst(new GotoInstruction(currentFunc, continueTargetStack.back()));
	return true;
}

/// @brief 赋值AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_assign(ast_node * node)
{
	ast_node * son1_node = node->sons[0];
	ast_node * son2_node = node->sons[1];

	// 赋值节点，自右往左运算

	// 赋值运算符的左侧操作数
	ast_node * left = ir_visit_ast_node(son1_node);
	if (!left) {
		// 某个变量没有定值
		// 这里缺省设置变量不存在则创建，因此这里不会错误
		return false;
	}

	// 赋值运算符的右侧操作数
	ast_node * right = ir_visit_ast_node(son2_node);
	if (!right) {
		// 某个变量没有定值
		return false;
	}

	// 这里只处理整型的数据，如需支持实数，则需要针对类型进行处理

	MoveInstruction * movInst = new MoveInstruction(module->getCurrentFunction(), left->val, right->val);

	// 创建临时变量保存IR的值，以及线性IR指令
	node->blockInsts.addInst(right->blockInsts);
	node->blockInsts.addInst(left->blockInsts);
	node->blockInsts.addInst(movInst);

	// 这里假定赋值的类型是一致的
	node->val = left->val;

	return true;
}

/// @brief 变量声明语句节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_declare_statment(ast_node * node)
{
	bool result = false;

	for (auto & child: node->sons) {

		// 遍历每个变量声明
		result = ir_variable_declare(child);
		if (!result) {
			break;
		}

		node->blockInsts.addInst(child->blockInsts);
	}

	return result;
}

/// @brief 变量定声明节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_variable_declare(ast_node * node)
{
	// 共有两个孩子，第一个类型，第二个变量名

	// TODO 这里可强化类型等检查
	Type * declType = node->sons[0]->type;
	ast_node * varDefNode = node->sons[1];
	bool isStaticDecl = node->isStaticDecl;

	if (isStaticDecl && module->getCurrentFunction()) {
		node->val = module->newStaticLocalVarValue(declType, varDefNode->name);
	} else {
		node->val = module->newVarValue(declType, varDefNode->name);
	}
	if (!node->val) {
		return false;
	}

	// VarDef 的孩子可能包含数组维度节点(AST_OP_ARRAY_DIMS)和初始化表达式
	ast_node * initExprNode = nullptr;
	for (auto child: varDefNode->sons) {
		if (!child) {
			continue;
		}
		if (child->node_type == ast_operator_type::AST_OP_ARRAY_DIMS) {
			continue;
		}
		initExprNode = child;
		break;
	}

	if (initExprNode) {
		if (isStaticDecl) {
			int32_t initConstVal = 0;
			if (!eval_static_const_expr(initExprNode, initConstVal)) {
				minic_log(LOG_ERROR, "static变量初始化必须是常量表达式");
				return false;
			}

			if (!module->getCurrentFunction()) {
				auto * gv = dynamic_cast<GlobalVariable *>(node->val);
				module->registerGlobalInit(gv, initConstVal);
				return true;
			}

			Function * currentFunc = module->getCurrentFunction();
			GlobalVariable * initFlag =
				module->newUniqueGlobalVariable(IntegerType::getTypeInt(), "__sinit_" + varDefNode->name);

			ConstInt * zero = module->newConstInt(0);
			ConstInt * one = module->newConstInt(1);
			ConstInt * initVal = module->newConstInt(initConstVal);

			BinaryInstruction * cmpInst = new BinaryInstruction(
				currentFunc, IRInstOperator::IRINST_OP_ICMP_EQ, initFlag, zero, IntegerType::getTypeBool());
			LabelInstruction * doInitLabel = new LabelInstruction(currentFunc);
			LabelInstruction * contLabel = new LabelInstruction(currentFunc);

			node->blockInsts.addInst(cmpInst);
			node->blockInsts.addInst(new CondBrInstruction(currentFunc, cmpInst, doInitLabel, contLabel));
			node->blockInsts.addInst(doInitLabel);
			node->blockInsts.addInst(new MoveInstruction(currentFunc, node->val, initVal));
			node->blockInsts.addInst(new MoveInstruction(currentFunc, initFlag, one));
			node->blockInsts.addInst(new GotoInstruction(currentFunc, contLabel));
			node->blockInsts.addInst(contLabel);
			return true;
		}

		if (!module->getCurrentFunction()) {
			int32_t initConstVal = 0;
			if (!eval_static_const_expr(initExprNode, initConstVal)) {
				minic_log(LOG_ERROR, "全局变量初始化必须是常量表达式");
				return false;
			}

			auto * gv = dynamic_cast<GlobalVariable *>(node->val);
			module->registerGlobalInit(gv, initConstVal);
			return true;
		}

		if (declType && declType->isArrayType()) {
			minic_log(LOG_ERROR, "数组初始化暂不支持");
			return false;
		}

		Function * currentFunc = module->getCurrentFunction();
		if (!currentFunc) {
			minic_log(LOG_ERROR, "全局变量初始化暂不支持");
			return false;
		}

		ast_node * initNode = ir_visit_ast_node(initExprNode);
		if (!initNode || !initNode->val) {
			return false;
		}

		node->blockInsts.addInst(initNode->blockInsts);
		node->blockInsts.addInst(new MoveInstruction(currentFunc, node->val, initNode->val));
	}

	return true;
}

/// @brief 函数调用AST节点翻译成线性中间IR
/// @param node AST节点
/// @return 翻译是否成功，true：成功，false：失败
bool IRGenerator::ir_function_call(ast_node * node)
{
	std::vector<Value *> realParams;

	// 获取当前正在处理的函数
	Function * currentFunc = module->getCurrentFunction();

	// 函数调用的节点包含两个节点：
	// 第一个节点：函数名节点
	// 第二个节点：实参列表节点

	std::string funcName = node->sons[0]->name;
	int64_t lineno = node->sons[0]->line_no;

	ast_node * paramsNode = node->sons[1];

	// 根据函数名查找函数，看是否存在。若不存在则出错
	// 这里约定函数必须先定义后使用
	auto calledFunction = module->findFunction(funcName);
	if (nullptr == calledFunction) {
		minic_log(LOG_ERROR, "函数(%s)未定义或声明", funcName.c_str());
		return false;
	}

	// 当前函数存在函数调用
	currentFunc->setExistFuncCall(true);

	// 如果没有孩子，也认为是没有参数
	if (!paramsNode->sons.empty()) {

		int32_t argsCount = (int32_t) paramsNode->sons.size();

		// 当前函数中调用函数实参个数最大值统计，实际上是统计实参传参需在栈中分配的大小
		// 因为目前的语言支持的int和float都是四字节的，只统计个数即可
		if (argsCount > currentFunc->getMaxFuncCallArgCnt()) {
			currentFunc->setMaxFuncCallArgCnt(argsCount);
		}

		// 遍历参数列表，孩子是表达式
		// 这里自左往右计算表达式
		for (auto son: paramsNode->sons) {

			// 遍历Block的每个语句，进行显示或者运算
			ast_node * temp = ir_visit_ast_node(son);
			if (!temp) {
				return false;
			}

			realParams.push_back(temp->val);
			node->blockInsts.addInst(temp->blockInsts);
		}
	}

	// TODO 这里请追加函数调用的语义错误检查，这里只进行了函数参数的个数检查等，其它请自行追加。
	if (realParams.size() != calledFunction->getParams().size()) {
		// 函数参数的个数不一致，语义错误
		minic_log(LOG_ERROR, "第%lld行的被调用函数(%s)未定义或声明", (long long) lineno, funcName.c_str());
		return false;
	}

	// 返回调用有返回值，则需要分配临时变量，用于保存函数调用的返回值
	Type * type = calledFunction->getReturnType();

	FuncCallInstruction * funcCallInst = new FuncCallInstruction(currentFunc, calledFunction, realParams, type);

	// 创建函数调用指令
	node->blockInsts.addInst(funcCallInst);

	// 函数调用结果Value保存到node中，可能为空，上层节点可利用这个值
	node->val = funcCallInst;

	return true;
}
