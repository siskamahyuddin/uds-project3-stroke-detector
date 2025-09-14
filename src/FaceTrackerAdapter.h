#pragma once
#include "ofMain.h"
#include "ofxFaceTracker2.h"
#include <vector>
#include "Landmarks68.h"
#include "Math2D.h"
#include "GuideOval.h"

struct DerolledData {
    std::vector<glm::vec2> points;
    glm::vec2 leftEyeC{0,0};
    glm::vec2 rightEyeC{0,0};
    glm::vec2 faceCenter{0,0};
    float iod = 1.f;
    bool  insideGuide = false;
    bool  valid = false;
};

class FaceTrackerAdapter {
public:
    void setup(const std::string& modelPath);
    void update(ofVideoGrabber& grabber);
    bool hasFace() const;
    bool getDerolled(DerolledData& out) const;
    ofxFaceTracker2& tracker() { return tracker_; }
private:
    ofxFaceTracker2 tracker_;
};
