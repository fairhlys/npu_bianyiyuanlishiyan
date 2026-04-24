#pragma once

#include "FrontEndExecutor.h"

class Antlr4Executor : public FrontEndExecutor {
public:
	explicit Antlr4Executor(std::string _filename) : FrontEndExecutor(_filename) {}
	bool run() override;
};
