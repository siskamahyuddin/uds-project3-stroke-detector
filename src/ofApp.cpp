#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofSetDataPathRoot(ofFile(__BASE_FILE__).getEnclosingDirectory()+"../bin/data/model/");
// Setup grabber
    grabber.setup(1280,720);

// Setup tracker
    tracker.setup();
}

//--------------------------------------------------------------
void ofApp::update(){
    grabber.update();

// Update tracker when there are new frames
    if(grabber.isFrameNew()){
        tracker.update(grabber);
    }

}

//--------------------------------------------------------------
void ofApp::draw(){
// Draw camera image
    grabber.draw(0, 0);

// Draw tracker landmarks
    tracker.drawDebug();

// Draw estimated 3d pose
    tracker.drawDebugPose();

// Draw text UI
    ofDrawBitmapStringHighlight("Framerate : "+ofToString(ofGetFrameRate()), 10, 20);
    ofDrawBitmapStringHighlight("Tracker thread framerate : "+ofToString(tracker.getThreadFps()), 10, 40);

// #ifndef __OPTIMIZE__
    ofSetColor(ofColor::red);
    ofDrawBitmapString("Warning! Run this app in release mode to get proper performance!",10,60);
    ofSetColor(ofColor::white);
// #endif

}
