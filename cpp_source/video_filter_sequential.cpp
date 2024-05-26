#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>

using namespace std;
using namespace cv;

struct ThreadData {
    Mat frame;
    Mat filtered_h1;
    Mat filtered_h2;
    int frameOrder;
};

// Função para aplicar um filtro passa alta em uma imagem
Mat applyHighPassFilter(const Mat image, const Mat filter) {
    Mat result;
    filter2D(image, result, -1, filter);
    return result;
}

void run(VideoCapture& video, bool show) {
    // Loop para ler e processar cada frame do vídeo
    Mat frame;
    VideoWriter videoWriter;
    Size frameSize;
    int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');
    double fps;
    bool isFirstFrame = true;
    int count = 0;

    int numFrames = video.get(cv::CAP_PROP_FRAME_COUNT);
    std::cout << "Numero de frames: " << numFrames << endl << "-------------------------" << endl;

    while (video.read(frame)) {
        ThreadData threadData1;
        threadData1.frame = frame;
        threadData1.frameOrder = count++;
        
        // Define os filtros passa alta
        Mat h1 = (Mat_<int>(3, 3) << 0, -1, 0, -1, 4, -1, 0, -1, 0);
        Mat h2 = (Mat_<int>(3, 3) << -1, -1, -1, -1, 8, -1, -1, -1, -1);

        // Processa o frame
        threadData1.filtered_h1 = applyHighPassFilter(threadData1.frame, h1);
        threadData1.filtered_h2 = applyHighPassFilter(threadData1.filtered_h1, h2);

        std::cout << "Processing Frame: " << threadData1.frameOrder + 1 << " Out of " << numFrames << endl;

        if (isFirstFrame) {
            frameSize = threadData1.filtered_h2.size();
            fps = video.get(CAP_PROP_FPS);
            videoWriter.open("out/output_video.avi", codec, fps, frameSize);
            isFirstFrame = false;
        }

        videoWriter.write(threadData1.filtered_h2);

        if (show) {
            imshow("Filtro h2", threadData1.filtered_h2);
            imshow("Frame original", frame);
            if (waitKey(10) == 'q')
                break;
        }
    }

    videoWriter.release();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <video-file-path> [show] [num_threads]" << endl;
        return -1;
    }

    bool show = (argc >= 3 && string(argv[2]) == "show");

    VideoCapture video(argv[1]);
    if (!video.isOpened()) {
        std::cout << "Erro ao abrir o vídeo." << endl;
        return -1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    run(video, show);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    double seconds = duration.count();

    // Exibe o tempo de execução em segundos
    std::cout << "Sequential version execution time: " << duration.count() << " seconds" << endl;

    // Libera os recursos
    video.release();
    destroyAllWindows();

    return 0;
}
