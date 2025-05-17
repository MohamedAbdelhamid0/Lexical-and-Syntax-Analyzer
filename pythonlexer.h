#ifndef PYTHONLEXER_H
#define PYTHONLEXER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>

enum class TokenType {
    KEYWORD, IDENTIFIER, HexadecimalNumber, BinaryNumber, OCTALNUMBER, NUMBER, COMPLEX_NUMBER, STRING, OPERATOR,
    ADDOPERATOR, MINUSOPERATOR, MULTIPLYOPERATOR, DELIMITER, EQUALOPERATOR, BITOROPERATOR, BITANDOPERATOR,MULTIPLYASSIGN,
    PERCENTAGEOPERATOR, WHITESPACE,COMPAREOPERATOR, DIVIDEOPERATOR, POWEROPERATOR, INDENT, DEDENT, NEWLINE, COMMENT, ENDOFFILE,
    SUB_ASSIGN,ADD_ASSIGN,NOTASSIGN,
};

struct Token {
    std::string lexeme;
    TokenType type;
    int line;
    int column;
};

struct LexicalError {
    std::string message;
    int line;
    int column;
};

class SymbolTable {
private:
    struct SymbolEntry {
        int id;
        std::string dataType;
        std::string value;
    };

    std::unordered_map<std::string, SymbolEntry> symbols;
    int current_id = 1;

public:
    int addIdentifier(const std::string& identifier, int line) {
        if (!symbols.count(identifier)) {
            symbols[identifier] = { current_id++, "unknown", "N/A" };
        }
        return symbols[identifier].id;
    }

    void setIdentifierInfo(const std::string& identifier, const std::string& dataType, const std::string& value) {
        if (symbols.count(identifier)) {
            symbols[identifier].dataType = dataType;
            symbols[identifier].value = value;
        }
    }

    std::optional<int> lookup(const std::string& identifier) const {
        auto it = symbols.find(identifier);
        return it != symbols.end() ? std::optional<int>(it->second.id) : std::nullopt;
    }

    const std::unordered_map<std::string, SymbolEntry>& getSymbols() const {
        return symbols;
    }

    int getId(const std::string& identifier) const {
        auto it = symbols.find(identifier);
        return it != symbols.end() ? it->second.id : -1;
    }

    std::string getDataType(const std::string& identifier) const {
        auto it = symbols.find(identifier);
        return it != symbols.end() ? it->second.dataType : "unknown";
    }

    std::string getValue(const std::string& identifier) const {
        auto it = symbols.find(identifier);
        return it != symbols.end() ? it->second.value : "N/A";
    }
};

class PythonLexer {
private:
    std::string source;
    size_t pos = 0;
    int line = 1;
    int column = 1;
    SymbolTable symbolTable;
    std::vector<Token> tokens;
    std::vector<LexicalError> errors;
    std::unordered_map<std::string, std::string> typeAnnotations;
    bool isFunctionCall = false;
    std::unordered_set<std::string> addedBuiltins; // Track added built-in functions
    std::vector<int> indentStack;  // Stack of indentation levels
    int currentIndent = 0;         // Current indentation level
    bool atStartOfLine = true;     // Flag for start of line

    const std::unordered_set<std::string> keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return",
        "try", "while", "with", "yield"
    };

    const std::unordered_set<std::string> builtinFunctions = {
        "print"
    };

    const std::unordered_set<std::string> typeHints = {
        "int", "float", "str", "bool", "complex"
    };

    char current() const { return pos < source.size() ? source[pos] : '\0'; }
    char peek() const { return pos + 1 < source.size() ? source[pos + 1] : '\0'; }

    void advance();
    void addToken(const std::string& lexeme, TokenType type);
    void addError(const std::string& message);
    bool isOperatorChar(char c);
    bool isDelimiter(char c);
    bool isHexadecimal(const std::string& str);
    bool isTab(char c);
    void processNumber();
    void processString(char quote);
    void processIdentifier();
    void processComment();
    void processOperator();
    double evalRPN(const std::vector<Token>& rpn);
    std::vector<Token> toRPN(const std::vector<Token>& input);
    void processAssignments();
    bool processTypeAnnotation();
    void handleIndentation();
public:
    PythonLexer(const std::string& input);
    std::pair<std::vector<Token>, std::vector<LexicalError>> tokenize();
    const SymbolTable& getSymbolTable() const { return symbolTable; }
};

std::string tokenTypeToString(TokenType type);

#endif // PYTHONLEXER_H
