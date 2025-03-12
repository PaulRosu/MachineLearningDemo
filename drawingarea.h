#ifndef DRAWINGAREA_H
#define DRAWINGAREA_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QVector>
#include <QPoint>
#include <QColor>

// Symbol types that can be drawn
enum class SymbolType {
    Cross,  // X
    Plus,   // +
    Star    // *
};

// Structure to store point data
struct PointData {
    QPoint logicalPos;  // Position in logical coordinates (-300 to 300)
    QColor color;
    SymbolType symbolType;
    bool hasCircle;     // Whether to draw a small circle around the point
    QColor circleColor;
};

// Structure to store area circle data
struct AreaCircle {
    QPoint center;      // Center in logical coordinates
    int radius;         // Radius in logical units
    QColor color;
};

class DrawingArea : public QWidget
{
    Q_OBJECT

public:
    explicit DrawingArea(QWidget *parent = nullptr);
    
    // Clear all drawings
    void clearCanvas();
    
    // Add a point with a symbol
    void addPoint(int logicalX, int logicalY, const QColor &color, SymbolType symbol);
    
    // Add a point with a symbol and a circle around it
    void addPointWithCircle(int logicalX, int logicalY, const QColor &pointColor, 
                           SymbolType symbol, const QColor &circleColor);
    
    // Add an area circle
    void addAreaCircle(int logicalX, int logicalY, int radius, const QColor &color);
    
    // Remove all points
    void clearPoints();
    
    // Remove all area circles
    void clearAreaCircles();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // Conversion between logical coordinates and widget coordinates
    QPoint logicalToWidget(const QPoint &logicalPos) const;
    QPoint widgetToLogical(const QPoint &widgetPos) const;
    int logicalToWidgetSize(int logicalSize) const;
    
    // Drawing functions
    void drawSymbol(QPainter &painter, const QPoint &pos, const QColor &color, SymbolType type, int size);
    void drawPointCircle(QPainter &painter, const QPoint &pos, const QColor &color);
    void drawAreaCircle(QPainter &painter, const QPoint &center, int radius, const QColor &color);
    
    // Data storage
    QVector<PointData> points;
    QVector<AreaCircle> areaCircles;
    
    // Drawing properties
    int symbolSize;  // Size of symbols in widget pixels
};

#endif // DRAWINGAREA_H 