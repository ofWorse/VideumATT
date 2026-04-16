#include "mainwindow.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>

namespace
{
QString boxToLabelLine(const BoundingBox &box, const QSize &imageSize)
{
    const double xCenter = (box.rect.x() + box.rect.width() * 0.5) / imageSize.width();
    const double yCenter = (box.rect.y() + box.rect.height() * 0.5) / imageSize.height();
    const double w = static_cast<double>(box.rect.width()) / imageSize.width();
    const double h = static_cast<double>(box.rect.height()) / imageSize.height();
    return QString("%1 %2 %3 %4 %5")
        .arg(box.classId)
        .arg(xCenter, 0, 'f', 6)
        .arg(yCenter, 0, 'f', 6)
        .arg(w, 0, 'f', 6)
        .arg(h, 0, 'f', 6);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent) {
    setWindowTitle("VideumATT");
    resize(1480, 860);

    chooseDatasetBtn = new QPushButton("Open Dataset Folder", this);
    datasetLabel = new QLabel("Dataset: not selected", this);
    imageList = new QListWidget(this);
    imageList->setMinimumWidth(250);

    classNameEdit = new QLineEdit(this);
    classNameEdit->setPlaceholderText("New object class name...");
    addClassBtn = new QPushButton("Add Class", this);
    classList = new QListWidget(this);
    activeClassCombo = new QComboBox(this);
    activeClassCombo->addItem("Select class to annotate");

    canvas = new AnnotationCanvas(this);
    annotationList = new QListWidget(this);
    removeBoxBtn = new QPushButton("Remove Selected Box", this);

    prevBtn = new QPushButton("Prev Image", this);
    nextBtn = new QPushButton("Next Image", this);
    saveBtn = new QPushButton("Save Project State", this);
    trainBtn = new QPushButton("Start train", this);
    trainLog = new QTextEdit(this);
    trainLog->setReadOnly(true);
    trainLog->setMinimumHeight(180);

    auto *root = new QVBoxLayout(this);
    auto *topControls = new QHBoxLayout();
    topControls->addWidget(chooseDatasetBtn);
    topControls->addWidget(datasetLabel, 1);
    root->addLayout(topControls);

    auto *mainRow = new QHBoxLayout();
    auto *left = new QVBoxLayout();
    left->addWidget(new QLabel("Images", this));
    left->addWidget(imageList, 1);
    left->addWidget(new QLabel("Object classes", this));
    left->addWidget(classNameEdit);
    left->addWidget(addClassBtn);
    left->addWidget(classList, 1);
    left->addWidget(new QLabel("Active class", this));
    left->addWidget(activeClassCombo);
    mainRow->addLayout(left, 1);

    auto *center = new QVBoxLayout();
    center->addWidget(canvas, 1);
    auto *nav = new QHBoxLayout();
    nav->addWidget(prevBtn);
    nav->addWidget(nextBtn);
    nav->addWidget(saveBtn);
    nav->addStretch();
    nav->addWidget(trainBtn);
    center->addLayout(nav);
    center->addWidget(new QLabel("Training log", this));
    center->addWidget(trainLog);
    mainRow->addLayout(center, 3);

    auto *right = new QVBoxLayout();
    right->addWidget(new QLabel("Boxes forcurrent image", this));
    right->addWidget(annotationList, 1);
    right->addWidget(removeBoxBtn);
    mainRow->addLayout(right, 1);

    root->addLayout(mainRow, 1);
    setLayout(root);

    connect(chooseDatasetBtn, &QPushButton::clicked, this, &MainWindow::chooseDataset);
    connect(addClassBtn, &QPushButton::clicked, this, &MainWindow::addClass);
    connect(imageList, &QListWidget::itemSelectionChanged, this, &MainWindow::onImageSelectionChanged);
    connect(canvas, &AnnotationCanvas::boxCreated, this, &MainWindow::onBoxCreated);
    connect(removeBoxBtn, &QPushButton::clicked, this, &MainWindow::removeSelectedBox);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveState);
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::prevImage);
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::nextImage);
    connect(trainBtn, &QPushButton::clicked, this, &MainWindow::startTrain);

    connect(&model, &ProjectModel::datasetLoaded, this, &MainWindow::refreshUiFromModel);
    connect(&model, &ProjectModel::classListChanged, this, &MainWindow::refreshUiFromModel);
    connect(&model, &ProjectModel::currentImageChanged, this, &MainWindow::refreshCurrentImage);
    connect(&model, &ProjectModel::annotationsChanged, this, &MainWindow::refreshAnnotationsList);

    connect(activeClassCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx)
    {
        canvas->setActiveClassId(idx - 1);
    });

    connect(&trainer, &TrainManager::logLine, this, &MainWindow::appendLog);
    connect(&trainer, &TrainManager::trainFailedToStart, this, [this](const QString &errorText)
    {
        appendLog(QString("Training failed to start: %1").arg(errorText));
        QMessageBox::critical(this, "Training", errorText);
    });
    connect(&trainer,
            &TrainManager::trainFinished,
            this,
            [this](int exitCode, QProcess::ExitStatus status)
            {
                appendLog(QString("Training finished. Exit code: %1, status: %2")
                              .arg(exitCode)
                              .arg(status == QProcess::NormalExit ? "NormalExit" : "CrashExit"));
                QMessageBox::information(this, "Training", "Training process has finished.");
                trainBtn->setEnabled(true);
            });
}

void MainWindow::chooseDataset(void)
{
    const QString dir = QFileDialog::getExistingDirectory(this, "Select dataset folder");
    if(dir.isEmpty())
    {
        return;
    }

    if(!model.loadDataset(dir))
    {
        QMessageBox::critical(this, "Dataset", "Failed to load the selected folder.");
        return;
    }

    datasetLabel->setText("Dataset: " + model.getDatasetDir());
    appendLog("Loaded dataset from " + model.getDatasetDir());
}

void MainWindow::addClass(void)
{
    if(model.getDatasetDir().isEmpty())
    {
        QMessageBox::warning(this, "Class", "Load dataset first.");
        return;
    }
    if(!model.addClassName(classNameEdit->text()))
    {
        QMessageBox::warning(this, "Class", "Class name is empty or already exists.");
        return;
    }
    classNameEdit->clear();
}

void MainWindow::onImageSelectionChanged(void)
{
    if(imageList->selectedItems().isEmpty())
    {
        return;
    }
    const int row = imageList->row(imageList->selectedItems().first());
    model.setCurrentIndex(row);
}

void MainWindow::onBoxCreated(const BoundingBox &box)
{
    model.addCurrentBox(box);
}

void MainWindow::removeSelectedBox(void)
{
    if(annotationList->selectedItems().isEmpty())
    {
        return;
    }
    const int row = annotationList->row(annotationList->selectedItems().first());
    if(!model.removeCurrentBox(row))
    {
        QMessageBox::warning(this, "Boxes", "Failed to remove selected box.");
    }
}

void MainWindow::saveState(void)
{
    QString err;
    if(!model.saveProjectState(&err))
    {
        QMessageBox::critical(this, "Save", err);
        return;
    }
    appendLog("Project state saved.");
}

void MainWindow::prevImage(void)
{
    if(model.getImages().isEmpty())
    {
        return;
    }
    const int idx = qMax(0, model.getCurrentIndex() - 1);
    model.setCurrentIndex(idx);
    imageList->setCurrentRow(idx);
}

void MainWindow::nextImage(void)
{
    if(model.getImages().isEmpty())
    {
        return;
    }
    const int idx = qMin(model.getImages().size() - 1, model.getCurrentIndex() + 1);
    model.setCurrentIndex(idx);
    imageList->setCurrentRow(idx);
}

void MainWindow::startTrain(void)
{
    if(model.getDatasetDir().isEmpty())
    {
        QMessageBox::warning(this, "Training", "Load dataset first.");
        return;
    }
    if(model.getClassNames().isEmpty())
    {
        QMessageBox::warning(this, "Training", "Add at least one class.");
        return;
    }

    QString saveErr;
    if(!model.saveProjectState(&saveErr))
    {
        QMessageBox::critical(this, "Save", saveErr);
        return;
    }

    QString exportErr;
    const QString datasetYaml = exportYoloDataset(&exportErr);
    if(datasetYaml.isEmpty())
    {
        QMessageBox::critical(this, "Export", exportErr);
        return;
    }

    trainBtn->setEnabled(false);
    appendLog("Launching training...");
#ifdef APP_SOURCE_DIR
    const QString sourceDir = QString::fromUtf8(APP_SOURCE_DIR);
#else
    const QString sourceDir = QCoreApplication::applicationDirPath();
#endif
    if(!trainer.start(sourceDir, datasetYaml))
    {
        trainBtn->setEnabled(true);
    }
}

void MainWindow::refreshUiFromModel(void)
{
    imageList->clear();
    classList->clear();
    activeClassCombo->clear();
    activeClassCombo->addItem("Select class to annotate");

    for(const ImageEntry &img : model.getImages())
    {
        imageList->addItem(QFileInfo(img.absolutePath).fileName());
    }
    for(const QString &name : model.getClassNames())
    {
        classList->addItem(name);
        activeClassCombo->addItem(name);
    }

    canvas->setClassNames(model.getClassNames());

    if(model.getCurrentIndex() >= 0 && model.getCurrentIndex() < imageList->count())
    {
        imageList->setCurrentRow(model.getCurrentIndex());
    }
    refreshCurrentImage();
}

void MainWindow::refreshCurrentImage(void)
{
    const ImageEntry current = model.currentImage();
    if(current.absolutePath.isEmpty())
    {
        canvas->setImage(QImage());
        annotationList->clear();
        return;
    }

    QImageReader reader(current.absolutePath);
    const QImage image = reader.read();
    if(image.isNull())
    {
        appendLog("Failed to load image: " + current.absolutePath);
    }
    canvas->setImage(image);
    canvas->setBoxes(current.boxes);
    refreshAnnotationsList();
}

void MainWindow::refreshAnnotationsList(void)
{
    annotationList->clear();
    const QVector<BoundingBox> boxes = model.currentBoxes();
    for(int i = 0; i < boxes.size(); ++i)
    {
        const BoundingBox &b = boxes[i];
        const QString className =
            (b.classId >= 0 && b.classId < model.getClassNames().size()) ? model.getClassNames()[b.classId]
                                                                        : QString("Class %1").arg(b.classId);
        const QString text = QString("#%1 | %2 | x=%3 y=%4 w=%5 h=%6")
                                 .arg(i)
                                 .arg(className)
                                 .arg(b.rect.x())
                                 .arg(b.rect.y())
                                 .arg(b.rect.width())
                                 .arg(b.rect.height());
        annotationList->addItem(text);
    }
    canvas->setBoxes(boxes);
}

void MainWindow::appendLog(const QString &line)
{
    if(line.isEmpty())
    {
        return;
    }
    const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    trainLog->append(QString("[%1] %2").arg(ts, line));
}

QString MainWindow::exportYoloDataset(QString *errorMessage) const
{
    const QString baseDir = QDir(model.getDatasetDir()).absoluteFilePath(".annotate/export_yolo");
    QDir dir;
    if(!dir.mkpath(baseDir))
    {
        if(errorMessage)
        {
            *errorMessage = "Failed to create export_yolo directory.";
        }
        return {};
    }

    const QString imagesDir = QDir(baseDir).absoluteFilePath("images");
    const QString labelsDir = QDir(baseDir).absoluteFilePath("labels");
    if(!dir.mkpath(imagesDir) || !dir.mkpath(labelsDir))
    {
        if(errorMessage)
        {
            *errorMessage = "Failed to create images/labels directories.";
        }
        return {};
    }

    for(const ImageEntry &entry : model.getImages())
    {
        const QFileInfo fi(entry.absolutePath);
        const QString dstImagePath = QDir(imagesDir).absoluteFilePath(fi.fileName());
        QString copyErr;
        if(!copyImageToExport(entry.absolutePath, dstImagePath, &copyErr))
        {
            if(errorMessage)
            {
                *errorMessage = copyErr;
            }
            return {};
        }

        QImageReader reader(entry.absolutePath);
        const QSize imageSize = reader.size();
        if(!imageSize.isValid())
        {
            if(errorMessage)
            {
                *errorMessage = "Failed to read image size for: " + entry.absolutePath;
            }
            return {};
        }

        const QString labelPath = QDir(labelsDir).absoluteFilePath(fi.completeBaseName() + ".txt");
        QFile labelFile(labelPath);
        if(!labelFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            if(errorMessage)
            {
                *errorMessage = "Failed to write label file: " + labelPath;
            }
            return {};
        }

        QTextStream out(&labelFile);
        for(const BoundingBox &box : entry.boxes)
        {
            if(box.classId < 0 || box.classId >= model.getClassNames().size())
            {
                continue;
            }
            out << boxToLabelLine(box, imageSize) << '\n';
        }
        labelFile.close();
    }

    const QString classesPath = QDir(baseDir).absoluteFilePath("classes.txt");
    QFile classesFile(classesPath);
    if(!classesFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        if(errorMessage)
        {
            *errorMessage = "Failed to write classes.txt.";
        }
        return {};
    }
    QTextStream classOut(&classesFile);
    for(const QString &name : model.getClassNames())
    {
        classOut << name << '\n';
    }
    classesFile.close();

    const QString yamlPath = QDir(baseDir).absoluteFilePath("dataset.yaml");
    QFile yaml(yamlPath);
    if(!yaml.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        if(errorMessage)
        {
            *errorMessage = "Failed to write dataset.yaml.";
        }
        return {};
    }
    QTextStream y(&yaml);
    QString baseDirUnix = baseDir;
    baseDirUnix.replace("\\", "/");
    y << "path: " << baseDirUnix << '\n';
    y << "train: images\n";
    y << "val: images\n";
    y << "nc: " << model.getClassNames().size() << '\n';
    y << "names: [";
    for(int i = 0; i < model.getClassNames().size(); ++i)
    {
        y << '"' << model.getClassNames().at(i) << '"';
        if(i + 1 < model.getClassNames().size())
        {
            y << ", ";
        }
    }
    y << "]\n";
    yaml.close();

    return yamlPath;
}

bool MainWindow::copyImageToExport(const QString &srcPath,
                                   const QString &dstPath,
                                   QString *errorMessage) const
{
    if(QFileInfo::exists(dstPath))
    {
        if(!QFile::remove(dstPath))
        {
            if(errorMessage)
            {
                *errorMessage = "Failed to overwrite image: " + dstPath;
            }
            return false;
        }
    }
    if(!QFile::copy(srcPath, dstPath))
    {
        if(errorMessage)
        {
            *errorMessage = "Failed to copy image: " + srcPath;
        }
        return false;
    }
    return true;
}
