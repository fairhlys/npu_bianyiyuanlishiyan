///
/// @file CondBrInstruction.cpp
/// @brief 条件分支指令实现
///
#include "CondBrInstruction.h"
#include "VoidType.h"

CondBrInstruction::CondBrInstruction(
	Function * _func, Value * _condVal, LabelInstruction * _trueTarget, LabelInstruction * _falseTarget)
	: Instruction(_func, IRInstOperator::IRINST_OP_BR, VoidType::getType()), cond(IRBrCond::NZ), compareMode(false),
	  trueTarget(_trueTarget), falseTarget(_falseTarget)
{
	addOperand(_condVal);
}

CondBrInstruction::CondBrInstruction(
	Function * _func,
	IRBrCond _cond,
	Value * _lhs,
	Value * _rhs,
	LabelInstruction * _trueTarget,
	LabelInstruction * _falseTarget)
	: Instruction(_func, IRInstOperator::IRINST_OP_BR, VoidType::getType()), cond(_cond), compareMode(true),
	  trueTarget(_trueTarget), falseTarget(_falseTarget)
{
	addOperand(_lhs);
	addOperand(_rhs);
}

void CondBrInstruction::toString(std::string & str)
{
	if (!compareMode) {
		Value * condVal = getOperand(0);
		str =
			"bc " + condVal->getIRName() + ", label " + trueTarget->getIRName() + ", label " + falseTarget->getIRName();
	} else {
		Value * lhs = getOperand(0);
		Value * rhs = getOperand(1);
		str = "bc " + lhs->getIRName() + ", label " + trueTarget->getIRName() + ", label " + falseTarget->getIRName();
		(void) rhs;
	}
}

void CondBrInstruction::setTrueTarget(LabelInstruction * _trueTarget)
{
	trueTarget = _trueTarget;
}

void CondBrInstruction::setFalseTarget(LabelInstruction * _falseTarget)
{
	falseTarget = _falseTarget;
}
