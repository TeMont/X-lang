#pragma once

#include "../compiler.hpp"

class scopeCompiler
{
public:
	static void compScope(const node::Scope &scope);
	static void compIfPred(const node::IfPred &pred, const std::string &endLabel);
	static void compIfStmt(const node::StmtIf &stmtIf);
	static void compForLoop(const node::StmtForLoop &forLoop);
	static void compWhileLoop(const node::StmtWhileLoop &whileLoop);
};