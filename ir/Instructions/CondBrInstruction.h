///
/// @file CondBrInstruction.h
/// @brief 条件分支指令
///
#pragma once

#include "Instruction.h"
#include "LabelInstruction.h"

enum class IRBrCond : std::int8_t {
	NZ,
	EQ,
	NE,
	LT,
	LE,
	GT,
	GE,
};

class CondBrInstruction final : public Instruction {

public:
	/// @brief 按“非零为真”跳转
	CondBrInstruction(
		Function * _func, Value * _condVal, LabelInstruction * _trueTarget, LabelInstruction * _falseTarget);

	/// @brief 按关系比较跳转
	CondBrInstruction(
		Function * _func,
		IRBrCond _cond,
		Value * _lhs,
		Value * _rhs,
		LabelInstruction * _trueTarget,
		LabelInstruction * _falseTarget);

	void toString(std::string & str) override;

	[[nodiscard]] IRBrCond getCond() const
	{
		return cond;
	}

	[[nodiscard]] LabelInstruction * getTrueTarget() const
	{
		return trueTarget;
	}

	[[nodiscard]] LabelInstruction * getFalseTarget() const
	{
		return falseTarget;
	}

	[[nodiscard]] bool isCompareMode() const
	{
		return compareMode;
	}

	/// @brief 重定向真分支目标
	/// @param _trueTarget 新的真分支目标
	void setTrueTarget(LabelInstruction * _trueTarget);

	/// @brief 重定向假分支目标
	/// @param _falseTarget 新的假分支目标
	void setFalseTarget(LabelInstruction * _falseTarget);

private:
	IRBrCond cond = IRBrCond::NZ;
	bool compareMode = false;
	LabelInstruction * trueTarget = nullptr;
	LabelInstruction * falseTarget = nullptr;
};
