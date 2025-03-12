#include "drawingarea.h"
#include <QResizeEvent>

DrawingArea::DrawingArea(QWidget *parent)
    : QWidget(parent)
    , symbolSize(10)  // Default symbol size
{
    // Set background to white
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
    
    // Set minimum size
    setMinimumSize(400, 400);
}

void DrawingArea::clearCanvas()
{
    clearPoints();
    clearAreaCircles();
    update();
}

void DrawingArea::clearPoints()
{
    points.clear();
    update();
}

void DrawingArea::clearAreaCircles()
{
    areaCircles.clear();
    update();
}

void DrawingArea::addPoint(int logicalX, int logicalY, const QColor &color, SymbolType symbol)
{
    PointData point;
    point.logicalPos = QPoint(logicalX, logicalY);
    point.color = color;
    point.symbolType = symbol;
    point.hasCircle = false;
    
    points.append(point);
    update();
}

void DrawingArea::addPointWithCircle(int logicalX, int logicalY, const QColor &pointColor, 
                                   SymbolType symbol, const QColor &circleColor)
{
    PointData point;
    point.logicalPos = QPoint(logicalX, logicalY);
    point.color = pointColor;
    point.symbolType = symbol;
    point.hasCircle = true;
    point.circleColor = circleColor;
    
    points.append(point);
    update();
}

void DrawingArea::addAreaCircle(int logicalX, int logicalY, int radius, const QColor &color)
{
    AreaCircle circle;
    circle.center = QPoint(logicalX, logicalY);
    circle.radius = radius;
    circle.color = color;
    
    areaCircles.append(circle);
    update();
}

QPoint DrawingArea::logicalToWidget(const QPoint &logicalPos) const
{
    // Map from logical coordinates (-300,300) to widget coordinates
    int centerX = width() / 2;
    int centerY = height() / 2;
    
    double xScale = width() / 600.0;
    double yScale = height() / 600.0;
    
    int widgetX = centerX + static_cast<int>(logicalPos.x() * xScale);
    int widgetY = centerY - static_cast<int>(logicalPos.y() * yScale);  // Flip Y as Qt's Y is top-down
    
    return QPoint(widgetX, widgetY);
}

QPoint DrawingArea::widgetToLogical(const QPoint &widgetPos) const
{
    // Map from widget coordinates to logical coordinates (-300,300)
    int centerX = width() / 2;
    int centerY = height() / 2;
    
    double xScale = width() / 600.0;
    double yScale = height() / 600.0;
    
    int logicalX = static_cast<int>((widgetPos.x() - centerX) / xScale);
    int logicalY = static_cast<int>((centerY - widgetPos.y()) / yScale);  // Flip Y
    
    return QPoint(logicalX, logicalY);
}

int DrawingArea::logicalToWidgetSize(int logicalSize) const
{
    // Convert a logical size to widget pixels
    double scale = qMin(width(), height()) / 600.0;
    return static_cast<int>(logicalSize * scale);
}

void DrawingArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw a grid (optional, for visualization purposes)
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    
    // Draw X and Y axes
    QPoint origin = logicalToWidget(QPoint(0, 0));
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(0, origin.y(), width(), origin.y());  // X-axis
    painter.drawLine(origin.x(), 0, origin.x(), height()); // Y-axis
    
    // Draw area circles (draw first so they're in the background)
    for (const AreaCircle &circle : areaCircles) {
        QPoint center = logicalToWidget(circle.center);
        int radius = logicalToWidgetSize(circle.radius);
        drawAreaCircle(painter, center, radius, circle.color);
    }
    
    // Draw points and their circles
    for (const PointData &point : points) {
        QPoint pos = logicalToWidget(point.logicalPos);
        
        // Draw circle around point if needed
        if (point.hasCircle) {
            drawPointCircle(painter, pos, point.circleColor);
        }
        
        // Draw the symbol
        drawSymbol(painter, pos, point.color, point.symbolType, symbolSize);
    }
}

void DrawingArea::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Adjust symbol size based on widget size
    symbolSize = qMax(5, qMin(width(), height()) / 60);
}

void DrawingArea::drawSymbol(QPainter &painter, const QPoint &pos, const QColor &color, SymbolType type, int size)
{
    painter.setPen(QPen(color, 2));
    
    int halfSize = size / 2;
    
    switch (type) {
        case SymbolType::Cross:
            painter.drawLine(pos.x() - halfSize, pos.y() - halfSize, 
                            pos.x() + halfSize, pos.y() + halfSize);
            painter.drawLine(pos.x() + halfSize, pos.y() - halfSize, 
                            pos.x() - halfSize, pos.y() + halfSize);
            break;
            
        case SymbolType::Plus:
            painter.drawLine(pos.x() - halfSize, pos.y(), pos.x() + halfSize, pos.y());
            painter.drawLine(pos.x(), pos.y() - halfSize, pos.x(), pos.y() + halfSize);
            break;
            
        case SymbolType::Star:
            painter.drawLine(pos.x() - halfSize, pos.y(), pos.x() + halfSize, pos.y());
            painter.drawLine(pos.x(), pos.y() - halfSize, pos.x(), pos.y() + halfSize);
            painter.drawLine(pos.x() - halfSize, pos.y() - halfSize, 
                            pos.x() + halfSize, pos.y() + halfSize);
            painter.drawLine(pos.x() + halfSize, pos.y() - halfSize, 
                            pos.x() - halfSize, pos.y() + halfSize);
            break;
    }
}

void DrawingArea::drawPointCircle(QPainter &painter, const QPoint &pos, const QColor &color)
{
    painter.setPen(QPen(color, 1));
    painter.setBrush(Qt::NoBrush);
    int circleSize = symbolSize + 4;  // Just a bit larger than the symbol
    painter.drawEllipse(pos, circleSize, circleSize);
}

void DrawingArea::drawAreaCircle(QPainter &painter, const QPoint &center, int radius, const QColor &color)
{
    QColor fillColor = color;
    fillColor.setAlpha(40);  // Make the fill semi-transparent
    
    painter.setPen(QPen(color, 2));
    painter.setBrush(fillColor);
    painter.drawEllipse(center, radius, radius);
} 