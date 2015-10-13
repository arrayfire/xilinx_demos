XILINX_OPENCL := $(XILINX_SDACCEL)
DSA := xilinx:adm-pcie-7v3:1ddr:1.0
XOCC := $(XILINX_SDACCEL)/bin/xocc
CPP := g++

OPENCL_INC := $(XILINX_OPENCL)/runtime/include/1_2
OPENCL_LIB := $(XILINX_OPENCL)/runtime/lib/x86_64

CXXFLAGS := -Wall -Werror -Wno-error=uninitialized -pthread
CLFLAGS := -g --xdevice $(DSA)
XOCCFLAGS := -t sw_emu

.PHONY: all
all: exe fast_xclbin fast_pipeline_xclbin

.PHONY:
fast_xclbin: fast.xclbin
fast_pipeline_xclbin: fast_pipeline.xclbin

.PHONY: exe
exe: fast

fast: main.cpp oclErrorCodes.cpp oclHelper.cpp oclHelper.h pgm.cpp pgm.h
	$(CXX) $(CXXFLAGS) -I$(OPENCL_INC) -L$(OPENCL_LIB) -lOpenCL -o $@ main.cpp oclErrorCodes.cpp oclHelper.cpp pgm.cpp

fast.xclbin: fast.cl
	$(XOCC) $(XOCCFLAGS) $(CLFLAGS) $< -o $@

fast_pipeline.xclbin: fast_pipeline.cl
	$(XOCC) $(XOCCFLAGS) $(CLFLAGS) $< -o $@

clean:
	rm -rf fast.xclbin fast_pipeline.xclbin fast xocc* sdaccel*
