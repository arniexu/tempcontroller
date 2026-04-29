import argparse
from pathlib import Path

import cv2


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Extract frames from a video file."
    )
    parser.add_argument("input", type=Path, help="Input video path")
    parser.add_argument("output", type=Path, help="Output directory for frames")
    parser.add_argument(
        "--every-n",
        type=int,
        default=1,
        help="Extract every Nth frame when --timestamps is not provided",
    )
    parser.add_argument(
        "--timestamps",
        nargs="*",
        type=float,
        default=None,
        help="Exact timestamps in seconds to extract",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=0,
        help="Maximum number of frames to write when using --every-n (0 means no limit)",
    )
    parser.add_argument(
        "--prefix",
        default="frame",
        help="Output filename prefix",
    )
    return parser


def open_capture(video_path: Path) -> cv2.VideoCapture:
    capture = cv2.VideoCapture(str(video_path))
    if not capture.isOpened():
        raise RuntimeError(f"Unable to open video: {video_path}")
    return capture


def save_frame(output_dir: Path, prefix: str, frame_index: int, time_seconds: float, frame) -> Path:
    output_path = output_dir / f"{prefix}_{frame_index:05d}_{time_seconds:08.3f}s.jpg"
    if not cv2.imwrite(str(output_path), frame):
        raise RuntimeError(f"Unable to write frame: {output_path}")
    return output_path


def extract_by_step(capture: cv2.VideoCapture, output_dir: Path, prefix: str, every_n: int, limit: int) -> list[Path]:
    saved_paths: list[Path] = []
    frame_index = 0
    fps = capture.get(cv2.CAP_PROP_FPS)
    if fps <= 0:
        fps = 0.0

    while True:
        ok, frame = capture.read()
        if not ok:
            break

        if frame_index % every_n == 0:
            time_seconds = (frame_index / fps) if fps > 0 else 0.0
            saved_paths.append(save_frame(output_dir, prefix, frame_index, time_seconds, frame))
            if limit > 0 and len(saved_paths) >= limit:
                break

        frame_index += 1

    return saved_paths


def extract_by_timestamps(capture: cv2.VideoCapture, output_dir: Path, prefix: str, timestamps: list[float]) -> list[Path]:
    saved_paths: list[Path] = []

    for timestamp in timestamps:
        capture.set(cv2.CAP_PROP_POS_MSEC, timestamp * 1000.0)
        ok, frame = capture.read()
        if not ok:
            raise RuntimeError(f"Unable to read frame at {timestamp:.3f}s")

        frame_index = int(capture.get(cv2.CAP_PROP_POS_FRAMES))
        saved_paths.append(save_frame(output_dir, prefix, frame_index, timestamp, frame))

    return saved_paths


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if not args.input.exists():
        raise FileNotFoundError(f"Input video does not exist: {args.input}")
    if args.every_n <= 0:
        raise ValueError("--every-n must be greater than 0")

    args.output.mkdir(parents=True, exist_ok=True)

    capture = open_capture(args.input)
    try:
        if args.timestamps:
            saved_paths = extract_by_timestamps(capture, args.output, args.prefix, args.timestamps)
        else:
            saved_paths = extract_by_step(capture, args.output, args.prefix, args.every_n, args.limit)
    finally:
        capture.release()

    print(f"saved={len(saved_paths)}")
    for path in saved_paths:
        print(path)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())