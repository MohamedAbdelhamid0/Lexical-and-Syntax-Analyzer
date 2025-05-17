#include "pythonlexer.h"
#include <cctype>
#include <algorithm>
#include <string>
#include <cmath>
#include <stack>
#include <vector>
#include <stdexcept>
// Move using declarations to file scope
using std::string;
using std::isalpha;
using std::isalnum;
using std::isxdigit;
using std::isdigit;

// Helper function to convert a string to lowercase
std::string toLower(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

PythonLexer::PythonLexer(const std::string& input) : source(input), addedBuiltins() {}

void PythonLexer::advance() {
    if (current() == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    pos++;
}

void PythonLexer::addToken(const std::string& lexeme, TokenType type) {
    tokens.push_back({ lexeme, type, line, column - static_cast<int>(lexeme.length()) });
}

bool PythonLexer::isOperatorChar(char c) {
    static const std::string opChars = "+-*/%=&|<>!^~.";
    return opChars.find(c) != std::string::npos;
}

bool PythonLexer::isDelimiter(char c) {
    static const std::string delimiters = ":,;()[]{}@";
    return delimiters.find(c) != std::string::npos;
}

bool PythonLexer::isHexadecimal(const std::string& str) {
    if (str.size() < 3 || (str[0] != '0' && (str[1] != 'x' && str[1] != 'X'))) {
        return false;
    }
    for (size_t i = 2; i < str.size(); ++i) {
        if (!std::isxdigit(str[i]) && str[i] != '_') {
            return false;
        }
    }
    return true;
}

bool PythonLexer::isTab(char c) {
    return c == '\t';
}

void PythonLexer::processNumber() {
    string num;
    bool valid = true;
    bool hasDigits = false;

    // Helper function to validate underscore placement
    auto validateUnderscores = [&num]() -> bool {
        if (num.empty()) return true;
        // No leading or trailing underscores, no consecutive underscores
        if (num.front() == '_' || num.back() == '_' || num.find("__") != string::npos) {
            return false;
        }
        return true;
    };

    // Check for hexadecimal (0x or 0X), binary (0b or 0B), or octal (0o or 0O)
    if (current() == '0' && pos + 1 < source.size()) {
        num += current();
        advance();  // Consume the '0'

        if (current() == 'x' || current() == 'X') {  // Hexadecimal (0x or 0X)
            num += current();
            advance();
            bool hasHexDigits = false;
            while (isxdigit(current()) || current() == '_') {
                char c = current();
                num += c;
                advance();
                if (isxdigit(c)) hasHexDigits = true;  // Check the consumed character
            }
            if (!hasHexDigits) {
                addError("Invalid hexadecimal number: " + num + " (no hexadecimal digits after 0x)");
                // Consume any trailing alphanumeric or underscore characters as part of the invalid token
                while (isalnum(current()) || current() == '_') {
                    num += current();
                    advance();
                }
                return;
            }

            if (!validateUnderscores()) {
                addError("Invalid underscore placement in hexadecimal number: " + num);
                return;
            }

            // Check for invalid trailing characters (e.g., "0x12G")
            if (isalnum(current()) || current() == '_') {
                string invalid = num;
                while (isalnum(current()) || current() == '_') {
                    invalid += current();
                    advance();
                }
                addError("Invalid hexadecimal number: " + invalid + " (invalid trailing characters)");
                return;
            }

            addToken(num, TokenType::HexadecimalNumber);
            return;
        } else if (current() == 'b' || current() == 'B') {  // Binary (0b or 0B)
            num += current();
            advance();
            bool hasBinaryDigits = false;
            while (current() == '0' || current() == '1' || current() == '_') {
                char c = current();
                num += c;
                advance();
                if (c == '0' || c == '1') hasBinaryDigits = true;  // Check the consumed character
            }
            if (!hasBinaryDigits) {
                addError("Invalid binary number: " + num + " (no binary digits after 0b)");
                return;
            }

            if (!validateUnderscores()) {
                addError("Invalid underscore placement in binary number: " + num);
                return;
            }

            // Check for invalid trailing characters (e.g., "0b1021")
            if (isalnum(current()) || current() == '_') {
                string invalid = num;
                while (isalnum(current()) || current() == '_') {
                    invalid += current();
                    advance();
                }
                addError("Invalid binary number: " + invalid + " (invalid trailing characters)");
                return;
            }

            addToken(num, TokenType::BinaryNumber);
            return;
        } else if (current() == 'o' || current() == 'O') {  // Octal (0o or 0O)
            num += current();
            advance();
            bool hasOctalDigits = false;
            while ((current() >= '0' && current() <= '7') || current() == '_') {
                char c = current();
                num += c;
                advance();
                if (c >= '0' && c <= '7') hasOctalDigits = true;  // Check the consumed character
            }
            if (!hasOctalDigits) {
                addError("Invalid octal number: " + num + " (no octal digits after 0o)");
                return;
            }

            if (!validateUnderscores()) {
                addError("Invalid underscore placement in octal number: " + num);
                return;
            }

            // Check for invalid digits (e.g., "0o89")
            if (isdigit(current()) && (current() == '8' || current() == '9')) {
                string invalid = num;
                while (isdigit(current())) {
                    invalid += current();
                    advance();
                }
                addError("Invalid octal number: " + invalid + " (contains digits 8 or 9)");
                return;
            }

            // Check for invalid trailing characters (e.g., "0o7g")
            if (isalnum(current()) || current() == '_') {
                string invalid = num;
                while (isalnum(current()) || current() == '_') {
                    invalid += current();
                    advance();
                }
                addError("Invalid octal number: " + invalid + " (invalid trailing characters)");
                return;
            }

            addToken(num, TokenType::OCTALNUMBER);
            return;
        } else {
            // If we have a '0' followed by digits but no 'x', 'b', or 'o', it's an invalid leading zero
            if (isdigit(current()) && current() != '0') {
                num += current();
                advance();
                while (isdigit(current()) || current() == '_') {
                    num += current();
                    advance();
                }
                addError("Invalid number: " + num + " (leading zeros are not allowed in decimal numbers)");
                return;
            }
            // Otherwise, we might have a decimal number starting with '0', which we'll handle below
            if (!isdigit(current()) && current() != '.' && current() != 'e' && current() != 'E') {
                addToken(num, TokenType::NUMBER);
                return;
            }
        }
    }

    // Handle decimal number (not prefixed by 0x, 0b, or 0o)
    while (isdigit(current()) || current() == '_') {
        if (isdigit(current())) hasDigits = true;
        num += current();
        advance();
    }

    if (!hasDigits) {
        addError("Invalid number: " + num + " (no digits found)");
        return;
    }

    // Handle decimal point for floating-point numbers
    bool hasDecimal = false;
    if (current() == '.') {
        hasDecimal = true;
        num += '.';
        advance();
        bool hasFractionalDigits = false;
        while (isdigit(current()) || current() == '_') {
            num += current();
            advance();
            if (isdigit(current())) hasFractionalDigits = true;
        }
        // Check for additional decimal points (e.g., 1.2.2.2)
        while (current() == '.') {
            num += current();
            advance();
            // Consume any digits after the additional decimal point
            while (isdigit(current()) || current() == '_') {
                num += current();
                advance();
            }
            hasDecimal = true; // Mark that we've seen another decimal point
        }
        if (hasDecimal && num.find('.') != num.rfind('.')) { // If there are multiple decimal points
            addError("Invalid floating-point number: " + num + " (multiple decimal points)");
            return;
        }
        if (!hasFractionalDigits && !hasDigits) {
            addError("Invalid floating-point number: " + num + " (no digits before or after decimal point)");
            return;
        }
    }

    // Check for scientific notation (e.g., 1e-10, 2.5E+3)
    bool hasExponent = false;
    if (current() == 'e' || current() == 'E') {
        hasExponent = true;
        num += current();
        advance();
        // Check for sign
        if (current() == '+' || current() == '-') {
            num += current();
            advance();
        }
        // Ensure there are digits after the 'e' or 'E'
        bool hasExponentDigits = false;
        while (isdigit(current()) || current() == '_') {
            num += current();
            advance();
            if (isdigit(current())) hasExponentDigits = true;
        }
        if (!hasExponentDigits) {
            addError("Invalid scientific notation: " + num + " (missing exponent digits)");
            return;
        }
        // Check for invalid double signs (e.g., "1e--10")
        if (num.find("e--") != string::npos || num.find("E--") != string::npos ||
            num.find("e++") != string::npos || num.find("E++") != string::npos ||
            num.find("e+-") != string::npos || num.find("E+-") != string::npos ||
            num.find("e-+") != string::npos || num.find("E-+") != string::npos) {
            addError("Invalid scientific notation: " + num + " (invalid exponent sign combination)");
            return;
        }
    }

    // Validate underscore placement for decimal/floating-point/scientific notation
    if (num.find("_") != string::npos) {
        if (!validateUnderscores()) {
            addError("Invalid underscore placement in number: " + num);
            return;
        }
        // Additional check: underscores cannot be adjacent to decimal point or 'e'/'E'
        if (hasDecimal && (num.find("._") != string::npos || num.find("_.") != string::npos)) {
            addError("Invalid underscore placement in number: " + num + " (underscore adjacent to decimal point)");
            return;
        }
        if (hasExponent) {
            size_t ePos = num.find('e') != string::npos ? num.find('e') : num.find('E');
            if (ePos > 0 && num[ePos - 1] == '_') {
                addError("Invalid underscore placement in number: " + num + " (underscore before 'e'/'E')");
                return;
            }
            if (ePos + 1 < num.size() && num[ePos + 1] == '_') {
                addError("Invalid underscore placement in number: " + num + " (underscore after 'e'/'E')");
                return;
            }
            // Check after the sign in the exponent (e.g., "1e-_10")
            if (ePos + 2 < num.size() && (num[ePos + 1] == '+' || num[ePos + 1] == '-') && num[ePos + 2] == '_') {
                addError("Invalid underscore placement in number: " + num + " (underscore after exponent sign)");
                return;
            }
        }
    }

    // Check for complex number (ends with 'j' or 'J')
    if (current() == 'j' || current() == 'J') {
        num += current();
        advance();

        // Always treat complex numbers as invalid
        addError("Invalid token: " + num + " (complex numbers are not supported)");

        // Optionally consume any trailing alphanumeric or underscore characters
        while (isalnum(current()) || current() == '_') {
            num += current();
            advance();
        }

        return;
    }

    // Check for invalid trailing characters (e.g., "123abc")
    if (isalpha(current()) || current() == '_') {
        string invalid = num;
        while (isalnum(current()) || current() == '_') {
            invalid += current();
            advance();
        }
        addError("Invalid number: " + invalid + " (invalid trailing characters)");
        return;
    }

    if (hasExponent || hasDecimal) {
        addToken(num, TokenType::NUMBER);  // Floating-point or scientific notation
    } else if (num.size() > 1 && num[0] == '0' && (num[1] == 'o' || num[1] == 'O')) {
        addToken(num, TokenType::OCTALNUMBER);  // Single '0o' can be treated as octal
    } else {
        addToken(num, TokenType::NUMBER);  // Decimal integer
    }
}
void PythonLexer::processString(char quote) {
    std::string str;
    int startLine = line;
    int startColumn = column;
    advance();

    bool isTriple = false;
    if (current() == quote && peek() == quote) {
        isTriple = true;
        advance();
        advance();
    }

    if (current() == '\\') {
        advance();
        if (current() != 'n' && current() != 't' && current() != '\\' && current() != '"' && current() != '\'') {
            addError("Invalid escape sequence: \\" + std::string(1, current()));
        }
    }

    if (isTriple) {
        while (true) {
            if (current() == '\0') {
                addError("Unterminated triple-quoted string starting at line " +
                         std::to_string(startLine) + " column " + std::to_string(startColumn));
                return;
            }

            if (current() == quote && peek() == quote && (pos + 2 < source.size() && source[pos + 2] == quote)) {
                advance();
                advance();
                advance();
                break;
            }
            str += current();
            advance();
        }
        addToken(str, TokenType::STRING);
    } else {
        while (current() != quote && current() != '\0') {
            if (current() == '\n') {
                addError("Unterminated string literal starting at line " +
                         std::to_string(startLine) + " column " + std::to_string(startColumn));
                while (current() != '\n' && current() != '\0') {
                    advance();
                }
                return;
            }
            if (current() == '\\') {
                advance();
                if (current() == '\0') break;
            }
            str += current();
            advance();
        }

        if (current() != quote) {
            addError("Unterminated string literal starting at line " +
                     std::to_string(startLine) + " column " + std::to_string(startColumn));
            return;
        }
        advance();
        addToken(str, TokenType::STRING);
    }
}

void PythonLexer::processIdentifier() {
    std::string ident;

    // This check is redundant since tokenize() now handles invalid prefixes,
    // but we'll keep it as a safety net
    if (std::isdigit(current())) {
        std::string invalid;
        while (std::isalnum(current()) || current() == '_') {
            invalid += current();
            advance();
        }
        addError("Invalid identifier starts with digit: " + invalid);
        return;
    }

    if (current() == '_') {
        ident += current();
        advance();
        if (std::isdigit(current())) {
            std::string invalid = ident;
            invalid += current();
            advance();
            while (std::isalnum(current()) || current() == '_') {
                invalid += current();
                advance();
            }
            addError("Invalid identifier starts with underscore followed by digit: " + invalid);
            return;
        }
    }

    while (std::isalnum(current()) || current() == '_') {
        ident += current();
        advance();
    }

    std::string lowerIdent = toLower(ident);

    // Check if the identifier is a keyword
    std::string keywordMatch;
    for (const auto& keyword : keywords) {
        std::string lowerKeyword = toLower(keyword);
        if (lowerIdent == lowerKeyword) {
            keywordMatch = keyword;
            break;
        }
    }

    // Check if the identifier is a built-in function
    bool isBuiltinFunction = false;
    std::string builtinMatch;
    for (const auto& builtin : builtinFunctions) {
        std::string lowerBuiltin = toLower(builtin);
        if (lowerIdent == lowerBuiltin) {
            isBuiltinFunction = true;
            builtinMatch = builtin;
            break;
        }
    }

    if (!keywordMatch.empty()) {
        addToken(ident, TokenType::KEYWORD);
    } else if (isBuiltinFunction) {
        addToken(ident, TokenType::IDENTIFIER);
        // Add to symbol table if not already added
        if (addedBuiltins.find(ident) == addedBuiltins.end()) {
            int id = symbolTable.addIdentifier(ident, line);
            symbolTable.setIdentifierInfo(ident, "function", "built-in");
            addedBuiltins.insert(ident); // Mark as added
        }
    } else {
        size_t tempPos = pos;
        int tempColumn = column;
        while (std::isspace(current()) && current() != '\n') {
            advance();
        }
        isFunctionCall = (current() == '(');
        pos = tempPos;
        column = tempColumn;

        if (!isFunctionCall) {
            symbolTable.addIdentifier(ident, line);
        }
        addToken(ident, TokenType::IDENTIFIER);
    }
}

void PythonLexer::processComment() {
    std::string com;
    if (current() == '#') {
        advance();

        while (current() != '\n' && current() != '\0') {
            com += current();
            advance();
        }
        addToken(com, TokenType::COMMENT);
    }

    if (current() == '"' || current() == '\'') {
        char quote = current();
        bool isTriple = (peek() == quote && pos + 1 < source.size() && source[pos + 1] == quote);

        if (isTriple) {
            advance();
            advance();
            advance();

            std::string docstring;
            while (true) {
                if (current() == '\0') {
                    addError("Unterminated multi-line comment (docstring)");
                    return;
                }

                if (current() == quote && peek() == quote && (pos + 2 < source.size() && source[pos + 2] == quote)) {
                    advance();
                    advance();
                    advance();
                    break;
                }
                docstring += current();
                advance();
            }
            addToken(docstring, TokenType::COMMENT);
        }
    }
}
void PythonLexer::handleIndentation() {
    static std::vector<int> indentStack = {0}; // Track indentation levels
    int currentIndent = 0;

    // Count spaces or tabs for indentation
    while (pos < source.size() && (source[pos] == ' ' || source[pos] == '\t')) {
        currentIndent += (source[pos] == '\t' ? 8 : 1); // Treat tab as 8 spaces
        advance();
    }

    // Handle increasing indent
    if (currentIndent > indentStack.back()) {
        indentStack.push_back(currentIndent);
        addToken("", TokenType::INDENT);  // Add INDENT token
    }
    // Handle decreasing indent
    else if (currentIndent < indentStack.back()) {
        while (!indentStack.empty() && currentIndent < indentStack.back()) {
            indentStack.pop_back();  // Pop the stack when indentation decreases
            addToken("", TokenType::DEDENT);  // Add DEDENT token
        }

        // Check for mismatch in indentation levels
        if (currentIndent != indentStack.back()) {
            addError("Inconsistent indentation level");
        }
    }
}

void PythonLexer::processOperator() {
    std::string op(1, current());
    const char next = peek();

    // Check for := specifically and flag it as invalid for assignments
    if (current() == ':' && next == '=') {
        op = ":=";
        addError("Invalid assignment operator: " + op + " (only '=' is allowed for variable assignments)");
        advance();
        advance();
        return;
    }

    if (current() == '=') {
        if (next == '=') {
            op = "==";
            addToken(op, TokenType::COMPAREOPERATOR);
            advance();
            advance();
            return;
        } else {
            addToken("=", TokenType::EQUALOPERATOR);
            advance();
            return;
        }
    }
    if (current() == '!' ) {
        if(next == '='){
            op = "!=";
            addToken(op, TokenType::COMPAREOPERATOR);
            advance();
            advance();
            return;
        }else{
            addToken("!",TokenType::NOTASSIGN);
            advance();
            return;
        }
    }
    if (current() == '<') {
        if (next == '=') {
            op = "<=";
            addToken(op, TokenType::COMPAREOPERATOR);
            advance();
            advance();
            return;
        } else if (next == '<') {
            op = "<<";
            addToken(op, TokenType::OPERATOR);
            advance();
            advance();
            return;
        } else {
            addToken("<", TokenType::COMPAREOPERATOR);
            advance();
            return;
        }
    }
    if (current() == '>') {
        if (next == '=') {
            op = ">=";
            addToken(op, TokenType::COMPAREOPERATOR);
            advance();
            advance();
            return;
        } else if (next == '>') {
            op = ">>";
            addToken(op, TokenType::OPERATOR);
            advance();
            advance();
            return;
        } else {
            addToken(">", TokenType::COMPAREOPERATOR);
            advance();
            return;
        }
    }

    if (current() == '-') {
        if (next == '=') {
            op = "-=";
            addToken(op, TokenType::SUB_ASSIGN);
            advance();
            advance();
            return;
        } else {
            addToken("-", TokenType::MINUSOPERATOR);
            advance();
            return;
        }
    }
    // Handle += and -= operators
    if (current() == '+') {
        if (next == '=') {
            op = "+=";
            addToken(op, TokenType::ADD_ASSIGN);
            advance();
            advance();
            return;
        } else {
            addToken("+", TokenType::ADDOPERATOR);
            advance();
            return;
        }
    }

    if (current() == '*') {
        if (next == '=') {
            op = "*=";
            addToken(op,TokenType::MULTIPLYASSIGN);
            advance();
            advance();
            return;
        } else if (next == '*') {
            op = "**";
            addToken(op, TokenType::POWEROPERATOR);
            advance();
            advance();
            return;
        } else {
            addToken("*", TokenType::MULTIPLYOPERATOR);
            advance();
            return;
        }
    }
    if (current() == '/') {
        if (next == '=') {
            op = "/=";
            addError("Invalid assignment operator: " + op + " (only '=' is allowed for variable assignments)");
            advance();
            advance();
            return;
        } else {
            addToken("/", TokenType::DIVIDEOPERATOR);
            advance();
            return;
        }
    }
    if (current() == '%') {
        if (next == '=') {
            op = "%=";
            addError("Invalid assignment operator: " + op + " (only '=' is allowed for variable assignments)");
            advance();
            advance();
            return;
        } else {
            addToken("%", TokenType::PERCENTAGEOPERATOR);
            advance();
            return;
        }
    }
    if (current() == '&') {
        addToken("&", TokenType::BITANDOPERATOR);
        advance();
        return;
    }
    if (current() == '|') {
        addToken("|", TokenType::BITOROPERATOR);
        advance();
        return;
    }
    if (current() == '^') {
        addToken("^", TokenType::POWEROPERATOR);
        advance();
        return;
    }
    if (current() == '.') {
        addToken(".", TokenType::OPERATOR);
        advance();
        return;
    }

    addError("Unexpected operator: " + std::string(1, current()));
    advance();
}

bool PythonLexer::processTypeAnnotation() {
    size_t startPos = pos;
    int startColumn = column;
    std::string typeName;

    if (std::isalpha(current()) || current() == '_') {
        while (std::isalnum(current()) || current() == '_') {
            typeName += current();
            advance();
        }

        while (std::isspace(current()) && current() != '\n') {
            advance();
        }

        std::string ident;
        if (std::isalpha(current()) || current() == '_') {
            while (std::isalnum(current()) || current() == '_') {
                ident += current();
                advance();
            }

            std::string lowerTypeName = toLower(typeName);
            if (typeHints.count(lowerTypeName)) {
                typeAnnotations[ident] = lowerTypeName;
                addToken(ident, TokenType::IDENTIFIER);
                symbolTable.addIdentifier(ident, line);
                return true;
            }
        }
    }

    pos = startPos;
    column = startColumn;
    return false;
}


// Convert infix tokens to Reverse Polish Notation (RPN) using the Shunting-Yard algorithm
std::vector<Token> PythonLexer::toRPN(const std::vector<Token>& input) {
    std::vector<Token> output;
    std::stack<Token> ops;

    auto precedence = [&](const Token& t) {
        switch (t.type) {
        case TokenType::POWEROPERATOR:      return 4;
        case TokenType::MULTIPLYOPERATOR:
        case TokenType::DIVIDEOPERATOR:
        case TokenType::PERCENTAGEOPERATOR: return 3;
        case TokenType::ADDOPERATOR:
        case TokenType::MINUSOPERATOR:      return 2;
        default:                            return 0;
        }
    };
    auto isLeftAssoc = [&](const Token& t) {
        return t.type != TokenType::POWEROPERATOR;
    };

    for (size_t i = 0; i < input.size(); ++i) {
        const Token& t = input[i];

        // <<<— updated operand check:
        if (t.type == TokenType::NUMBER        ||
            t.type == TokenType::HexadecimalNumber ||
            t.type == TokenType::BinaryNumber  ||
            t.type == TokenType::OCTALNUMBER   ||
            t.type == TokenType::IDENTIFIER    ||
            t.type == TokenType::KEYWORD
            )
        {
            output.push_back(t);
        }
        else if (t.type == TokenType::ADDOPERATOR   ||
                 t.type == TokenType::MINUSOPERATOR ||
                 t.type == TokenType::MULTIPLYOPERATOR ||
                 t.type == TokenType::DIVIDEOPERATOR   ||
                 t.type == TokenType::PERCENTAGEOPERATOR||
                 t.type == TokenType::POWEROPERATOR) {
            // (same unary minus + precedence logic as before)
            Token op = t;
            bool unary = (t.type == TokenType::MINUSOPERATOR) &&
                         (i == 0 || input[i-1].lexeme == "(" ||
                          input[i-1].type == TokenType::ADDOPERATOR ||
                          input[i-1].type == TokenType::MINUSOPERATOR ||
                          input[i-1].type == TokenType::MULTIPLYOPERATOR ||
                          input[i-1].type == TokenType::DIVIDEOPERATOR ||
                          input[i-1].type == TokenType::PERCENTAGEOPERATOR ||
                          input[i-1].type == TokenType::POWEROPERATOR);
            if (unary) op.type = TokenType::MINUSOPERATOR;
            while (!ops.empty()) {
                const Token& top = ops.top();
                if ((isLeftAssoc(op) && precedence(op) <= precedence(top)) ||
                    (!isLeftAssoc(op) && precedence(op) <  precedence(top))) {
                    output.push_back(top);
                    ops.pop();
                } else break;
            }
            ops.push(op);
        }
        else if (t.lexeme == "(") {
            ops.push(t);
        }
        else if (t.lexeme == ")") {
            while (!ops.empty() && ops.top().lexeme != "(") {
                output.push_back(ops.top());
                ops.pop();
            }
            if (ops.empty()) throw std::runtime_error("Mismatched parentheses");
            ops.pop();
        }
        // ignore other token types
    }

    while (!ops.empty()) {
        if (ops.top().lexeme == "(" || ops.top().lexeme == ")")
            throw std::runtime_error("Mismatched parentheses");
        output.push_back(ops.top());
        ops.pop();
    }
    return output;
}
// Evaluate an RPN token sequence; looks up identifiers in symbolTable
double PythonLexer::evalRPN(const std::vector<Token>& rpn) {
    std::stack<double> st;
    for (const Token& t : rpn) {
        if (t.type == TokenType::NUMBER) {
            st.push(std::stod(t.lexeme));
        }
        else if (t.type == TokenType::HexadecimalNumber) {
            st.push(static_cast<double>(std::stoll(t.lexeme, nullptr, 16)));
        }
        else if (t.type == TokenType::BinaryNumber) {
            st.push(static_cast<double>(std::stoll(t.lexeme.substr(2), nullptr, 2)));
        }
        else if (t.type == TokenType::OCTALNUMBER) {
            st.push(static_cast<double>(std::stoll(t.lexeme.substr(2), nullptr, 8)));
        }
        else if (t.type == TokenType::IDENTIFIER) {
            if (!symbolTable.getSymbols().count(t.lexeme))
                throw std::runtime_error("Undefined identifier: " + t.lexeme);
            
            // Get the value and type from symbol table
            auto value = symbolTable.getValue(t.lexeme);
            auto type = symbolTable.getDataType(t.lexeme);
            
            // Check if the variable is uninitialized or unknown
            if (type == "unknown" || value == "N/A") {
                throw std::runtime_error("Cannot perform operation with uninitialized variable: " + t.lexeme);
            }
            
            // Only try to convert to number if it's a numeric type
            if (type != "int" && type != "float") {
                throw std::runtime_error("Cannot perform numeric operation with " + type + " variable: " + t.lexeme);
            }
            
            try {
                st.push(std::stod(value));
            } catch (const std::exception& e) {
                throw std::runtime_error("Invalid numeric value for variable " + t.lexeme + ": " + value);
            }
        }
        else if (t.type == TokenType::KEYWORD) {
            std::string val = toLower(t.lexeme);
            if (val == "true") st.push(1.0);
            else if (val == "false") st.push(0.0);
            else throw std::runtime_error("Unexpected keyword in expression: " + t.lexeme);
        }
        else {
            if (t.type == TokenType::MINUSOPERATOR && st.size() == 1) {
                double a = st.top(); st.pop();
                st.push(-a);
            } else {
                if (st.size() < 2) throw std::runtime_error("Invalid expression");
                double b = st.top(); st.pop();
                double a = st.top(); st.pop();
                double res;
                switch (t.type) {
                case TokenType::ADDOPERATOR:       res = a + b; break;
                case TokenType::MINUSOPERATOR:     res = a - b; break;
                case TokenType::MULTIPLYOPERATOR:  res = a * b; break;
                case TokenType::DIVIDEOPERATOR:
                    if (b == 0) throw std::runtime_error("Division by zero");
                    res = a / b; break;
                case TokenType::PERCENTAGEOPERATOR:
                    if (b == 0) throw std::runtime_error("Modulo by zero");
                    res = std::fmod(a, b); break;
                case TokenType::POWEROPERATOR:     res = std::pow(a, b); break;
                default:  throw std::runtime_error("Unknown operator");
                }
                st.push(res);
            }
        }
    }
    if (st.size() != 1) return 0.0;
    return st.top();
}

// Refactored assignment processing: delegates expression parsing and evaluation
void PythonLexer::processAssignments() {
    size_t i = 0;
    while (i < tokens.size()) {
        // Disallow patterns like -x = …
        if ((tokens[i].type == TokenType::MINUSOPERATOR ||
             tokens[i].type == TokenType::ADDOPERATOR) &&
            i + 2 < tokens.size() &&
            tokens[i+1].type == TokenType::IDENTIFIER &&
            tokens[i+2].type == TokenType::EQUALOPERATOR) {
            addError("Invalid assignment target: cannot assign to an expression like '"
                     + tokens[i].lexeme + tokens[i+1].lexeme + "'");
            i += 3; continue;
        }

        // identifier = …
        if (tokens[i].type == TokenType::IDENTIFIER &&
            i + 1 < tokens.size() &&
            tokens[i+1].type == TokenType::EQUALOPERATOR) {
            std::string lhs   = tokens[i].lexeme;
            int         line  = tokens[i].line;
            std::vector<Token> expr;
            size_t     j = i + 2;

            // collect RHS
            while (j < tokens.size() &&
                   tokens[j].type != TokenType::NEWLINE &&
                   tokens[j].type != TokenType::ENDOFFILE) {
                expr.push_back(tokens[j++]);
            }

            // If *any* lexical error happened between the assignment's start and its end, bail
            int startLine = tokens[i].line;
            // tokens[j] is the NEWLINE or ENDOFFILE after the RHS
            int endLine   = (j < tokens.size() ? tokens[j].line : tokens.back().line);

            bool bad = false;
            for (const auto &err : errors) {
                if (err.line >= startLine && err.line <= endLine) {
                    bad = true;
                    break;
                }
            }
            if (bad) {
                symbolTable.setIdentifierInfo(lhs, "unknown", "N/A");
                i = j;
                continue;
            }

            // now your existing single-token shortcuts…
            if (expr.size() == 1) {
                const Token &t = expr[0];
                switch (t.type) {
                case TokenType::STRING:
                    symbolTable.setIdentifierInfo(lhs, "string", t.lexeme);
                    i = j; continue;
                case TokenType::KEYWORD: {
                    auto v = toLower(t.lexeme);
                    if (v=="true"||v=="false") {
                        symbolTable.setIdentifierInfo(lhs, "bool", v);
                        i = j; continue;
                    }
                    break;
                }
                case TokenType::HexadecimalNumber:
                case TokenType::BinaryNumber:
                case TokenType::OCTALNUMBER:
                    symbolTable.setIdentifierInfo(lhs, "int", t.lexeme);
                    i = j; continue;
                case TokenType::NUMBER: {
                    bool isF = t.lexeme.find('.')!=std::string::npos
                               || t.lexeme.find('e')!=std::string::npos
                               || t.lexeme.find('E')!=std::string::npos;
                    symbolTable.setIdentifierInfo(lhs,
                                                  isF?"float":"int", t.lexeme);
                    i = j; continue;
                }
                case TokenType::IDENTIFIER: {
                    if (symbolTable.getSymbols().count(t.lexeme)) {
                        auto v  = symbolTable.getValue(t.lexeme);
                        auto dt = symbolTable.getDataType(t.lexeme);
                        symbolTable.setIdentifierInfo(lhs, dt, v);
                    } else {
                        addError("Undefined identifier in assignment: " + t.lexeme);
                    }
                    i = j; continue;
                }
                default: break;
                }
            }

            // General RPN case
            try {
                auto   rpn    = toRPN(expr);
                double result = evalRPN(rpn);
                bool   isInt  = (std::floor(result)==result);
                std::string dtype = isInt ? "int" : "float";
                std::string sval  = isInt
                                       ? std::to_string((long long)result)
                                       : std::to_string(result);
                symbolTable.setIdentifierInfo(lhs, dtype, sval);
            } catch (const std::exception& e) {
                addError(e.what());
                // treat as unknown on any evaluation exception
                symbolTable.setIdentifierInfo(lhs, "unknown", "N/A");
            }

            i = j;
        }
        else {
            ++i;
        }
    }
}




void PythonLexer::addError(const std::string& message) {
    errors.push_back({ message, line, column });
}

std::pair<std::vector<Token>, std::vector<LexicalError>> PythonLexer::tokenize() {
    while (current() != '\0') {
        char c = current();
        switch (c) {
        // 1) Newline
        case '\n':
            addToken("\n", TokenType::NEWLINE);
            advance();
            handleIndentation();
            break;

            // 2) Whitespace
        case ' ': case '\r': case '\t': case '\v': case '\f':
            advance();
            break;

            // 3) Comment
        case '#':
            processComment();
            break;

            // 4) Number (decimal)
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            processNumber();
            break;

            // 5) String literal
        case '"': case '\'':
            processString(c);
            break;

            // 6) Valid identifier start
        default:
            if (std::isalpha(c) || c == '_') {
                if (!processTypeAnnotation()) {
                    processIdentifier();
                }
                break;
            }

            // 7) Invalid identifier start like @, $, etc.
            else if (c == '@' || c == '$' ||  c == '`' || c == '\\') {
                int errLine = line;
                int errColumn = column;
                std::string bad = { c };
                advance();
                while (std::isalnum(current()) || current() == '_') {
                    bad += current();
                    advance();
                }
                addError("Invalid identifier at line " + std::to_string(errLine) +
                         " column " + std::to_string(errColumn) +
                         ": '" + bad + "' (identifiers must start with a letter or underscore)");
                break;
            }

            // 8) Operators
            else if (isOperatorChar(c)) {
                processOperator();
                break;
            }

            // 9) Delimiters
            else if (isDelimiter(c)) {
                addToken(std::string(1, c), TokenType::DELIMITER);
                advance();
                break;
            }

            // 10) Catch-all: unknown/unexpected characters
            else {
                int errLine = line;
                int errColumn = column;
                std::string bad = { c };
                advance();
                while (std::isalnum(current()) || current() == '_') {
                    bad += current();
                    advance();
                }
                addError("Invalid character sequence at line " + std::to_string(errLine) +
                         " column " + std::to_string(errColumn) +
                         ": '" + bad + "' (unknown or unsupported characters)");
                break;
            }
        }
    }

    addToken("", TokenType::ENDOFFILE);
    processAssignments();
    return { tokens, errors };
}


std::string tokenTypeToString(TokenType type) {
    switch (type) {
    case TokenType::KEYWORD:            return "KEYWORD";
    case TokenType::IDENTIFIER:         return "IDENTIFIER";
    case TokenType::HexadecimalNumber:  return "HEXADDECIMAL_NUMBER";
    case TokenType::BinaryNumber:       return "BINARY_NUMBER";
    case TokenType::OCTALNUMBER:        return "OCTAL_NUMBER";
    case TokenType::NUMBER:             return "NUMBER";
    case TokenType::COMPLEX_NUMBER:     return "COMPLEX_NUMBER";
    case TokenType::STRING:             return "STRING";
    case TokenType::OPERATOR:           return "OPERATOR";
    case TokenType::ADDOPERATOR:        return "ADD_OPERATOR";
    case TokenType::MINUSOPERATOR:      return "MINUS_OPERATOR";
    case TokenType::MULTIPLYOPERATOR:   return "MULTIPLY_OPERATOR";
    case TokenType::DELIMITER:          return "DELIMITER";
    case TokenType::EQUALOPERATOR:      return "EQUAL_OPERATOR";
    case TokenType::BITOROPERATOR:      return "BITWISE_OR_OPERATOR";
    case TokenType::BITANDOPERATOR:     return "BITWISE_AND_OPERATOR";
    case TokenType::PERCENTAGEOPERATOR: return "PERCENTAGE_OPERATOR";
    case TokenType::COMPAREOPERATOR:    return "COMPARE_OPERATOR";
    case TokenType::DIVIDEOPERATOR:     return "DIVIDE_OPERATOR";
    case TokenType::POWEROPERATOR:      return "POWER_OPERATOR";
    case TokenType::INDENT:             return "INDENT";
    case TokenType::DEDENT:             return "DEDENT";
    case TokenType::NEWLINE:            return "NEWLINE";
    case TokenType::COMMENT:            return "COMMENT";
    case TokenType::ENDOFFILE:          return "ENDOFFILE";
    case TokenType::ADD_ASSIGN:          return "Plusequal";
    case TokenType::SUB_ASSIGN:          return "minusequal";
    default:                            return "UNKNOWN";
    }
}
