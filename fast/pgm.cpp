/*******************************************************
 * Copyright (c) 2015, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include "pgm.h"

int readPGM(int** img, size_t* w, size_t* h, std::string fName)
{
    std::ifstream inFile;
    inFile.open(fName.c_str());

    char format[256];
    char expectedFormat[] = "P2";
    inFile.getline(format, 256);

    char comments[256];
    inFile.getline(comments, 256);

    if (strcmp(expectedFormat, format) != 0 ||
        comments[0] != '#')
        return PGM_WRONG_FORMAT;

    int width, height;
    inFile >> width >> height;
    if (width <= 0 || height <= 0)
        return PGM_WRONG_DIMENSIONS;

    *img = new int[width * height];

    *w = (size_t)width;
    *h = (size_t)height;

    for (size_t i = 0; i < (size_t)height; i++) {
        for (size_t j = 0; j < (size_t)width; j++) {
            int idx = i*width + j;
            int p;
            inFile >> p;
            (*img)[idx] = p;
        }
    }

    inFile.close();

    return PGM_SUCCESS;
}

void writePGM(std::string fName, int* img, size_t w, size_t h)
{
    std::ofstream outFile;
    outFile.open(fName.c_str());
    outFile << "P2" << std::endl;
    outFile << "# Simple square sample" << std::endl;
    outFile << w << " " << h << std::endl;
    for (size_t i = 0; i < h; i++) {
        for (size_t j = 0; j < w; j++) {
            size_t idx = i*w + j;
            outFile << img[idx] << " ";
        }
        outFile << std::endl;
    }

    outFile.close();
}
