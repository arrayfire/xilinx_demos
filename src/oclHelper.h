// Copyright 2014, Xilinx Inc.
// All rights reserved.

#ifndef _OCL_HELP_H_
#define _OCL_HELP_H_

#include <CL/cl.h>

#define DIVUP(A, B) (((A) + (B) - 1) / (B))

#define CL_CHECK(fn) do {                                                                   \
        cl_int _err = fn;                                                                   \
        if (_err != CL_SUCCESS) {                                                           \
            std::cout << "OpenCL error (" << _err << ", " << oclErrorCode(_err) << ")" <<   \
            " in file:line " << __FILE__ << ":" << __LINE__ << std::endl;                   \
            return -1;                                                                      \
        }                                                                                   \
    } while(0)

struct oclHardware {
    cl_platform_id mPlatform;
    cl_context mContext;
    cl_device_id mDevice;
    cl_command_queue mQueue;
    short mMajorVersion;
    short mMinorVersion;
};

struct oclSoftware {
    cl_program mProgram;
    cl_kernel mKernel;
    char mKernelName[128];
    char mFileName[1024];
    char mCompileOptions[1024];
};

oclHardware getOclHardware(cl_device_type type);

int getOclSoftware(oclSoftware &software, const oclHardware &hardware);

void release(oclSoftware& software);

void release(oclHardware& hardware);

const char *oclErrorCode(cl_int code);

#endif
