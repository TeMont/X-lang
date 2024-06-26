# X-lang
Compiled programming language created with support of basic features.
The compiler is made using NASM & gcc Linker.

# done
- [X] Variables
- [X] Declaring Without Value
- [X] Return Statement
- [X] String Variables
- [X] Integer Variables
- [X] Bool Variables
- [X] Char Variables
- [X] Math operations with precedence
- [X] Logical Operators
- [X] If Statement
- [X] Elif Statement
- [X] Else Statement
- [X] One-Line Comments
- [X] Multi-Line Comments
- [X] Output
- [X] Input
- [X] Const Variables
- [X] Increment and Decrement
- [X] While Loop
- [X] For Loop
- [X] Negative Integers
- [X] Float Variables
- [X] Float Increment and Decrement
- [X] += -= *= /= operators
- [X] Break / Continue keyword
- [X] Switch-case statement
- [X] 'default' in switch-case

# GUIDE
Dependencies: Nasm x86-64, gcc Linker

### Building
Dependencies For Building: C++23
```bash
git clone https://github.com/TeMont/X-lang.git
cd X-lang
mkdir build
cd build
cmake ..
cmake --build .
```
# Code Example

```c
int firstNum;
int secondNum;
int result;
char symbol;

firstNum = stdInput("Enter First Num: ");
symbol = stdInput("Enter Action Symbol: ");
secondNum = stdInput("Enter Second Num: ");

if (symbol == '+')
{
    result = firstNum + secondNum;
}
elif (symbol == '-')
{
    result = firstNum - secondNum;
}
elif (symbol == '*')
{
    result = firstNum * secondNum;
}
elif (symbol == '/')
{
    result = firstNum / secondNum;
}
else
{
    stdOut("You Entered Incorrect Symbol");
    return 1;
}
stdOut(result);
return 0;
```

# Code Compile
### Terminal
```bash
./XComp.exe <filename>.xy
./<filename>.exe
```