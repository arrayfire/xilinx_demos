/* Copyright (C) 2015 ArrayFire LLC - All Rights Reserved
 * Unauthorized copying of this file via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef SRC_CAFWORKER_H_
#define SRC_CAFWORKER_H_

#include <QThread>
#include <QString>
#include <atomic>
#include <chrono>
#include <map>
#include <cmath>
#include <string>

#include <QImage>

#include "fast.h"

using namespace std;
using namespace std::chrono;

enum eDemoTypes
{
    NONE,
    FAST,
    ORB,
    RESIZE_NEAREST_SHRINK_2X,
    RESIZE_NEAREST_EXPAND_2X,
    ROTATE,
    ORB_FEATURE_TRACKING,
	OBJECT_TRACKING,
};

typedef map<eDemoTypes, QString> mapDemoTypes;

class CAFWorker : public QThread
{
    Q_OBJECT

protected:
    atomic<bool> mRun;
    atomic<bool> mLoopDemo;
    atomic<bool> mReuseImage;

    high_resolution_clock::time_point mStart;

    mapDemoTypes mDemoTypes;

    QString mDirectory;    /// < Directory containing images which will be parsed
    QString mDemoName;  /// < Name of the demo that will be executed.

    unsigned int mFrameCounter;
    unsigned int mAlgorithmCounter;

    std::string execPath;

public:
    CAFWorker();
    virtual ~CAFWorker();

    //void AFImageToUchar(af::array image, uchar ** ouput_image,
    //        int & t_left_image, int & left_image_height);

    void exportImages(QImage left_image, QImage right_image);

    //static af::array load_image(std::string filename);

    void convertRGB2Gray(int** out_ptr, int& width, int& height, QImage& image);

    void setupDemo(const QString & imageDirectory, const QString & demo_name);
    void setLoop(bool repeatDemo);

    mapDemoTypes getDemoTypes() { return mDemoTypes; };
    eDemoTypes getDemoType(string demoName);

    double elapsedSeconds();

    void stop();
    void run();

    void setExecPath(std::string s);
    std::string getExecPath();

    signals:

    void imageUpdate(QString filename, double scale, double rotation);
    void imageUpdate(uchar * left_image, int left_image_width, int left_image_height,
            uchar * right_image, int right_image_width, int right_image_height);
    void imageUpdate(QImage left_image, QImage right_image);


    //void statusUpdate(int rendered_frames, int processed_frames, double elapsedTime);
    void statusUpdate(int rendered_frames, float processed_frames, double elapsedTime);

    void featuresFound(int n_features, int * x, int * y);

    void featuresFound(int n_features, float * origin_x, float * origin_y, float * dest_x, float * dest_y);

    void objectFound(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);

    void renderScene();

};

#endif /* SRC_CAFWORKER_H_ */
