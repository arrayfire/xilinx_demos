/* Copyright (C) 2015 ArrayFire LLC - All Rights Reserved
 * Unauthorized copying of this file via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include <QDir>
#include <QStringList>

#include <QDebug>
#include "CAFWorker.h"

CAFWorker::CAFWorker() {
    mRun = false;
    mLoopDemo = false;
    mReuseImage = false;

    // define supported demo types here, remember to add the enum to CAFWorker.h
    mDemoTypes[NONE] = QString("FAST");
}

CAFWorker::~CAFWorker() {
    // TODO Auto-generated destructor stub
}

/// Compute the number of ellapsed seconds since the timer started
double CAFWorker::elapsedSeconds()
{
    // Compute the time that has elapsed since the timer was started
    auto now = high_resolution_clock::now();
    auto time = duration_cast<milliseconds>(now - mStart).count();

    return double(time) / 1000;
}

/// Export the current images from ArrayFire to pointers and send the data
/// to the QT GUI.
///
/// Note: Emit the renderScene() signal after all data to displayed has been
/// copied to cause the GUI to render.
void CAFWorker::exportImages(QImage left_image, QImage right_image)
{
    uchar * t_left_image = nullptr;
    uchar * t_right_image = nullptr;
    int left_width = 0, left_height = 0;
    int right_width = 0, right_height = 0;

    //AFImageToUchar(left_image, &t_left_image, left_width, left_height);
    //AFImageToUchar(right_image, &t_right_image, right_width, right_height);
    t_left_image = left_image.bits();
    left_width = left_image.width();
    left_height = left_image.height();

    t_right_image = right_image.bits();
    right_width = right_image.width();
    right_height = right_image.height();

    // send the image as a QT signal.
    emit imageUpdate(t_left_image, left_width, left_height,
            t_right_image, right_width, right_height);
}

/// Get the current demo type
eDemoTypes CAFWorker::getDemoType(string demoName)
{
    for(auto pair: mDemoTypes)
    {
        if(pair.second.toStdString() == demoName)
            return pair.first;
    }

    return NONE;
}

void CAFWorker::convertRGB2Gray(int** out_ptr, int& width, int& height, QImage& image)
{
    const uchar* image_ptr = image.bits();
    //QRgb* in_data = (QRgb*)image_ptr;
    width = image.width();
    height = image.height();

    *out_ptr = new int[width * height];
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            //(*out_ptr)[h*width+w] = qGray(in_data[h*width+w]);
            uchar r = image_ptr[h*width*4 + w*4 + 1];
            uchar g = image_ptr[h*width*4 + w*4 + 2];
            uchar b = image_ptr[h*width*4 + w*4 + 3];
            int gray = round(r*0.2126f + g*0.7152f + b*0.0722f);
            (*out_ptr)[h*width + w] = gray;
        }
    }
}

/// Instruct this thread to stop. Call thread.wait() to wait for termination.
void CAFWorker::stop()
{
    mRun = false;
}

/// Set the image source directory and demo name
void CAFWorker::setupDemo(const QString & imageDirectory, const QString & demo_name)
{
    mDirectory = imageDirectory;
    mDemoName = demo_name;
}

/// Toggle to  instruct the thread to loop over the directory of images
void CAFWorker::setLoop(bool repeatDemo)
{
    mLoopDemo = repeatDemo;
}

/// Compute the time ellapsed in milliseconds.
int deltaTimeMilliseconds(high_resolution_clock::time_point timer_start)
{
    auto current_time = high_resolution_clock::now();
    return  duration_cast<milliseconds>(current_time - timer_start).count();
}

/// Compute the time ellapsed in microseconds.
int deltaTimeMicroseconds(high_resolution_clock::time_point timer_start)
{
    auto current_time = high_resolution_clock::now();
    return  duration_cast<microseconds>(current_time - timer_start).count();
}

/// Main thread function
void CAFWorker::run()
{
    // Indicate that we should be running
    mRun = true;
    int desiredFramerate = 40;

    int msPerFrame = int(1.0 / desiredFramerate * 1000);

    // get a list of images in the directory
    QStringList nameFilter;
    nameFilter << "*.png" << "*.jpg" << "*.jpeg";    // support PNG and JPEG files
    QDir directory(mDirectory);
    QStringList imageFiles = directory.entryList(nameFilter);

    // locals for the loop
    QString fileName;
    QString imageName;
    bool firstAlgoLoop = true;
    bool noMatch = false;
    eDemoTypes demoType = getDemoType(mDemoName.toStdString());

    mFrameCounter = 1;
    mAlgorithmCounter = 1;
    float algoFPS = 0.0f;
    mStart = high_resolution_clock::now();

    for(int i = 0; i < imageFiles.size(); i++)
    {
        // stop if requested
        if(!mRun)
            break;

        // load an image
        imageName = imageFiles[i];
        fileName = mDirectory + '/' + imageName;
        QImage image(fileName.toUtf8().constData());

        int* image_ptr = nullptr;
        int image_width = 0;
        int image_height = 0;
        convertRGB2Gray(&image_ptr, image_width, image_height, image);

        // signal to the GUI that it should update the image
        //emit statusUpdate(mFrameCounter, mAlgorithmCounter, elapsedSeconds());
        emit statusUpdate(mFrameCounter, algoFPS, elapsedSeconds());

        // inner loop that does work on the GPU as fast as possible between
        // rendering frames
        firstAlgoLoop = true;
        auto algorithmTimer = high_resolution_clock::now();
        while(deltaTimeMilliseconds(algorithmTimer) < msPerFrame)
        {
            switch(demoType)
            {
            case NONE:
            case FAST:
                if(firstAlgoLoop)
                {
                    emit imageUpdate(image, image);

                    std::vector<int> x, y, score;
                    auto fastTimer = high_resolution_clock::now();
                    fast(x, y, score, image_ptr, image_width, image_height, 200, execPath);
                    algoFPS = 1e6f / deltaTimeMicroseconds(fastTimer);

                    int N = x.size();
                    int* h_x = new int[N];
                    int* h_y = new int[N];
                    for (int j = 0; j < N; j++) {
                        h_x[j] = x[j];
                        h_y[j] = y[j];
                    }
                    emit featuresFound((int)N, h_x, h_y);

                    emit renderScene();
                }
                else {
                    usleep(500);
                }

                break;
            }

            firstAlgoLoop = false;
            mAlgorithmCounter++;
        }

        if (image_ptr != nullptr)
            delete[] image_ptr;

        // Start the loop over if necessary
        if(i == imageFiles.size() - 1 && mLoopDemo)
            i = 0;

        // increment the frame counter
        mFrameCounter++;

    }

    // indicate the thread has completed
    emit finished();

}

void CAFWorker::setExecPath(std::string s)
{
    execPath = s;
}

std::string CAFWorker::getExecPath()
{
    return execPath;
}
