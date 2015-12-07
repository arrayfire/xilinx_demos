/* Copyright (C) 2015 ArrayFire LLC - All Rights Reserved
 * Unauthorized copying of this file via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include "guiMain.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QPen>
#include <QGLWidget>

#include <string>

using namespace std;

guiMain::guiMain(QWidget *parent_widget)
    : QMainWindow(parent_widget), mWorker()
{
    // init all display objects
    this->setupUi(this);
    this->setWindowTitle("ArrayFire Image Procesing Demo");
    chkLoop->setChecked(true);

    // poor-man's double buffering. Create two QGraphicsScenes.
    mCurrentGraphicsScene = 0;
    mGraphicsScenes.push_back(new QGraphicsScene());
    mGraphicsScenes.push_back(new QGraphicsScene());

    // Distance, in pixels, between two adjacent images.
    mDisplayGapSize = 10; // pixels

    // Configure the text in the overlay label.
    mThroughputLabel = new QLabel(leftGraphicsView);
    mThroughputLabel->move(5, 5);
    mThroughputLabel->setFixedWidth(400);
    mThroughputLabel->setFixedHeight(125);
    mThroughputLabel->setStyleSheet("QLabel { font: 20pt; background-color : gray; color : white; }");

    mCreditLabel = new QLabel(leftGraphicsView);
    mCreditLabel->move(5, 130);
    mCreditLabel->setText(QString("Video frames (c) copyright 2008, Blender Foundation\nhttp://www.bigbuckbunny.org"));

    setAlgorithimNames();

    // Connect signals and slots
    connect(&mWorker, SIGNAL(finished()), this, SLOT(checkButtons()));
    connect(&mWorker, SIGNAL(statusUpdate(int, float, double)),
            this, SLOT(updateThroughput(int, float, double)));

    // plotting functions
    connect(&mWorker, SIGNAL(featuresFound(int, int *, int *)),
            this, SLOT(plotFeatures(int, int *, int *)));
    connect(&mWorker, SIGNAL(featuresFound(int, float *, float *, float *, float *)),
            this, SLOT(plotLines(int, float *, float *, float *, float *)));
    connect(&mWorker, SIGNAL(objectFound(float, float, float, float, float, float, float, float)),
    		this, SLOT(plotBox(float, float, float, float, float, float, float, float)));

    connect(&mWorker, SIGNAL(imageUpdate(QImage, QImage)),
            this, SLOT(updateImage(QImage, QImage)));

    connect(&mWorker, SIGNAL(renderScene(void)),
            this, SLOT(swapBuffers()));

    // Colorblind-safe color choices for points and lines.
    mColors.push_back(QColor(165,0,38));
    mColors.push_back(QColor(215,48,39));
    mColors.push_back(QColor(244,109,67));
    mColors.push_back(QColor(253,174,97));
    mColors.push_back(QColor(254,224,144));
    mColors.push_back(QColor(255,255,191));    // lime-green, useful default color
    mColors.push_back(QColor(224,243,248));
    mColors.push_back(QColor(171,217,233));
    mColors.push_back(QColor(116,173,209));
    mColors.push_back(QColor(69,117,180));
    mColors.push_back(QColor(49,54,149));
}

guiMain::~guiMain()
{
    // stop the worker thread if it is running, wait for it to terminate
    if(mWorker.isRunning())
    {
        mWorker.stop();
        mWorker.wait();
    }

    // delete the back QGraphicsScene (the other scene is owned by the
    // QGraphicsView, leftGraphicsView
    QGraphicsScene * scene = mGraphicsScenes[mCurrentGraphicsScene];
}

void guiMain::checkButtons()
{
    if(mWorker.isRunning())
        btnRun->setText("Stop");
    else
        btnRun->setText("Start");
}

/// Returns a pointer to the current back scene (like GL_BACK)
QGraphicsScene * guiMain::getScene()
{
    return mGraphicsScenes[mCurrentGraphicsScene];
}

/// Cause the background scene to be visible by swapping buffers.
void guiMain::swapBuffers()
{
    // swap the scenes
    QGraphicsScene * scene = getScene();
    this->leftGraphicsView->setScene(scene);
    mCurrentGraphicsScene = (mCurrentGraphicsScene + 1) % mGraphicsScenes.size();
}

/// Plot a box whose corners are located at the specified locations
void guiMain::plotBox(
		float x0, float y0,
		float x1, float y1,
		float x2, float y2,
		float x3, float y3)
{
    QPen pen;
    pen.setWidth(3);
    pen.setBrush(Qt::green);

    QGraphicsScene * scene = getScene();

    int offset = mImageWidth + mDisplayGapSize;

    // Ensure homography will not fall outside the image limits
    float py0 = min(max(0.f, y0), max((float)mImageHeight-1, (float)mRightImageHeight-1));
    float py1 = min(max(0.f, y1), max((float)mImageHeight-1, (float)mRightImageHeight-1));
    float py2 = min(max(0.f, y2), max((float)mImageHeight-1, (float)mRightImageHeight-1));
    float py3 = min(max(0.f, y3), max((float)mImageHeight-1, (float)mRightImageHeight-1));
    float px0 = min(max(0.f, offset+x0), (float)mImageWidth+mRightImageWidth+mDisplayGapSize-1);
    float px1 = min(max(0.f, offset+x1), (float)mImageWidth+mRightImageWidth+mDisplayGapSize-1);
    float px2 = min(max(0.f, offset+x2), (float)mImageWidth+mRightImageWidth+mDisplayGapSize-1);
    float px3 = min(max(0.f, offset+x3), (float)mImageWidth+mRightImageWidth+mDisplayGapSize-1);

    // The QRect class draws rectangles that are aligned to the x-y axes,
    // but in our case we might have rectangles that are rotated. Thus
    // we draw the rectangles one side at a time.
    scene->addLine(px0, py0, px1, py1, pen);
    scene->addLine(px1, py1, px2, py2, pen);
    scene->addLine(px2, py2, px3, py3, pen);
    scene->addLine(px3, py3, px0, py0, pen);
}

/// Plots discovered features as circles on the current back buffer
void guiMain::plotFeatures(int n_features, int * x, int * y)
{
    int offset = mImageWidth + mDisplayGapSize;
    QGraphicsScene * scene = getScene();

    QPen pen;
    pen.setColor(mColors[mColors.size() / 2]);

    for(int i = 0; i < n_features; i++)
    {
        scene->addEllipse((float)x[i] - 2 + offset, (float)y[i] - 2, 5, 5, pen);
    }

    // free buffers
    delete[] x;
    delete[] y;
}

/// Plots lines connecting discovered features in two images
void guiMain::plotLines(int n_features, float * origin_x, float * origin_y,
        float * dest_x, float * dest_y)
{
    QPen pen;
    QBrush brush;

    int offset = mImageWidth + mDisplayGapSize;

    QGraphicsScene * scene = getScene();

    for(int i = 0; i < n_features; i++)
    {
        // Circles for left and right images:
        scene->addEllipse(origin_x[i] - 2, origin_y[i] - 2, 5, 5, pen);
        scene->addEllipse(dest_x[i] - 2 + offset, dest_y[i] - 2, 5, 5, pen);

        // Line connecting the origin and destination circles
        scene->addLine(origin_x[i] + 3, origin_y[i],
                dest_x[i] - 2 + offset, dest_y[i], pen);

        // switch the color
        pen.setColor(mColors[i % mColors.size()]);
    }

    // free buffers
    delete[] origin_x;
    delete[] origin_y;
    delete[] dest_x;
    delete[] dest_y;
}

/// Updates the display with current throughput information.
void guiMain::updateThroughput(int frame_count, float algo_fps, double elapsedSeconds)
{
    double render_fps = frame_count / elapsedSeconds;

    QString s_count = QString::number(frame_count);
    QString s_time  = QString::number(elapsedSeconds);
    QString s_render_fps   = QString::number(render_fps);
    QString s_algo_fps   = QString::number(algo_fps);

    QString label_text = QString("Frames: %1\nSeconds: %2\nRendering FPS: %3\nProcessing FPS: %4").arg(s_count, s_time, s_render_fps, s_algo_fps);

    mThroughputLabel->setText(label_text);
}

/// Updates the images in the back buffer. Call swapBuffers to display them.
void guiMain::updateImage(QImage left_image, QImage right_image)
{
    mImageWidth = left_image.width();
    mImageHeight = left_image.height();
    mRightImageWidth = right_image.width();
    mRightImageHeight = right_image.height();
    QGraphicsScene * scene = getScene();

    // Move the graphics items from the left to right scenes
    auto items = scene->items();
    for(auto item: items)
    {
        scene->removeItem(item);
        delete item;
    }

    // Add the left image to the scene
    QGraphicsPixmapItem * item = new QGraphicsPixmapItem( QPixmap::fromImage(left_image));
    scene->addItem(item);
    // add the right image to the scene and shift it.
    item = new QGraphicsPixmapItem( QPixmap::fromImage(right_image));
    scene->addItem(item);
    item->setOffset(mImageWidth + mDisplayGapSize, 0);
}

/// Open a dialog to let the user choose a directory of images to process
void guiMain::on_btnOpenDirectory_clicked()
{
    // Open a dialog, get a list of file that the user selected:
     QFileDialog dialog(this);
     dialog.setFileMode(QFileDialog::Directory);
     dialog.setOption(QFileDialog::ShowDirsOnly);

    QStringList directories;
    if (dialog.exec())
    {
        directories = dialog.selectedFiles();
        // Make the directory if it does not exist
        if(!QDir(directories[0]).exists())
            QDir().mkdir(directories[0]);

        txtDirectory->setText(directories[0]);
    }
}

/// Start or stop the ArrayFire thread
void guiMain::on_btnRun_clicked()
{
    //
    QString image_directory = txtDirectory->text();
    QString demo_function = cboDemoFunction->currentText();

    if(image_directory.size() == 0)
    {
        // An error was thrown. Display a message to the user
        QMessageBox msgBox;
        msgBox.setWindowTitle("Additional input needed");
        msgBox.setText(QString("Please select a directory of images to parse."));
        msgBox.exec();

        return;
    }

    mWorker.setExecPath(getExecPath());

    if(mWorker.isRunning())
    {
        mWorker.stop();
    }
    else
    {
        mWorker.setupDemo(image_directory, demo_function);
        mWorker.start();
    }

    checkButtons();
}

/// Enable or disable the ArrayFire thread looping through the images
void guiMain::on_chkLoop_stateChanged ( int state )
{
    if(state == Qt::Checked)
        mWorker.setLoop(true);
    else
        mWorker.setLoop(false);

}

/// Populates the drop-down menu with a series of algorithms
void guiMain::setAlgorithimNames()
{
    mapDemoTypes demoTypes = mWorker.getDemoTypes();

    for(auto dType: demoTypes)
        cboDemoFunction->addItem(dType.second);
}

void guiMain::setExecPath(std::string s)
{
    execPath = s;
}

std::string guiMain::getExecPath()
{
    return execPath;
}
