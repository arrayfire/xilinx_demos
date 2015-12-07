/* Copyright (C) 2015 ArrayFire LLC - All Rights Reserved
 * Unauthorized copying of this file via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef MAIN_CPP_
#define MAIN_CPP_

#include <QtGui>
#include <QApplication>

using namespace std;

#include "guiMain.h"

int main(int argc, char* argv[])
{
    // Pass off to the GUI:
    QApplication app(argc, argv);

    // Get path of executable
    std::string filePath(argv[0]);
    size_t last = filePath.find_last_of('/');
    std::string path = filePath.substr(0, last);

    guiMain main_window;
    main_window.setExecPath(path);
    main_window.show();

    return app.exec();
}

#endif // MAIN_CPP_
