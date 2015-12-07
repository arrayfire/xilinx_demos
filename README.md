## Xilinx Demos

Repository with ArrayFire demos for Xilinx FPGAs, showing its capabilities with OpenCL. Currently only a demo of the FAST feature detector is available.

### Requirements

* SDAccel 2015.1 release 5 or newer
* GCC 4.6.x or newer
* CMake
* Qt 4.6.x

### Instructions

Currently, the FAST demo contained here has been only tested with the Alpha Data ADM-PCIE-7V3 card, but it should work properly with any SDAccel supported cards.

For a different card, the following modifications (consult SDAccel's documentation for information on precise device names) may be necessary prior to the instructions that will follow:

* Environment variable `XCL_PLATFORM` in file `fast/setupenv.sh`
* Target device in line starting with `add_device` in file `fast/board_compile.tcl`

The instructions that will follow assume that you are using bash, for other shell, please refer to its manual for the necessary changes.

#### Building

* Check that you have at least SDAccel 2015.1 release 5 installed and xdma kernel module properly compiled and installed.
* Set the path to SDAccel installation directory (the default usually is `/opt/Xilinx/SDAccel/<version>`, where `<version>` should be changed to the version number installed, here we use `2015.1`):

```
export XILINX_SDACCEL=/opt/Xilinx/SDAccel/2015.1
export PATH=${XILINX_SDACCEL}/bin:$PATH
```

* Compile the FPGA binaries (the image width is fixed to 640 pixels, if you need a different width, make sure to change the `WIDTH` macro in `fast/fast_pipeline_nonmax.cl` before proceeding). This may take more than one hour, depending on the CPU:

```
cd xilinx_demos/fast
sdaccel board_compile.tcl
```

* Install the necessary kernel and runtime files to the `bin` directory:

```
./install.sh
```

* Build the host application (if the system is CentOS 6.6 or other OS that does not have C++11 support, use the GCC compiler provided with SDAccel, otherwise ignore `CMAKE_CXX_COMPILE` and `CMAKE_C_COMPILER` flags):

```
cd ..
mkdir build && cd build
cmake -DCMAKE_CXX_COMPILER=${XILINX_SDACCEL}/lnx64/tools/gcc/bin/c++ -DCMAKE_C_COMPILER=${XILINX_SDACCEL}/lnx64/tools/gcc/bin/gcc -DOpenCL_INCLUDE_DIR=${XILINX_SDACCEL}/runtime/include/1_2 -DOpenCL_LIBRARY=${XILINX_SDACCEL}/runtime/lib/x86_64/libOpenCL.so ..
make -j4
```

#### Running

* Verify that the environment variable `XILINX_SDACCEL` in `bin/setupenv.sh` is pointing to the SDAccel installation path.
* Execute the GUI binary:

```
cd ../bin
source setupenv.sh
./xilinx-demo
```

* In the interface click on Open Directory and select a directory containing the images to detect features on (all of them must be 640 pixels wide, or have the same width as chosen during the building step).
* From the list select "FAST" (leave loop ticked if you want it to run continuously).
* Press start.

### Known bugs

A problem with the scheduler may cause rendering FPS to be too low. For a workaround, try increasing variable `desiredFramerate` in `src/CAFWorker.cpp` from 30 to 40 or 50.

### Troubleshooting

* Error: `./xilinx-demo: /usr/lib64/libstdc++.so.6: version GLIBCXX_3.4.14 not found (required by ./xilinx-demo)`
* Possible cause: You don’t have libstdc++.so.6 from the compiler provided by Xilinx.
* Solution: Check that you have the file bin/lnx64.o/libstdc++.so.6 and that it is in your `LD_LIBRARY_PATH`, check that you ran `install.sh` in the `fast/` directory, then check that you are in `bin/` directory and that you ran `source setupenv.sh`.

* Error:
```
Loading fast_pipeline.xclbin
[/tmp/15013/tempFile_0]
  End-of-central-directory signature not found.  Either this file is not
  a zipfile, or it constitutes one disk of a multi-part archive.  In the
  latter case the central directory and zipfile comment will be found on
  the last disk(s) of this archive.
unzip:  cannot find zipfile directory in one of /tmp/15013/tempFile_0 or
        /tmp/15013/tempFile_0.zip, and cannot find /tmp/15013/tempFile_0.ZIP, period.
chmod: cannot access `/tmp/15013/sys_emulation_0': No such file or directory
sh: ./elaborate_sysemulation.sh: No such file or directory
FATAL ERROR : Simulation process did not launch
libprotobuf ERROR google/protobuf/message_lite.cc:123] Can't parse message of type "response_packet_info" because it is missing required fields: size
```
* Possible cause 1: The xdma kernel module is not loaded.
* Solution 1: Try loading the xdma kernel module.
* Possible cause 2: Runtime files are not in the bin/ directory.
* Solution 2: Verify that you installed the necessary kernel and runtime files (check building instructions)
* Possible cause 3: The environment variables were not set.
* Solution 3: When inside `bin/`, execute `source setupenv.sh`
