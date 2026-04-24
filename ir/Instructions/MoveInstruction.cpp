/// @file MoveInstruction.cpp
/// @brief Move指令，也就是DragonIR的Asssign指令
///
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-09-29
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// </table>
///

#include "VoidType.h"
#include "MemVariable.h"

#include "MoveInstruction.h"

///
/// @brief 构造函数
/// @param _func 所属的函数
/// @param result 结构操作数
/// @param srcVal1 源操作数
///
MoveInstruction::MoveInstruction(Function * _func, Value * _result, Value * _srcVal1)
	: Instruction(_func, IRInstOperator::IRINST_OP_ASSIGN, VoidType::getType())
{
	addOperand(_result);
	addOperand(_srcVal1);
}

/// @brief 转换成字符串显示
/// @param str 转换后的字符串
void MoveInstruction::toString(std::string & str)
{

	Value *dstVal = getOperand(0), *srcVal = getOperand(1);

	// 检查源操作数是否是 MemVariable
	// MemVariable 代表内存中的某个值，需要通过解引用来访问
	MemVariable * srcMem = dynamic_cast<MemVariable *>(srcVal);
	MemVariable * dstMem = dynamic_cast<MemVariable *>(dstVal);
	
	if (dstMem) {
		// 目标是内存变量：
		// 1) 指针赋值（地址初始化），输出普通赋值；
		// 2) 其余情况输出 store。
		if (dstVal->getType() && dstVal->getType()->isPointerType() && srcVal->getType() &&
			srcVal->getType()->isPointerType()) {
			str = dstVal->getIRName() + " = " + srcVal->getIRName();
		} else {
			str = "*" + dstVal->getIRName() + " = " + srcVal->getIRName();
		}
	} else if (srcMem) {
		// 源是内存变量，这是一个 load 操作
		str = dstVal->getIRName() + " = *" + srcVal->getIRName();
	} else {
		// 普通赋值
		str = dstVal->getIRName() + " = " + srcVal->getIRName();
	}
}
