
## sequencial
g++ -o video_filter_sequential video_filter_sequential.cpp `pkg-config --cflags --libs opencv4`
./video_filter_sequential video.mp4

## threads
g++ -o video_filter_pthreads video_filter_pthreads.cpp `pkg-config --cflags --libs opencv4` -lpthread
./video_filter_pthreads video.mp4


## open mp
g++ -o video_filter_openmp video_filter_openmp.cpp `pkg-config --cflags --libs opencv4` -fopenmp
./video_filter_openmp video.mp4

