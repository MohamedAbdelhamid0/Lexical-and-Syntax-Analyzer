// syntaxanalyzer.h
#ifndef SYNTAXANALYZER_H
#define SYNTAXANALYZER_H

#include <vector>
#include <string>
#include <QString>
#include <QTreeWidget>
#include "pythonlexer.h"

// Parse tree node
struct ParseNode {
    QString name;            // Node type or token
    QString value;          // Optional token value
    QVector<ParseNode*> children;
    ParseNode(const QString& n, const QString& v = "")
        : name(n), value(v) {}
};

// Syntax error struct
struct SyntaxError {
    std::string message;
    int line;
    int column;
};

// LL(1) Syntax Analyzer for Python subset
class SyntaxAnalyzer {
public:
    SyntaxAnalyzer(const std::vector<Token>& tokens)
        : tokens(tokens), pos(0) {}
    ~SyntaxAnalyzer();

    // Build parse tree; returns root node (or nullptr on top-level failure)
    ParseNode* parseProgram();

    // Populate a QTreeWidget with the parse tree
    void populateTree(QTreeWidget* tree);

    // Retrieve collected syntax errors
    const std::vector<SyntaxError>& getErrors() const { return syntaxErrors; }

private:
    const std::vector<Token>& tokens;
    size_t pos;
    std::vector<SyntaxError> syntaxErrors;

    // Helper methods
    bool checkIndentation(const std::string& stmtType);
    bool match(const std::string& lexeme);
    bool isAtEnd() const;
    const Token& currentToken() const;
    void advance();
    void addSyntaxError(const std::string& msg, int line, int column);

    // Build QTree recursively
    void buildTree(QTreeWidgetItem* parent, ParseNode* node);

    // Parsing methods for grammar rules
    ParseNode* parseStmt();
    ParseNode* parseIfChain(ParseNode* node);
    ParseNode* parseIfCore();
    ParseNode* parseForStmt();
    ParseNode* parseWhileStmt();
    ParseNode* parseFuncDef();
    ParseNode* parseAssignment();
    ParseNode* parseExprStmt();
    ParseNode* parseExpression();
    ParseNode* parseTerm();
    ParseNode* parseFactor();
    ParseNode* parseParamList();
    ParseNode* parseComparison();
    ParseNode* parseReturnStmt();
    ParseNode* parsePassStmt();
    ParseNode* parseBreakStmt();
    ParseNode* parseContinueStmt();
};

#endif // SYNTAXANALYZER_H
