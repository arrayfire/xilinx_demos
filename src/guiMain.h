/* Copyright (C) 2015 ArrayFire LLC - All Rights Reserved
 * Unauthorized copying of this file via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef CGUIMAIN_H
#define CGUIMAIN_H

#include <QMainWindow>
#include <QString>
#include <QLabel>
#include <QGraphicsScene>
#include <QColor>
#include <string>

#include "CAFWorker.h"
#include "ui_guiMain.h"

class guiMain : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

private:
    int mImageWidth;
    int mImageHeight;
    int mRightImageWidth;
    int mRightImageHeight;
    int mDisplayGapSize;
    vector<QColor> mColors;
    std::string execPath;

public:
    guiMain(QWidget *parent = 0);
    virtual ~guiMain();

    vector<QGraphicsScene*> mGraphicsScenes;
    int mCurrentGraphicsScene;

//    QGraphicsScene mGraphicsScene;
    QLabel * mThroughputLabel;
    QLabel * mCreditLabel;

    CAFWorker mWorker; /// < Worker thread to keep UI from blocking while demo is running

protected:

    QGraphicsScene * getScene();

    void setAlgorithimNames();


public slots:

    void checkButtons();
    /// Plot a box whose corners are located at the specified locations
    void plotBox(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);
    void plotFeatures(int n_features, int * x, int * y);
    void plotLines(int n_features, float * origin_x, float * origin_y,
            float * dest_x, float * dest_y);

    void swapBuffers();

    //void updateImage(uchar * left_image_data, int left_width, int left_height,
    //        uchar * right_image_data, int right_width, int right_height);
    void updateImage(QImage left_image, QImage right_image);

    //void updateThroughput(int frame_count, int algorithim_count, double elapsedSeconds);
    void updateThroughput(int frame_count, float algo_fps, double elapsedSeconds);
    void on_btnOpenDirectory_clicked();
    void on_btnRun_clicked();
    void on_chkLoop_stateChanged ( int state );

    void setExecPath(std::string s);
    std::string getExecPath();

};

#endif // CGUIMAIN_H
