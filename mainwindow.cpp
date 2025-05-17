#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pythonlexer.h"
#include "syntaxanalyzer.h"
#include "parsetreedisplay.h"

#include <QString>
#include <QTextStream>
#include <vector>
#include <algorithm>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , graphicalViewActive(true) // Default to graphical view
{
    ui->setupUi(this);

    // Connect the analyze button to the slot
    connect(ui->analyzeButton, &QPushButton::clicked, this, &MainWindow::analyze);

    // Connect the clear button to the slot
    connect(ui->clearButton, &QPushButton::clicked, this, &MainWindow::clear);

    // Connect the open file button to the slot
    connect(ui->openFileButton, &QPushButton::clicked, this, &MainWindow::openFile);

    // Configure the symbol table widget
    ui->symbolTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Configure the parse tree widget (text-based tree)
    ui->parseTree->setHeaderLabel("Parse Tree");
    ui->parseTree->setAlternatingRowColors(true);
    ui->parseTree->setUniformRowHeights(true);
    ui->parseTree->setAnimated(true);
    ui->parseTree->setAllColumnsShowFocus(true);
    ui->parseTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Create the graphical parse tree view
    parseTreeGraphical = new ParseTreeDisplay(this);
    
    // Create checkbox for switching between views
    QCheckBox* viewToggle = new QCheckBox("Use Graphical View", this);
    viewToggle->setChecked(graphicalViewActive);
    connect(viewToggle, &QCheckBox::toggled, this, &MainWindow::switchTreeView);
    
    // Create zoom control buttons
    QPushButton* zoomInBtn = new QPushButton("+", this);
    zoomInBtn->setToolTip("Zoom In (Ctrl++)");
    zoomInBtn->setFixedSize(30, 30);
    connect(zoomInBtn, &QPushButton::clicked, this, [this]() {
        if (graphicalViewActive) {
            parseTreeGraphical->zoomIn();
        }
    });
    
    QPushButton* zoomOutBtn = new QPushButton("-", this);
    zoomOutBtn->setToolTip("Zoom Out (Ctrl+-)");
    zoomOutBtn->setFixedSize(30, 30);
    connect(zoomOutBtn, &QPushButton::clicked, this, [this]() {
        if (graphicalViewActive) {
            parseTreeGraphical->zoomOut();
        }
    });
    
    QPushButton* resetZoomBtn = new QPushButton("1:1", this);
    resetZoomBtn->setToolTip("Reset Zoom (Ctrl+0)");
    resetZoomBtn->setFixedSize(30, 30);
    connect(resetZoomBtn, &QPushButton::clicked, this, [this]() {
        if (graphicalViewActive) {
            parseTreeGraphical->resetZoom();
        }
    });
    
    // Create container for zoom buttons
    QHBoxLayout* zoomLayout = new QHBoxLayout();
    zoomLayout->addWidget(zoomInBtn);
    zoomLayout->addWidget(zoomOutBtn);
    zoomLayout->addWidget(resetZoomBtn);
    zoomLayout->addStretch();
    
    // Add the components to the layout
    QVBoxLayout* parseTreeLayout = qobject_cast<QVBoxLayout*>(ui->parseTreeWidget->layout());
    if (parseTreeLayout) {
        parseTreeLayout->insertWidget(1, viewToggle); // Insert after label, before tree widget
        parseTreeLayout->insertLayout(2, zoomLayout); // Insert zoom controls after the checkbox
        
        // Remove the default tree widget temporarily
        parseTreeLayout->removeWidget(ui->parseTree);
        ui->parseTree->setVisible(false);
        
        // Add the graphical tree view
        parseTreeLayout->addWidget(parseTreeGraphical);
    }

    // Set initial splitter sizes
    ui->mainSplitter->setSizes(QList<int>() << 600 << 600);
    ui->lexicalSplitter->setSizes(QList<int>() << 200 << 200 << 200);
    ui->syntaxSplitter->setSizes(QList<int>() << 300 << 300);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::switchTreeView(bool useGraphicalView)
{
    if (graphicalViewActive == useGraphicalView) return;
    
    graphicalViewActive = useGraphicalView;
    
    QVBoxLayout* parseTreeLayout = qobject_cast<QVBoxLayout*>(ui->parseTreeWidget->layout());
    if (!parseTreeLayout) return;
    
    // Get the last widget in the layout (the current tree view)
    QLayoutItem* item = parseTreeLayout->itemAt(parseTreeLayout->count() - 1);
    if (!item || !item->widget()) return;
    
    // Remove the current view
    QWidget* currentView = item->widget();
    parseTreeLayout->removeWidget(currentView);
    currentView->setVisible(false);
    
    // Add the new view
    if (useGraphicalView) {
        parseTreeLayout->addWidget(parseTreeGraphical);
        parseTreeGraphical->setVisible(true);
    } else {
        parseTreeLayout->addWidget(ui->parseTree);
        ui->parseTree->setVisible(true);
    }
}

void MainWindow::analyze()
{
    // Get the input code from the GUI
    QString code = ui->codeInput->toPlainText();

    // Ensure code ends with a newline for consistent parsing
    if (!code.isEmpty() && !code.endsWith('\n')) {
        code += '\n';
    }

    std::string codeStr = code.toStdString();

    // Create a new PythonLexer instance for this analysis
    PythonLexer lexer(codeStr);
    auto [tokens, lexicalErrors] = lexer.tokenize();

    // Debug: Print all tokens
    std::cout << "\nAll tokens:" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "[Line " << token.line << ":" << token.column << "] "
                  << tokenTypeToString(token.type) << ": '" << token.lexeme << "'" << std::endl;
    }

    // Display tokens with line numbers
    QString tokenOutput;
    for (const auto& token : tokens) {
        tokenOutput += QString("[Line %1:%2] '%3' (%4)\n")
            .arg(token.line)
            .arg(token.column)
            .arg(QString::fromStdString(token.lexeme))
            .arg(QString::fromStdString(tokenTypeToString(token.type)));
    }
    ui->tokenOutput->setPlainText(tokenOutput);

    // Display lexical errors with line numbers
    QString lexicalErrorOutput;
    for (const auto& error : lexicalErrors) {
        lexicalErrorOutput += QString("[Line %1:%2] Lexical Error: %3\n")
            .arg(error.line)
            .arg(error.column)
            .arg(QString::fromStdString(error.message));
    }
    ui->lexicalErrorOutput->setPlainText(lexicalErrorOutput);

    // Clear any existing parse tree
    ui->parseTree->clear();
    parseTreeGraphical->clear();

    // If there are lexical errors, don't proceed with parsing
    if (!lexicalErrors.empty()) {
        ui->syntaxErrorOutput->setPlainText("Parse tree not displayed due to lexical errors.");
        return;
    }

    // Filter out comment tokens before parsing
    std::vector<Token> parseTokens;
    parseTokens.reserve(tokens.size());
    for (const auto& token : tokens) {
        if (token.type != TokenType::COMMENT) {
            parseTokens.push_back(token);
        }
    }

    // Create and run the syntax analyzer with filtered tokens
    SyntaxAnalyzer parser(parseTokens);
    ParseNode* tree = parser.parseProgram();

    // Display syntax errors
    QString syntaxErrorOutput;
    const auto& syntaxErrors = parser.getErrors();
    for (const auto& err : syntaxErrors) {
        syntaxErrorOutput += QString("[Line %1:%2] Syntax Error: %3\n")
            .arg(err.line)
            .arg(err.column)
            .arg(QString::fromStdString(err.message));
    }
    ui->syntaxErrorOutput->setPlainText(syntaxErrorOutput);

    // Only display the parse tree if there are no errors at all
    if (tree != nullptr && syntaxErrors.empty()) {
        // Display success message
        ui->syntaxErrorOutput->setPlainText("No errors detected.");
        
        // Update the appropriate tree view
        if (!graphicalViewActive) {
            parser.populateTree(ui->parseTree);
        } else {
            parseTreeGraphical->setParseTree(tree);
        }
    } else {
        // Add message about not showing the parse tree
        if (!syntaxErrorOutput.isEmpty()) {
            ui->syntaxErrorOutput->appendPlainText("\nParse tree not displayed due to syntax errors.");
        }
    }

    // Display symbol table
    const auto& symbols = lexer.getSymbolTable().getSymbols();
    std::vector<std::string> identifiers;
    for (const auto& entry : symbols) {
        identifiers.push_back(entry.first);
    }

    // Sort by ID
    std::sort(identifiers.begin(), identifiers.end(),
              [&lexer](const auto& a, const auto& b) {
                  return lexer.getSymbolTable().getId(a) < lexer.getSymbolTable().getId(b);
              });

    // Populate the table with ID, Identifier, Data Type, and Value
    ui->symbolTable->setRowCount(identifiers.size());
    for (size_t i = 0; i < identifiers.size(); ++i) {
        const auto& identifier = identifiers[i];
        const auto& symbolTable = lexer.getSymbolTable();

        // ID column
        QTableWidgetItem* idItem = new QTableWidgetItem(QString::number(symbolTable.getId(identifier)));
        idItem->setFlags(idItem->flags() ^ Qt::ItemIsEditable);
        ui->symbolTable->setItem(i, 0, idItem);

        // Identifier column
        QTableWidgetItem* identItem = new QTableWidgetItem(QString::fromStdString(identifier));
        identItem->setFlags(identItem->flags() ^ Qt::ItemIsEditable);
        ui->symbolTable->setItem(i, 1, identItem);

        // Data Type column
        QTableWidgetItem* typeItem = new QTableWidgetItem(QString::fromStdString(symbolTable.getDataType(identifier)));
        typeItem->setFlags(typeItem->flags() ^ Qt::ItemIsEditable);
        ui->symbolTable->setItem(i, 2, typeItem);

        // Value column
        QTableWidgetItem* valueItem = new QTableWidgetItem(QString::fromStdString(symbolTable.getValue(identifier)));
        valueItem->setFlags(valueItem->flags() ^ Qt::ItemIsEditable);
        ui->symbolTable->setItem(i, 3, valueItem);
    }
}

void MainWindow::clear()
{
    ui->codeInput->clear();
    ui->tokenOutput->clear();
    ui->lexicalErrorOutput->clear();
    ui->syntaxErrorOutput->clear();
    ui->parseTree->clear();
    parseTreeGraphical->clear();
    ui->symbolTable->setRowCount(0);
}

void MainWindow::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open Python File", "", "Python Files (*.py);;All Files (*)");
    
    if (filePath.isEmpty()) {
        return;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file: " + file.errorString());
        return;
    }
    
    QTextStream in(&file);
    ui->codeInput->setPlainText(in.readAll());
    file.close();
}
