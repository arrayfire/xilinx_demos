// Copyright 2014, Xilinx Inc.
// All rights reserved.

#include "fast.h"

typedef struct
{
    int f[3];
} feat_t;

bool feat_cmp(feat_t i, feat_t j)
{
    if (i.f[2] != j.f[2])
        return (i.f[2] > j.f[2]);

    return false;
}

void vec_to_feat(std::vector<feat_t>& feat, std::vector<int>& x, std::vector<int>& y, std::vector<int>& score)
{
    size_t nfeat = x.size();
    feat.resize(nfeat);
    for (size_t i = 0; i < nfeat; i++) {
        feat[i].f[0] = x[i];
        feat[i].f[1] = y[i];
        feat[i].f[2] = score[i];
    }
}

void feat_to_vec(std::vector<int>& x, std::vector<int>& y, std::vector<int>& score, std::vector<feat_t>& feat, int feat_to_retain)
{
    //size_t nfeat = feat.size();
    size_t nfeat = std::min(feat_to_retain, (int)feat.size());
    x.resize(nfeat);
    y.resize(nfeat);
    score.resize(nfeat);
    for (size_t i = 0; i < nfeat; i++) {
        x[i] = feat[i].f[0];
        y[i] = feat[i].f[1];
        score[i] = feat[i].f[2];
    }
}

static int runOpenCL(std::vector<int>& x, std::vector<int>& y, std::vector<int>& score, double& delay,
                     const int* imgPtr, const int imgWidth, const int imgHeight, const int maxFeatures,
                     std::string kernelFile, cl_device_type deviceType, int iteration,
                     int fast_thr, bool verbose)
{
    static oclHardware hardware = getOclHardware(deviceType);
    if (!hardware.mQueue) {
        return -1;
    }
    //std::cout << "verbose: " << verbose << std::endl;

    static oclSoftware software;
    if (strcmp(software.mKernelName, "locate_features") != 0) {
        std::memset(&software, 0, sizeof(oclSoftware));
        std::strcpy(software.mKernelName, "locate_features");
        std::strcpy(software.mFileName, kernelFile.c_str());

        getOclSoftware(software, hardware);
    }

    size_t imgEl = imgWidth*imgHeight;

    cl_int err = 0;

    cl_mem d_img = clCreateBuffer(hardware.mContext, CL_MEM_READ_ONLY, imgEl * sizeof(int), NULL, &err);
    CL_CHECK(err);
    cl_mem d_score = clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE, imgEl * sizeof(int), NULL, &err);
    CL_CHECK(err);

    CL_CHECK(clEnqueueWriteBuffer(hardware.mQueue, d_img, CL_TRUE, 0,
                                  imgEl * sizeof(int), imgPtr, 0, 0, 0));

    int arg = 0;

    const unsigned edge = 3;
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(cl_mem), &d_img));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(int), &imgWidth));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(int), &imgHeight));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(cl_mem), &d_score));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(int), &fast_thr));
    CL_CHECK(clSetKernelArg(software.mKernel, arg++, sizeof(unsigned), &edge));

    size_t localSize[2] = { FAST_THREADS_X, FAST_THREADS_Y };

    size_t globalSize[2] = { 1, 1 };

    if (verbose) {
        std::cout << "Global size = (" << globalSize[0] << "," << globalSize[1] << ",1)" << std::endl;
        std::cout << "Local  size = (" << localSize[0] << "," << localSize[1] << ",1)" << std::endl;
    }

    std::vector<int> h_score_init(imgEl, 0);
    int* h_score = new int[imgEl];

    std::vector<int> tmp_x, tmp_y, tmp_score;

    Timer timer;
    for(int i = 0; i < iteration; i++)
    {
        // Here we start measurings host time for kernel execution
        CL_CHECK(clEnqueueWriteBuffer(hardware.mQueue, d_score, CL_TRUE, 0,
                                      imgEl * sizeof(int), &h_score_init[0], 0, 0, 0));

        CL_CHECK(clEnqueueNDRangeKernel(hardware.mQueue, software.mKernel, 2, 0,
                                        globalSize, localSize, 0, 0, 0));

        CL_CHECK(clFinish(hardware.mQueue));

        CL_CHECK(clEnqueueReadBuffer(hardware.mQueue, d_score, CL_TRUE, 0,
                                     imgEl * sizeof(int), h_score, 0, 0, 0));

        if (i == iteration-1) {
            //auto startTime = std::chrono::high_resolution_clock::now();
            for (size_t j = 0; j < imgHeight; j++) {
                for (size_t k = 0; k < imgWidth; k++) {
                    size_t idx = j*imgWidth + k;
                    int s = h_score[idx];
                    if (s != 0) {
                        tmp_x.push_back(k);
                        tmp_y.push_back(j);
                        tmp_score.push_back(s);

                        if (verbose)
                            std::cout << "(" << k << ", " << j << "): " << s << std::endl;
                    }
                }
            }
            //auto endTime = std::chrono::high_resolution_clock::now();
            //int detTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
            //std::cout << "Feature detection time: " << detTime << std::endl;

            //startTime = std::chrono::high_resolution_clock::now();
            std::vector<feat_t> out_feat;
            vec_to_feat(out_feat, tmp_x, tmp_y, tmp_score);
            std::sort(out_feat.begin(), out_feat.end(), feat_cmp);

            feat_to_vec(x, y, score, out_feat, maxFeatures);
            //endTime = std::chrono::high_resolution_clock::now();
            //int sortTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
            //std::cout << "Feature sorting time: " << sortTime << std::endl;
        }
    }
    delay = timer.stop();

    delete[] h_score;
    clReleaseMemObject(d_img);
    clReleaseMemObject(d_score);

    return 0;
}

int fast(std::vector<int>& v_x, std::vector<int>& v_y, std::vector<int>& v_score,
         const int* imgPtr, const int imgWidth, const int imgHeight, const int maxFeatures,
         const std::string execPath)
{
    cl_device_type deviceType = CL_DEVICE_TYPE_ACCELERATOR;
    std::string kernelFile(execPath + "/fast_pipeline_nonmax.xclbin");
    int iteration = 1;
    int fast_thr = 20;
    bool verbose = false;
    double delay = 0;

    runOpenCL(v_x, v_y, v_score, delay, imgPtr, imgWidth, imgHeight, maxFeatures,
              kernelFile, deviceType, iteration, fast_thr, verbose);

    return 0;
}
