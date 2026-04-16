#include "annotationcanvas.h"

#include <QMouseEvent>
#include <QPainter>

AnnotationCanvas::AnnotationCanvas(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(640, 480);
    setMouseTracking(true);
}

void AnnotationCanvas::setImage(const QImage &image)
{
    this->image = image;
    update();
}

void AnnotationCanvas::setBoxes(const QVector<BoundingBox> &boxes)
{
    this->boxes = boxes;
    update();
}

void AnnotationCanvas::setClassNames(const QStringList &classNames)
{
    this->classNames = classNames;
    update();
}

void AnnotationCanvas::setActiveClassId(int classId)
{
    activeClassId = classId;
}

void AnnotationCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.fillRect(rect(), QColor(24, 24, 24));

    if(image.isNull())
    {
        p.setPen(Qt::lightGray);
        p.drawText(rect(), Qt::AlignCenter, "Load dataset and choose an image.");
        return;
    }

    const QRect dst = imageTargetRect();
    p.drawImage(dst, image);

    for(const BoundingBox &box : boxes)
    {
        const QRect wr = imageRectToWidgetRect(box.rect);
        const QColor c = colorForClass(box.classId);

        p.setPen(QPen(c, 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(wr);

        QString label = QString::number(box.classId);
        if(box.classId >= 0 && box.classId < classNames.size())
        {
            label = classNames[box.classId];
        }

        const QFontMetrics fm(p.font());
        const int textW = fm.horizontalAdvance(label) + 8;
        const int textH = fm.height() + 4;
        QRect lr(wr.left(), wr.top() - textH, textW, textH);
        if(lr.top() < dst.top())
        {
            lr.moveTop(wr.top());
        }

        p.fillRect(lr, c);
        p.setPen(Qt::black);
        p.drawText(lr.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
    }

    if(drawing)
    {
        QRect imageRect(dragStartImage, dragCurrentImage);
        imageRect = imageRect.normalized();
        const QRect wr = imageRectToWidgetRect(imageRect);
        p.setPen(QPen(colorForClass(activeClassId), 2, Qt::DashLine));
        p.drawRect(wr);
    }
}

void AnnotationCanvas::mousePressEvent(QMouseEvent *event)
{
    if(event->button() != Qt::LeftButton || image.isNull() || activeClassId < 0)
    {
        return;
    }
    if(!imageTargetRect().contains(event->pos()))
    {
        return;
    }

    drawing = true;
    dragStartImage = widgetToImage(event->pos());
    dragCurrentImage = dragStartImage;
    update();
}

void AnnotationCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if(!drawing)
    {
        return;
    }
    dragCurrentImage = widgetToImage(event->pos());
    update();
}

void AnnotationCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if(!drawing || event->button() != Qt::LeftButton)
    {
        return;
    }

    drawing = false;
    dragCurrentImage = widgetToImage(event->pos());

    QRect imageRect(dragStartImage, dragCurrentImage);
    imageRect = imageRect.normalized();
    imageRect = imageRect.intersected(QRect(QPoint(0, 0), image.size()));

    if(imageRect.width() >= 4 && imageRect.height() >= 4)
    {
        BoundingBox box;
        box.rect = imageRect;
        box.classId = activeClassId;
        emit boxCreated(box);
    }

    update();
}

QRect AnnotationCanvas::imageTargetRect(void) const
{
    if(image.isNull())
    {
        return {};
    }

    const QSize canvasSize = size();
    const QSize imageSize = image.size();
    const qreal sx = static_cast<qreal>(canvasSize.width()) / imageSize.width();
    const qreal sy = static_cast<qreal>(canvasSize.height()) / imageSize.height();
    const qreal s = qMin(sx, sy);

    const int w = static_cast<int>(imageSize.width() * s);
    const int h = static_cast<int>(imageSize.height() * s);
    const int x = (canvasSize.width() - w) / 2;
    const int y = (canvasSize.height() - h) / 2;
    return QRect(x, y, w, h);
}

QPoint AnnotationCanvas::widgetToImage(const QPoint &widgetPoint) const
{
    if(image.isNull())
    {
        return {};
    }
    const QRect dst = imageTargetRect();
    if(!dst.isValid())
    {
        return {};
    }

    const qreal nx = static_cast<qreal>(widgetPoint.x() - dst.left()) / dst.width();
    const qreal ny = static_cast<qreal>(widgetPoint.y() - dst.top()) / dst.height();
    const int ix = qBound(0, static_cast<int>(nx * image.width()), image.width() - 1);
    const int iy = qBound(0, static_cast<int>(ny * image.height()), image.height() - 1);
    return QPoint(ix, iy);
}

QRect AnnotationCanvas::imageRectToWidgetRect(const QRect &imageRect) const
{
    if(image.isNull())
    {
        return {};
    }
    const QRect dst = imageTargetRect();
    const qreal sx = static_cast<qreal>(dst.width()) / image.width();
    const qreal sy = static_cast<qreal>(dst.height()) / image.height();

    const int x = dst.left() + static_cast<int>(imageRect.left() * sx);
    const int y = dst.top() + static_cast<int>(imageRect.top() * sy);
    const int w = static_cast<int>(imageRect.width() * sx);
    const int h = static_cast<int>(imageRect.height() * sy);
    return QRect(x, y, w, h);
}

QColor AnnotationCanvas::colorForClass(int classId) const
{
    if(classId < 0)
    {
        return QColor(255, 255, 255);
    }
    const int hue = (classId * 47) % 360;
    return QColor::fromHsv(hue, 220, 255);
}

