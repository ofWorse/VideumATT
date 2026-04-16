#include "detector_onnx_opencv.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <iostream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: inference_example <model.onnx> [video_path]\n";
        return 1;
    }

    DetectorOnnxOpenCV detector;
    if (!detector.load(argv[1])) {
        std::cerr << "Failed to load ONNX model.\n";
        return 2;
    }

    cv::VideoCapture cap;
    if (argc >= 3) {
        cap.open(argv[2]);
    } else {
        cap.open(0);
    }
    if (!cap.isOpened()) {
        std::cerr << "Failed to open video source.\n";
        return 3;
    }

    cv::Mat frame;
    while (cap.read(frame)) {
        const auto detections = detector.infer(frame);
        for (const auto &d : detections) {
            cv::rectangle(frame, d.box, cv::Scalar(0, 255, 0), 2);
            std::string label =
                "id=" + std::to_string(d.classId) + " conf=" + std::to_string(d.confidence);
            cv::putText(frame,
                        label,
                        cv::Point(d.box.x, std::max(20, d.box.y - 8)),
                        cv::FONT_HERSHEY_SIMPLEX,
                        0.5,
                        cv::Scalar(0, 255, 0),
                        1);
        }

        cv::imshow("Inference", frame);
        const int key = cv::waitKey(1);
        if (key == 27) {
            break;
        }
    }
    return 0;
}

