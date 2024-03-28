#include "lexer.hpp"

std::optional<Token> parser::peek(int offset) const
{
    if (m_index + offset >= m_tokens.size())
    {
        return {};
    }
    else
    {
        return m_tokens[m_index + offset];
    }
}

Token parser::consume()
{
    return m_tokens[m_index++];
}

std::optional<node::Expr> parser::parseExpr(std::string ExpectedType)
{
    if (peek().has_value())
    {
        node::Expr expr;
        if (peek().value().type == Tokens::INT_LITERAL)
        {
            if (ExpectedType == INT_TYPE || ExpectedType == ANY_TYPE)
            {
                node::ExprIntLit intExpr;
                intExpr.int_lit = consume();
                expr.var = intExpr;
            }
            else
            {
                std::cerr << "ERR006 Value Doesn't Matches Type";
                exit(EXIT_FAILURE);
            }
        }
        else if (peek().value().type == Tokens::STRING_LITERAL)
        {
            if (ExpectedType == STR_TYPE || ExpectedType == ANY_TYPE)
            {
                node::ExprStrLit strExpr;
                strExpr.str_lit = consume();
                expr.var = strExpr;
            }
            else
            {
                std::cerr << "ERR006 Value Doesn't Matches Type";
                exit(EXIT_FAILURE);
            }
        }
        else if (peek().value().type == Tokens::IDENT)
        {
            node::ExprIdent identExpr;
            identExpr.ident = consume();
            expr.var = identExpr;
        }
        else
        {
            return {};
        }
        return expr;
    }
    else
    {
        return {};
    }
}

std::optional<node::Stmt> parser::parseStmt()
{
    std::optional<node::Stmt> stmt_node;
    if (peek().has_value())
    {
        if (peek().value().type == Tokens::RETURN)
        {
            consume();
            if (auto node_expr = parseExpr(INT_TYPE))
            {

                stmt_node = {{node::StmtReturn{node_expr.value()}}};

                if (peek().has_value() && peek().value().type == Tokens::SEMICOLON)
                {
                    consume();
                }
                else
                {
                    std::cerr << "ERR001 Invalid Syntax Expected ';'\n";
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (peek().value().type == Tokens::INT_LET)
        {
            consume();
            if (peek().has_value() && peek().value().type == Tokens::IDENT)
            {
                auto var_ident = consume();
                if (peek().has_value() && peek().value().type == Tokens::EQUALS)
                {
                    consume();
                    if (auto node_expr = parseExpr(INT_TYPE))
                    {
                        stmt_node = {{node::StmtIntLet{var_ident, node_expr.value()}}};
                        if (peek().has_value() && peek().value().type == Tokens::SEMICOLON)
                        {
                            consume();
                        }
                        else
                        {
                            std::cerr << "ERR001 Invalid Syntax Expected ';'";
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        std::cerr << "ERR001 Invalid Syntax Expected Expression";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    stmt_node = {{node::StmtIntLet{var_ident}}};
                    if (peek().has_value() && peek().value().type == Tokens::SEMICOLON)
                    {
                        consume();
                    }
                    else
                    {
                        std::cerr << "ERR001 Invalid Syntax Expected ';'";
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                std::cerr << "ERR002 Expected An Identifier";
                exit(EXIT_FAILURE);
            }
        }
        else if (peek().value().type == Tokens::STRING_LET)
        {
            consume();
            if (peek().has_value() && peek().value().type == Tokens::IDENT)
            {
                auto var_ident = consume();
                if (peek().has_value() && peek().value().type == Tokens::EQUALS)
                {
                    consume();
                    if (peek().has_value() && peek().value().type == Tokens::QOUTE)
                    {
                        consume();
                        if (auto node_expr = parseExpr(STR_TYPE))
                        {
                            stmt_node = {{node::StmtStrLet{var_ident, node_expr.value()}}};
                            if (peek().has_value() && peek().value().type == Tokens::QOUTE)
                            {
                                consume();
                                if (peek().has_value() && peek().value().type == Tokens::SEMICOLON)
                                {
                                    consume();
                                }
                                else
                                {
                                    std::cerr << "ERR001 Invalid Syntax Expected ';'";
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                std::cerr << "ERR001 Invalid Syntax Expected '\"'";
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            std::cerr << "ERR001 Invalid Syntax Expected Expression";
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (peek().has_value() && peek().value().type == Tokens::IDENT)
                    {
                        if (auto node_expr = parseExpr(STR_TYPE))
                        {
                            stmt_node = {{node::StmtStrLet{var_ident, node_expr.value()}}};
                            if (peek().has_value() && peek().value().type == Tokens::SEMICOLON)
                            {
                                consume();
                            }
                            else
                            {
                                std::cerr << "ERR001 Invalid Syntax Expected ';'";
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    else
                    {
                        std::cerr << "ERR001 Invalid Syntax Expected '\"'";
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    std::cerr << "ERR001 Invalid Syntax Expected '='";
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                std::cerr << "ERR002 Expected An Identifier";
                exit(EXIT_FAILURE);
            }
        }

        else if (peek().value().type == Tokens::IDENT)
        {
            auto var_ident = consume();
            if (peek().value().type == Tokens::EQUALS)
            {
                consume();
                if (peek().has_value() && peek().value().type == Tokens::QOUTE)
                {
                    if (auto node_expr = parseExpr(STR_TYPE))
                    {
                        stmt_node = {{node::StmtStrVar{var_ident, node_expr.value()}}};
                        if (peek().has_value() && peek().value().type == Tokens::QOUTE)
                        {
                            consume();
                            if (peek().has_value() && peek().value().type == Tokens::SEMICOLON)
                            {
                                consume();
                            }
                            else
                            {
                                std::cerr << "ERR001 Invalid Syntax Expected ';'";
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            std::cerr << "ERR001 Invalid Syntax Expected '\"'";
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        std::cerr << "ERR001 Invalid Syntax Expected String Literal";
                        exit(EXIT_FAILURE);
                    }
                }
                else if (auto node_expr = parseExpr(ANY_TYPE))
                {
                    stmt_node = {{node::StmtIntVar{var_ident, node_expr.value()}}};
                    if (peek().has_value() && peek().value().type == Tokens::SEMICOLON)
                    {
                        consume();
                    }
                    else
                    {
                        std::cerr << "ERR001 Invalid Syntax Expected ';'";
                        exit(EXIT_FAILURE);
                    }
                    
                }
            }
            else
            {
                std::cerr << "ERR001 Invalid Syntax Expected '='";
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            std::cerr << peek().value().type;
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        return {};
    }
    return stmt_node;
}

std::optional<node::Prog> parser::parseProg()
{
    node::Prog prog;
    while (peek().has_value())
    {
        if (auto stmt = parseStmt())
        {
            prog.statements.push_back(stmt.value());
        }
        else
        {
            std::cerr << "ERR003 Invalid Statement";
            exit(EXIT_FAILURE);
        }
    }
    return prog;
}