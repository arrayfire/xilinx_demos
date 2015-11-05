// Copyright 2014, Xilinx Inc.
// All rights reserved.

#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include "oclHelper.h"
#include "pgm.h"

const int FAST_THREADS_X = 1;
const int FAST_THREADS_Y = 1;
const int FAST_THREADS_NONMAX_X = 32;
const int FAST_THREADS_NONMAX_Y = 8;

class Timer {
    time_t mTimeStart;
    time_t mTimeEnd;
public:
    Timer() {
        mTimeStart = std::time(0);
        mTimeEnd = mTimeStart;
    }
    double stop() {
        mTimeEnd = std::time(0);
        return std::difftime(mTimeEnd, mTimeStart);
    }
    void reset() {
        mTimeStart = time(0);
        mTimeEnd = mTimeStart;
    }
};

typedef std::pair<int, int> Position;

const static struct option long_options[] = {
    {"device",        required_argument, 0, 'd'},
    {"kernel",        required_argument, 0, 'k'},
    {"img_file",      required_argument, 0, 'f'},
    {"iteration",     optional_argument, 0, 'i'},
    {"verbose",       no_argument,       0, 'v'},
    {"help",          no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

static void printHelp()
{
    std::cout << "usage: %s <options>\n";
    std::cout << "  -d <cpu|gpu|acc>\n";
    std::cout << "  -k <kernel_file> \n";
    std::cout << "  -f <img_file>\n";
    std::cout << "  -i <iteration_count>\n";
    std::cout << "  -t <fast_thr>\n";
    std::cout << "  -v\n";
    std::cout << "  -h\n";
}

int runTest(std::string imgFile, std::vector<int> fpgaX, std::vector<int> fpgaY, std::vector<int> fpgaScore)
{
    int extIndex = imgFile.find_last_of(".");
    std::string testFile = imgFile.substr(0, extIndex) + std::string(".test");
    std::ifstream testStream;
    testStream.open(testFile.c_str());

    size_t nsamples = 0;
    testStream >> nsamples;

    if (nsamples != fpgaX.size())
        return 1;

    int* goldX = new int[nsamples];
    int* goldY = new int[nsamples];
    int* goldScore = new int[nsamples];

    for (size_t i = 0; i < nsamples; i++) {
        testStream >> goldX[i] >> goldY[i] >> goldScore[i];
    }

    for (size_t i = 0; i < nsamples; i++) {
        if (fpgaX[i] != goldX[i] || fpgaY[i] != goldY[i] || fpgaScore[i] != goldScore[i])
            return 1;
    }

    delete[] goldX;
    delete[] goldY;
    delete[] goldScore;

    testStream.close();

    return 0;
}

static int runOpenCL(std::string imgFile, std::string kernelFile, cl_device_type deviceType,
                     int iteration, int fast_thr, bool verbose, double &delay)
{
    oclHardware hardware = getOclHardware(deviceType);
    if (!hardware.mQueue) {
        return -1;
    }
    std::cout << "verbose: " << verbose << std::endl;

    oclSoftware software;
    std::memset(&software, 0, sizeof(oclSoftware));
    std::strcpy(software.mKernelName, "locate_features");
    std::strcpy(software.mFileName, kernelFile.c_str());

    getOclSoftware(software, hardware);

    int* h_img;
    size_t w, h;
    readPGM(&h_img, &w, &h, imgFile);
    size_t img_el = w*h;

    int wi = (int)w, hi = (int)h;

    cl_int err = 0;

    cl_mem d_img = clCreateBuffer(hardware.mContext, CL_MEM_READ_ONLY, img_el * sizeof(int), NULL, &err);
    CL_CHECK(err);
    cl_mem d_score = clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE, img_el * sizeof(int), NULL, &err);
    CL_CHECK(err);

    CL_CHECK(clEnqueueWriteBuffer(hardware.mQueue, d_img, CL_TRUE, 0,
                                  img_el * sizeof(int), h_img, 0, 0, 0));

    int arg = 0;

    const unsigned edge = 3;
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(cl_mem), &d_img));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(int), &wi));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(int), &hi));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(cl_mem), &d_score));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(int), &fast_thr));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(unsigned), &edge));

    size_t localSize[2] = { FAST_THREADS_X, FAST_THREADS_Y };

    size_t globalSize[2] = { 1, 1 };

    if (verbose) {
        std::cout << "Global size = (" << globalSize[0] << "," << globalSize[1] << ",1)" << std::endl;
        std::cout << "Local  size = (" << localSize[0] << "," << localSize[1] << ",1)" << std::endl;
    }

    std::vector<int> h_score_init(w * h, 0);
    int* h_score = new int[img_el];

    std::vector<int> x, y, score;

    Timer timer;
    for(int i = 0; i < iteration; i++)
    {
        // Here we start measurings host time for kernel execution
        CL_CHECK(clEnqueueWriteBuffer(hardware.mQueue, d_score, CL_TRUE, 0,
                                      img_el * sizeof(int), &h_score_init[0], 0, 0, 0));

        CL_CHECK(clEnqueueNDRangeKernel(hardware.mQueue, software.mKernel, 2, 0,
                                        globalSize, localSize, 0, 0, 0));

        CL_CHECK(clFinish(hardware.mQueue));

        CL_CHECK(clEnqueueReadBuffer(hardware.mQueue, d_score, CL_TRUE, 0,
                                     img_el * sizeof(int), h_score, 0, 0, 0));

        if (i == iteration-1) {
            for (size_t j = 0; j < h; j++) {
                for (size_t k = 0; k < w; k++) {
                    size_t idx = j*w + k;
                    int s = h_score[idx];
                    if (s != 0) {
                        x.push_back(k);
                        y.push_back(j);
                        score.push_back(s);

                        if (verbose)
                            std::cout << "(" << k << ", " << j << "): " << s << std::endl;
                    }
                }
            }
        }
    }
    delay = timer.stop();

    int testRes = runTest(imgFile, x, y, score);

    delete[] h_img;
    delete[] h_score;
    clReleaseMemObject(d_img);
    clReleaseMemObject(d_score);

    return testRes;
}


int main(int argc, char** argv)
{
    cl_device_type deviceType = CL_DEVICE_TYPE_ACCELERATOR;;
    int option_index = 0;
    std::string kernelFile("fast.cl");
    std::string imgFile("./data/square.pgm");
    int iteration = 1;
    int fast_thr = 20;
    bool verbose = false;
    // Commandline
    int c;
    while ((c = getopt_long(argc, argv, "d:k:f:i:t:vh", long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 0:
            if (long_options[option_index].flag != 0)
                break;
        case 'd':
            if (strcmp(optarg, "gpu") == 0)
                deviceType = CL_DEVICE_TYPE_GPU;
            else if (strcmp(optarg, "cpu") == 0)
                deviceType = CL_DEVICE_TYPE_CPU;
            else if (strcmp(optarg, "soft") == 0)
                deviceType = CL_DEVICE_TYPE_DEFAULT;
            else if (strcmp(optarg, "acc") != 0) {
                std::cout << "Incorrect platform specified\n";
                printHelp();
                return -1;
            }
            break;
        case 'k':
            kernelFile = optarg;
            break;
        case 'f':
            imgFile = optarg;
            break;
        case 'i':
            iteration = atoi(optarg);
            break;
        case 't':
            fast_thr = atoi(optarg);
            break;
        case 'h':
            printHelp();
            return 0;
        case 'v':
            verbose = true;
            break;
        default:
            printHelp();
            return 1;
        }
    }

    double delay = 0;
    if ((deviceType != CL_DEVICE_TYPE_DEFAULT) && runOpenCL(imgFile, kernelFile, deviceType,
                                                            iteration, fast_thr, verbose, delay)) {
        std::cout << "FAILED TEST\n";
        std::cout << "OpenCL total time: " << delay << " sec\n";
        std::cout << "OpenCL average time per iteration: " << delay/iteration << " sec\n";
        return 1;
    }

    std::cout << "OpenCL total time: " << delay << " sec\n";
    std::cout << "OpenCL average time per iteration: " << delay/iteration << " sec\n";
    std::cout << "PASSED TEST\n";
    return 0;
}
