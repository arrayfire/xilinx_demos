/*******************************************************
 * Copyright (c) 2015, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#ifndef _PGM_H_
#define _PGM_H_

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

enum PGM_ERR
{
    PGM_SUCCESS,
    PGM_WRONG_FORMAT,
    PGM_WRONG_DIMENSIONS
};

int readPGM(int** img, size_t* w, size_t* h, std::string fName);

void writePGM(std::string fName, int* img, size_t w, size_t h);

#endif
