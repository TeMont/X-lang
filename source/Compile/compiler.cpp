#include "compiler.hpp"

bool createObjectFile(const std::string &path)
{
    if (system(("nasm -fwin64 " + path + ".asm").c_str()))
    {
        std::cerr << "Error Creating Object File";
        exit(EXIT_FAILURE);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return true;
}

bool linkObjectFiles(const std::string &path)
{
    if (system(("gcc.exe " + path + ".obj -g -o" + path + ".exe -m64").c_str()))
    {
        std::cerr << "Error Linking File";
        exit(EXIT_FAILURE);
    }
    return true;
}

std::string compiler::createLabel()
{
    std::stringstream ss;
    ss << "label" << m_labelCount;
    m_labelCount++;
    return ss.str();
}

std::string compiler::createSCLabel()
{
    std::stringstream ss;
    ss << "SC" << m_SCCount;
    m_SCCount++;
    return ss.str();
}

uint64_t compiler::m_labelCount = 0;
uint64_t compiler::m_SCCount = 0;
std::stringstream compiler::m_output;
std::stringstream compiler::m_SC;
std::stringstream compiler::m_bssSC;

void compiler::compInput(const node::StmtInput &stmtInput)
{
    if (expressionCompiler::compExpr(*stmtInput.msg, STR_TYPE))
    {
        varCompiler::pop("rdx");
    }
    else if (expressionCompiler::compExpr(*stmtInput.msg, CHAR_TYPE))
    {
        varCompiler::pop("rdx");
        m_output << "\tmov rsi, OutputBuffer\n";
        m_output << "\tmov [rsi], dx\n";
        m_output << "\tmov rdx, rsi\n";
    }
    else if (expressionCompiler::compExpr(*stmtInput.msg, FLOAT_TYPE, false))
    {
        varCompiler::pop("rdx");
        m_output << "\tmovq xmm0, rdx\n";
        m_output << "\tmov rsi, OutputBuffer\n";
        m_output << "\tcall _ftoa\n";
        m_output << "\tmov rdx, rsi\n";
    }
    else if (expressionCompiler::compExpr(*stmtInput.msg, INT_TYPE) ||
        expressionCompiler::compExpr(*stmtInput.msg, BOOL_TYPE))
    {
        varCompiler::pop("rdx");
        m_output << "\tmov rax, rdx\n";
        m_output << "\tmov rsi, OutputBuffer\n";
        m_output << "\tcall _itoa\n";
        m_output << "\tmov rdx, rsi\n";
    }
    m_output << "\tmov rsi, InputBuffer\n";
    m_output << "\tmov rax, 256\n";
    m_output << "\tcall _scanf\n";
}

void compiler::compStmt(const node::Stmt &stmt)
{
    struct stmtVisitor
    {
        void operator()(const node::StmtReturn &stmtRet) const
        {
            m_output << ";;\treturn\n";
            if (!expressionCompiler::compExpr(*stmtRet.Expr, INT_TYPE) &&
                !expressionCompiler::compExpr(*stmtRet.Expr, CHAR_TYPE) &&
                !expressionCompiler::compExpr(*stmtRet.Expr, BOOL_TYPE))
            {
                std::cerr << "[Compile Error] ERR006 Value Doesnt Matches Type";
                exit(EXIT_FAILURE);
            }
            varCompiler::pop("rcx");
            m_output << "\tcall ExitProcess\n";
            m_output << ";;\t/return\n";
        }

        void operator()(const node::StmtIf &stmtIf) const
        {
            scopeCompiler::compIfStmt(stmtIf);
        }

        void operator()(const node::StmtOutput &stmtOutput) const
        {
            m_output << ";;\tOutput\n";
            if (expressionCompiler::compExpr(*stmtOutput.Expr, STR_TYPE))
            {
                varCompiler::pop("rdx");
                m_output << "\tcall _printf\n";
                m_output << ";;\t/Output\n";
                return;
            }
            if (expressionCompiler::compExpr(*stmtOutput.Expr, CHAR_TYPE))
            {
                varCompiler::pop("rdx");
                m_output << "\tmov rsi, OutputBuffer\n";
                m_output << "\tmov [rsi], dx\n";
                m_output << "\tmov rdx, rsi\n";
                m_output << "\tcall _printf\n";
            }
            else if (expressionCompiler::compExpr(*stmtOutput.Expr, FLOAT_TYPE, false))
            {
                varCompiler::pop("rdx");
                m_output << "\tmovq xmm0, rdx\n";
                m_output << "\tmov rsi, OutputBuffer\n";
                m_output << "\tcall _ftoa\n";
                m_output << "\tmov rdx, rsi\n";
                m_output << "\tcall _printf\n";
            }
            else if (expressionCompiler::compExpr(*stmtOutput.Expr, INT_TYPE) ||
                expressionCompiler::compExpr(*stmtOutput.Expr, BOOL_TYPE))
            {
                varCompiler::pop("rdx");
                m_output << "\tmov rax, rdx\n";
                m_output << "\tmov rsi, OutputBuffer\n";
                m_output << "\tcall _itoa\n";
                m_output << "\tmov rdx, rsi\n";
                m_output << "\tcall _printf\n";
            }
            m_output << "\tmov rsi, OutputBuffer\n";
            m_output << "\tmov rdx, 20\n";
            m_output << "\tcall _clearBuffer\n";
            m_output << ";;\t/Output\n";
        }

        void operator()(const node::StmtInput &stmtInput) const
        {
            m_output << ";;\tInput\n";
            compInput(stmtInput);
            m_output << "\tmov rsi, OutputBuffer\n";
            m_output << "\tmov rdx, 20\n";
            m_output << "\tcall _clearBuffer\n";
            m_output << "\tmov rsi, InputBuffer\n";
            m_output << "\tmov rdx, 256\n";
            m_output << "\tcall _clearBuffer\n";
            m_output << ";;\t/Input\n";
        }

        void operator()(const node::StmtWhileLoop &whileLoop) const
        {
            scopeCompiler::compWhileLoop(whileLoop);
        }

        void operator()(const node::StmtForLoop &forLoop) const
        {
            scopeCompiler::compForLoop(forLoop);
        }

        void operator()(const node::IncDec &incDec) const
        {
            if (!varCompiler::m_vars.contains(incDec.ident.value.value()))
            {
                std::cerr << "[Compile Error] ERR004 Identifier '" << incDec.ident.value.value()
                    << "' Was Not Declared";
                exit(EXIT_FAILURE);
            }
            expressionCompiler::compIncDec(incDec.ident, incDec.isInc,
                                           varCompiler::m_vars[incDec.ident.value.value()].Type);
        }

        void operator()(const node::StmtLet &stmtLet) const
        {
            varCompiler::compLet(stmtLet);
        }

        void operator()(const node::StmtVar &stmtVar) const
        {
            varCompiler::compVar(stmtVar);
        }

        void operator()(const node::StmtCont &stmtCont) const
        {
            scopeCompiler::compContinueStmt();
        }

        void operator()(const node::StmtBreak &stmtBreak) const
        {
            scopeCompiler::compBreakStmt();
        }

        void operator()(const node::StmtSwitch &stmtSwitch) const
        {
            scopeCompiler::compSwitchStmt(stmtSwitch);
        }
    };
    stmtVisitor visitor;
    std::visit(visitor, stmt.var);
}

std::stringstream compiler::compile()
{
    m_output << "extern GetStdHandle, WriteConsoleA, ReadConsoleA, ExitProcess\n\n"
        "stdout_query equ -11\n"
        "stdin_query equ -10\n"
        "section .data\n"
        "\tstdout dw 0\n"
        "\tstdin dw 0\n"
        "\tbytesWritten dw 0\n"
        "\tbytesRead dw 0\n\n"
        "section .bss\n"
        "\tOutputBuffer resb 20\n"
        "\tInputBuffer resb 256\n\n"
        "section .text\n"
        "\n"
        "_printf:\n"
        "\t; INPUT:\n"
        "\t; RDX - string\n"
        "\tcall _countStrLen\n"
        "\tmov r8, rcx\n"
        "\tmov rcx, stdout_query\n"
        "\tcall GetStdHandle\n"
        "\tmov [rel stdout], rax\n"
        "\tmov rcx, [rel stdout]\n"
        "\tmov r9, bytesWritten\n"
        "\txor r10, r10\n"
        "\tcall WriteConsoleA\n"
        "\tret\n"
        "\n"
        "_scanf:\n"
        "\t; INPUT:\n"
        "\t; RDX - message\n"
        "\t; RSI - buffer for input\n"
        "\t; RAX - buffer size\n"
        "\t; OUTPUT:\n"
        "\t; RSI - buffer with user input\n"
        "\tpush rax\n"
        "\tpush rsi\n"
        "\tpush rdx\n"
        "\tmov rdx, rax\n"
        "\tcall _clearBuffer\n"
        "\tpop rdx\n"
        "\tcall _printf\n"
        "\tmov rcx, stdin_query\n"
        "\tcall GetStdHandle\n"
        "\tmov [rel stdin], rax\n"
        "\tmov rcx, [rel stdin]\n"
        "\tpop rdx\n"
        "\tpop r8\n"
        "\tmov r9, bytesRead\n"
        "\tcall ReadConsoleA\n"
        "\tret\n"
        "\n"
        "_countStrLen:\n"
        "\t; INPUT:\n"
        "\t; RDX - string\n"
        "\t; OUTPUT:\n"
        "\t; RCX - string length\n"
        "\txor rcx, rcx\n"
        "\tcontinue_count:\n"
        "\tmov al, byte [rdx + rcx]\n"
        "\tcmp al, 0\n"
        "\tje end_len_count\n"
        "\tinc rcx\n"
        "\tjmp continue_count\n"
        "\tend_len_count:\n"
        "\tret\n"
        "\n"
        "_itoa:\n"
        "\t; INPUT:\n"
        "\t; RSI - output string\n"
        "\t; RAX - integer\n"
        "\t; OUTPUT:\n"
        "\t; RSI - string\n"
        "\tpush rax\n"
        "\tpush rsi\n"
        "\txor r9, r9\n"
        "\ttest rax, rax\n"
        "\tjns .positive\n"
        "\tneg rax\n"
        "\tmov r9, 1\n"
        "\tjmp .start\n"
        "\t.positive:\n"
        "\tmov r9, 0\n"
        "\t.start:\n"
        "\tpush rax\n"
        "\tmov rdi, 1\n"
        "\tmov rcx, 1\n"
        "\tmov rbx, 10\n"
        "\t.get_divisor:\n"
        "\txor rdx, rdx\n"
        "\tdiv rbx\n"
        "\tcmp rax, 0\n"
        "\tje ._after\n"
        "\timul rcx, 10\n"
        "\tinc rdi\n"
        "\tjmp .get_divisor\n"
        "\t._after:\n"
        "\tpop rax\n"
        "\tpush rdi\n"
        "\ttest r9, 1\n"
        "\tjz .to_string\n"
        "\tmov byte [rsi], '-'\n"
        "\tinc rsi\n"
        "\txor r9, r9\n"
        "\t.to_string:\n"
        "\txor rdx, rdx\n"
        "\tdiv rcx\n"
        "\tadd al, '0'\n"
        "\tmov [rsi], al\n"
        "\tinc rsi\n"
        "\tpush rdx\n"
        "\txor rdx, rdx\n"
        "\tmov rax, rcx\n"
        "\tmov rbx, 10\n"
        "\tdiv rbx\n"
        "\tmov rcx, rax\n"
        "\tpop rax\n"
        "\tcmp rcx, 0\n"
        "\tjg .to_string\n"
        "\tpop rdx\n"
        "\tpop rsi\n"
        "\tpop rax\n"
        "\tret\n"
        "\n"
        "_ftoa:\n"
        "\t; INPUT:\n"
        "\t; RSI - output string\n"
        "\t; XMM0 - float value\n"
        "\t; OUTPUT:\n"
        "\t; RSI - updated string\n"
        "\tpush rsi\n"
        "\tmov rdx,__?float32?__(10.0)\n"
        "\tmovq xmm1, rdx\n"
        "\tmov rcx, 4\n"
        "\tfloat_mul:\n"
        "\tmulss xmm0, xmm1\n"
        "\tloop float_mul\n"
        "\troundss xmm0, xmm0, 0\n"
        "\tcvttss2si rdx, xmm0\n"
        "\tcvtsi2ss xmm0, rdx\n"
        "\tmov rcx, 4\n"
        "\tfloat_div:\n"
        "\tmov rdx,__?float32?__(0.1)\n"
        "\tmovq xmm1, rdx\n"
        "\tmulss xmm0, xmm1\n"
        "\tloop float_div\n"
        "\tcvttss2si rax, xmm0\n"
        "\tcall _itoa\n"
        "\tincreaseBuffer:\n"
        "\tcmp byte [rsi], 00h\n"
        "\tje endIncreasing\n"
        "\tinc rsi\n"
        "\tjmp increaseBuffer\n"
        "\tendIncreasing:\n"
        "\tmov byte [rsi], '.'\n"
        "\tinc rsi\n"
        "\tcvtsi2ss xmm1, rax\n"
        "\tsubss xmm0, xmm1\n"
        "\tmov r10, 4\n"
        "\tmov rdx, __?float32?__(0.0)\n"
        "\tmovq xmm1, rdx\n"
        "\tcomiss xmm0, xmm1\n"
        "\tjb negative_float\n"
        "\tjmp convert_fraction\n"
        "\tnegative_float:\n"
        "\tmov rdx, __?float32?__(-1.0)\n"
        "\tmovq xmm1, rdx\n"
        "\tmulss xmm0, xmm1\n"
        "\tconvert_fraction:\n"
        "\tmov rdx, __?float32?__(10.0)\n"
        "\tmovq xmm1, rdx\n"
        "\tmulss xmm0, xmm1\n"
        "\tcvttss2si rax, xmm0\n"
        "\tcall _itoa\n"
        "\tcvtsi2ss xmm1, rax\n"
        "\tsubss xmm0, xmm1\n"
        "\tinc rsi\n"
        "\tdec r10\n"
        "\tcmp r10, 0\n"
        "\tjl end_convert\n"
        "\tjmp convert_fraction\n"
        "\tend_convert:\n"
        "\tmov rcx, 4\n"
        "\tclear_zeroes:\n"
        "\tdec rsi\n"
        "\tmov al, byte [rsi]\n"
        "\tcmp al, '0'\n"
        "\tjne finish_clearing\n"
        "\tmov byte [rsi], 00h\n"
        "\tloop clear_zeroes\n"
        "\tfinish_clearing:\n"
        "\tpop rsi\n"
        "\tret\n"
        "\n"
        "_stoi:\n"
        "\t; INPUT:\n"
        "\t; RSI - buffer to convert\n"
        "\t; OUTPUT:\n"
        "\t; RDI - integer\n"
        "\txor rdi, rdi\n"
        "\tmov rbx, 10\n"
        "\txor rax, rax\n"
        "\tmov rcx, 1\n"
        "\tmovzx rdx, byte[rsi]\n"
        "\tcmp rdx, '-'\n"
        "\tje negative\n"
        "\tcmp rdx, '+'\n"
        "\tje positive\n"
        "\tcmp rdx, '0'\n"
        "\tjl done\n"
        "\tcmp rdx, '9'\n"
        "\tjg done\n"
        "\tjmp next_digit\n"
        "\tpositive:\n"
        "\tinc rsi\n"
        "\tjmp next_digit\n"
        "\tnegative:\n"
        "\tinc rsi\n"
        "\tmov rcx, 0\n"
        "\tnext_digit:\n"
        "\tmovzx rdx, byte[rsi]\n"
        "\ttest rdx, rdx\n"
        "\tjz done\n"
        "\tcmp rdx, 13\n"
        "\tje done\n"
        "\tcmp rdx, '0'\n"
        "\tjl done\n"
        "\tcmp rdx, '9'\n"
        "\tjg done\n"
        "\timul rdi, rbx\n"
        "\tsub rdx, '0'\n"
        "\tadd rdi, rdx\n"
        "\tinc rsi\n"
        "\tjmp next_digit\n"
        "\tdone:\n"
        "\tcmp rcx, 0\n"
        "\tje apply_negative\n"
        "\tret\n"
        "\tapply_negative:\n"
        "\tneg rdi\n"
        "\tret\n"
        "\n"
        "_clearBuffer:\n"
        "\t; INPUT:\n"
        "\t; RSI - buffer to clear\n"
        "\t; RDX - buffer size\n"
        "\tclear:\n"
        "\tcmp rdx, 0\n"
        "\tje end\n"
        "\tcmp BYTE [rsi], 00H\n"
        "\tje end\n"
        "\tmov al, 00H\n"
        "\tmov [rsi], al\n"
        "\tinc rsi\n"
        "\tdec rdx\n"
        "\tjmp clear\n"
        "\tend:\n"
        "\tret\n"
        "\n"
        "\t_stof:\n"
        "\t; INPUT:\n"
        "\t; RSI - buffer to convert\n"
        "\t; OUTPUT:\n"
        "\t; XMM0 - integer\n"
        "\tcall _stoi\n"
        "\tcmp byte [rsi], '.'\n"
        "\tjne finish\n"
        "\tinc rsi\n"
        "\tpush rdi\n"
        "\tmov rdx, rsi\n"
        "\tcall _countStrLen\n"
        "\tpush rcx\n"
        "\tcall _stoi\n"
        "\tcvtsi2ss xmm0, rdi\n"
        "\tpop rcx\n"
        "\tmov rdx,__?float32?__(0.1)\n"
        "\tmovq xmm1, rdx\n"
        "\tdivide:\n"
        "\tmulss xmm0, xmm1\n"
        "\tloop divide\n"
        "\tpop rdx\n"
        "\tcvtsi2ss xmm1, rdx\n"
        "\taddss xmm0, xmm1\n"
        "\tret\n"
        "\tfinish:\n"
        "\tcvtsi2ss xmm0, rdi\n"
        "\tret\n"
        "global main\n"
        "main:\n";

    for (const node::Stmt &stmt : m_prog.statements)
    {
        compStmt(stmt);
    }

    std::stringstream output;
    output << m_SC.str();
    output << "section .bss\n";
    output << m_bssSC.str();
    output << m_output.str();

    return output;
}
