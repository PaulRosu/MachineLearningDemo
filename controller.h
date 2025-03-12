#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QColor>
#include <QVector>
#include <QSettings>
#include <QRandomGenerator>
#include <QFile>
#include <QTextStream>
#include "drawingarea.h"

// Structure for area definition
struct AreaDefinition {
    int areaNumber;
    double centerX;
    double centerY;
    double sigmaX;
    double sigmaY;
    SymbolType symbolType;  // Symbol type for this area
    QColor color;
};

// Structure for point data to save
struct PointDataSave {
    int x;
    int y;
    int areaNumber;
};

class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();
    
    // Set the drawing area to control
    void setDrawingArea(DrawingArea *area);
    
    // Area definitions management
    void addAreaDefinition(const AreaDefinition &area);
    void updateAreaDefinition(int row, const AreaDefinition &area);
    void removeAreaDefinition(int row);
    int getAreaDefinitionsCount() const;
    AreaDefinition getAreaDefinition(int row) const;
    
    // Save/load settings
    void saveSettings();
    void loadSettings();
    
    // Save/load points
    void savePoints();
    void loadPoints();
    
    // Redraw points from the saved points list
    void redrawPoints();

public slots:
    // Basic drawing operations
    void clearCanvas();
    void addPoint(int x, int y, const QColor &color, SymbolType symbol);
    void addPointWithCircle(int x, int y, const QColor &pointColor, 
                          SymbolType symbol, const QColor &circleColor);
    void addAreaCircle(int x, int y, int radius, const QColor &color);
    
    // Demo operations for the UI
    void onClearCanvas();
    
    // Points generation 
    void onGeneratePoints();
    
    // File operations
    void onLoadDrawing();
    void onClearPoints();
    void onMarkOutsidePoints();

private:
    DrawingArea *drawingArea;
    QVector<AreaDefinition> areaDefinitions;
    QVector<PointDataSave> generatedPoints;
    
    // Settings and file paths
    QString settingsFilePath;
    QString pointsFilePath;
    
    // Helper methods for point generation
    double gaussProbability(double x, double center, double sigma) const;
    bool generateCoordinate(double center, double sigma, int &coordinate);
    void generatePointsAccordingToSpecification();
    
    // Helper to redraw area circles
    void redrawAreaCircles();
    
    // Helper to check if a point is outside its area
    bool isPointOutsideArea(const PointDataSave &point, const AreaDefinition &area) const;
};

#endif // CONTROLLER_H 