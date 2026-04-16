#include "projectmodel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
QStringList imageFilters(void)
{
    return QStringList({"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.tif", "*.tiff"});
}
}

ProjectModel::ProjectModel(QObject *parent)
    : QObject(parent) {}

bool ProjectModel::loadDataset(const QString &datasetDirPath)
{
    QDir dir(datasetDirPath);
    if(!dir.exists())
    {
        return false;
    }

    datasetDir = dir.absolutePath();
    images.clear();
    classNames.clear();
    currentIndex = -1;

    const QFileInfoList fileList = dir.entryInfoList(imageFilters(), QDir::Files, QDir::Name);
    images.reserve(fileList.size());
    for(const QFileInfo &fi : fileList)
    {
        ImageEntry entry;
        entry.absolutePath = fi.absoluteFilePath();
        images.push_back(entry);
    }

    loadProjectStateIfExists();

    if(!images.isEmpty())
    {
        currentIndex = 0;
    }

    emit datasetLoaded();
    emit classListChanged();
    emit currentImageChanged();
    emit annotationsChanged();
    return true;
}

bool ProjectModel::saveProjectState(QString *errorMessage) const
{
    if(datasetDir.isEmpty())
    {
        if(errorMessage)
        {
            *errorMessage = "Dataset is not loaded.";
        }
        return false;
    }

    QDir dataset(datasetDir);
    if(!dataset.mkpath(".annotate"))
    {
        if(errorMessage)
        {
            *errorMessage = "Failed to create .annotate directory.";
        }
        return false;
    }

    QJsonObject root;
    QJsonArray classArray;
    for(const QString &name : classNames)
    {
        classArray.append(name);
    }
    root.insert("classes", classArray);

    QJsonObject imagesObject;
    for(const ImageEntry &img : images)
    {
        QJsonArray boxArray;
        for(const BoundingBox &box : img.boxes)
        {
            QJsonObject boxObj;
            boxObj.insert("x", box.rect.x());
            boxObj.insert("y", box.rect.y());
            boxObj.insert("w", box.rect.width());
            boxObj.insert("h", box.rect.height());
            boxObj.insert("classId", box.classId);
            boxArray.append(boxObj);
        }
        imagesObject.insert(QFileInfo(img.absolutePath).fileName(), boxArray);
    }
    root.insert("images", imagesObject);

    QFile file(stateFilePath());
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if(errorMessage)
        {
            *errorMessage = "Failed to write state file.";
        }
        return false;
    }

    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ProjectModel::addClassName(const QString &className)
{
    const QString cleaned = className.trimmed();
    if(cleaned.isEmpty() || classNames.contains(cleaned, Qt::CaseInsensitive))
    {
        return false;
    }

    classNames.push_back(cleaned);
    emit classListChanged();
    return true;
}

const QStringList &ProjectModel::getClassNames() const
{
    return classNames;
}

const QVector<ImageEntry> &ProjectModel::getImages() const
{
    return images;
}

int ProjectModel::getCurrentIndex(void) const
{
    return currentIndex;
}

void ProjectModel::setCurrentIndex(int index)
{
    if(index < 0 || index >= images.size() || index == currentIndex)
    {
        return;
    }
    currentIndex = index;
    emit currentImageChanged();
    emit annotationsChanged();
}

ImageEntry ProjectModel::currentImage() const
{
    if(currentIndex < 0 || currentIndex >= images.size())
    {
        return {};
    }
    return images[currentIndex];
}

QVector<BoundingBox> ProjectModel::currentBoxes() const
{
    if(currentIndex < 0 || currentIndex >= images.size())
    {
        return {};
    }
    return images[currentIndex].boxes;
}

void ProjectModel::setCurrentBoxes(const QVector<BoundingBox> &boxes)
{
    if(currentIndex < 0 || currentIndex >= images.size())
    {
        return;
    }
    images[currentIndex].boxes = boxes;
    emit annotationsChanged();
}

void ProjectModel::addCurrentBox(const BoundingBox &box)
{
    if(currentIndex < 0 || currentIndex >= images.size())
    {
        return;
    }
    images[currentIndex].boxes.push_back(box);
    emit annotationsChanged();
}

bool ProjectModel::removeCurrentBox(int boxIndex) {
    if(currentIndex < 0 || currentIndex >= images.size())
    {
        return false;
    }
    QVector<BoundingBox> &boxes = images[currentIndex].boxes;
    if(boxIndex < 0 || boxIndex >= boxes.size())
    {
        return false;
    }
    boxes.removeAt(boxIndex);
    emit annotationsChanged();
    return true;
}

QString ProjectModel::getDatasetDir(void) const
{
    return datasetDir;
}

void ProjectModel::loadProjectStateIfExists(void)
{
    QFile file(stateFilePath());
    if(!file.exists())
    {
        return;
    }
    if(!file.open(QIODevice::ReadOnly))
    {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if(!doc.isObject())
    {
        return;
    }

    const QJsonObject root = doc.object();
    const QJsonArray classArray = root.value("classes").toArray();
    for(const QJsonValue &val : classArray)
    {
        const QString name = val.toString().trimmed();
        if(!name.isEmpty())
        {
            classNames.push_back(name);
        }
    }

    const QJsonObject imagesObject = root.value("images").toObject();
    for(ImageEntry &img : images)
    {
        const QString key = QFileInfo(img.absolutePath).fileName();
        const QJsonArray boxArray = imagesObject.value(key).toArray();
        for(const QJsonValue &boxVal : boxArray)
        {
            const QJsonObject boxObj = boxVal.toObject();
            BoundingBox box;
            box.rect = QRect(
                boxObj.value("x").toInt(),
                boxObj.value("y").toInt(),
                boxObj.value("w").toInt(),
                boxObj.value("h").toInt());
            box.classId = boxObj.value("classId").toInt(-1);
            if(box.rect.width() > 0 && box.rect.height() > 0 && box.classId >= 0)
            {
                img.boxes.push_back(box);
            }
        }
    }
}

QString ProjectModel::stateFilePath(void) const
{
    return QDir(datasetDir).absoluteFilePath(".annotate/project.json");
}
