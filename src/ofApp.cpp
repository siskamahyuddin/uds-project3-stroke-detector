#include "ofApp.h"
#include "ofxCv.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace ofxCv;

void ofApp::setup(){
    ofSetWindowTitle("Webcam + Face Detection (Haar)");
    ofSetFrameRate(60);
    ofBackground(15,15,24);

    cam.setDeviceID(0);
    cam.setDesiredFrameRate(30);
    cam.setup(w, h);

    camView.allocate(w, h, OF_IMAGE_COLOR);
    edgeView.allocate(w, h, OF_IMAGE_GRAYSCALE); // optional

    // Load the cascade model from data/models/
    std::string cascadePath = ofToDataPath("models/haarcascade_frontalface_default.xml", true);
    cascadeLoaded = faceCascade.load(cascadePath);
    ofLogNotice() << "Cascade loaded: " << std::boolalpha << cascadeLoaded;

    if(!cascadeLoaded){
        ofLogError() << "Put haarcascade_frontalface_default.xml in bin/data/models/";
    }
}

void ofApp::update(){
    cam.update();
    if(!cam.isFrameNew()) return;

    camView.setFromPixels(cam.getPixels());
    Mat mat = toCv(camView);      // RGB order (OF)

    // Optional: Canny (from your previous step)
    {
        Mat gray, blurred, edges;
        cvtColor(mat, gray, COLOR_RGB2GRAY);
        GaussianBlur(gray, blurred, Size(5,5), 1.2);
        Canny(blurred, edges, cannyLow, cannyHigh);
        toOf(edges, edgeView);
        edgeView.update();
    }

    // --- Face detection ---
    faces.clear();
    if(cascadeLoaded){
        Mat gray;
        cvtColor(mat, gray, COLOR_RGB2GRAY);
        equalizeHist(gray, gray);  // improves robustness under lighting changes

        faceCascade.detectMultiScale(
            gray,
            faces,
            scaleFactor,
            minNeighbors,
            0, // flags (deprecated)
            Size(minFacePx, minFacePx)  // min size of face
            // , Size() // max size (let OpenCV decide)
        );
    }
}

void ofApp::keyPressed(int key){
    // Canny (optional)
    if(key=='q'||key=='Q') cannyLow  = std::min(255, cannyLow  + 5);
    if(key=='a'||key=='A') cannyLow  = std::max(0,   cannyLow  - 5);
    if(key=='w'||key=='W') cannyHigh = std::min(255, cannyHigh + 5);
    if(key=='s'||key=='S') cannyHigh = std::max(0,   cannyHigh - 5);

    // Haar params
    if(key=='1') scaleFactor = std::min(1.5f, scaleFactor + 0.02f);
    if(key=='2') scaleFactor = std::max(1.02f, scaleFactor - 0.02f);

    if(key=='3') minNeighbors = std::min(10, minNeighbors + 1);
    if(key=='4') minNeighbors = std::max(0,  minNeighbors - 1);

    if(key=='5') minFacePx = std::min(200, minFacePx + 10);
    if(key=='6') minFacePx = std::max(20,  minFacePx - 10);
}

void ofApp::draw(){
    ofSetColor(255);
    camView.draw(0, 0);

    // Optional: show edges on the right
    edgeView.draw(camView.getWidth(), 0);

    // Overlay: face rectangles + labels
    ofPushStyle();
    ofNoFill();
    ofSetLineWidth(2);
    ofSetColor(0, 255, 0);

    // draw rectangle for each face
    for(const auto& r : faces){
        ofDrawRectangle(r.x, r.y, r.width, r.height);
        ofDrawBitmapStringHighlight(
            "face",
            r.x, std::max(12, r.y - 4)
        );
    }
    ofPopStyle();

    // Show information
    int y = camView.getHeight() + 24;
    // count how many faces captured
    ofDrawBitmapStringHighlight("Faces: " + ofToString(faces.size()), 20, y);
    ofDrawBitmapStringHighlight("Haar: scale=" + ofToString(scaleFactor,2) +
                                " neighbors=" + ofToString(minNeighbors) +
                                " minFace=" + ofToString(minFacePx) + "px", 20, y+22);
    ofDrawBitmapStringHighlight("Canny: low="+ofToString(cannyLow)+" high="+ofToString(cannyHigh), 20, y+44);
    ofDrawBitmapStringHighlight("Keys: [1/2] scale +/-  |  [3/4] neighbors +/-  |  [5/6] minFace +/-", 20, y+66);
}



