#include "detector_onnx_opencv.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace {
void drawDetections(cv::Mat &frame, const std::vector<Detection> &detections) {
    for (const auto &d : detections) {
        cv::rectangle(frame, d.box, cv::Scalar(0, 255, 0), 2);
        const std::string label =
            "id=" + std::to_string(d.classId) + " conf=" + std::to_string(d.confidence);
        cv::putText(frame,
                    label,
                    cv::Point(d.box.x, std::max(20, d.box.y - 8)),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.55,
                    cv::Scalar(0, 255, 0),
                    2);
    }
}

bool isImagePath(const std::string &path) {
    const auto dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    std::string ext = path.substr(dotPos + 1);
    for (char &c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp" || ext == "tif" ||
           ext == "tiff";
}
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage:\n";
        std::cerr << "  simple_infer <model.onnx> <image_or_video> [output_path]\n";
        std::cerr << "Examples:\n";
        std::cerr << "  simple_infer best.onnx test.jpg out.jpg\n";
        std::cerr << "  simple_infer best.onnx video.mp4 out.mp4\n";
        return 1;
    }

    const std::string modelPath = argv[1];
    const std::string inputPath = argv[2];
    const std::string outputPath = (argc >= 4) ? argv[3] : "";

    DetectorOnnxOpenCV detector;
    if (!detector.load(modelPath)) {
        std::cerr << "Failed to load ONNX model: " << modelPath << "\n";
        return 2;
    }

    if (isImagePath(inputPath)) {
        cv::Mat image = cv::imread(inputPath);
        if (image.empty()) {
            std::cerr << "Failed to read image: " << inputPath << "\n";
            return 3;
        }

        const auto detections = detector.infer(image);
        drawDetections(image, detections);

        if (!outputPath.empty()) {
            if (!cv::imwrite(outputPath, image)) {
                std::cerr << "Failed to write output image: " << outputPath << "\n";
                return 4;
            }
            std::cout << "Saved image result to: " << outputPath << "\n";
        }

        cv::imshow("Detections", image);
        cv::waitKey(0);
        return 0;
    }

    cv::VideoCapture cap(inputPath);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open video: " << inputPath << "\n";
        return 5;
    }

    cv::VideoWriter writer;
    if (!outputPath.empty()) {
        const int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        const int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        const double fps = cap.get(cv::CAP_PROP_FPS) > 1.0 ? cap.get(cv::CAP_PROP_FPS) : 25.0;
        if (!writer.open(outputPath,
                         cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                         fps,
                         cv::Size(width, height))) {
            std::cerr << "Failed to open output video: " << outputPath << "\n";
            return 6;
        }
    }

    cv::Mat frame;
    while (cap.read(frame)) {
        const auto detections = detector.infer(frame);
        drawDetections(frame, detections);

        if (writer.isOpened()) {
            writer.write(frame);
        }

        cv::imshow("Detections", frame);
        const int key = cv::waitKey(1);
        if (key == 27) {
            break;
        }
    }

    if (writer.isOpened()) {
        std::cout << "Saved video result to: " << outputPath << "\n";
    }
    return 0;
}

