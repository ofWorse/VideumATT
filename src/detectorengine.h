#pragma once

#include <QString>
#include <QVector>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

struct Detection {
    int classId = -1;
    float confidence = 0.0f;
    cv::Rect box;
};

class DetectorEngine {
public:
    bool loadModel(const QString &modelPath, QString *errorMessage);
    bool isLoaded() const;
    QVector<Detection> detect(const cv::Mat &frame,
                              float confThreshold,
                              float nmsThreshold,
                              QString *errorMessage);

private:
    QVector<Detection> decodeYoloV8(const cv::Mat &output,
                                    const cv::Size &frameSize,
                                    float confThreshold,
                                    float nmsThreshold) const;

    cv::dnn::Net net_;
    cv::Size inputSize_ = cv::Size(640, 640);
};
