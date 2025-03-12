#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGridLayout>
#include <QPushButton>
#include <QColorDialog>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QSplitter>

#include "drawingarea.h"
#include "controller.h"

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
    void onAddAreaClicked();
    void onRemoveAreaClicked();
    void onColorButtonClicked();
    void onAreaSelectionChanged();
    void onAreaDataChanged();
    void onSplitterMoved(int pos, int index);

private:
    void setupUi();
    void createConnections();
    void updateAreaTable();
    QColor getCurrentColor() const;
    void saveSettings();
    void loadSettings();
    
    Ui::MainWindow *ui;
    DrawingArea *drawingArea;
    Controller *controller;
    
    // UI components for controls
    QWidget *centralWidget;
    QSplitter *mainSplitter;
    QGroupBox *controlsGroup;
    QVBoxLayout *controlsLayout;
    
    // Area definition table
    QTableWidget *areaTable;
    QPushButton *addAreaButton;
    QPushButton *removeAreaButton;
    QColor currentColor;
    
    // Buttons
    QPushButton *clearButton;
    QPushButton *runDemoButton;
    QPushButton *saveButton;
    QPushButton *loadButton;
    QPushButton *generatePointsButton;
    QPushButton *clearPointsButton;
    QPushButton *markOutsideButton;
    
    // Settings
    QString settingsFilePath;
};
#endif // MAINWINDOW_H
