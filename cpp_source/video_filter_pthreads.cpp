#include <opencv2/opencv.hpp>
#include <iostream>
#include <pthread.h>
#include <chrono>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

using namespace cv;
using namespace std;

struct ThreadData {
    Mat frame;
    Mat filtered_h1;
    Mat filtered_h2;
    int frameOrder;
};

Mat h1 = (Mat_<double>(3, 3) << 0, -1, 0, -1, 4, -1, 0, -1, 0);
Mat h2 = (Mat_<double>(3, 3) << -1, -1, -1, -1, 8, -1, -1, -1, -1);

int numThreads = 2;
queue<ThreadData> frameQueue;
unordered_map<int, ThreadData> processedFrames;
mutex mtxFrameQueue;
mutex mtxProcessedFrames;
condition_variable cvFrameQueue;
bool isReadingComplete = false;
bool isProcessingComplete = false;

void* processFrames(void* arg) {
    while (true) {
        unique_lock<mutex> lock(mtxFrameQueue);
        cvFrameQueue.wait(lock, [] { return !frameQueue.empty() || isReadingComplete; });

        if (frameQueue.empty() && isReadingComplete)
            break;

        ThreadData threadData;
        threadData = frameQueue.front();
        frameQueue.pop();
        lock.unlock();

        // Processa o frame
        threadData.filtered_h1 = threadData.frame.clone();
        threadData.filtered_h2 = threadData.frame.clone();
        filter2D(threadData.filtered_h1, threadData.filtered_h1, -1, h1);
        filter2D(threadData.filtered_h2, threadData.filtered_h1, -1, h2);

        unique_lock<mutex> lockProcessed(mtxProcessedFrames);
        processedFrames[threadData.frameOrder] = threadData;
    }

    return nullptr;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <video-file-path> [show] [num_threads]" << endl;
        return -1;
    }

    bool show = (argc >= 3 && string(argv[2]) == "show");
    numThreads = (argc == 4) ? stoi(argv[3]) : 2;

    VideoCapture cap(argv[1]);
    if (!cap.isOpened()) {
        cout << "Error opening video stream or file" << endl;
        return -1;
    }

    if (show) {
        namedWindow("Original Frame", WINDOW_NORMAL);
        namedWindow("Filtered Frame with h1", WINDOW_NORMAL);
        namedWindow("Filtered Frame with h2", WINDOW_NORMAL);
    }

    auto start = chrono::high_resolution_clock::now();

    pthread_t* threads = new pthread_t[numThreads];
    for (int i = 0; i < numThreads; ++i) {
        pthread_create(&threads[i], NULL, processFrames, NULL);
    }

    Mat frame;
    int frameOrder = 0;
    while (cap.read(frame)) {
        ThreadData threadData;
        threadData.frame = frame.clone();
        threadData.frameOrder = frameOrder++;

        unique_lock<mutex> lock(mtxFrameQueue);
        frameQueue.push(threadData);
        cvFrameQueue.notify_one();
    }

    isReadingComplete = true;
    cvFrameQueue.notify_all();

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], NULL);
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> duration = end - start;
    cout << "Pthreads version execution time: " << duration.count() << " seconds" << endl;

    if (show) {
        for (const auto& entry : processedFrames) {
            imshow("Original Frame", entry.second.frame);
            imshow("Filtered Frame with h1", entry.second.filtered_h1);
            imshow("Filtered Frame with h2", entry.second.filtered_h2);
            if (waitKey(25) == 27)
                break;
        }
        destroyAllWindows();
    }

    delete[] threads;
    cap.release();

    return 0;
}
