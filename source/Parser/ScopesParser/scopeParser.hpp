#pragma once

#include "../parser.hpp"

class scopeParser
{
public:
	static node::Scope parseScope();
	static node::StmtForLoop parseForLoop();
	static node::StmtWhileLoop parseWhileLoop();
	static node::StmtIf parseIfStmt();
	static std::optional<node::IfPred> parseIfPred();
};