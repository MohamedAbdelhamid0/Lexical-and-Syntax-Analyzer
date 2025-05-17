#ifndef PARSETREEDISPLAY_H
#define PARSETREEDISPLAY_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QMap>
#include <QWheelEvent>
#include <QKeyEvent>
#include "syntaxanalyzer.h" // For ParseNode

// Node representation in the graphical display
struct GraphNode {
    QGraphicsEllipseItem* shape;
    QGraphicsTextItem* text;
    QPointF pos;
    QList<QGraphicsLineItem*> edges;
};

class ParseTreeDisplay : public QGraphicsView {
    Q_OBJECT

public:
    explicit ParseTreeDisplay(QWidget* parent = nullptr);

    // Set the root node of the parse tree to display
    void setParseTree(ParseNode* rootNode);

    // Clear the display
    void clear();

    // Zoom functions
    void zoomIn();
    void zoomOut();
    void resetZoom();

private:
    QGraphicsScene* scene;
    ParseNode* root;
    QMap<ParseNode*, GraphNode*> nodeMap;

    // Constants for layout
    const qreal NODE_WIDTH = 150;
    const qreal NODE_HEIGHT = 50;
    const qreal VERTICAL_SPACING = 80;
    const qreal HORIZONTAL_SPACING = 20;

    // Zoom settings
    qreal zoomFactor;
    const qreal ZOOM_IN_FACTOR = 1.25;
    const qreal ZOOM_OUT_FACTOR = 0.8;
    const qreal MAX_ZOOM = 5.0;
    const qreal MIN_ZOOM = 0.1;

    // Calculate positions and layout tree
    void layoutTree();
    void calculateNodePositions(ParseNode* node, qreal& maxWidth, int depth = 0);
    void renderNode(ParseNode* node);
    void createNodeShape(GraphNode* gNode, const QString& label);
    void createEdges();

protected:
    // Handle resize events
    void resizeEvent(QResizeEvent* event) override;

    // Mouse wheel event for zooming
    void wheelEvent(QWheelEvent* event) override;

    // Key press event for keyboard zoom controls
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // PARSETREEDISPLAY_H
