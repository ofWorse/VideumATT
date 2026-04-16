#pragma once

#include <QObject>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QVector>

struct BoundingBox
{
    QRect rect;
    int classId = -1;
};

struct ImageEntry
{
    QString absolutePath;
    QVector<BoundingBox> boxes;
};

class ProjectModel : public QObject
{
    Q_OBJECT

private:
    QString datasetDir;
    QStringList classNames;
    QVector<ImageEntry> images;
    int currentIndex = -1;

public:
    explicit ProjectModel(QObject *parent = nullptr);

    bool loadDataset(const QString &datasetDir);
    bool saveProjectState(QString *errorMessage = nullptr) const;

    bool addClassName(const QString &className);

    const QStringList &getClassNames(void) const;
    const QVector<ImageEntry> &getImages(void) const;

    int getCurrentIndex(void) const;
    void setCurrentIndex(int index);

    ImageEntry currentImage(void) const;
    QVector<BoundingBox> currentBoxes(void) const;
    void setCurrentBoxes(const QVector<BoundingBox> &boxes);
    void addCurrentBox(const BoundingBox &box);
    bool removeCurrentBox(int boxIndex);

    QString getDatasetDir(void) const;

private:
    void loadProjectStateIfExists(void);
    QString stateFilePath(void) const;

signals:
    void datasetLoaded(void);
    void classListChanged(void);
    void currentImageChanged(void);
    void annotationsChanged(void);
};

