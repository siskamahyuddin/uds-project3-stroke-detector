#pragma once
#include "ofMain.h"
#include "ofxFaceTracker2.h"
#include "SmileFlow.h"

struct RenderData {
    bool mirrorView = true;
    ofTrueTypeFont* fontMedium = nullptr;
    ofTrueTypeFont* fontLarge  = nullptr;
    SmileFlow::Stage stage = SmileFlow::STAGE_HOME;
    bool abnormal = false;
    std::vector<std::string> lines;
    float stabilityProgress = 0.f;
    float smileIntensity = 0.f;
    float smileAsymmetry = 0.f;
    ofVideoGrabber*  grabber = nullptr;
    ofxFaceTracker2* tracker = nullptr;
};

class ViewRenderer {
public:
    void draw(const RenderData& rd);
};
