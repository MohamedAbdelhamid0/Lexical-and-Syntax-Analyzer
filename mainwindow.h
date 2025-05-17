#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "syntaxanalyzer.h"
#include "parsetreedisplay.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void analyze();
    void clear();
    void openFile();
    
    // New method to switch between tree views
    void switchTreeView(bool useGraphicalView);

private:
    Ui::MainWindow *ui;
    
    // ParseTreeDisplay for graphical view
    ParseTreeDisplay* parseTreeGraphical;
    
    // Flag to track which view is active
    bool graphicalViewActive;
};

#endif // MAINWINDOW_H
