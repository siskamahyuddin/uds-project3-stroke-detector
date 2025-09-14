#pragma once
#include "ofMain.h"
#include "FaceTrackerAdapter.h"
#include "SmileFlow.h"
#include "ViewRenderer.h"

class ofApp : public ofBaseApp {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;
    void windowResized(int w, int h) override;
private:
    ofVideoGrabber grabber_;
    FaceTrackerAdapter tracker_;
    SmileFlow         flow_;
    ViewRenderer      view_;
    bool        mirrorView_ = true;
    ofTrueTypeFont fontLarge_, fontMedium_;
};
