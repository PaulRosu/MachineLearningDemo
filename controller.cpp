#include "controller.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QPainter>
#include <QRandomGenerator>
#include <QDir>
#include <QStandardPaths>
#include <QtMath>
#include <QDateTime>
#include <QApplication>
#include <QCoreApplication>

Controller::Controller(QObject *parent)
    : QObject(parent)
    , drawingArea(nullptr)
{
    // Set up file paths to be relative to the application directory instead of AppData
    QString appDir = QCoreApplication::applicationDirPath();
    
    settingsFilePath = appDir + "/areaDefinitions.ini";
    pointsFilePath = appDir + "/points.csv";
    
    // Load settings and points if they exist
    loadSettings();
    loadPoints();
}

Controller::~Controller()
{
    // Save settings and points when the application closes
    saveSettings();
    savePoints();
}

void Controller::setDrawingArea(DrawingArea *area)
{
    drawingArea = area;
    
    // Redraw all area circles
    redrawAreaCircles();
    
    // Draw all saved points
    redrawPoints();
}

void Controller::clearCanvas()
{
    if (drawingArea) {
        // Only clear the points, not the area circles
        drawingArea->clearPoints();
        
        // Redraw area circles
        redrawAreaCircles();
    }
}

void Controller::redrawAreaCircles()
{
    if (!drawingArea) {
        return;
    }
    
    drawingArea->clearAreaCircles();
    
    for (const AreaDefinition &areaDef : areaDefinitions) {
        int radius = qMax(static_cast<int>(areaDef.sigmaX * 3), 
                         static_cast<int>(areaDef.sigmaY * 3));
        drawingArea->addAreaCircle(areaDef.centerX, areaDef.centerY, radius, areaDef.color);
    }
}

void Controller::addPoint(int x, int y, const QColor &color, SymbolType symbol)
{
    if (drawingArea) {
        drawingArea->addPoint(x, y, color, symbol);
    }
}

void Controller::addPointWithCircle(int x, int y, const QColor &pointColor, 
                                  SymbolType symbol, const QColor &circleColor)
{
    if (drawingArea) {
        drawingArea->addPointWithCircle(x, y, pointColor, symbol, circleColor);
    }
}

void Controller::addAreaCircle(int x, int y, int radius, const QColor &color)
{
    if (drawingArea) {
        drawingArea->addAreaCircle(x, y, radius, color);
    }
}

void Controller::addAreaDefinition(const AreaDefinition &area)
{
    areaDefinitions.append(area);
    
    if (drawingArea) {
        int radius = qMax(static_cast<int>(area.sigmaX * 3), 
                         static_cast<int>(area.sigmaY * 3));
        drawingArea->addAreaCircle(area.centerX, area.centerY, radius, area.color);
    }
    
    saveSettings();
}

void Controller::updateAreaDefinition(int row, const AreaDefinition &area)
{
    if (row >= 0 && row < areaDefinitions.size()) {
        areaDefinitions[row] = area;
        
        // Redraw area circles
        redrawAreaCircles();
        
        saveSettings();
    }
}

void Controller::removeAreaDefinition(int row)
{
    if (row >= 0 && row < areaDefinitions.size()) {
        areaDefinitions.removeAt(row);
        
        // Redraw area circles
        redrawAreaCircles();
        
        saveSettings();
    }
}

int Controller::getAreaDefinitionsCount() const
{
    return areaDefinitions.size();
}

AreaDefinition Controller::getAreaDefinition(int row) const
{
    if (row >= 0 && row < areaDefinitions.size()) {
        return areaDefinitions[row];
    }
    
    // Return a default area definition if row is out of bounds
    AreaDefinition defaultArea;
    defaultArea.areaNumber = 0;
    defaultArea.centerX = 0;
    defaultArea.centerY = 0;
    defaultArea.sigmaX = 50;
    defaultArea.sigmaY = 50;
    defaultArea.symbolType = SymbolType::Plus; // Default to Plus symbol
    defaultArea.color = Qt::blue;
    return defaultArea;
}

void Controller::saveSettings()
{
    QSettings settings(settingsFilePath, QSettings::IniFormat);
    
    settings.beginWriteArray("AreaDefinitions");
    for (int i = 0; i < areaDefinitions.size(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("AreaNumber", areaDefinitions[i].areaNumber);
        settings.setValue("CenterX", areaDefinitions[i].centerX);
        settings.setValue("CenterY", areaDefinitions[i].centerY);
        settings.setValue("SigmaX", areaDefinitions[i].sigmaX);
        settings.setValue("SigmaY", areaDefinitions[i].sigmaY);
        settings.setValue("SymbolType", static_cast<int>(areaDefinitions[i].symbolType));
        settings.setValue("Color", areaDefinitions[i].color);
    }
    settings.endArray();
}

void Controller::loadSettings()
{
    QSettings settings(settingsFilePath, QSettings::IniFormat);
    
    areaDefinitions.clear();
    
    int size = settings.beginReadArray("AreaDefinitions");
    for (int i = 0; i < size; i++) {
        settings.setArrayIndex(i);
        AreaDefinition area;
        area.areaNumber = settings.value("AreaNumber").toInt();
        
        // Support both old (MeanX) and new (CenterX) settings keys
        if (settings.contains("CenterX")) {
            area.centerX = settings.value("CenterX").toDouble();
        } else {
            area.centerX = settings.value("MeanX").toDouble();
        }
        
        if (settings.contains("CenterY")) {
            area.centerY = settings.value("CenterY").toDouble();
        } else {
            area.centerY = settings.value("MeanY").toDouble();
        }
        
        area.sigmaX = settings.value("SigmaX").toDouble();
        area.sigmaY = settings.value("SigmaY").toDouble();
        
        // Default to Plus if symbolType is not saved
        if (settings.contains("SymbolType")) {
            area.symbolType = static_cast<SymbolType>(settings.value("SymbolType").toInt());
        } else {
            area.symbolType = SymbolType::Plus;
        }
        
        area.color = settings.value("Color").value<QColor>();
        areaDefinitions.append(area);
    }
    settings.endArray();
}

void Controller::savePoints()
{
    QFile file(pointsFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        
        // Write header
        out << "x;y;AreaNumber\n";
        
        // Write points
        for (const PointDataSave &point : generatedPoints) {
            out << QString::number(point.x) << ";" 
                << QString::number(point.y) << ";" 
                << QString::number(point.areaNumber) << "\n";
        }
        
        file.close();
    }
}

void Controller::loadPoints()
{
    generatedPoints.clear();
    
    QFile file(pointsFilePath);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        
        // Skip header line
        if (!in.atEnd()) {
            in.readLine();
        }
        
        // Read points
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(';');
            
            if (parts.size() >= 3) {
                PointDataSave point;
                point.x = parts[0].toInt();
                point.y = parts[1].toInt();
                point.areaNumber = parts[2].toInt();
                generatedPoints.append(point);
            }
        }
        
        file.close();
        
        // Draw the loaded points if we have a drawing area
        redrawPoints();
    }
}

void Controller::redrawPoints()
{
    if (!drawingArea) {
        return;
    }
    
    drawingArea->clearPoints();
    
    for (const PointDataSave &point : generatedPoints) {
        // Find the area color and symbol type
        QColor color = Qt::black;
        SymbolType symbol = SymbolType::Cross;
        
        for (const AreaDefinition &area : areaDefinitions) {
            if (area.areaNumber == point.areaNumber) {
                color = area.color;
                symbol = area.symbolType;
                break;
            }
        }
        
        // Draw point with proper symbol
        drawingArea->addPoint(point.x, point.y, color, symbol);
    }
}

// Calculate Gaussian probability density function value
double Controller::gaussProbability(double x, double center, double sigma) const
{
    // Gaussian probability density function:
    // f(x) = (1 / (sigma * sqrt(2*pi))) * e^(-(x-center)^2 / (2*sigma^2))
    
    // For efficiency, we can simplify and normalize this to max = 1 at x = center
    // f(x) = e^(-(x-center)^2 / (2*sigma^2))
    
    double exponent = -((x - center) * (x - center)) / (2 * sigma * sigma);
    return exp(exponent);
}

// Generate a single coordinate using acceptance-rejection sampling
bool Controller::generateCoordinate(double center, double sigma, int &coordinate)
{
    QRandomGenerator *rng = QRandomGenerator::global();
    
    // Step 2: Randomly choose a value in the range (-300, 300)
    coordinate = rng->bounded(-300, 301);
    
    // Step 3: Calculate the probability using Gauss function
    double probability = gaussProbability(coordinate, center, sigma);
    
    // Step 4: Randomly generate a probability in [0,1]
    double randomProb = rng->generateDouble();
    
    // Step 5: Check if calculated probability > random probability
    return probability > randomProb;
}

// Generate points according to the specification
void Controller::generatePointsAccordingToSpecification()
{
    if (areaDefinitions.size() == 0) {
        QMessageBox::warning(nullptr, tr("No Areas Defined"),
                            tr("Please define at least one area before generating points."));
        return;
    }
    
    if (!drawingArea) {
        return;
    }
    
    QRandomGenerator *rng = QRandomGenerator::global();
    
    // Clear previous points
    generatedPoints.clear();
    drawingArea->clearPoints();
    
    // Make sure area circles are visible
    redrawAreaCircles();
    
    // Calculate total number of points to generate (10000 points total)
    int totalPoints = 10000;
    
    // Calculate how many points to generate for each area
    int pointsPerArea = totalPoints / areaDefinitions.size();
    int remainingPoints = totalPoints % areaDefinitions.size(); // handle any remainder
    
    // Generate points for each area
    for (int areaIndex = 0; areaIndex < areaDefinitions.size(); areaIndex++) {
        AreaDefinition &area = areaDefinitions[areaIndex];
        
        // How many points to generate for this area
        int pointsForArea = pointsPerArea;
        if (areaIndex < remainingPoints) {
            pointsForArea++; // Distribute remainder evenly
        }
        
        // Generate points for this area
        for (int i = 0; i < pointsForArea; i++) {
            // Generate x coordinate
            int x;
            while (!generateCoordinate(area.centerX, area.sigmaX, x)) {
                // Keep trying until we get a successful x coordinate
            }
            
            // Generate y coordinate
            int y;
            while (!generateCoordinate(area.centerY, area.sigmaY, y)) {
                // Keep trying until we get a successful y coordinate
            }
            
            // Add the point to our list
            PointDataSave point;
            point.x = x;
            point.y = y;
            point.areaNumber = area.areaNumber;
            generatedPoints.append(point);
            
            // Draw the point with the area's symbol type
            drawingArea->addPoint(x, y, area.color, area.symbolType);
            
            // Process events every so often to keep UI responsive
            if (i % 100 == 0) {
                QApplication::processEvents();
            }
        }
    }
}

void Controller::onClearCanvas()
{
    clearCanvas();
}

void Controller::onGeneratePoints()
{
    if (!drawingArea) {
        return;
    }
    
    // Generate points using the specified algorithm
    generatePointsAccordingToSpecification();
    
    // Save the generated points
    savePoints();
    
    QMessageBox::information(nullptr, tr("Points Generated"),
                            tr("Generated and saved %1 points.\nPoints are distributed across all defined areas.").arg(generatedPoints.size()));
}

void Controller::onLoadDrawing()
{
    loadPoints();
    QMessageBox::information(nullptr, tr("Load Points"),
                             tr("Loaded %1 points from %2").arg(generatedPoints.size()).arg(pointsFilePath));
}

void Controller::onClearPoints()
{
    if (!drawingArea) {
        return;
    }
    
    // Clear points from the drawing area
    drawingArea->clearPoints();
    
    // Make sure area circles are still visible
    redrawAreaCircles();
    
    // Clear the points list
    generatedPoints.clear();
    
    // Delete the points file
    QFile file(pointsFilePath);
    if (file.exists()) {
        file.remove();
    }
    
    QMessageBox::information(nullptr, tr("Clear Points"),
                            tr("All points have been cleared."));
}

// Helper to check if a point is outside its area
bool Controller::isPointOutsideArea(const PointDataSave &point, const AreaDefinition &area) const
{
    // Define a threshold for determining when a point is "outside" the area
    // A point is considered outside if its probability is less than 5% of the max probability
    const double threshold = 0.05; 
    
    // Calculate the probability at this point's location
    double probX = gaussProbability(point.x, area.centerX, area.sigmaX);
    double probY = gaussProbability(point.y, area.centerY, area.sigmaY);
    
    // The combined probability is the product of the x and y probabilities
    double probability = probX * probY;
    
    // If the probability is below the threshold, the point is considered outside
    return probability < threshold;
}

void Controller::onMarkOutsidePoints()
{
    // Check if there are area definitions and points
    if (areaDefinitions.isEmpty()) {
        QMessageBox::warning(nullptr, tr("No Areas Defined"),
                            tr("Please define at least one area before marking outside points."));
        return;
    }
    
    if (generatedPoints.isEmpty()) {
        QMessageBox::warning(nullptr, tr("No Points"),
                            tr("No points to analyze. Please generate or load points first."));
        return;
    }
    
    if (!drawingArea) {
        return;
    }
    
    // Count of outside points
    int outsideCount = 0;
    
    // First, clear the drawing area
    drawingArea->clearPoints();
    
    // Make sure area circles are visible
    redrawAreaCircles();
    
    // Check each point to see if it's outside its assigned area
    for (const PointDataSave &point : generatedPoints) {
        bool found = false;
        bool isOutside = false;
        QColor color = Qt::black;
        SymbolType symbol = SymbolType::Cross;
        
        // Find the area this point belongs to
        for (const AreaDefinition &area : areaDefinitions) {
            if (area.areaNumber == point.areaNumber) {
                found = true;
                color = area.color;
                symbol = area.symbolType;
                
                // Check if the point is outside its area
                isOutside = isPointOutsideArea(point, area);
                break;
            }
        }
        
        // Draw the point
        if (found && isOutside) {
            // Mark outside points with a circle in the area's color
            drawingArea->addPointWithCircle(point.x, point.y, color, symbol, color);
            outsideCount++;
        } else {
            // Draw normal points as before
            drawingArea->addPoint(point.x, point.y, color, symbol);
        }
    }
    
    // Show information about the results
    QMessageBox::information(nullptr, tr("Outside Points Marked"),
                            tr("Found %1 points outside their assigned areas (from %2 total points).")
                            .arg(outsideCount)
                            .arg(generatedPoints.size()));
} 