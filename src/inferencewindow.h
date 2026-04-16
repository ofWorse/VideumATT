#pragma once

#include "detectorengine.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

class InferenceWindow : public QWidget {
    Q_OBJECT
public:
    explicit InferenceWindow(QWidget *parent = nullptr);
    ~InferenceWindow() override;

private slots:
    void browseModel();
    void loadModel();
    void browseImage();
    void browseVideo();
    void startCamera();
    void stopStream();
    void processNextFrame();

private:
    void buildUi();
    void appendLog(const QString &line);
    void showFrameWithDetections(const cv::Mat &frame, const QVector<Detection> &detections);
    void updateDetectionsList(const QVector<Detection> &detections);
    cv::Mat drawDetections(const cv::Mat &frame, const QVector<Detection> &detections) const;
    QImage matToQImage(const cv::Mat &frame) const;
    QString classNameForId(int classId) const;
    bool openVideoCapture(const QString &sourceName, int cameraIndex = -1);
    void closeVideoCapture();

    DetectorEngine detector_;
    cv::VideoCapture capture_;
    QTimer timer_;
    QString currentStreamName_;
    QStringList classNames_;

    QLineEdit *modelPathEdit_ = nullptr;
    QPushButton *modelBrowseBtn_ = nullptr;
    QPushButton *modelLoadBtn_ = nullptr;
    QLineEdit *classesPathEdit_ = nullptr;

    QDoubleSpinBox *confSpin_ = nullptr;
    QDoubleSpinBox *nmsSpin_ = nullptr;

    QPushButton *imageBtn_ = nullptr;
    QPushButton *videoBtn_ = nullptr;
    QComboBox *cameraCombo_ = nullptr;
    QPushButton *cameraBtn_ = nullptr;
    QPushButton *stopBtn_ = nullptr;

    QLabel *frameLabel_ = nullptr;
    QListWidget *detectionsList_ = nullptr;
    QTextEdit *logEdit_ = nullptr;
};
