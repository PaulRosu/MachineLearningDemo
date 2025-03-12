#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QItemDelegate>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

// Custom delegate for color column
class ColorDelegate : public QItemDelegate
{
public:
    ColorDelegate(QObject *parent = nullptr) : QItemDelegate(parent) {}
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override {
        QPushButton *button = new QPushButton(parent);
        button->setText("Choose...");
        return button;
    }
    
    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        QPushButton *button = static_cast<QPushButton*>(editor);
        QColor color = index.model()->data(index, Qt::BackgroundRole).value<QColor>();
        if (color.isValid()) {
            QString styleSheet = QString("background-color: %1;").arg(color.name());
            button->setStyleSheet(styleSheet);
        }
    }
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QColor color = index.model()->data(index, Qt::BackgroundRole).value<QColor>();
        if (color.isValid()) {
            painter->fillRect(option.rect, color);
            painter->setPen(Qt::black);
            painter->drawRect(option.rect);
        }
    }
};

// Custom delegate for symbol type column
class SymbolDelegate : public QItemDelegate
{
public:
    SymbolDelegate(QObject *parent = nullptr) : QItemDelegate(parent) {}
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override {
        QComboBox *comboBox = new QComboBox(parent);
        comboBox->addItem("Cross (x)", static_cast<int>(SymbolType::Cross));
        comboBox->addItem("Plus (+)", static_cast<int>(SymbolType::Plus));
        comboBox->addItem("Star (*)", static_cast<int>(SymbolType::Star));
        return comboBox;
    }
    
    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        QComboBox *comboBox = static_cast<QComboBox*>(editor);
        int value = index.model()->data(index, Qt::UserRole).toInt();
        int comboIndex = comboBox->findData(value);
        if (comboIndex >= 0) {
            comboBox->setCurrentIndex(comboIndex);
        }
    }
    
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                     const QModelIndex &index) const override {
        QComboBox *comboBox = static_cast<QComboBox*>(editor);
        int value = comboBox->currentData().toInt();
        model->setData(index, comboBox->currentText(), Qt::DisplayRole);
        model->setData(index, value, Qt::UserRole);
    }
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QItemDelegate::paint(painter, option, index);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , drawingArea(new DrawingArea(this))
    , controller(new Controller(this))
    , currentColor(Qt::blue)
{
    ui->setupUi(this);
    
    // Set up settings file path to be relative to the application directory
    QString appDir = QCoreApplication::applicationDirPath();
    settingsFilePath = appDir + "/appSettings.ini";
    
    // Set window title
    setWindowTitle(tr("Machine Learning Visualization"));
    
    // Set minimum size
    setMinimumSize(1000, 600);
    
    // Set up our custom UI
    setupUi();
    
    // Connect controller
    controller->setDrawingArea(drawingArea);
    createConnections();
    
    // Update the area table with existing areas from the controller
    updateAreaTable();
    
    // Load application settings
    loadSettings();
}

MainWindow::~MainWindow()
{
    // Save settings before closing
    saveSettings();
    delete ui;
}

void MainWindow::setupUi()
{
    // Create central widget
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main layout with splitter
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    centralWidget->setLayout(layout);
    
    // Create splitter
    mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    layout->addWidget(mainSplitter);
    
    // Add drawing area (left side)
    mainSplitter->addWidget(drawingArea);
    drawingArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Create controls group (right side)
    controlsGroup = new QGroupBox(tr("Controls"), centralWidget);
    controlsLayout = new QVBoxLayout(controlsGroup);
    controlsGroup->setLayout(controlsLayout);
    mainSplitter->addWidget(controlsGroup);
    
    // Set initial sizes for splitter
    QList<int> sizes;
    sizes << 700 << 300; // Default 70/30 split
    mainSplitter->setSizes(sizes);
    
    // Make splitter handle more visible
    mainSplitter->setHandleWidth(5);
    mainSplitter->setChildrenCollapsible(false);
    
    // Add table for area definitions
    QLabel *areaLabel = new QLabel(tr("Area Definitions:"), controlsGroup);
    controlsLayout->addWidget(areaLabel);
    
    areaTable = new QTableWidget(0, 7, controlsGroup);
    areaTable->setHorizontalHeaderLabels(QStringList() << "Area #" << "Center X" << "Center Y" 
                                       << "Sigma X" << "Sigma Y" << "Symbol" << "Color");
    areaTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    areaTable->verticalHeader()->setVisible(false);
    areaTable->setItemDelegateForColumn(5, new SymbolDelegate(this));
    areaTable->setItemDelegateForColumn(6, new ColorDelegate(this));
    areaTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    areaTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    areaTable->setSelectionMode(QAbstractItemView::SingleSelection);
    controlsLayout->addWidget(areaTable);
    
    // Add area buttons
    QHBoxLayout *areaButtonLayout = new QHBoxLayout();
    addAreaButton = new QPushButton(tr("Add Area"), controlsGroup);
    removeAreaButton = new QPushButton(tr("Remove Area"), controlsGroup);
    removeAreaButton->setEnabled(false); // Initially disabled until an area is selected
    
    areaButtonLayout->addWidget(addAreaButton);
    areaButtonLayout->addWidget(removeAreaButton);
    controlsLayout->addLayout(areaButtonLayout);
    
    // Add a separator
    QFrame *line = new QFrame(controlsGroup);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    controlsLayout->addWidget(line);
    
    // Point generation and control buttons
    generatePointsButton = new QPushButton(tr("Generate Points"), controlsGroup);
    controlsLayout->addWidget(generatePointsButton);
    
    clearPointsButton = new QPushButton(tr("Clear Points"), controlsGroup);
    controlsLayout->addWidget(clearPointsButton);
    
    // Mark Outside button
    markOutsideButton = new QPushButton(tr("Mark Outside"), controlsGroup);
    controlsLayout->addWidget(markOutsideButton);
    
    // Clear button
    clearButton = new QPushButton(tr("Clear Canvas"), controlsGroup);
    controlsLayout->addWidget(clearButton);
    
    // Load button
    loadButton = new QPushButton(tr("Load Points"), controlsGroup);
    controlsLayout->addWidget(loadButton);
    
    // Add a spacer to separate control sections
    controlsLayout->addSpacing(20);
    
    // Add information label about the coordinate system
    QLabel *infoLabel = new QLabel(tr(
        "Coordinate System:\n"
        "- Logical grid: 600x600 points\n"
        "- Origin (0,0) at center\n"
        "- X: -300 (left) to +300 (right)\n"
        "- Y: -300 (bottom) to +300 (top)\n\n"
        "For each area, define:\n"
        "- Center X and Y (center coordinates)\n"
        "- Sigma X and Y (dispersion parameters)\n"
        "- Symbol for visualization (+ by default)\n"
        "- Color for visualization\n\n"
        "When generating points, 10,000 points will be distributed equally among all defined areas."
    ), controlsGroup);
    infoLabel->setWordWrap(true);
    controlsLayout->addWidget(infoLabel);
    
    // Add stretch to push controls to the top
    controlsLayout->addStretch();
}

void MainWindow::createConnections()
{
    // Connect area buttons
    connect(addAreaButton, &QPushButton::clicked, this, &MainWindow::onAddAreaClicked);
    connect(removeAreaButton, &QPushButton::clicked, this, &MainWindow::onRemoveAreaClicked);
    connect(areaTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onAreaSelectionChanged);
    connect(areaTable, &QTableWidget::itemChanged, this, &MainWindow::onAreaDataChanged);
    
    // Connect UI controls to controller slots
    connect(clearButton, &QPushButton::clicked, controller, &Controller::onClearCanvas);
    connect(loadButton, &QPushButton::clicked, controller, &Controller::onLoadDrawing);
    connect(generatePointsButton, &QPushButton::clicked, controller, &Controller::onGeneratePoints);
    connect(clearPointsButton, &QPushButton::clicked, controller, &Controller::onClearPoints);
    connect(markOutsideButton, &QPushButton::clicked, controller, &Controller::onMarkOutsidePoints);
    
    // Connect splitter movement
    connect(mainSplitter, &QSplitter::splitterMoved, this, &MainWindow::onSplitterMoved);
}

void MainWindow::onSplitterMoved(int pos, int index)
{
    // Save splitter position when moved
    saveSettings();
}

void MainWindow::saveSettings()
{
    QSettings settings(settingsFilePath, QSettings::IniFormat);
    
    // Save splitter position
    settings.setValue("SplitterSizes", mainSplitter->saveState());
    
    // Save window geometry
    settings.setValue("WindowGeometry", saveGeometry());
}

void MainWindow::loadSettings()
{
    QSettings settings(settingsFilePath, QSettings::IniFormat);
    
    // Restore splitter position
    if (settings.contains("SplitterSizes")) {
        mainSplitter->restoreState(settings.value("SplitterSizes").toByteArray());
    }
    
    // Restore window geometry
    if (settings.contains("WindowGeometry")) {
        restoreGeometry(settings.value("WindowGeometry").toByteArray());
    }
}

void MainWindow::updateAreaTable()
{
    // Disconnect to prevent signals during update
    disconnect(areaTable, &QTableWidget::itemChanged, this, &MainWindow::onAreaDataChanged);
    
    // Clear existing rows
    areaTable->clearContents();
    areaTable->setRowCount(0);
    
    // Add rows for each area definition
    for (int i = 0; i < controller->getAreaDefinitionsCount(); i++) {
        AreaDefinition area = controller->getAreaDefinition(i);
        
        int row = areaTable->rowCount();
        areaTable->insertRow(row);
        
        // Area number
        QTableWidgetItem *areaNumItem = new QTableWidgetItem(QString::number(area.areaNumber));
        areaNumItem->setTextAlignment(Qt::AlignCenter);
        areaTable->setItem(row, 0, areaNumItem);
        
        // Center X
        QTableWidgetItem *centerXItem = new QTableWidgetItem(QString::number(area.centerX));
        centerXItem->setTextAlignment(Qt::AlignCenter);
        areaTable->setItem(row, 1, centerXItem);
        
        // Center Y
        QTableWidgetItem *centerYItem = new QTableWidgetItem(QString::number(area.centerY));
        centerYItem->setTextAlignment(Qt::AlignCenter);
        areaTable->setItem(row, 2, centerYItem);
        
        // Sigma X
        QTableWidgetItem *sigmaXItem = new QTableWidgetItem(QString::number(area.sigmaX));
        sigmaXItem->setTextAlignment(Qt::AlignCenter);
        areaTable->setItem(row, 3, sigmaXItem);
        
        // Sigma Y
        QTableWidgetItem *sigmaYItem = new QTableWidgetItem(QString::number(area.sigmaY));
        sigmaYItem->setTextAlignment(Qt::AlignCenter);
        areaTable->setItem(row, 4, sigmaYItem);
        
        // Symbol Type
        QString symbolText;
        switch (area.symbolType) {
            case SymbolType::Cross: symbolText = "Cross (x)"; break;
            case SymbolType::Plus: symbolText = "Plus (+)"; break;
            case SymbolType::Star: symbolText = "Star (*)"; break;
        }
        QTableWidgetItem *symbolItem = new QTableWidgetItem(symbolText);
        symbolItem->setTextAlignment(Qt::AlignCenter);
        symbolItem->setData(Qt::UserRole, static_cast<int>(area.symbolType));
        areaTable->setItem(row, 5, symbolItem);
        
        // Color
        QTableWidgetItem *colorItem = new QTableWidgetItem("");
        colorItem->setBackground(area.color);
        colorItem->setData(Qt::BackgroundRole, area.color);
        areaTable->setItem(row, 6, colorItem);
    }
    
    // Reconnect signals
    connect(areaTable, &QTableWidget::itemChanged, this, &MainWindow::onAreaDataChanged);
}

QColor MainWindow::getCurrentColor() const
{
    return currentColor;
}

void MainWindow::onAddAreaClicked()
{
    // Create a new area definition with default values
    AreaDefinition newArea;
    newArea.areaNumber = controller->getAreaDefinitionsCount() + 1;
    newArea.centerX = 0;
    newArea.centerY = 0;
    newArea.sigmaX = 50;
    newArea.sigmaY = 50;
    newArea.symbolType = SymbolType::Plus; // Default to Plus symbol
    newArea.color = currentColor;
    
    // Add to controller
    controller->addAreaDefinition(newArea);
    
    // Update table
    updateAreaTable();
    
    // Select the newly added row
    areaTable->selectRow(areaTable->rowCount() - 1);
}

void MainWindow::onRemoveAreaClicked()
{
    QList<QTableWidgetItem*> selectedItems = areaTable->selectedItems();
    if (!selectedItems.isEmpty()) {
        int row = selectedItems.first()->row();
        controller->removeAreaDefinition(row);
        updateAreaTable();
    }
}

void MainWindow::onColorButtonClicked()
{
    QColor color = QColorDialog::getColor(currentColor, this, tr("Select Area Color"), 
                                         QColorDialog::ShowAlphaChannel);
    
    if (color.isValid()) {
        currentColor = color;
        
        // Update the color for the selected row
        QList<QTableWidgetItem*> selectedItems = areaTable->selectedItems();
        if (!selectedItems.isEmpty()) {
            int row = selectedItems.first()->row();
            
            // Get the current area definition
            AreaDefinition area = controller->getAreaDefinition(row);
            area.color = color;
            
            // Update in controller
            controller->updateAreaDefinition(row, area);
            
            // Update table
            updateAreaTable();
            
            // Reselect the row
            areaTable->selectRow(row);
        }
    }
}

void MainWindow::onAreaSelectionChanged()
{
    // Enable/disable remove button based on selection
    removeAreaButton->setEnabled(!areaTable->selectedItems().isEmpty());
    
    // If a row is selected and the color column is clicked, show color dialog
    QList<QTableWidgetItem*> selectedItems = areaTable->selectedItems();
    if (!selectedItems.isEmpty()) {
        int row = selectedItems.first()->row();
        int column = areaTable->currentColumn();
        
        // Check if the selected cell is the color cell (column 6)
        if (column == 6) {
            // Get the current color from the table
            QColor color = areaTable->item(row, column)->background().color();
            if (color.isValid()) {
                currentColor = color;
            }
            
            // Show color dialog
            onColorButtonClicked();
        }
    }
}

void MainWindow::onAreaDataChanged()
{
    // Get the changed row
    int row = areaTable->currentRow();
    if (row >= 0) {
        // Get data from table
        AreaDefinition area;
        area.areaNumber = areaTable->item(row, 0)->text().toInt();
        area.centerX = areaTable->item(row, 1)->text().toDouble();
        area.centerY = areaTable->item(row, 2)->text().toDouble();
        area.sigmaX = areaTable->item(row, 3)->text().toDouble();
        area.sigmaY = areaTable->item(row, 4)->text().toDouble();
        
        // Get symbol type from the item
        area.symbolType = static_cast<SymbolType>(areaTable->item(row, 5)->data(Qt::UserRole).toInt());
        
        // Get color from the color item
        QColor color = areaTable->item(row, 6)->background().color();
        area.color = color;
        
        // Update the area definition in the controller
        controller->updateAreaDefinition(row, area);
    }
}
