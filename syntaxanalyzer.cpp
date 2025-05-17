// syntaxanalyzer.cpp
#include "syntaxanalyzer.h"
#include <iostream>
using namespace std;

SyntaxAnalyzer::~SyntaxAnalyzer() {
    // Ideally delete all nodes; omitted for brevity
}

ParseNode* SyntaxAnalyzer::parseProgram() {
    ParseNode* root = new ParseNode("Program");

    // Debug: Print token stream
    std::cout << "Token stream:" << std::endl;
    for (size_t i = 0; i < tokens.size(); i++) {
        std::cout << "Token " << i << ": type=" << tokenTypeToString(tokens[i].type)
        << ", lexeme='" << tokens[i].lexeme
        << "', line=" << tokens[i].line
        << ", col=" << tokens[i].column << std::endl;
    }
    std::cout << std::endl;

    while (!isAtEnd()) {
        // Skip blank lines
        if (currentToken().type == TokenType::NEWLINE) {
            advance();
            continue;
        }

        size_t startPos = pos;
        ParseNode* stmt = parseStmt();

        // Only push if a valid node was returned
        if (stmt) {
            root->children.push_back(stmt);
        }

        // ——— SAFETY: always ensure progress ———
        if (pos == startPos) {
            advance(); // move forward to escape the loop
        }
    }

    return root;
}

void SyntaxAnalyzer::populateTree(QTreeWidget* tree) {
    tree->clear();
    ParseNode* root = parseProgram();
    // Display parse tree
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(tree);
    rootItem->setText(0, root->name);
    buildTree(rootItem, root);
    tree->addTopLevelItem(rootItem);
    tree->expandAll();
}

ParseNode* SyntaxAnalyzer::parseComparison() {
    // Start by parsing a simple arithmetic expression
    auto left = parseExpression();
    if (!left) return nullptr;

    // Loop to handle comparison operators chained: ==, !=, <, <=, >, >=
    while (true) {
        std::string op;
        if (match("==")) op = "==";
        else if (match("!=")) op = "!=";
        else if (match("<=")) op = "<=";
        else if (match(">=")) op = ">=";
        else if (match("<"))  op = "<";
        else if (match(">"))  op = ">";
        else break;

        auto right = parseExpression();
        if (!right) return nullptr;

        // Build a comparison operator node
        auto cmpNode = new ParseNode("CompareOp", QString::fromStdString(op));
        cmpNode->children.push_back(left);
        cmpNode->children.push_back(right);

        left = cmpNode; // allow chaining comparisons
    }

    return left;
}

ParseNode* SyntaxAnalyzer::parseReturnStmt() {
    auto node = new ParseNode("ReturnStmt");
    // we know 'return' was just matched
    if (currentToken().type != TokenType::NEWLINE && currentToken().lexeme != ":") {
        auto expr = parseExpression();
        if (expr) {
            node->children.push_back(expr);
        } else {
            addSyntaxError("Invalid expression in return",
                           currentToken().line, currentToken().column);
        }
    }
    return node;
}

//——— Pass statement ———
// 'pass'
ParseNode* SyntaxAnalyzer::parsePassStmt() {
    auto node = new ParseNode("PassStmt");
    return node;
}

//——— Break statement ———
// 'break'
ParseNode* SyntaxAnalyzer::parseBreakStmt() {
    auto node = new ParseNode("BreakStmt");
    return node;
}

//——— Continue statement ———
// 'break'
ParseNode* SyntaxAnalyzer::parseContinueStmt() {
    auto node = new ParseNode("ContinueStmt");
    return node;
}

//——— Statement dispatch ———

ParseNode* SyntaxAnalyzer::parseStmt() {
    std::cout << "parseStmt: current token type=" << tokenTypeToString(currentToken().type)
    << ", lexeme='" << currentToken().lexeme
    << "', line=" << currentToken().line
    << ", col=" << currentToken().column << std::endl;

    // Handle DEDENT - it's not a statement, just return nullptr to end the block
    if (currentToken().type == TokenType::DEDENT) {
        std::cout << "  Found DEDENT - ending block" << std::endl;
        advance(); // consume the DEDENT
        return nullptr;
    }

    // 1) Skip blank lines, comments, and handle indentation
    while (!isAtEnd() && (currentToken().type == TokenType::NEWLINE || 
                         currentToken().type == TokenType::COMMENT)) {
        advance();
        std::cout << "  Skipped newline or comment" << std::endl;
    }

    // Handle whitespace/indentation without consuming statement tokens
    while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                          currentToken().type == TokenType::INDENT ||
                          currentToken().type == TokenType::DEDENT)) {
        std::cout << "  Skipped " << tokenTypeToString(currentToken().type) << std::endl;
        advance();
    }

    // Skip any inline comments before processing the statement
    while (!isAtEnd() && currentToken().type == TokenType::COMMENT) {
        advance();
        std::cout << "  Skipped inline comment" << std::endl;
    }

    // 2) Skip stray colons
    while (!isAtEnd() && currentToken().lexeme == ":") {
        std::cout << "  Skipped colon" << std::endl;
        advance();
    }

    // 3) Statement heads
    if (match("if")) {
        std::cout << "  Parsing if statement" << std::endl;
        auto core = parseIfCore();
        return parseIfChain(core);
    }
    if (match("for"))   return parseForStmt();
    if (match("while")) return parseWhileStmt();
    if (match("def"))   return parseFuncDef();
    if (match("return")) return parseReturnStmt();
    if (match("pass"))   return parsePassStmt();
    if (match("else")) {
        addSyntaxError("'else' without matching 'if'",
                       currentToken().line, currentToken().column);
        return nullptr;
    }
    if (match("elif")) {
        addSyntaxError("'elif' without matching 'if'",
                       currentToken().line, currentToken().column);
        return nullptr;
    }
    if (match("break"))  return parseBreakStmt();
    if (match("continue")) return parseContinueStmt();

    // 4) Built-in functions like print
    if (currentToken().type == TokenType::IDENTIFIER) {
        std::string funcName = currentToken().lexeme;
        std::cout << "  Found identifier: " << funcName << std::endl;

        // Check if it's a built-in function
        if (funcName == "print" || funcName == "len" || funcName == "input") {
            auto node = new ParseNode("FuncCall", QString::fromStdString(funcName));
            advance(); // consume function name

            // For print statements, parentheses are optional
            if (funcName == "print") {
                std::cout << "  Handling print statement" << std::endl;

                // Skip any whitespace after print
                while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
                    advance();
                }

                // If there are parentheses, parse them
                if (match("(")) {
                    std::cout << "  Found opening parenthesis" << std::endl;
                    // Parse arguments
                    if (!isAtEnd() && currentToken().lexeme != ")") {
                        do {
                            // Skip any whitespace before argument
                            while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
                                advance();
                            }

                            auto arg = parseExpression();
                            if (!arg) {
                                delete node;
                                return nullptr;
                            }
                            node->children.push_back(arg);

                            // Skip any whitespace after argument
                            while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
                                advance();
                            }
                        } while (match(","));
                    }

                    if (!match(")")) {
                        addSyntaxError("Expected ')' after arguments",
                                       currentToken().line, currentToken().column);
                        delete node;
                        return nullptr;
                    }
                } else {
                    std::cout << "  No parentheses, parsing single argument" << std::endl;
                    // No parentheses - parse a single argument
                    // Skip any whitespace before the argument
                    while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
                        advance();
                    }

                    if (currentToken().type == TokenType::NEWLINE ||
                        currentToken().type == TokenType::ENDOFFILE) {
                        addSyntaxError("Expected an argument after print",
                                       currentToken().line, currentToken().column);
                        delete node;
                        return nullptr;
                    }

                    auto arg = parseExpression();
                    if (arg) {
                        node->children.push_back(arg);
                    } else {
                        delete node;
                        return nullptr;
                    }
                }
                return node;
            }

            // For other built-in functions, require parentheses
            if (!match("(")) {
                addSyntaxError("Expected '(' after '" + funcName + "'",
                               currentToken().line, currentToken().column);
                return nullptr;
            }

            // Parse arguments
            if (!isAtEnd() && currentToken().lexeme != ")") {
                do {
                    // Skip any whitespace before argument
                    while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
                        advance();
                    }

                    auto arg = parseExpression();
                    if (!arg) {
                        delete node;
                        return nullptr;
                    }
                    node->children.push_back(arg);

                    // Skip any whitespace after argument
                    while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
                        advance();
                    }
                } while (match(","));
            }

            if (!match(")")) {
                addSyntaxError("Expected ')' after arguments",
                               currentToken().line, currentToken().column);
                delete node;
                return nullptr;
            }

            return node;
        }

        // 5) Assignment: x = ..., x += ..., etc.
        if (pos + 1 < tokens.size() &&
            (tokens[pos + 1].lexeme == "=" ||
             tokens[pos + 1].lexeme == "+=" ||
             tokens[pos + 1].lexeme == "-=" ||
             tokens[pos + 1].lexeme == "*=" ||
             tokens[pos + 1].lexeme == "/=")) {
            return parseAssignment();
        }

        // 6) Function call or identifier expression
        auto exprStmt = parseExprStmt();
        if (!exprStmt) {
            addSyntaxError("Invalid expression or unknown statement",
                           currentToken().line, currentToken().column);
            return nullptr;
        }
        return exprStmt;
    }

    // 7) Fallback: expression statement
    auto exprStmt = parseExprStmt();
    if (!exprStmt) {
        addSyntaxError("Invalid expression or unknown statement",
                       currentToken().line, currentToken().column);
        return nullptr;
    }

    return exprStmt;
}


//——— If statement ———

// Given an initial IfStmt *node*, attaches any number of
//   'elif' Comparison ':' Stmt
// followed optionally by one
//   'else' ':' Stmt
// clauses, and then returns the extended node.
ParseNode* SyntaxAnalyzer::parseIfChain(ParseNode* node) {
    // Skip any newlines before elif/else
    while (!isAtEnd() && currentToken().type == TokenType::NEWLINE) {
        advance();
    }

    // Handle whitespace/indentation
    while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                          currentToken().type == TokenType::DEDENT)) {
        advance();
    }

    // handle zero or more "elif"
    while (match("elif")) {
        auto elifNode = new ParseNode("Elif");
        bool validElif = true;

        // Parse elif condition
        auto cond = parseComparison();
        if (!cond) {
            addSyntaxError("Invalid expression in elif condition",
                           currentToken().line, currentToken().column);
            delete elifNode;
            validElif = false;
        }

        if (!match(":")) {
            if (currentToken().lexeme == "=") {
                addSyntaxError("Invalid '=' in condition; did you mean '=='?",
                               currentToken().line, currentToken().column);
            } else {
                addSyntaxError("Expected ':' after elif condition",
                               currentToken().line, currentToken().column);
            }
            if (cond) delete cond;
            delete elifNode;
            validElif = false;
        }

        // Check indentation for elif block
        if (!checkIndentation("elif")) {
            if (cond) delete cond;
            delete elifNode;
            validElif = false;
        }

        auto body = parseStmt();
        if (!body) {
            if (cond) delete cond;
            delete elifNode;
            validElif = false;
        }

        // Only add valid elif nodes to the chain
        if (validElif) {
            elifNode->children.push_back(cond);
            elifNode->children.push_back(body);
            node->children.push_back(elifNode);
        }

        // Skip newlines between elif/else blocks
        while (!isAtEnd() && currentToken().type == TokenType::NEWLINE) {
            advance();
        }

        // Handle whitespace/indentation before next elif/else
        while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                              currentToken().type == TokenType::DEDENT)) {
            advance();
        }

        // If we hit an error, try to recover by skipping to next elif/else or end of block
        if (!validElif) {
            while (!isAtEnd() &&
                   currentToken().type != TokenType::DEDENT &&
                   currentToken().lexeme != "elif" &&
                   currentToken().lexeme != "else") {
                advance();
            }
        }
    }

    // Handle whitespace/indentation before else
    while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                          currentToken().type == TokenType::DEDENT)) {
        advance();
    }

    // optional "else"
    if (match("else")) {
        if (!match(":")) {
            addSyntaxError("Expected ':' after else",
                           currentToken().line, currentToken().column);
            return node;
        }

        // Check indentation for else block
        if (!checkIndentation("else")) {
            return node;
        }

        auto elseNode = new ParseNode("Else");
        auto body = parseStmt();
        if (!body) {
            delete elseNode;
            return node;
        }

        elseNode->children.push_back(body);
        node->children.push_back(elseNode);
    }

    return node;
}

// Add this helper method near the top of the file
bool SyntaxAnalyzer::checkIndentation(const std::string& stmtType) {
    // Skip any newlines
    while (!isAtEnd() && currentToken().type == TokenType::NEWLINE) {
        advance();
    }

    // Skip any whitespace
    while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
        advance();
    }

    // Check for INDENT token
    if (currentToken().type != TokenType::INDENT) {
        addSyntaxError("Expected indented block after '" + stmtType + "'",
                       currentToken().line, currentToken().column);
        return false;
    }

    advance(); // consume the INDENT token
    return true;
}

// Update the parseIfCore method
ParseNode* SyntaxAnalyzer::parseIfCore() {
    auto node = new ParseNode("IfStmt");

    // 1) Parse condition
    auto cond = parseComparison();
    if (!cond) {
        addSyntaxError("Invalid expression in if condition",
                       currentToken().line, currentToken().column);
        delete node;
        return nullptr;
    }
    node->children.push_back(cond);

    // 2) Expect colon
    if (!match(":")) {
        if (currentToken().lexeme == "=") {
            addSyntaxError("Invalid '=' in condition; did you mean '=='?",
                           currentToken().line, currentToken().column);
        } else {
            addSyntaxError("Expected ':' after if condition",
                           currentToken().line, currentToken().column);
        }
        delete node;
        return nullptr;
    }

    // 3) Check indentation
    if (!checkIndentation("if")) {
        delete node;
        return nullptr;
    }

    // 4) Parse the body
    auto body = parseStmt();
    if (!body) {
        delete node;
        return nullptr;
    }
    node->children.push_back(body);

    return node;
}

//——— For statement ———

ParseNode* SyntaxAnalyzer::parseForStmt() {
    auto node = new ParseNode("ForStmt");

    // 1) Parse target list
    auto targets = new ParseNode("TargetList");
    do {
        if (currentToken().type != TokenType::IDENTIFIER) {
            addSyntaxError("Expected identifier in for loop",
                           currentToken().line, currentToken().column);
            while (!isAtEnd() && currentToken().lexeme != ":" &&
                   currentToken().type != TokenType::NEWLINE)
                advance();
            if (match(":")) {}
            return nullptr;
        }
        targets->children.push_back(
            new ParseNode("Identifier",
                          QString::fromStdString(currentToken().lexeme)));
        advance();
    } while (match(","));
    node->children.push_back(targets);

    // 2) Expect 'in'
    if (!match("in")) {
        addSyntaxError("Expected 'in' in for loop",
                       currentToken().line, currentToken().column);
        while (!isAtEnd() && currentToken().lexeme != ":" &&
               currentToken().type != TokenType::NEWLINE)
            advance();
        if (match(":")) {}
        return nullptr;
    }

    // 3) Parse iterable
    auto iterable = parseComparison();
    if (!iterable) return nullptr;
    node->children.push_back(iterable);

    // 4) Expect colon
    if (!match(":")) {
        addSyntaxError("Expected ':' after for header",
                       currentToken().line, currentToken().column);
        while (!isAtEnd() && currentToken().type != TokenType::NEWLINE) {
            advance();
        }
        return nullptr;
    }

    // 5) Check indentation
    if (!checkIndentation("for")) {
        return node;
    }

    // 6) Parse the body
    auto body = parseStmt();
    if (!body) return nullptr;
    node->children.push_back(body);

    return node;
}


//——— While statement ———

ParseNode* SyntaxAnalyzer::parseWhileStmt() {
    auto node = new ParseNode("WhileStmt");

    // 1) Optional parentheses
    bool sawParen = match("(");

    // 2) Parse condition
    auto cond = parseComparison();
    if (!cond) {
        addSyntaxError("Invalid expression in while condition",
                       currentToken().line, currentToken().column);
        while (!isAtEnd() && currentToken().lexeme != ":" &&
               currentToken().type != TokenType::NEWLINE)
            advance();
    } else {
        node->children.push_back(cond);
    }

    // 3) Close parenthesis if opened
    if (sawParen && !match(")")) {
        addSyntaxError("Expected ')' after while condition",
                       currentToken().line, currentToken().column);
    }

    // 4) Check for assignment instead of comparison
    if (currentToken().lexeme == "=") {
        addSyntaxError("Invalid '=' in condition; did you mean '=='?",
                       currentToken().line, currentToken().column);
        advance();
        while (!isAtEnd() && currentToken().lexeme != ":" &&
               currentToken().type != TokenType::NEWLINE)
            advance();
    }

    // 5) Expect colon
    if (!match(":")) {
        addSyntaxError("Expected ':' after while condition",
                       currentToken().line, currentToken().column);
        while (!isAtEnd() && currentToken().type != TokenType::NEWLINE) {
            advance();
        }
        return node;
    }

    // 6) Check indentation
    if (!checkIndentation("while")) {
        return node;
    }

    // 7) Parse the body
    auto body = parseStmt();
    if (body) {
        node->children.push_back(body);
    }

    return node;
}

//——— Def statement ———

ParseNode* SyntaxAnalyzer::parseFuncDef() {
    auto node = new ParseNode("FuncDef");

    // 1) Parse function name
    if (currentToken().type != TokenType::IDENTIFIER) {
        addSyntaxError("Expected function name after def",
                       currentToken().line, currentToken().column);
        return nullptr;
    }
    node->children.push_back(
        new ParseNode("Identifier",
                      QString::fromStdString(currentToken().lexeme)));
    advance();

    // 2) Parse parameter list
    if (!match("(")) {
        addSyntaxError("Expected '(' after function name",
                       currentToken().line, currentToken().column);
        delete node;
        return nullptr;
    }

    if (currentToken().type == TokenType::IDENTIFIER) {
        auto params = parseParamList();
        if (params) {
            node->children.push_back(params);
        }
    }

    if (!match(")")) {
        // If the next token is another identifier, it's almost certainly a missing comma
        if (currentToken().type == TokenType::IDENTIFIER) {
            addSyntaxError("Expected ',' between parameters",
                           currentToken().line, currentToken().column);
        } else {
            addSyntaxError("Expected ')' after parameters",
                           currentToken().line, currentToken().column);
        }
        delete node;
        return nullptr;
    }

    // 3) Expect colon
    if (!match(":")) {
        addSyntaxError("Expected ':' after def header",
                       currentToken().line, currentToken().column);
        delete node;
        return nullptr;
    }

    // 4) Check for proper indentation
    if (!checkIndentation("def")) {
        delete node;
        return nullptr;
    }

    // 5) Parse the function body
    auto body = parseStmt();
    if (!body) {
        delete node;
        return nullptr;
    }
    node->children.push_back(body);
    return node;
}

//——— Param list ———

ParseNode* SyntaxAnalyzer::parseParamList() {
    auto node = new ParseNode("ParamList");

    bool expectComma = false;

    while (!isAtEnd()) {
        if (currentToken().type != TokenType::IDENTIFIER) {
            // If expecting comma but got an identifier, comma is missing
            if (expectComma) {
                addSyntaxError("Expected ',' between parameters",
                               currentToken().line, currentToken().column);
                return nullptr;
            } else {
                break; // maybe it's closing ')'
            }
        }

        // Add param
        node->children.push_back(
            new ParseNode("Param", QString::fromStdString(currentToken().lexeme)));
        advance();

        // After a param, expect either ',' or ')'
        if (match(",")) {
            expectComma = false;
        } else {
            expectComma = true;
            break;
        }
    }

    return node;
}

//——— Assignment ———

ParseNode* SyntaxAnalyzer::parseAssignment() {
    auto node = new ParseNode("Assignment");

    // Get the target identifier
    if (currentToken().type != TokenType::IDENTIFIER) {
        addSyntaxError("Expected identifier before assignment operator",
                       currentToken().line, currentToken().column);
        return nullptr;
    }

    node->children.push_back(
        new ParseNode("Identifier",
                      QString::fromStdString(currentToken().lexeme)));
    advance();

    // Handle both simple and compound assignments
    std::string op;
    if (match("=")) {
        op = "=";
    } else if (match("+=")) {
        op = "+=";
    } else if (match("-=")) {
        op = "-=";
    } else if (match("*=")) {
        op = "*=";
    } else if (match("/=")) {
        op = "/=";
    } else {
        addSyntaxError("Expected assignment operator",
                       currentToken().line, currentToken().column);
        delete node;
        return nullptr;
    }

    node->value = QString::fromStdString(op); // Store the operator type

    // Parse the right-hand side expression
    auto rhs = parseExpression();
    if (!rhs) {
        delete node;
        return nullptr;
    }
    node->children.push_back(rhs);
    return node;
}

//——— Expression statement ———

ParseNode* SyntaxAnalyzer::parseExprStmt() {
    auto node = new ParseNode("ExprStmt");
    auto expr = parseExpression();
    if (!expr) return nullptr;
    node->children.push_back(expr);
    return node;
}

//——— Expression (E ::= T { (+|-) T }) ———

ParseNode* SyntaxAnalyzer::parseExpression() {
    // Skip any leading whitespace or comments
    while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                         currentToken().type == TokenType::COMMENT)) {
        advance();
    }

    // Block endings are not expressions
    if (currentToken().type == TokenType::DEDENT ||
        currentToken().type == TokenType::NEWLINE ||
        currentToken().type == TokenType::ENDOFFILE) {
        return nullptr;
    }

    auto left = parseTerm();
    if (!left) return nullptr;

    while (!isAtEnd()) {
        // Skip whitespace and comments between terms
        while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                             currentToken().type == TokenType::COMMENT)) {
            advance();
        }

        // Stop at block endings
        if (currentToken().type == TokenType::DEDENT ||
            currentToken().type == TokenType::NEWLINE ||
            currentToken().type == TokenType::ENDOFFILE ||
            currentToken().lexeme == ":") {
            break;
        }

        std::string op;
        if (match("+")) op = "+";
        else if (match("-")) op = "-";
        else break;

        // Skip whitespace and comments after operator
        while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                             currentToken().type == TokenType::COMMENT)) {
            advance();
        }

        auto right = parseTerm();
        if (!right) return nullptr;

        auto opNode = new ParseNode("Operator", QString::fromStdString(op));
        opNode->children.push_back(left);
        opNode->children.push_back(right);
        left = opNode;
    }

    return left;
}

//——— Term (T ::= F { (*|/) F }) ———

ParseNode* SyntaxAnalyzer::parseTerm() {
    // Skip any leading whitespace or comments
    while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                         currentToken().type == TokenType::COMMENT)) {
        advance();
    }

    auto left = parseFactor();
    if (!left) return nullptr;

    while (true) {
        // Skip any whitespace or comments before operator
        while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                             currentToken().type == TokenType::COMMENT)) {
            advance();
        }

        std::string op;
        if (match("*")) op = "*";
        else if (match("/")) op = "/";
        else if (match("%")) op = "%";
        else break;

        // Skip any whitespace or comments after operator
        while (!isAtEnd() && (currentToken().type == TokenType::WHITESPACE ||
                             currentToken().type == TokenType::COMMENT)) {
            advance();
        }

        auto right = parseFactor();
        if (!right) return nullptr;

        auto opNode = new ParseNode("Operator", QString::fromStdString(op));
        opNode->children.push_back(left);
        opNode->children.push_back(right);
        left = opNode;
    }
    return left;
}


//——— Factor (F ::= '(' E ')' | number | identifier ) ———

ParseNode* SyntaxAnalyzer::parseFactor() {
    std::cout << "parseFactor: current token type=" << tokenTypeToString(currentToken().type)
    << ", lexeme='" << currentToken().lexeme
    << "', line=" << currentToken().line
    << ", col=" << currentToken().column << std::endl;

    // Skip any leading whitespace
    while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
        advance();
        std::cout << "  Skipped whitespace" << std::endl;
    }

    // Parenthesized expression
    if (match("(")) {
        std::cout << "  Parsing parenthesized expression" << std::endl;
        auto expr = parseExpression();
        if (!match(")")) {
            addSyntaxError("Expected ')' after expression",
                           currentToken().line, currentToken().column);
            delete expr;
            return nullptr;
        }
        return expr;
    }

    const Token& tok = currentToken();
    std::cout << "  Checking token: type=" << tokenTypeToString(tok.type)
              << ", lexeme='" << tok.lexeme << "'" << std::endl;

    // String literals
    if (tok.type == TokenType::STRING) {
        std::cout << "  Found string literal" << std::endl;
        auto leaf = new ParseNode("String", QString::fromStdString(tok.lexeme));
        advance();
        return leaf;
    }

    // Boolean literals
    if (tok.type == TokenType::KEYWORD &&
        (tok.lexeme == "True" || tok.lexeme == "False"))
    {
        std::cout << "  Found boolean literal" << std::endl;
        auto leaf = new ParseNode("Bool", QString::fromStdString(tok.lexeme));
        advance();
        return leaf;
    }

    // Identifier or function call
    if (tok.type == TokenType::IDENTIFIER) {
        std::cout << "  Found identifier: " << tok.lexeme << std::endl;
        std::string name = tok.lexeme;
        advance();

        // Skip whitespace after identifier
        while (!isAtEnd() && currentToken().type == TokenType::WHITESPACE) {
            advance();
            std::cout << "  Skipped whitespace after identifier" << std::endl;
        }

        // Function call: IDENTIFIER '(' [args] ')'
        if (match("(")) {
            std::cout << "  Found function call" << std::endl;
            auto callNode = new ParseNode("FuncCall", QString::fromStdString(name));

            // Parse zero or more comma‑separated arguments
            if (!isAtEnd() && currentToken().lexeme != ")") {
                do {
                    auto arg = parseExpression();
                    if (!arg) {
                        delete callNode;
                        return nullptr;
                    }
                    callNode->children.push_back(arg);
                } while (match(","));
            }

            if (!match(")")) {
                addSyntaxError("Expected ')' after function call arguments",
                               currentToken().line, currentToken().column);
                delete callNode;
                return nullptr;
            }
            return callNode;
        }

        // Plain identifier
        std::cout << "  Creating identifier node for: " << name << std::endl;
        return new ParseNode("Identifier", QString::fromStdString(name));
    }

    // Number literals
    if (tok.type == TokenType::NUMBER ||
        tok.type == TokenType::HexadecimalNumber ||
        tok.type == TokenType::BinaryNumber ||
        tok.type == TokenType::OCTALNUMBER)
    {
        std::cout << "  Found number literal" << std::endl;
        QString nodeName;
        switch (tok.type) {
        case TokenType::NUMBER:            nodeName = "Number"; break;
        case TokenType::HexadecimalNumber: nodeName = "Hex";    break;
        case TokenType::BinaryNumber:      nodeName = "Binary"; break;
        case TokenType::OCTALNUMBER:       nodeName = "Octal";  break;
        default:                           nodeName = "Number"; break;
        }

        auto leaf = new ParseNode(nodeName, QString::fromStdString(tok.lexeme));
        advance();
        return leaf;
    }

    // If we get here, we couldn't parse a factor
    if (currentToken().type == TokenType::NEWLINE ||
        currentToken().type == TokenType::ENDOFFILE) {
        std::cout << "  Hit end of line or file" << std::endl;
        return nullptr;
    }

    std::cout << "  Failed to parse factor" << std::endl;
    addSyntaxError("Expected an identifier, number, or expression",
                   currentToken().line, currentToken().column);
    return nullptr;
}

void SyntaxAnalyzer::buildTree(QTreeWidgetItem* parent, ParseNode* node) {
    for (auto child : node->children) {
        QTreeWidgetItem* item = new QTreeWidgetItem(parent);
        item->setText(0, child->name + (child->value.isEmpty() ? "" : ": " + child->value));
        buildTree(item, child);
    }
}

const Token& SyntaxAnalyzer::currentToken() const {
    return tokens[pos];
}

void SyntaxAnalyzer::advance() {
    if (!isAtEnd()) pos++;
}

bool SyntaxAnalyzer::match(const std::string& lexeme) {
    if (!isAtEnd() && currentToken().lexeme == lexeme) {
        advance();
        return true;
    }
    return false;
}

bool SyntaxAnalyzer::isAtEnd() const {
    return pos >= tokens.size() || currentToken().type == TokenType::ENDOFFILE;
}

void SyntaxAnalyzer::addSyntaxError(const std::string& msg, int line, int column) {
    syntaxErrors.push_back({ msg, line, column });
}

