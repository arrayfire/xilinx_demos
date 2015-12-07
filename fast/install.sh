#!/bin/sh

BIN_PATH=../bin

cp -a board_compilation_solution/pkg/pcie/lib ${BIN_PATH}
cp -a board_compilation_solution/pkg/pcie/runtime ${BIN_PATH}
cp board_compilation_solution/pkg/pcie/*.xclbin ${BIN_PATH}

echo "# Export path to SDAccel installation directory
export XILINX_SDACCEL=$XILINX_SDACCEL

# Set paths including necessary shared libraries and runtime files
export LD_LIBRARY_PATH=\$PWD/lnx64.o:\$PWD/runtime/lib/x86_64
export XILINX_OPENCL=\$PWD

# Set the platform type (currently only the one below is tested, refer to
# SDAccel documentation for different platforms)
export XCL_PLATFORM=xilinx_adm-pcie-7v3_1ddr_1_1" > ${BIN_PATH}/setupenv.sh
