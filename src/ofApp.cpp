#include "ofApp.h"
#include "Landmarks68.h"

void ofApp::setup() {
    fontMedium_.load("verdana.ttf", 28, true, true);
    fontLarge_.load("verdana.ttf", 48, true, true);

    grabber_.setup(1280, 720);
    tracker_.setup("model/shape_predictor_68_face_landmarks.dat");

    mirrorView_ = true;
    ofSetFrameRate(60);
    ofSetBackgroundColor(15, 15, 15);
}

void ofApp::update() {
    static float last = ofGetElapsedTimef();
    float now = ofGetElapsedTimef();
    float dt = now - last;
    last = now;

    grabber_.update();
    tracker_.update(grabber_);

    DerolledData der;
    bool haveDer = tracker_.getDerolled(der);

    SmileFlow::Inputs in;
    in.hasFace        = tracker_.hasFace();
    in.insideGuide    = haveDer ? der.insideGuide : false;
    in.dt             = dt;
    in.iod            = haveDer ? der.iod : 1.f;
    in.faceCenter     = haveDer ? der.faceCenter : glm::vec2(0,0);
    in.derolledPoints = haveDer ? der.points : std::vector<glm::vec2>{};
    if (haveDer && der.points.size() > LM_RIGHT_MOUTH_CORNER) {
        in.haveMouth  = true;
        in.mouthLeft  = der.points[LM_LEFT_MOUTH_CORNER];
        in.mouthRight = der.points[LM_RIGHT_MOUTH_CORNER];
    }
    flow_.update(in);
}

void ofApp::draw() {
    RenderData rd;
    rd.mirrorView = mirrorView_;
    rd.fontMedium = &fontMedium_;
    rd.fontLarge  = &fontLarge_;
    rd.stage      = flow_.stage();
    rd.abnormal   = flow_.abnormal();
    rd.lines      = flow_.uiLines();
    rd.stabilityProgress = flow_.stabilityProgress();
    rd.smileIntensity    = flow_.smileIntensity();
    rd.smileAsymmetry    = flow_.smileAsymmetry();
    rd.grabber = &grabber_;
    rd.tracker = &tracker_.tracker();
    view_.draw(rd);
}

void ofApp::keyPressed(int key) {
    if (key == 'r' || key == 'R') {
        flow_.reset();
    } else {
        if (flow_.stage() == SmileFlow::STAGE_HOME) {
            flow_.reset();
        }
    }
}

void ofApp::windowResized(int, int) {
    // No-op
}
