#pragma once

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>

#include <string>
#include <vector>

struct Detection {
    cv::Rect box;
    int classId = -1;
    float confidence = 0.0f;
};

class DetectorOnnxOpenCV {
public:
    DetectorOnnxOpenCV() = default;
    explicit DetectorOnnxOpenCV(const std::string &onnxPath);

    bool load(const std::string &onnxPath);
    std::vector<Detection> infer(const cv::Mat &frame,
                                 float confThreshold = 0.25f,
                                 float nmsThreshold = 0.45f) const;

private:
    static std::vector<int> nmsIndices(const std::vector<cv::Rect> &boxes,
                                       const std::vector<float> &scores,
                                       float confThreshold,
                                       float nmsThreshold);

    cv::dnn::Net net_;
    int inputWidth_ = 640;
    int inputHeight_ = 640;
};

