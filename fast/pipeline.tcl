# SDAccel command script
# Design = fast example

# Define a solution name
create_solution -name pipeline_solution -dir . -force

set_param compiler.preserveHlsOutput 1

# Define the target platform of the application
add_device -vbnv xilinx:adm-pcie-7v3:1ddr:1.1

# Host source files
add_files "main.cpp"
add_files "oclErrorCodes.cpp"
add_files "oclHelper.cpp"
add_files "pgm.cpp"

# Header files
add_files "oclHelper.h"
set_property file_type "c header files" [get_files "oclHelper.h"]
add_files "pgm.h"
set_property file_type "c header files" [get_files "pgm.h"]

# Kernel definition
create_kernel locate_features -type clc
add_files -kernel [get_kernels locate_features] "fast_pipeline.cl"

# Define binary containers
create_opencl_binary fast
set_property region "OCL_REGION_0" [get_opencl_binary fast]
create_compute_unit -opencl_binary [get_opencl_binary fast] -kernel [get_kernels locate_features] -name K1

# Compile the design for CPU based emulation
compile_emulation -flow cpu -opencl_binary [get_opencl_binary fast]

# Generate the system estimate report
report_estimate

# Run the design in CPU emulation mode
run_emulation -flow cpu -args "-d acc -k fast.xclbin -f ../../../../data/square.pgm"
