#include "ViewRenderer.h"

void ViewRenderer::draw(const RenderData& rd) {
    ofBackground(15, 15, 15);

    if (!rd.grabber || !rd.grabber->isInitialized() || !rd.grabber->getPixels().isAllocated()) {
        ofSetColor(0, 0, 255);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
        ofSetColor(255);
        ofDrawBitmapString("Camera not initialized! Check camera connection and permissions.", 40, 80);
        return;
    }

    ofSetColor(255);
    int winW = ofGetWidth();
    int winH = ofGetHeight();

    ofTrueTypeFont& instrFont  = *rd.fontMedium;
    ofTrueTypeFont& bannerFont = *rd.fontLarge;

    float maxWidth = 0, totalHeight = 0;
    for (const auto& line : rd.lines) {
        maxWidth = std::max(maxWidth, instrFont.stringWidth(line));
        totalHeight += instrFont.stringHeight(line) + 8;
    }

    float bannerHeight = 0, bannerW = 0;
    std::string result = "";
    ofColor resultColor;
    if (rd.stage == SmileFlow::STAGE_EVALUATE) {
        result = rd.abnormal ? "ABNORMALITY DETECTED" : "NORMAL";
        resultColor = rd.abnormal ? ofColor(230, 60, 60) : ofColor(60, 200, 120);
        bannerW = bannerFont.stringWidth(result);
        bannerHeight = bannerFont.stringHeight(result) + 42 + 24;
    } else {
        bannerHeight = bannerFont.stringHeight("NORMAL") + 42 + 24;
    }

    float camAspect = 1280.0f / 720.0f;
    float winAspect = float(winW) / float(winH);
    float camDrawW, camDrawH;
    if (winAspect > camAspect) {
        camDrawH = winH * 0.75;
        camDrawW = camDrawH * camAspect;
    } else {
        camDrawW = winW * 0.9;
        camDrawH = camDrawW / camAspect;
    }
    float camX = (winW - camDrawW) / 2.0f;
    float camY = (winH - camDrawH) / 2.0f;

    float spaceAbove = (totalHeight + 50);
    float spaceBelow = (bannerHeight + 60);
    float availableH = camDrawH - spaceAbove - spaceBelow;
    float ovalCenterY = camY + spaceAbove + availableH/2.0f;
    float ovalCenterX = camX + camDrawW/2.0f;

    float ovalA = camDrawW * 0.13f;
    float ovalB = camDrawH * 0.28f;

    ofPushMatrix();
    ofTranslate(camX, camY);
    ofScale(camDrawW / 1280.0f, camDrawH / 720.0f);
    if (rd.mirrorView) {
        ofTranslate(1280, 0);
        ofScale(-1, 1);
    }
    rd.grabber->draw(0, 0);

    ofPushStyle();
    bool isOkColor = false;
    if (rd.tracker && !rd.tracker->getInstances().empty()) {
        auto& inst = rd.tracker->getInstances().front();
        auto& lms  = inst.getLandmarks();
        auto pts   = lms.getImagePoints();
        int insideCount = 0;
        for (const auto& p : pts) {
            float nx = (p.x - 1280.0f/2) / (1280.0f*0.13f);
            float ny = (p.y - 720.0f/2) / (720.0f*0.28f);
            if (nx*nx + ny*ny <= 1.f) insideCount++;
        }
        bool inside = (pts.empty() ? false : (insideCount >= (int)(0.92f * pts.size())));
        isOkColor = inside && (rd.stage != SmileFlow::STAGE_ALIGN);
    }
    ofNoFill();
    ofSetLineWidth(1.5f);
    ofSetColor(isOkColor ? ofColor(40, 220, 120) : ofColor(230, 70, 70));
    float ovalCenterY_cam = (ovalCenterY - camY) / (camDrawH / 720.0f);
    float ovalCenterX_cam = (ovalCenterX - camX) / (camDrawW / 1280.0f);
    ofDrawEllipse(ovalCenterX_cam, ovalCenterY_cam,
                  2.f * (ovalA / (camDrawW / 1280.0f)),
                  2.f * (ovalB / (camDrawH / 720.0f)));
    ofPopStyle();

    if (rd.tracker) {
        rd.tracker->drawDebug();
    }
    ofSetLineWidth(1.0f);

    ofPopMatrix();

    if (rd.stage == SmileFlow::STAGE_HOME) {
        ofSetColor(30, 30, 30, 180);
        ofDrawRectangle(0, 0, winW, winH);
        ofSetColor(255);
        std::string title = "Facial Stroke Detector";
        bannerFont.drawString(title, winW/2 - bannerFont.stringWidth(title)/2, winH/5);
        std::vector<std::string> lines = {
            "Welcome!", "",
            "How it works:",
            "- Align your head inside the oval",
            "- Hold still until progress completes",
            "- Show your teeth (smile)",
            "- System checks for mouth asymmetry", "", "Press any key to begin"
        };
        for(size_t i=0;i<lines.size();++i){
            instrFont.drawString(lines[i], winW/2 - instrFont.stringWidth(lines[i])/2, winH/5 + 80 + i*38);
        }
        return;
    }

    ofPushStyle();
    float y_above = ovalCenterY - ovalB - totalHeight - 22;
    ofSetColor(0, 0, 0, 100);
    ofDrawRectangle(ovalCenterX - maxWidth/2 - 20, y_above - 18, maxWidth + 40, totalHeight + 24);
    ofSetColor(255);
    float y = y_above;
    for (const auto& line : rd.lines) {
        float w = instrFont.stringWidth(line);
        instrFont.drawString(line, ovalCenterX - w/2, y + instrFont.stringHeight(line));
        y += instrFont.stringHeight(line) + 8;
    }

    if (rd.stage == SmileFlow::STAGE_EVALUATE) {
        float ovalBot = ovalCenterY + ovalB;
        float resultBoxHeight = bannerFont.stringHeight(result) + 42 + 24;
        float centerY = ovalBot + (winH - ovalBot) / 2.0f;

        ofSetColor(0, 0, 0, 100);
        ofDrawRectangle(ovalCenterX - bannerW/2 - 24, centerY - resultBoxHeight/2, bannerW + 48, resultBoxHeight);
        ofSetColor(resultColor);
        bannerFont.drawString(result, ovalCenterX - bannerW/2, centerY + bannerFont.stringHeight(result)/2);
    }

    ofSetColor(255);
    ofDrawBitmapStringHighlight("Framerate : " + ofToString(ofGetFrameRate()), 20, winH - 30);
    ofPopStyle();
}
