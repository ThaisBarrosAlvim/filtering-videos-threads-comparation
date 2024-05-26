#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>

using namespace std;
using namespace cv;

// Structure to hold frame data and its processing results
struct ThreadData {
    Mat frame;
    Mat filtered_h1;
    Mat filtered_h2;
    int frameOrder;
};

// Function to apply a high-pass filter to an image
Mat applyHighPassFilter(const Mat image, const Mat filter) {
    Mat result;
    filter2D(image, result, -1, filter);
    return result;
}

// Function to process the video
void run(VideoCapture& video, bool show) {
    Mat frame;
    VideoWriter videoWriter;
    Size frameSize;
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');
    double fps;
    bool isFirstFrame = true;
    int count = 0;

    int numFrames = video.get(cv::CAP_PROP_FRAME_COUNT);
    cout << "Number of frames: " << numFrames << endl;

    // Loop to read and process each frame of the video
    while (video.read(frame)) {
        ThreadData threadData1;
        threadData1.frame = frame;
        threadData1.frameOrder = count++;

        // Define high-pass filters
        Mat h1 = (Mat_<int>(3, 3) << 0, -1, 0, -1, 4, -1, 0, -1, 0);
        Mat h2 = (Mat_<int>(3, 3) << -1, -1, -1, -1, 8, -1, -1, -1, -1);

        // Process the frame
        threadData1.filtered_h1 = applyHighPassFilter(threadData1.frame, h1);
        threadData1.filtered_h2 = applyHighPassFilter(threadData1.filtered_h1, h2);

        cout << "Processing Frame: " << threadData1.frameOrder + 1 << " Out of " << numFrames << endl;

        if (isFirstFrame) {
            frameSize = threadData1.filtered_h2.size();
            fps = video.get(CAP_PROP_FPS);
            videoWriter.open("out/output_video.avi", codec, fps, frameSize);
            isFirstFrame = false;
        }

        videoWriter.write(threadData1.filtered_h2);

        if (show) {
            imshow("Filtered h2", threadData1.filtered_h2);
            imshow("Original Frame", frame);
            if (waitKey(10) == 'q')
                break;
        }
    }

    videoWriter.release();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <video-file-path> [show]" << endl;
        return -1;
    }

    bool show = (argc >= 3 && string(argv[2]) == "show");

    VideoCapture video(argv[1]);
    if (!video.isOpened()) {
        cout << "Error opening video stream or file" << endl;
        return -1;
    }

    auto start = chrono::high_resolution_clock::now();
    run(video, show);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    double seconds = duration.count();

    // Display the execution time in seconds
    cout << "Sequential version execution time: " << duration.count() << " seconds" << endl;

    // Release resources
    video.release();
    destroyAllWindows();

    return 0;
}
