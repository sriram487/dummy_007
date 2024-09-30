#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <chrono>

void draw_star(int x, int y, float magnitude, cv::Mat* background, int ROI) {
    double H = 90000 * exp(-magnitude + 1);
    double sigma = 0.5;

    for (int u = x - ROI; u <= x + ROI; u++) {
        for (int v = y - ROI; v <= y + ROI; v++) {
            double dist = (u - x) * (u - x) + (v - y) * (v - y);
            double diff = dist / (2 * (sigma * sigma));
            double exponent_exp = exp(-diff);
            int raw_intensity = (int)round((H / (2 * CV_PI * (sigma * sigma))) * exponent_exp);

            if (u >= 0 && u < background->cols && v >= 0 && v < background->rows) {
                (*background).at<uchar>(v, u) = (uchar)fmin(raw_intensity, 255);
            }
        }
    }
}

void display_image(cv::Mat& image) {
    cv::imshow("Star Image", image);
    cv::waitKey(1); // Update the window with the new image
}


std::vector<std::string> get_csv_files(const std::string& directory) {
    std::vector<std::string> csv_files;
    DIR* dir = opendir(directory.c_str());
    struct dirent* entry;

    if (dir) {
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename.find(".csv") != std::string::npos) {
                csv_files.push_back(directory + "/" + filename);
            }
        }
        closedir(dir);
    }

    return csv_files;
}

int main() {
    // Directory containing CSV files
    std::string directory = "./data/sample_2/"; // Update this path as needed
    std::vector<std::string> csv_files = get_csv_files(directory);

    for (const auto& file_path : csv_files) {
        // Load star data from CSV

        auto start_time = std::chrono::high_resolution_clock::now(); // Start timing

        FILE* file = fopen(file_path.c_str(), "r");
        if (!file) {
            fprintf(stderr, "Error opening file: %s\n", file_path.c_str());
            continue;
        }

        float stars[1000][3]; // Assuming max 1000 stars
        int star_count = 0;

        // Parse CSV file
        while (fscanf(file, "%f,%f,%f\n", &stars[star_count][0], &stars[star_count][1], &stars[star_count][2]) == 3) {
            star_count++;
        }
        fclose(file);

        // Create a background image
        cv::Mat background = cv::Mat::zeros(1400, 1400, CV_8UC1);

        // Draw stars on the background
        for (int i = 0; i < star_count; i++) {
            int x = (int)stars[i][0];
            int y = (int)stars[i][1];
            float mag = stars[i][2];
            draw_star(x, y, mag, &background, 5);
        }

        // Convert the single channel image to a 3-channel image for display
        cv::Mat display_image_mat;
        cv::cvtColor(background, display_image_mat, cv::COLOR_GRAY2BGR);

        // Display the image
        display_image(display_image_mat);

        auto end_time = std::chrono::high_resolution_clock::now(); // End timing
        std::chrono::duration<double> duration = end_time - start_time;

        std::cout << "Time taken to process and display images: " << duration.count() << " seconds." << std::endl;

        
    }

    return 0;
}
