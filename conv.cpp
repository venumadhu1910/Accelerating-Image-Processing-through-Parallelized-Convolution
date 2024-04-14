#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <random>
#include <cmath>
#include <algorithm>

// Function to generate random image data
std::vector<std::vector<double>> generateImageData(int size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    std::vector<std::vector<double>> image(size, std::vector<double>(size));
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            image[i][j] = dis(gen);
        }
    }
    return image;
}

// Function to perform convolution using multi-threading
void convolveMultithread(const std::vector<std::vector<double>>& image, const std::vector<std::vector<double>>& filter, int numThreads, std::vector<std::vector<double>>& output) {
    int size = image.size();
    int filterSize = filter.size();
    int chunkSize = size / numThreads;

    std::vector<std::thread> threads;
    std::mutex mtx;

    auto convolveChunk = [&](int start, int end) {
        for (int i = start; i < end; ++i) {
            for (int j = 0; j < size; ++j) {
                double sum = 0.0;
                for (int k = 0; k < filterSize; ++k) {
                    for (int l = 0; l < filterSize; ++l) {
                        int x = i - filterSize / 2 + k;
                        int y = j - filterSize / 2 + l;
                        if (x >= 0 && x < size && y >= 0 && y < size) {
                            sum += image[x][y] * filter[k][l];
                        }
                    }
                }
                mtx.lock();
                output[i][j] = sum;
                mtx.unlock();
            }
        }
    };

    for (int i = 0; i < numThreads; ++i) {
        int start = i * chunkSize;
        int end = (i == numThreads - 1) ? size : (i + 1) * chunkSize;
        threads.emplace_back(convolveChunk, start, end);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

// Function to calculate speedup
double calculateSpeedup(double singleThreadTime, double multiThreadTime) {
    return singleThreadTime / multiThreadTime;
}

// Function to calculate parallel efficiency
double calculateParallelEfficiency(double speedup, int numThreads) {
    return speedup / numThreads;
}

int main() {
    // Generate random image data
    int imageSize = 1000;
    std::vector<std::vector<double>> image = generateImageData(imageSize);

    // Define filter size
    int filterSize = 3;
    std::vector<std::vector<double>> filter(filterSize, std::vector<double>(filterSize, 1.0));

    // Define parameters
    std::vector<int> numThreadsList = {1, 2, 4, 8}; // Adjust this for different numbers of threads
    int maxN = 1000; // Maximum size of image

    // Performance metrics
    std::vector<double> speedupMultithread;
    std::vector<double> parallelEfficiency;

    // Single-threaded convolution for baseline time
    std::vector<std::vector<double>> output(imageSize, std::vector<double>(imageSize));
    auto startTime = std::chrono::high_resolution_clock::now();
    convolveMultithread(image, filter, 1, output);
    auto endTime = std::chrono::high_resolution_clock::now();
    double singleThreadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // Multi-threaded convolution for various N values
    for (int N = 100; N <= maxN; N += 100) {
        // Generate image data of size N
        std::vector<std::vector<double>> resizedImage = generateImageData(N);

        // Multi-threaded convolution for each N
        startTime = std::chrono::high_resolution_clock::now();
        convolveMultithread(resizedImage, filter, 1, output);
        endTime = std::chrono::high_resolution_clock::now();
        double multiThreadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Calculate speedup and parallel efficiency
        double speedup = calculateSpeedup(singleThreadTime, multiThreadTime);
        speedupMultithread.push_back(speedup);
        double efficiency = calculateParallelEfficiency(speedup, 1);
        parallelEfficiency.push_back(efficiency);
    }

    // Output performance metrics
    std::cout << "For different values of N, Plot speed up with respect to the number of processing elements\n";
    for (size_t i = 0; i < speedupMultithread.size(); ++i) {
        std::cout << "N: " << 100 + i * 100 << ", Speedup: " << speedupMultithread[i] << '\n';
    }

    // Multi-threaded convolution for various numbers of processing elements
    speedupMultithread.clear();
    for (int numThreads : numThreadsList) {
        startTime = std::chrono::high_resolution_clock::now();
        convolveMultithread(image, filter, numThreads, output);
        endTime = std::chrono::high_resolution_clock::now();
        double multiThreadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Calculate speedup and parallel efficiency
        double speedup = calculateSpeedup(singleThreadTime, multiThreadTime);
        speedupMultithread.push_back(speedup);
    }

    // Output performance metrics
    std::cout << "\nFor different numbers of processing elements, Plot speedup with respect to N\n";
    for (size_t i = 0; i < numThreadsList.size(); ++i) {
        std::cout << "Threads: " << numThreadsList[i] << ", Speedup: " << speedupMultithread[i] << '\n';
    }

    return 0;
}
