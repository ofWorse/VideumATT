#!/usr/bin/env python3
import argparse
import os
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description="Train YOLO model for exported dataset.yaml")
    parser.add_argument("--data", required=True, help="Absolute path to dataset.yaml")
    parser.add_argument("--epochs", type=int, default=50, help="Number of training epochs")
    parser.add_argument("--imgsz", type=int, default=640, help="Image size")
    args = parser.parse_args()

    try:
        from ultralytics import YOLO
    except Exception as exc:  # pragma: no cover
        print("ERROR: ultralytics package is not installed.", file=sys.stderr)
        print(f"DETAILS: {exc}", file=sys.stderr)
        print("Install with: pip install ultralytics", file=sys.stderr)
        return 2

    if not os.path.exists(args.data):
        print(f"ERROR: dataset yaml does not exist: {args.data}", file=sys.stderr)
        return 3

    project_dir = os.path.dirname(os.path.dirname(args.data))
    runs_dir = os.path.join(project_dir, "runs")
    os.makedirs(runs_dir, exist_ok=True)

    print("Starting YOLO training...")
    print(f"Dataset: {args.data}")
    print(f"Output dir: {runs_dir}")

    model = YOLO("yolov8n.pt")
    result = model.train(
        data=args.data,
        epochs=args.epochs,
        imgsz=args.imgsz,
        project=runs_dir,
        name="annotate_train",
        exist_ok=True,
    )

    best_path = os.path.join(runs_dir, "annotate_train", "weights", "best.pt")
    if not os.path.exists(best_path):
        print("ERROR: best.pt was not generated.", file=sys.stderr)
        return 4

    print(f"Training finished. best.pt: {best_path}")

    print("Exporting to ONNX...")
    model = YOLO(best_path)
    export_result = model.export(format="onnx", dynamic=True, simplify=True)
    print(f"ONNX export result: {export_result}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

