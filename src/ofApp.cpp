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
    
    // Draw tracker landmarks (addon’s built-in debug overlay)
    tracker.drawDebug();

    // Draw all 68 points and their indices (minimal addition)
    {
        // We access instances non-const because getLandmarks() is not a const method.
        auto& instances = tracker.getInstances();
        if (!instances.empty()) {
            // Draw for every detected face (usually 1)
            for (auto& inst : instances) {
                auto& lms = inst.getLandmarks();
                auto pts  = lms.getImagePoints(); // std::vector<glm::vec2>, typically 68 points

                ofPushStyle();
                ofSetColor(ofColor::yellow); // dots
                for (const auto& p : pts) {
                    ofDrawCircle(p, 2);
                }

                // Label each landmark with its index for easy reference
                ofSetColor(ofColor::white);
                for (size_t i = 0; i < pts.size(); ++i) {
                    // Small offset so the text doesn’t overlap the dot
                    ofDrawBitmapStringHighlight(ofToString(i), pts[i] + glm::vec2(4, -4));
                }
                ofPopStyle();
            }
        }
    }

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
