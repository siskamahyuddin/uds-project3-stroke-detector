#pragma once
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include <opencv2/opencv.hpp>

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);

    // Camera
    ofVideoGrabber cam;

    // Display buffers
    ofImage camView;     // color frame
    ofImage edgeView;    // (optional) Canny result — keep from earlier if you like

    // Canny controls (optional; keep from earlier)
    int w = 1280, h = 720;
    int cannyLow = 80, cannyHigh = 150;

    // --- Face detection ---
    cv::CascadeClassifier faceCascade;
    bool cascadeLoaded = false;
    std::vector<cv::Rect> faces;

    // Runtime controls
    float scaleFactor = 1.1f;   // image pyramid step for Haar
    int   minNeighbors = 3;     // detection confidence filter
    int   minFacePx = 60;       // minimum face size (pixels)
};
