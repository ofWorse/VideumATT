#pragma once

#include "annotationcanvas.h"
#include "projectmodel.h"
#include "trainmanager.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

class MainWindow : public QWidget
{
    Q_OBJECT

private:
    ProjectModel model;
    TrainManager trainer;

    QPushButton *chooseDatasetBtn = nullptr;
    QLabel *datasetLabel = nullptr;
    QListWidget *imageList = nullptr;

    QLineEdit *classNameEdit = nullptr;
    QPushButton *addClassBtn = nullptr;
    QListWidget *classList = nullptr;
    QComboBox *activeClassCombo = nullptr;

    AnnotationCanvas *canvas = nullptr;
    QListWidget *annotationList = nullptr;
    QPushButton *removeBoxBtn = nullptr;

    QPushButton *prevBtn = nullptr;
    QPushButton *nextBtn = nullptr;
    QPushButton *saveBtn = nullptr;
    QPushButton *trainBtn = nullptr;
    QTextEdit *trainLog = nullptr;

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void chooseDataset(void);
    void addClass(void);
    void onImageSelectionChanged(void);
    void onBoxCreated(const BoundingBox &box);
    void removeSelectedBox(void);
    void saveState(void);
    void prevImage(void);
    void nextImage(void);
    void startTrain(void);

    void refreshUiFromModel(void);
    void refreshCurrentImage(void);
    void refreshAnnotationsList(void);
    void appendLog(const QString &line);

private:
    QString exportYoloDataset(QString *errorMessage) const;
    bool copyImageToExport(const QString &srcPath, const QString &dstPath, QString *errorMessage) const;
};

