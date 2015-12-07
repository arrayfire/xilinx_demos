// Copyright 2014, Xilinx Inc.
// All rights reserved.

#ifndef _FAST_H_
#define _FAST_H_

#include <getopt.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <algorithm>
#include <chrono>
#include "oclHelper.h"

//const int FAST_THREADS_X = 16;
//const int FAST_THREADS_Y = 16;
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

static int runOpenCL(std::vector<int>& x, std::vector<int>& y, std::vector<int>& score, double& delay,
                     const int* imgPtr, const int imgWidth, const int imgHeight, const int maxFeatures,
                     std::string kernelFile, cl_device_type deviceType, int iteration,
                     int fast_thr, bool verbose);

int fast(std::vector<int>& v_x, std::vector<int>& v_y, std::vector<int>& v_score,
         const int* imgPtr, const int imgWidth, const int imgHeight, const int maxFeatures,
         const std::string execPath);

#endif
