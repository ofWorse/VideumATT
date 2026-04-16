#pragma once

#include "projectmodel.h"

#include <QImage>
#include <QVector>
#include <QWidget>

class AnnotationCanvas : public QWidget
{
    Q_OBJECT

private:
    QImage image;
    QVector<BoundingBox> boxes;
    QStringList classNames;
    int activeClassId = -1;

    bool drawing = false;
    QPoint dragStartImage;
    QPoint dragCurrentImage;

public:
    explicit AnnotationCanvas(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void setBoxes(const QVector<BoundingBox> &boxes);
    void setClassNames(const QStringList &classNames);
    void setActiveClassId(int classId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QRect imageTargetRect(void) const;
    QPoint widgetToImage(const QPoint &widgetPoint) const;
    QRect imageRectToWidgetRect(const QRect &imageRect) const;
    QColor colorForClass(int classId) const;

signals:
    void boxCreated(const BoundingBox &box);
};

