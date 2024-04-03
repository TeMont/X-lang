#include "compile.hpp"

bool CreateObjectFile(std::string Path)
{

    int res = system(("nasm -f win64 " + Path).c_str());
    if (res)
    {
        std::cerr << "Error Creating Object File";
        exit(EXIT_FAILURE);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 1;
}

bool LinkObjectFiles(std::string Path)
{

    int res = system(("golink /entry:_start /console kernel32.dll user32.dll " + Path).c_str());
    if (res)
    {
        std::cerr << "Error Linking File";
        exit(EXIT_FAILURE);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 1;
}

size_t compiler::m_stack_size;
std::stringstream compiler::m_output;
std::unordered_map<std::string, compiler::Var> compiler::m_vars;

void compiler::comp_val_expr(const node::ValExpr &expr, std::string ExpectedType)
{
    struct ExprVisitor
    {
        std::string ExpectedType;

        ExprVisitor(std::string expectedType) : ExpectedType(std::move(expectedType)) {}

        void operator()(const node::ExprIdent &expr_ident)
        {
            if (m_vars.count(expr_ident.ident.value.value()))
            {
                const auto &var = m_vars[expr_ident.ident.value.value()];
                if (ExpectedType == ANY_TYPE || ExpectedType == var.Type)
                {
                    push("QWORD [rsp + " + std::to_string((m_stack_size - var.stack_loc - 1) * 8) + "]");
                }
                else
                {
                    std::cerr << "ERR006 Value Doesn't Matches Type";
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                std::cerr << "ERR005 Undeclared Indentifier '" << expr_ident.ident.value.value() << "'";
                exit(EXIT_FAILURE);
            }
        }
        void operator()(const node::ExprIntLit &expr_int)
        {
            if (ExpectedType == ANY_TYPE || ExpectedType == INT_TYPE)
            {
                if ((expr_int.int_lit.value.has_value()))
                {
                    m_output << "\tmov rdx, " << expr_int.int_lit.value.value() << '\n';
                }
                push("rdx");
            }
            else
            {
                std::cerr << "ERR006 Value Doesn't Matches Type";
                exit(EXIT_FAILURE);
            }
        }
        void operator()(const node::ExprStrLit &expr_str)
        {
            if (ExpectedType == ANY_TYPE || ExpectedType == STR_TYPE)
            {
                if ((expr_str.str_lit.value.has_value()))
                {
                    m_output << "\tmov rdx, '" << expr_str.str_lit.value.value() << "'\n";
                }
                push("rdx");
            }
            else
            {
                std::cerr << "ERR006 Value Doesn't Matches Type";
                exit(EXIT_FAILURE);
            }
        }
    };

    ExprVisitor visitor(ExpectedType);
    std::visit(visitor, expr.var);
}

void compiler::comp_bin_expr(const node::BinExpr &expr, std::string ExpectedType)
{
    struct ExprVisitor
    {
        std::string ExpectedType;

        ExprVisitor(std::string expectedType) : ExpectedType(std::move(expectedType)) {}

        void operator()(const node::BinExprAdd* bin_expr_add)
        {
            comp_expr(*bin_expr_add->fvl, ExpectedType);
            comp_expr(*bin_expr_add->svl, ExpectedType);
            pop("rdi");
            pop("rdx");
            m_output << "\tadd rdx, rdi\n";
            push("rdx");
        }
        void operator()(const node::BinExprSub* bin_expr_sub)
        {
            comp_expr(*bin_expr_sub->fvl, ExpectedType);
            comp_expr(*bin_expr_sub->svl, ExpectedType);
            pop("rdi");
            pop("rdx");
            m_output << "\tsub rdx, rdi\n";
            push("rdx");
        }
        void operator()(const node::BinExprMul* bin_expr_mul)
        {
            comp_expr(*bin_expr_mul->fvl, ExpectedType);
            comp_expr(*bin_expr_mul->svl, ExpectedType);
            pop("rdi");
            pop("rdx");
            m_output << "\timul rdx, rdi\n";
            push("rdx");
        }
        void operator()(const node::BinExprDiv* bin_expr_div)
        {
            comp_expr(*bin_expr_div->fvl, ExpectedType);
            comp_expr(*bin_expr_div->svl, ExpectedType);
            pop("rdi");
            pop("rdx");
            m_output << "\tmov rax, rdx\n";
            m_output << "\tmov rdx, 0\n";
            m_output << "\tidiv rdi\n";
            m_output << "\tmov rdx, rax\n";
            push("rdx");
        }
    };

    ExprVisitor visitor(ExpectedType);
    std::visit(visitor, expr.var);
}

void compiler::comp_expr(const node::Expr &expr, std::string ExpectedType)
{
    struct ExprVisitor
    {
        std::string ExpectedType;

        ExprVisitor(std::string expectedType) : ExpectedType(std::move(expectedType)) {}

        void operator()(const node::BinExpr* BinExpr)
        {
            comp_bin_expr(*BinExpr, ExpectedType);
        }

        void operator()(const node::ValExpr* ValExpr)
        {
            comp_val_expr(*ValExpr, ExpectedType);
        }
    };

    ExprVisitor visitor(ExpectedType);
    std::visit(visitor, expr.var);
}

void compiler::comp_stmt(const node::Stmt &stmt)
{
    struct StmtVisitor
    {
        compiler *comp;
        void operator()(const node::StmtReturn &stmt_ret)
        {
            comp->comp_expr(*stmt_ret.Expr, INT_TYPE);
            pop("rcx");
            m_output << "\tcall ExitProcess"
                     << "\n";
        }
        void operator()(const node::StmtIntLet &stmt_int_let)
        {
            if (m_vars.count(stmt_int_let.ident.value.value()))
            {
                std::cerr << "ERR004 Identefier '" << stmt_int_let.ident.value.value() << "' Is Already Declared";
                exit(EXIT_FAILURE);
            }
            else
            {
                m_vars.insert({stmt_int_let.ident.value.value(), Var{m_stack_size, INT_TYPE}});
                if (&stmt_int_let.Expr->var != nullptr)
                {
                    comp->comp_expr(*stmt_int_let.Expr, INT_TYPE);
                }
                else 
                {
                    push("rdx");
                }
            }
        }
        void operator()(const node::StmtStrLet &stmt_str_let)
        {
            if (m_vars.count(stmt_str_let.ident.value.value()))
            {
                std::cerr << "ERR004 Identefier '" << stmt_str_let.ident.value.value() << "' Is Already Declared";
                exit(EXIT_FAILURE);
            }
            else
            {
                m_vars.insert({stmt_str_let.ident.value.value(), Var{m_stack_size, STR_TYPE}});
                
                if (&stmt_str_let.Expr->var != nullptr)
                {
                    comp->comp_expr(*stmt_str_let.Expr, STR_TYPE);
                }
                else 
                {
                    push("rdx");
                }
            }
        }
        void operator()(const node::StmtIntVar &stmt_int_var)
        {
            if (m_vars.count(stmt_int_var.ident.value.value()))
            {
                const auto &var = m_vars[stmt_int_var.ident.value.value()];
                comp->comp_expr(*stmt_int_var.Expr, INT_TYPE);
                pop("rdx");
                m_output << "\tmov [rsp + " + std::to_string((m_stack_size - var.stack_loc - 1) * 8) + "], rdx\n";
            }
            else
            {
                std::cerr << "ERR004 Identefier '" << stmt_int_var.ident.value.value() << "' Was Not Declared";
                exit(EXIT_FAILURE);
            }
        }
        void operator()(const node::StmtStrVar &stmt_str_var)
        {
            if (m_vars.count(stmt_str_var.ident.value.value()))
            {
                comp->comp_expr(*stmt_str_var.Expr, STR_TYPE);
            }
            else
            {
                std::cerr << "ERR004 Identefier '" << stmt_str_var.ident.value.value() << "' Was Not Declared";
                exit(EXIT_FAILURE);
            }
        }
    };
    StmtVisitor visitor;
    std::visit(visitor, stmt.var);
}

std::stringstream compiler::compile()
{
    m_output << "section .text\nglobal _start\nextern ExitProcess\n_start:\n";

    for (const node::Stmt &stmt : m_prog.statements)
    {
        comp_stmt(stmt);
    }

    std::stringstream output = std::move(m_output);
    return output;
}

void compiler::push(const std::string &reg)
{
    m_output << "\tpush " << reg << "\n";
    ++m_stack_size;
}

void compiler::pop(const std::string &reg)
{
    m_output << "\tpop " << reg << "\n";
    --m_stack_size;
}