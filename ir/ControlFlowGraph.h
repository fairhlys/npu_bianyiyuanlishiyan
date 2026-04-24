#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Function.h"

class Instruction;
class LabelInstruction;

class ControlFlowGraph {
public:
	explicit ControlFlowGraph(Function * function);

	void optimize();
	void addToGraph(void * graph, const std::string & functionName) const;

	struct BasicBlock {
		size_t index = 0;
		std::vector<Instruction *> instructions;
		std::vector<BasicBlock *> predecessors;
		std::vector<BasicBlock *> successors;
		LabelInstruction * entryLabel = nullptr;
		bool active = true;
		bool reachable = false;
	};

private:
	Function * function = nullptr;
	std::vector<std::unique_ptr<BasicBlock>> blocks;
	std::unordered_map<Instruction *, BasicBlock *> leaderToBlock;
	std::unordered_map<LabelInstruction *, BasicBlock *> labelToBlock;

	void build();
	void rebuildEdges();
	void markReachable();
	bool removeUnreachableBlocks();
	bool removeTrampolineBlock();
	bool mergeLinearBlock();
	void materialize();
	void redirectBranches(LabelInstruction * oldTarget, LabelInstruction * newTarget);
	LabelInstruction * ensureEntryLabel(BasicBlock * block);
	BasicBlock * blockForLabel(LabelInstruction * label) const;
	BasicBlock * nextActiveBlock(BasicBlock * block) const;
	static std::string escapeRecordText(const std::string & text);
	static std::string buildNodeLabel(const BasicBlock * block);
	static std::string buildNodeName(const std::string & functionName, const BasicBlock * block);
};
