#include "detector_onnx_opencv.h"

#include <algorithm>
#include <stdexcept>

DetectorOnnxOpenCV::DetectorOnnxOpenCV(const std::string &onnxPath) {
    if (!load(onnxPath)) {
        throw std::runtime_error("Failed to load ONNX model.");
    }
}

bool DetectorOnnxOpenCV::load(const std::string &onnxPath) {
    try {
        net_ = cv::dnn::readNetFromONNX(onnxPath);
    } catch (...) {
        return false;
    }
    return !net_.empty();
}

std::vector<Detection> DetectorOnnxOpenCV::infer(const cv::Mat &frame,
                                                 float confThreshold,
                                                 float nmsThreshold) const {
    std::vector<Detection> detections;
    if (frame.empty() || net_.empty()) {
        return detections;
    }

    cv::Mat blob = cv::dnn::blobFromImage(frame,
                                          1.0 / 255.0,
                                          cv::Size(inputWidth_, inputHeight_),
                                          cv::Scalar(),
                                          true,
                                          false);

    cv::dnn::Net net = net_;
    net.setInput(blob);
    cv::Mat output = net.forward();

    const int rows = output.size[1];
    const int dimensions = output.size[2];
    const float *data = reinterpret_cast<float *>(output.data);

    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    std::vector<int> classIds;

    const float xScale = static_cast<float>(frame.cols) / inputWidth_;
    const float yScale = static_cast<float>(frame.rows) / inputHeight_;

    for (int i = 0; i < rows; ++i) {
        const float objConf = data[4];
        if (objConf < confThreshold) {
            data += dimensions;
            continue;
        }

        int bestClass = -1;
        float bestScore = 0.0f;
        for (int c = 5; c < dimensions; ++c) {
            if (data[c] > bestScore) {
                bestScore = data[c];
                bestClass = c - 5;
            }
        }

        const float conf = objConf * bestScore;
        if (conf >= confThreshold) {
            const float cx = data[0];
            const float cy = data[1];
            const float w = data[2];
            const float h = data[3];

            const int left = static_cast<int>((cx - 0.5f * w) * xScale);
            const int top = static_cast<int>((cy - 0.5f * h) * yScale);
            const int width = static_cast<int>(w * xScale);
            const int height = static_cast<int>(h * yScale);

            boxes.emplace_back(left, top, width, height);
            scores.push_back(conf);
            classIds.push_back(bestClass);
        }

        data += dimensions;
    }

    const std::vector<int> keep = nmsIndices(boxes, scores, confThreshold, nmsThreshold);
    detections.reserve(keep.size());
    for (int idx : keep) {
        Detection d;
        d.box = boxes[idx] & cv::Rect(0, 0, frame.cols, frame.rows);
        d.classId = classIds[idx];
        d.confidence = scores[idx];
        detections.push_back(d);
    }
    return detections;
}

std::vector<int> DetectorOnnxOpenCV::nmsIndices(const std::vector<cv::Rect> &boxes,
                                                const std::vector<float> &scores,
                                                float confThreshold,
                                                float nmsThreshold) {
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, confThreshold, nmsThreshold, indices);
    return indices;
}

