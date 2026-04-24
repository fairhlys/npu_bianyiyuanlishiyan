#include "ControlFlowGraph.h"

#include <algorithm>
#include <queue>
#include <unordered_set>

#include "CondBrInstruction.h"
#include "GotoInstruction.h"
#include "LabelInstruction.h"
#include "Instruction.h"

#ifdef USE_GRAPHVIZ
#include <gvc.h>
#endif

using BasicBlock = ControlFlowGraph::BasicBlock;

namespace {
bool isBranchTerminator(Instruction * inst)
{
	if (!inst) {
		return false;
	}

	switch (inst->getOp()) {
		case IRInstOperator::IRINST_OP_GOTO:
		case IRInstOperator::IRINST_OP_BR:
		case IRInstOperator::IRINST_OP_EXIT:
			return true;
		default:
			return false;
	}
}

bool isLabelInstruction(Instruction * inst)
{
	return inst && inst->getOp() == IRInstOperator::IRINST_OP_LABEL;
}
} // namespace

ControlFlowGraph::ControlFlowGraph(Function * _function) : function(_function)
{}

BasicBlock * ControlFlowGraph::blockForLabel(LabelInstruction * label) const
{
	if (!label) {
		return nullptr;
	}

	auto iter = labelToBlock.find(label);
	if (iter == labelToBlock.end()) {
		return nullptr;
	}

	return iter->second;
}

BasicBlock * ControlFlowGraph::nextActiveBlock(BasicBlock * block) const
{
	if (!block) {
		return nullptr;
	}

	for (size_t index = block->index + 1; index < blocks.size(); ++index) {
		auto * candidate = blocks[index].get();
		if (candidate->active) {
			return candidate;
		}
	}

	return nullptr;
}

LabelInstruction * ControlFlowGraph::ensureEntryLabel(BasicBlock * block)
{
	if (!block) {
		return nullptr;
	}

	if (block->entryLabel) {
		return block->entryLabel;
	}

	auto * label = new LabelInstruction(function);
	block->instructions.insert(block->instructions.begin(), label);
	block->entryLabel = label;
	labelToBlock[label] = block;
	return label;
}

void ControlFlowGraph::redirectBranches(LabelInstruction * oldTarget, LabelInstruction * newTarget)
{
	if (!oldTarget || !newTarget || oldTarget == newTarget) {
		return;
	}

	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active) {
			continue;
		}

		for (auto inst: block->instructions) {
			if (!inst) {
				continue;
			}

			if (inst->getOp() == IRInstOperator::IRINST_OP_GOTO) {
				auto * gotoInst = dynamic_cast<GotoInstruction *>(inst);
				if (gotoInst && gotoInst->getTarget() == oldTarget) {
					gotoInst->setTarget(newTarget);
				}
			} else if (inst->getOp() == IRInstOperator::IRINST_OP_BR) {
				auto * brInst = dynamic_cast<CondBrInstruction *>(inst);
				if (!brInst) {
					continue;
				}

				if (brInst->getTrueTarget() == oldTarget) {
					brInst->setTrueTarget(newTarget);
				}
				if (brInst->getFalseTarget() == oldTarget) {
					brInst->setFalseTarget(newTarget);
				}
			}
		}
	}
}

void ControlFlowGraph::build()
{
	blocks.clear();
	leaderToBlock.clear();
	labelToBlock.clear();

	auto & insts = function->getInterCode().getInsts();
	if (insts.empty()) {
		return;
	}

	std::unordered_set<Instruction *> leaders;
	leaders.insert(insts.front());

	for (size_t index = 0; index < insts.size(); ++index) {
		auto * inst = insts[index];
		if (!inst) {
			continue;
		}

		auto op = inst->getOp();
		if (op == IRInstOperator::IRINST_OP_GOTO) {
			auto * gotoInst = dynamic_cast<GotoInstruction *>(inst);
			if (gotoInst && gotoInst->getTarget()) {
				leaders.insert(gotoInst->getTarget());
			}
			if (index + 1 < insts.size()) {
				leaders.insert(insts[index + 1]);
			}
		} else if (op == IRInstOperator::IRINST_OP_BR) {
			auto * brInst = dynamic_cast<CondBrInstruction *>(inst);
			if (brInst) {
				if (brInst->getTrueTarget()) {
					leaders.insert(brInst->getTrueTarget());
				}
				if (brInst->getFalseTarget()) {
					leaders.insert(brInst->getFalseTarget());
				}
			}
			if (index + 1 < insts.size()) {
				leaders.insert(insts[index + 1]);
			}
		} else if (op == IRInstOperator::IRINST_OP_EXIT) {
			if (index + 1 < insts.size()) {
				leaders.insert(insts[index + 1]);
			}
		}
	}

	BasicBlock * currentBlock = nullptr;
	for (size_t index = 0; index < insts.size(); ++index) {
		auto * inst = insts[index];
		if (!inst) {
			continue;
		}

		if (!currentBlock || leaders.count(inst)) {
			blocks.emplace_back(std::make_unique<BasicBlock>());
			currentBlock = blocks.back().get();
			currentBlock->index = blocks.size() - 1;
			leaderToBlock[inst] = currentBlock;
			if (isLabelInstruction(inst)) {
				currentBlock->entryLabel = static_cast<LabelInstruction *>(inst);
				labelToBlock[currentBlock->entryLabel] = currentBlock;
			}
		}

		currentBlock->instructions.push_back(inst);
		if (!currentBlock->entryLabel && isLabelInstruction(inst)) {
			currentBlock->entryLabel = static_cast<LabelInstruction *>(inst);
			labelToBlock[currentBlock->entryLabel] = currentBlock;
		}
	}
}

void ControlFlowGraph::rebuildEdges()
{
	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		block->predecessors.clear();
		block->successors.clear();
		block->reachable = false;
	}

	for (size_t index = 0; index < blocks.size(); ++index) {
		auto * block = blocks[index].get();
		if (!block->active || block->instructions.empty()) {
			continue;
		}

		auto * tail = block->instructions.back();
		if (tail->getOp() == IRInstOperator::IRINST_OP_GOTO) {
			auto * gotoInst = dynamic_cast<GotoInstruction *>(tail);
			auto * target = gotoInst ? blockForLabel(gotoInst->getTarget()) : nullptr;
			if (target && target->active) {
				block->successors.push_back(target);
			}
		} else if (tail->getOp() == IRInstOperator::IRINST_OP_BR) {
			auto * brInst = dynamic_cast<CondBrInstruction *>(tail);
			if (brInst) {
				auto * trueBlock = blockForLabel(brInst->getTrueTarget());
				auto * falseBlock = blockForLabel(brInst->getFalseTarget());
				if (trueBlock && trueBlock->active) {
					block->successors.push_back(trueBlock);
				}
				if (falseBlock && falseBlock->active && falseBlock != trueBlock) {
					block->successors.push_back(falseBlock);
				}
			}
		} else if (tail->getOp() != IRInstOperator::IRINST_OP_EXIT) {
			for (size_t nextIndex = index + 1; nextIndex < blocks.size(); ++nextIndex) {
				auto * nextBlock = blocks[nextIndex].get();
				if (nextBlock->active) {
					block->successors.push_back(nextBlock);
					break;
				}
			}
		}
	}

	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active) {
			continue;
		}
		for (auto * succ: block->successors) {
			succ->predecessors.push_back(block);
		}
	}
}

void ControlFlowGraph::markReachable()
{
	if (blocks.empty()) {
		return;
	}

	auto * entry = blocks.front().get();
	if (!entry->active) {
		for (auto & blockPtr: blocks) {
			if (blockPtr->active) {
				entry = blockPtr.get();
				break;
			}
		}
	}

	if (!entry || !entry->active) {
		return;
	}

	std::queue<BasicBlock *> pending;
	entry->reachable = true;
	pending.push(entry);

	while (!pending.empty()) {
		auto * block = pending.front();
		pending.pop();

		for (auto * succ: block->successors) {
			if (succ && succ->active && !succ->reachable) {
				succ->reachable = true;
				pending.push(succ);
			}
		}
	}
}

bool ControlFlowGraph::removeUnreachableBlocks()
{
	bool changed = false;
	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active) {
			continue;
		}
		if (!block->reachable) {
			block->active = false;
			changed = true;
		}
	}
	return changed;
}

bool ControlFlowGraph::removeTrampolineBlock()
{
	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active || block->instructions.empty()) {
			continue;
		}

		if (block->instructions.size() == 2 && isLabelInstruction(block->instructions[0]) &&
			block->instructions[1]->getOp() == IRInstOperator::IRINST_OP_GOTO) {
			auto * label = static_cast<LabelInstruction *>(block->instructions[0]);
			auto * gotoInst = dynamic_cast<GotoInstruction *>(block->instructions[1]);
			auto * targetBlock = gotoInst ? blockForLabel(gotoInst->getTarget()) : nullptr;
			if (!targetBlock) {
				continue;
			}

			auto * replacementLabel = ensureEntryLabel(targetBlock);
			redirectBranches(label, replacementLabel);
			block->active = false;
			return true;
		}

		if (block->instructions.size() == 1 && isLabelInstruction(block->instructions[0])) {
			auto * label = static_cast<LabelInstruction *>(block->instructions[0]);
			auto * targetBlock = nextActiveBlock(block);
			if (!targetBlock) {
				bool referenced = false;
				for (auto & otherBlockPtr: blocks) {
					auto * otherBlock = otherBlockPtr.get();
					if (!otherBlock->active) {
						continue;
					}
					for (auto inst: otherBlock->instructions) {
						if (inst->getOp() == IRInstOperator::IRINST_OP_GOTO) {
							auto * gotoInst = dynamic_cast<GotoInstruction *>(inst);
							if (gotoInst && gotoInst->getTarget() == label) {
								referenced = true;
							}
						} else if (inst->getOp() == IRInstOperator::IRINST_OP_BR) {
							auto * brInst = dynamic_cast<CondBrInstruction *>(inst);
							if (brInst && (brInst->getTrueTarget() == label || brInst->getFalseTarget() == label)) {
								referenced = true;
							}
						}
					}
				}

				if (referenced) {
					continue;
				}
				block->active = false;
				return true;
			}

			auto * replacementLabel = ensureEntryLabel(targetBlock);
			redirectBranches(label, replacementLabel);
			block->active = false;
			return true;
		}
	}

	return false;
}

bool ControlFlowGraph::mergeLinearBlock()
{
	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active || block->successors.size() != 1) {
			continue;
		}

		auto * successor = block->successors[0];
		if (!successor || !successor->active || successor->predecessors.size() != 1) {
			continue;
		}

		if (!block->instructions.empty() && block->instructions.back()->getOp() == IRInstOperator::IRINST_OP_GOTO) {
			auto * gotoInst = dynamic_cast<GotoInstruction *>(block->instructions.back());
			if (gotoInst && gotoInst->getTarget() == successor->entryLabel) {
				block->instructions.pop_back();
			}
		}

		if (!successor->instructions.empty()) {
			size_t startIndex = 0;
			if (isLabelInstruction(successor->instructions.front())) {
				startIndex = 1;
			}
			for (size_t index = startIndex; index < successor->instructions.size(); ++index) {
				block->instructions.push_back(successor->instructions[index]);
			}
		}

		successor->active = false;
		return true;
	}

	return false;
}

void ControlFlowGraph::materialize()
{
	std::unordered_map<LabelInstruction *, size_t> labelUseCount;
	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active) {
			continue;
		}

		for (auto inst: block->instructions) {
			if (inst->getOp() == IRInstOperator::IRINST_OP_GOTO) {
				auto * gotoInst = dynamic_cast<GotoInstruction *>(inst);
				if (gotoInst && gotoInst->getTarget()) {
					labelUseCount[gotoInst->getTarget()]++;
				}
			} else if (inst->getOp() == IRInstOperator::IRINST_OP_BR) {
				auto * brInst = dynamic_cast<CondBrInstruction *>(inst);
				if (brInst) {
					if (brInst->getTrueTarget()) {
						labelUseCount[brInst->getTrueTarget()]++;
					}
					if (brInst->getFalseTarget()) {
						labelUseCount[brInst->getFalseTarget()]++;
					}
				}
			}
		}
	}

	std::vector<Instruction *> newInsts;
	newInsts.reserve(function->getInterCode().getInsts().size());
	std::unordered_set<Instruction *> keepSet;

	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active) {
			continue;
		}

		for (auto inst: block->instructions) {
			if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
				auto * label = static_cast<LabelInstruction *>(inst);
				if (labelUseCount[label] == 0) {
					continue;
				}
			}

			if (keepSet.insert(inst).second) {
				newInsts.push_back(inst);
			}
		}
	}

	auto & oldInsts = function->getInterCode().getInsts();
	for (auto inst: oldInsts) {
		if (keepSet.find(inst) == keepSet.end()) {
			inst->clearOperands();
			delete inst;
		}
	}

	oldInsts.swap(newInsts);
}

void ControlFlowGraph::optimize()
{
	if (!function) {
		return;
	}

	build();
	if (blocks.empty()) {
		return;
	}

	bool changed;
	do {
		rebuildEdges();
		markReachable();
		changed = removeUnreachableBlocks();
		if (changed) {
			continue;
		}

		changed = removeTrampolineBlock();
		if (changed) {
			continue;
		}

		changed = mergeLinearBlock();
	} while (changed);

	rebuildEdges();
	materialize();
}

std::string ControlFlowGraph::escapeRecordText(const std::string & text)
{
	std::string escaped;
	escaped.reserve(text.size() + 8);
	for (char ch: text) {
		switch (ch) {
			case '\\':
				escaped += "\\\\";
				break;
			case '{':
				escaped += "\\{";
				break;
			case '}':
				escaped += "\\}";
				break;
			case '|':
				escaped += "\\|";
				break;
			case '"':
				escaped += "\\\"";
				break;
			default:
				escaped.push_back(ch);
				break;
		}
	}
	return escaped;
}

std::string ControlFlowGraph::buildNodeLabel(const BasicBlock * block)
{
	std::string label = "{";
	label += "BB" + std::to_string(block->index);

	if (!block->instructions.empty()) {
		for (auto inst: block->instructions) {
			std::string instText;
			inst->toString(instText);
			label += "|" + escapeRecordText(instText);
		}
	}

	label += "}";
	return label;
}

std::string ControlFlowGraph::buildNodeName(const std::string & functionName, const BasicBlock * block)
{
	return functionName + "_bb_" + std::to_string(block->index);
}

void ControlFlowGraph::addToGraph(void * graph, const std::string & functionName) const
{
#ifndef USE_GRAPHVIZ
	(void) graph;
	(void) functionName;
#else
	auto * g = reinterpret_cast<Agraph_t *>(graph);
	if (!g) {
		return;
	}

	std::unordered_map<const BasicBlock *, Agnode_t *> nodeMap;
	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active) {
			continue;
		}

		std::string nodeName = buildNodeName(functionName, block);
		Agnode_t * node = agnode(g, const_cast<char *>(nodeName.c_str()), 1);
		if (!node) {
			continue;
		}

		agsafeset(node, (char *) "shape", (char *) "Mrecord", (char *) "");
		agsafeset(node, (char *) "fontname", (char *) "SimSun", (char *) "");
		std::string label = buildNodeLabel(block);
		agsafeset(node, (char *) "label", const_cast<char *>(label.c_str()), (char *) "");
		nodeMap[block] = node;
	}

	for (auto & blockPtr: blocks) {
		auto * block = blockPtr.get();
		if (!block->active) {
			continue;
		}

		auto nodeIter = nodeMap.find(block);
		if (nodeIter == nodeMap.end()) {
			continue;
		}

		for (auto * succ: block->successors) {
			auto succIter = nodeMap.find(succ);
			if (succIter != nodeMap.end()) {
				agedge(g, nodeIter->second, succIter->second, nullptr, 1);
			}
		}
	}
#endif
}
