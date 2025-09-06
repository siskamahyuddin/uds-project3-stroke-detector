#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxDlib.h"
#include "ofxOpenCv.h"
#include "ofxFaceTracker2.h"

class ofApp : public ofBaseApp{

    public:
        void setup();
        void update();
        void draw();

        ofxFaceTracker2 tracker;
        ofVideoGrabber grabber;
};
