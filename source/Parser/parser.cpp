#include "parser.hpp"

size_t parser::m_index = 0;
std::vector<Token> parser::m_tokens = {};
node::Prog parser::prog = {};

[[nodiscard]] std::optional<Token> parser::peek(const int &offset)
{
    if (m_index + offset >= m_tokens.size())
    {
        return {};
    }
    return m_tokens[m_index + offset];
}

Token parser::consume()
{
    return m_tokens[m_index++];
}

void parser::tryConsume(const char &charToConsume)
{
    if (peek().has_value() && peek().value().type != tokensMap[std::string(1, charToConsume)] || !peek().has_value())
    {
        std::cerr << "[Parse Error] ERR001 Invalid Syntax Expected '" + std::string(1, charToConsume) + "'";
        exit(EXIT_FAILURE);
    }
    consume();
}

std::optional<node::StmtInput> parser::parseInputStmt()
{
    consume();
    tryConsume('(');
    if (auto const &nodeExpr = expressionParser::parseExpr(ANY_TYPE))
    {
        tryConsume(')');
        return node::StmtInput{new node::Expr(nodeExpr.value())};
    }
    std::cerr << "[Parse Error] ERR001 Invalid Syntax Expected Expression";
    exit(EXIT_FAILURE);
}

std::optional<node::Stmt> parser::parseStmt(bool expectSemi)
{
    std::optional<node::Stmt> stmtNode;
    if (!peek().has_value())
    {
        return {};
    }
    if (peek().value().type == RETURN)
    {
        consume();
        if (auto const &nodeExpr = expressionParser::parseExpr(ANY_TYPE))
        {
            stmtNode = {{node::StmtReturn{new node::Expr(nodeExpr.value())}}};
        }
        else
        {
            std::cerr << "[Parse Error] ERR001 Invalid Syntax Expected Expression";
            exit(EXIT_FAILURE);
        }
    }
    else if (peek().value().type == IF)
    {
        stmtNode = {scopeParser::parseIfStmt()};
        expectSemi = false;
    }
    else if (peek().value().type == ELIF)
    {
        std::cerr << "[Parse Error] ERR008 Illegal 'elif' without matching if";
        exit(EXIT_FAILURE);
    }
    else if (peek().value().type == ELSE)
    {
        std::cerr << "[Parse Error] ERR008 Illegal 'else' without matching if";
        exit(EXIT_FAILURE);
    }
    else if (peek().value().type == OUTPUT)
    {
        consume();
        tryConsume('(');
        if (auto const &nodeExpr = expressionParser::parseExpr(ANY_TYPE))
        {
            tryConsume(')');
            stmtNode = {node::StmtOutput{new node::Expr(nodeExpr.value())}};
        }
        else
        {
            std::cerr << "[Parse Error] ERR001 Invalid Syntax Expected Expression";
            exit(EXIT_FAILURE);
        }
    }
    else if (peek().value().type == INPUT)
    {
        auto tmpInpStmt = parseInputStmt().value();
        stmtNode = node::Stmt{tmpInpStmt};
    }
    else if (peek().value().type == WHILE)
    {
        stmtNode = {scopeParser::parseWhileLoop()};
        expectSemi = false;
    }
    else if (peek().value().type == FOR)
    {
        stmtNode = {scopeParser::parseForLoop()};
        expectSemi = false;
    }
    else if ((peek().value().type == INT_LET || peek().value().type == FLOAT_LET ||
            peek().value().type == STRING_LET || peek().value().type == BOOL_LET ||
            peek().value().type == CHAR_LET) || peek().value().type == CONST &&
        (peek(1).value().type == INT_LET ||
            peek(1).value().type == FLOAT_LET ||
            peek(1).value().type == STRING_LET ||
            peek(1).value().type == BOOL_LET ||
            peek(1).value().type == CHAR_LET))
    {
        auto letStmt = varParser::parseLet().value();
        stmtNode = {letStmt};
        varParser::m_vars.insert({letStmt.ident.value.value(), letToType[letStmt.letType]});
    }
    else if (peek().value().type == INC || peek().value().type == DEC ||
        peek(1).has_value() &&
        ((peek(1).value().type == INC || peek(1).value().type == DEC) && peek().value().type == IDENT))
    {
        if (auto const &incDecStmt = expressionParser::parseIncDec())
        {
            stmtNode = {incDecStmt.value()};
        }
    }
    else if (peek().value().type == IDENT)
    {
        const auto &varIdent = consume();
        if (!varParser::m_vars.contains(varIdent.value.value()))
        {
            std::cerr << "[Parse Error] ERR005 Undeclared Identifier '" << varIdent.value.value() << "'";
            exit(EXIT_FAILURE);
        }
        if (peek().has_value())
        {
            auto const &oper = consume();
            auto const &nodeExpr = expressionParser::parseExpr(varParser::m_vars[varIdent.value.value()]);
            if (!nodeExpr.has_value())
            {
                std::cerr << "[Parse Error] ERR006 Value Doesn't Matches Type";
                exit(EXIT_FAILURE);
            }
            std::unordered_map<Tokens, Tokens> assingOprToOpr =
            {
                {PLUSEQ, PLUS},
                {MINUSEQ, MINUS},
                {MULTEQ, MULT},
                {DIVEQ, DIV},
            };
            if (oper.type == PLUSEQ || oper.type == MINUSEQ || oper.type == MULTEQ || oper.type == DIVEQ)
            {
                stmtNode = {
                    node::StmtVar{
                        varIdent,
                        new node::Expr(new node::BinExpr(new node::Expr(new node::ValExpr(node::ExprIdent{varIdent})),
                                                         new node::Expr(nodeExpr.value()), assingOprToOpr[oper.type])),
                        varParser::m_vars[varIdent.value.value()]
                    }
                };
            }
            else if (oper.type == EQ)
            {
                stmtNode = {
                    {
                        node::StmtVar{
                            varIdent,
                            new node::Expr(nodeExpr.value()),
                            varParser::m_vars[varIdent.value.value()]
                        }
                    }
                };
            }
            else
            {
                std::cerr << "[Parse Error] ERR001 Invalid Syntax Expected '='";
                exit(EXIT_FAILURE);
            }
        }
    }
    else if (peek().value().type == CONTINUE)
    {
        stmtNode = {node::StmtCont{}};
        consume();
    }
    else if (peek().value().type == BREAK)
    {
        stmtNode = {node::StmtBreak{}};
        consume();
    }
    else if (peek().value().type == SWITCH)
    {
        stmtNode = {scopeParser::parseSwitchStmt().value()};
        expectSemi = false;
    }
    else
    {
        return {};
    }
    if (expectSemi)
    {
        tryConsume(';');
    }
    return stmtNode;
}

std::optional<node::Prog> parser::parseProg()
{
    while (peek().has_value())
    {
        if (auto stmt = parseStmt())
        {
            prog.statements.push_back(stmt.value());
        }
        else
        {
            std::cerr << "[Parse Error] ERR003 Invalid Statement";
            exit(EXIT_FAILURE);
        }
    }
    return prog;
}
