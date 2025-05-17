#include "parsetreedisplay.h"
#include <QResizeEvent>
#include <QPen>
#include <QBrush>
#include <cmath>
#include <QDebug>
#include <QScrollBar>
#include <QApplication>

ParseTreeDisplay::ParseTreeDisplay(QWidget* parent)
    : QGraphicsView(parent), root(nullptr), zoomFactor(1.0)
{
    // Create a new scene
    scene = new QGraphicsScene(this);
    setScene(scene);

    // Configure view
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Set a white background
    setBackgroundBrush(QBrush(Qt::white));

    // Enable focus to receive key events
    setFocusPolicy(Qt::StrongFocus);
}

void ParseTreeDisplay::setParseTree(ParseNode* rootNode)
{
    clear();
    root = rootNode;
    if (root) {
        layoutTree();
    }
}

void ParseTreeDisplay::clear()
{
    scene->clear();
    nodeMap.clear();
    root = nullptr;
}

void ParseTreeDisplay::zoomIn()
{
    // Apply zoom with scaling factor
    qreal newZoomFactor = zoomFactor * ZOOM_IN_FACTOR;
    if (newZoomFactor <= MAX_ZOOM) {
        zoomFactor = newZoomFactor;
        QTransform transform;
        transform.scale(zoomFactor, zoomFactor);
        setTransform(transform);
    }
}

void ParseTreeDisplay::zoomOut()
{
    // Apply zoom with scaling factor
    qreal newZoomFactor = zoomFactor * ZOOM_OUT_FACTOR;
    if (newZoomFactor >= MIN_ZOOM) {
        zoomFactor = newZoomFactor;
        QTransform transform;
        transform.scale(zoomFactor, zoomFactor);
        setTransform(transform);
    }
}

void ParseTreeDisplay::resetZoom()
{
    // Reset to original scale
    zoomFactor = 1.0;
    QTransform transform;
    transform.scale(zoomFactor, zoomFactor);
    setTransform(transform);

    // Reset view
    if (scene->items().count() > 0) {
        fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    }
}

void ParseTreeDisplay::wheelEvent(QWheelEvent* event)
{
    // Check if Ctrl key is pressed (for macOS, use Command key)
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        // Get the position before zooming
        QPointF oldPos = mapToScene(event->position().toPoint());

        // Zoom in or out based on wheel direction
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }

        // Get the position after zooming
        QPointF newPos = mapToScene(event->position().toPoint());

        // Adjust the view to keep the position under mouse cursor
        QPointF delta = newPos - oldPos;
        translate(delta.x(), delta.y());

        event->accept();
    } else {
        // Default behavior for scrolling without Ctrl
        QGraphicsView::wheelEvent(event);
    }
}

void ParseTreeDisplay::keyPressEvent(QKeyEvent* event)
{
    // Handle keyboard shortcuts for zooming
    switch (event->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (event->modifiers() & Qt::ControlModifier) {
            zoomIn();
            event->accept();
            return;
        }
        break;

    case Qt::Key_Minus:
        if (event->modifiers() & Qt::ControlModifier) {
            zoomOut();
            event->accept();
            return;
        }
        break;

    case Qt::Key_0:
        if (event->modifiers() & Qt::ControlModifier) {
            resetZoom();
            event->accept();
            return;
        }
        break;
    }

    // Pass unhandled keys to parent
    QGraphicsView::keyPressEvent(event);
}

void ParseTreeDisplay::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (scene->items().count() > 0) {
        // Fit the view to the scene contents when resized
        fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
        // Restore the current zoom factor after fitting
        QTransform transform;
        transform.scale(zoomFactor, zoomFactor);
        setTransform(transform);
    }
}

void ParseTreeDisplay::layoutTree()
{
    if (!root) return;

    // First pass: calculate node positions
    qreal maxWidth = 0;
    calculateNodePositions(root, maxWidth);

    // Second pass: create visual representations
    renderNode(root);

    // Third pass: create edges
    createEdges();

    // Fit scene in view
    scene->setSceneRect(scene->itemsBoundingRect().adjusted(-50, -50, 50, 50));
    fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void ParseTreeDisplay::calculateNodePositions(ParseNode* node, qreal& maxWidth, int depth)
{
    if (!node) return;

    GraphNode* gNode = new GraphNode();
    nodeMap[node] = gNode;

    // Determine node position based on children
    if (node->children.isEmpty()) {
        // Leaf node
        gNode->pos = QPointF(maxWidth, depth * (NODE_HEIGHT + VERTICAL_SPACING));
        maxWidth += NODE_WIDTH + HORIZONTAL_SPACING;
    } else {
        QList<QPointF> childPositions;
        // First position all children
        for (auto* child : node->children) {
            calculateNodePositions(child, maxWidth, depth + 1);
            if (nodeMap.contains(child)) {
                childPositions.append(nodeMap[child]->pos);
            }
        }

        // Skip if no children were successfully positioned
        if (childPositions.isEmpty()) {
            gNode->pos = QPointF(maxWidth, depth * (NODE_HEIGHT + VERTICAL_SPACING));
            maxWidth += NODE_WIDTH + HORIZONTAL_SPACING;
            return;
        }

        // Node is positioned at the horizontal center of its children
        qreal leftmostX = childPositions.first().x();
        qreal rightmostX = childPositions.last().x();
        qreal centerX = leftmostX + (rightmostX - leftmostX) / 2;

        // Ensure minimum width between siblings
        if (rightmostX - leftmostX < NODE_WIDTH && childPositions.size() > 1) {
            // Adjust positions to ensure minimum spacing
            qreal totalWidth = (childPositions.size() - 1) * (NODE_WIDTH + HORIZONTAL_SPACING);
            qreal startX = centerX - totalWidth / 2;

            for (int i = 0; i < childPositions.size(); i++) {
                ParseNode* child = node->children[i];
                if (nodeMap.contains(child)) {
                    GraphNode* childNode = nodeMap[child];
                    QPointF newPos(startX + i * (NODE_WIDTH + HORIZONTAL_SPACING),
                                   childNode->pos.y());
                    childNode->pos = newPos;
                }
            }

            // Recalculate center based on new positions
            leftmostX = nodeMap[node->children.first()]->pos.x();
            rightmostX = nodeMap[node->children.last()]->pos.x();
            centerX = leftmostX + (rightmostX - leftmostX) / 2;
        }

        gNode->pos = QPointF(centerX, depth * (NODE_HEIGHT + VERTICAL_SPACING));
    }
}

void ParseTreeDisplay::renderNode(ParseNode* node)
{
    if (!node || !nodeMap.contains(node)) return;

    GraphNode* gNode = nodeMap[node];

    // Create visual representation with clearer labeling
    QString label;

    if (node->value.isEmpty()) {
        // Just show the node name
        label = node->name;
    } else if (node->name == "Identifier") {
        // For identifiers, show the value clearly
        label = node->value;
    } else if (node->name.contains("Literal") ||
               node->name == "Number" ||
               node->name == "String" ||
               node->name == "Bool") {
        // For literals, emphasize the value
        label = node->value;
    } else {
        // For other nodes with values, show both
        label = node->name + ": " + node->value;
    }

    // If the label is empty (shouldn't happen but just in case)
    if (label.isEmpty()) {
        label = "Unknown";
    }

    createNodeShape(gNode, label);

    // Recursively render children
    for (auto* child : node->children) {
        renderNode(child);
    }
}

void ParseTreeDisplay::createNodeShape(GraphNode* gNode, const QString& label)
{
    // Create ellipse shape
    QPen outlinePen(Qt::black, 2);
    QBrush fillBrush(Qt::white);

    gNode->shape = scene->addEllipse(
        gNode->pos.x() - NODE_WIDTH/2,
        gNode->pos.y() - NODE_HEIGHT/2,
        NODE_WIDTH,
        NODE_HEIGHT,
        outlinePen,
        fillBrush
        );

    // Add text label with black color and proper font
    QFont labelFont("Arial", 10, QFont::Bold);
    gNode->text = scene->addText(label, labelFont);
    gNode->text->setDefaultTextColor(Qt::black);

    // Center text in the ellipse
    QRectF textRect = gNode->text->boundingRect();
    gNode->text->setPos(
        gNode->pos.x() - textRect.width()/2,
        gNode->pos.y() - textRect.height()/2
        );
}

void ParseTreeDisplay::createEdges()
{
    // Create edges between nodes
    for (auto it = nodeMap.begin(); it != nodeMap.end(); ++it) {
        ParseNode* node = it.key();
        GraphNode* gNode = it.value();

        for (auto* child : node->children) {
            if (nodeMap.contains(child)) {
                GraphNode* childGNode = nodeMap[child];

                // Get the positions of the parent and child nodes
                QPointF parentPos = gNode->pos;
                QPointF childPos = childGNode->pos;

                // Calculate the angle between the centers
                qreal dx = childPos.x() - parentPos.x();
                qreal dy = childPos.y() - parentPos.y();
                qreal angle = std::atan2(dy, dx);

                // For ellipses, we need to account for the different widths and heights
                // We use parametric form where x = a*cos(t), y = b*sin(t)
                // and find the t value where the line from center intersects

                // Calculate the starting point (edge of parent ellipse)
                // For an ellipse: (x/a)² + (y/b)² = 1
                // Using the angle, we find the point on the ellipse in that direction
                qreal a = NODE_WIDTH / 2;  // semi-major axis (half width)
                qreal b = NODE_HEIGHT / 2; // semi-minor axis (half height)

                // t is the parameter where the ray intersects the ellipse
                // For an ellipse, we can find t where the ray intersects using:
                qreal startRatio = 1.0 / std::sqrt((cos(angle)*cos(angle))/(a*a) + (sin(angle)*sin(angle))/(b*b));
                qreal startX = parentPos.x() + startRatio * cos(angle);
                qreal startY = parentPos.y() + startRatio * sin(angle);

                // Calculate the endpoint (edge of child ellipse)
                // We use π + angle to get the opposite direction
                qreal endAngle = angle + M_PI;
                qreal endRatio = 1.0 / std::sqrt((cos(endAngle)*cos(endAngle))/(a*a) + (sin(endAngle)*sin(endAngle))/(b*b));
                qreal endX = childPos.x() + endRatio * cos(endAngle);
                qreal endY = childPos.y() + endRatio * sin(endAngle);

                // Create the edge connecting the ellipse boundaries
                QLineF line(startX, startY, endX, endY);
                QPen edgePen(Qt::black, 1.5);

                QGraphicsLineItem* edge = scene->addLine(line, edgePen);

                // Add arrowhead at the child's end - make it larger and more visible
                const qreal arrowSize = 12;  // Slightly larger arrows

                // Calculate angle of the edge line itself
                qreal edgeAngle = std::atan2(line.dy(), line.dx());

                QPointF arrowP1 = line.p2() - QPointF(cos(edgeAngle + M_PI/6) * arrowSize,
                                                      sin(edgeAngle + M_PI/6) * arrowSize);
                QPointF arrowP2 = line.p2() - QPointF(cos(edgeAngle - M_PI/6) * arrowSize,
                                                      sin(edgeAngle - M_PI/6) * arrowSize);

                QPolygonF arrowHead;
                arrowHead.append(line.p2());
                arrowHead.append(arrowP1);
                arrowHead.append(arrowP2);

                // Add the filled arrow head
                scene->addPolygon(arrowHead, edgePen, QBrush(Qt::black));

                // Store the edge
                gNode->edges.append(edge);
            }
        }
    }
}
