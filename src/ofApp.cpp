#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
    fontMedium.load("verdana.ttf", 28, true, true);
    fontLarge.load("verdana.ttf", 48, true, true);
    fontSmall.load("verdana.ttf", 18, true, true);

    grabber.setup(1280, 720);
    tracker.setup("model/shape_predictor_68_face_landmarks.dat");

    mirrorView = true;
    ofSetFrameRate(60);
    stage = STAGE_HOME;
    ofSetBackgroundColor(15, 15, 15);
}

//--------------------------------------------------------------
void ofApp::update() {
    float now = ofGetElapsedTimef();
    static float last = now;
    float dt = now - last;
    last = now;

    grabber.update();

    if (grabber.isFrameNew()) {
        tracker.update(grabber);
    }

    if (!hasFace()) {
        smileIntensity = ofLerp(smileIntensity, 0.0f, 0.08f);
        asymmetryScore = ofLerp(asymmetryScore, 0.0f, 0.08f);
        havePrevCenter = false;
        if (stage != STAGE_EVALUATE) {
            stage = STAGE_ALIGN;
        }
        return;
    }

    std::vector<glm::vec2> ptsDerolled;
    glm::vec2 leftEyeC, rightEyeC, faceC;
    if (!getDerolledKeypoints(ptsDerolled, leftEyeC, rightEyeC, faceC)) {
        return;
    }
    float iod = interOcular(leftEyeC, rightEyeC);
    bool inside = mostlyInsideGuide(ptsDerolled);

    switch (stage) {
        case STAGE_ALIGN: {
            havePrevCenter = false;
            stableTime = 0.f;
            smileHoldTime = 0.f;
            calibrated = false;
            if (inside) {
                stage = STAGE_HOLD_STILL;
            }
            break;
        }
        case STAGE_HOLD_STILL: {
            updateStability(faceC, iod, inside, dt);
            if (!inside) {
                stage = STAGE_ALIGN;
                break;
            }
            if (stableTime >= HOLD_STILL_SECONDS) {
                baseLeftCorner  = ptsDerolled[LM_LEFT_MOUTH_CORNER];
                baseRightCorner = ptsDerolled[LM_RIGHT_MOUTH_CORNER];
                baseIOD         = iod;
                calibrated      = true;
                stage           = STAGE_PROMPT_SMILE;
            }
            break;
        }
        case STAGE_PROMPT_SMILE: {
            if (!inside) {
                stage = STAGE_ALIGN;
                break;
            }
            updateSmileMetrics(ptsDerolled, baseIOD);

            if (smileIntensity >= SMILE_MIN) {
                smileHoldTime += dt;
            } else {
                smileHoldTime = 0.f;
            }

            if (smileHoldTime >= SMILE_HOLD_SECONDS) {
                abnormal = (asymmetryScore > ASYM_THRESHOLD);
                stage = STAGE_EVALUATE;
            }
            break;
        }
        case STAGE_EVALUATE: {
            break;
        }
        default: break;
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(15, 15, 15);

    if (!grabber.isInitialized() || !grabber.getPixels().isAllocated()) {
        ofSetColor(0, 0, 255);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
        ofSetColor(255);
        ofDrawBitmapString("Camera not initialized! Check camera connection and permissions.", 40, 80);
        return;
    }

    ofSetColor(255);

    int winW = ofGetWidth();
    int winH = ofGetHeight();

    // --------- UI: Calculate instruction and result heights first ---------
    ofTrueTypeFont& instrFont = fontMedium;
    ofTrueTypeFont& bannerFont = fontLarge;

    std::vector<std::string> lines;
    switch (stage) {
        case STAGE_ALIGN:
            lines.push_back("Align your head inside the oval.");
            lines.push_back("Keep your head roughly upright.");
            break;
        case STAGE_HOLD_STILL:
            lines.push_back("Hold still...");
            lines.push_back("Capturing neutral baseline.");
            lines.push_back("Progress: " + ofToString(std::min(1.f, stableTime / HOLD_STILL_SECONDS) * 100.f, 0) + "%");
            break;
        case STAGE_PROMPT_SMILE:
            lines.push_back("Show your teeth (smile)!");
            lines.push_back("Hold for a moment...");
            lines.push_back("Smile intensity: " + ofToString(smileIntensity, 3) + "  |  Asym: " + ofToString(asymmetryScore, 3));
            break;
        case STAGE_EVALUATE:
            lines.push_back("Result:");
            lines.push_back("Press [R] to restart");
            break;
        default: break;
    }
    float maxWidth = 0, totalHeight = 0;
    for (const auto& line : lines) {
        maxWidth = std::max(maxWidth, instrFont.stringWidth(line));
        totalHeight += instrFont.stringHeight(line) + 8;
    }

    float bannerHeight = 0, bannerW = 0;
    std::string result = "";
    ofColor resultColor;
    if (stage == STAGE_EVALUATE) {
        result = abnormal ? "ABNORMALITY DETECTED" : "NORMAL";
        resultColor = abnormal ? ofColor(230, 60, 60) : ofColor(60, 200, 120);
        bannerW = bannerFont.stringWidth(result);
        bannerHeight = bannerFont.stringHeight(result) + 42 + 24; // stringHeight + padding
    } else {
        bannerHeight = bannerFont.stringHeight("NORMAL") + 42 + 24; // reserve space
    }

    // --------- Dynamic camera and oval positioning ---------
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

    // ------ Calculate vertical slots ------
    float spaceAbove = (totalHeight + 50); // instructions + margin
    float spaceBelow = (bannerHeight + 60); // result + margin (not used for new position)
    float availableH = camDrawH - spaceAbove - spaceBelow;
    float ovalCenterY = camY + spaceAbove + availableH/2.0f;
    float ovalCenterX = camX + camDrawW/2.0f;

    // Oval axes in image space for drawing overlays
    float ovalA = camDrawW * 0.13f;
    float ovalB = camDrawH * 0.28f;

    // Draw camera
    ofPushMatrix();
    ofTranslate(camX, camY);
    ofScale(camDrawW / 1280.0f, camDrawH / 720.0f);
    if (mirrorView) {
        ofTranslate(1280, 0);
        ofScale(-1, 1);
    }
    grabber.draw(0, 0);

    // Draw oval
    ofPushStyle();
    bool isOkColor = false;
    {
        bool inside = false;
        if (!tracker.getInstances().empty()) {
            auto& inst = tracker.getInstances().front();
            auto& lms  = inst.getLandmarks();
            auto pts   = lms.getImagePoints();
            int insideCount = 0;
            for (const auto& p : pts) {
                float nx = (p.x - 1280.0f/2) / (1280.0f*0.13f);
                float ny = (p.y - 720.0f/2) / (720.0f*0.28f);
                if (nx*nx + ny*ny <= 1.f) insideCount++;
            }
            inside = (pts.empty() ? false : (insideCount >= (int)(0.92f * pts.size())));
        }
        isOkColor = inside && (stage != STAGE_ALIGN);
    }
    ofNoFill();
    ofSetLineWidth(1.5f);
    ofSetColor(isOkColor ? ofColor(40, 220, 120) : ofColor(230, 70, 70));
    // Draw at calculated oval position (in camera space)
    float ovalCenterY_cam = (ovalCenterY - camY) / (camDrawH / 720.0f);
    float ovalCenterX_cam = (ovalCenterX - camX) / (camDrawW / 1280.0f);
    ofDrawEllipse(ovalCenterX_cam, ovalCenterY_cam, 2.f * (ovalA / (camDrawW / 1280.0f)), 2.f * (ovalB / (camDrawH / 720.0f)));
    ofPopStyle();

    tracker.drawDebug();
    drawPoseAxesAtNose(80);
    ofSetLineWidth(1.0f);

    ofPopMatrix();

    if(stage == STAGE_HOME) {
        ofSetColor(30, 30, 30, 180);
        ofDrawRectangle(0, 0, winW, winH);
        drawHomeScreen();
        return;
    }

    // Instructions above oval
    ofPushStyle();
    float y_above = ovalCenterY - ovalB - totalHeight - 22;
    ofSetColor(0, 0, 0, 100);
    ofDrawRectangle(ovalCenterX - maxWidth/2 - 20, y_above - 18, maxWidth + 40, totalHeight + 24);
    ofSetColor(255);
    float y = y_above;
    for (const auto& line : lines) {
        float w = instrFont.stringWidth(line);
        instrFont.drawString(line, ovalCenterX - w/2, y + instrFont.stringHeight(line));
        y += instrFont.stringHeight(line) + 8;
    }

    // --- Result below oval, centered between bottom of oval and bottom edge ---
    if (stage == STAGE_EVALUATE) {
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

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == 'r' || key == 'R') {
        resetAll();
    }
}

//--------------------------------------------------------------
bool ofApp::hasFace() const {
    return !tracker.getInstances().empty();
}

// Estimate roll and return derolled landmarks + eye centers
//--------------------------------------------------------------
bool ofApp::getDerolledKeypoints(std::vector<glm::vec2>& outPts,
                                 glm::vec2& outLeftEyeC,
                                 glm::vec2& outRightEyeC,
                                 glm::vec2& outFaceCenter) const {
    const auto& instances = tracker.getInstances();
    if (instances.empty()) return false;

    auto& inst = const_cast<ofxFaceTracker2Instance&>(instances.front());
    auto& lms  = inst.getLandmarks();
    auto pts   = lms.getImagePoints();

    auto meanRange = [&](int a, int b) {
        glm::vec2 m(0,0);
        int cnt = 0;
        for (int i = a; i <= b; ++i) { m += pts[i]; ++cnt; }
        if (cnt > 0) m /= (float)cnt;
        return m;
    };

    glm::vec2 leftEyeC  = meanRange(LM_LEFT_EYE_START,  LM_LEFT_EYE_END);
    glm::vec2 rightEyeC = meanRange(LM_RIGHT_EYE_START, LM_RIGHT_EYE_END);
    glm::vec2 center    = 0.5f * (leftEyeC + rightEyeC);

    glm::vec2 d = rightEyeC - leftEyeC;
    float roll = std::atan2(d.y, d.x);

    outPts.resize(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        outPts[i] = rotateAround(pts[i], center, -roll);
    }
    outLeftEyeC  = rotateAround(leftEyeC,  center, -roll);
    outRightEyeC = rotateAround(rightEyeC, center, -roll);
    outFaceCenter = center;
    return true;
}

// Check that most landmarks are inside the oval guide
//--------------------------------------------------------------
bool ofApp::mostlyInsideGuide(const std::vector<glm::vec2>& pts, float fractionNeeded) const {
    if (pts.empty()) return false;
    float ovalCenterX = 1280.0f * 0.5f;
    float ovalCenterY = 720.0f * 0.60f;
    float ovalA = 1280.0f * 0.13f;
    float ovalB = 720.0f * 0.28f;
    int insideCount = 0;
    for (const auto& p : pts) {
        float nx = (p.x - ovalCenterX) / ovalA;
        float ny = (p.y - ovalCenterY) / ovalB;
        if (nx*nx + ny*ny <= 1.f) insideCount++;
    }
    return insideCount >= (int)(fractionNeeded * pts.size());
}

// Update stability timer using face center motion normalized by IOD
//--------------------------------------------------------------
void ofApp::updateStability(const glm::vec2& center, float iod, bool insideGuide, float dt) {
    if (!insideGuide) {
        havePrevCenter = false;
        stableTime = 0.f;
        return;
    }
    if (!havePrevCenter) {
        prevCenter = center;
        havePrevCenter = true;
        stableTime = 0.f;
        return;
    }
    float moveNorm = glm::distance(center, prevCenter) / std::max(1.f, iod);
    prevCenter = center;

    if (moveNorm < MOVE_THRESH_NORM) {
        stableTime += dt;
    } else {
        stableTime = 0.f;
    }
}

// Compute smile intensity + asymmetry from derolled mouth corners
//--------------------------------------------------------------
void ofApp::updateSmileMetrics(const std::vector<glm::vec2>& ptsDerolled, float iod) {
    if (!calibrated || ptsDerolled.size() <= LM_RIGHT_MOUTH_CORNER) {
        smileIntensity = ofLerp(smileIntensity, 0.f, 0.2f);
        asymmetryScore = ofLerp(asymmetryScore, 0.f, 0.2f);
        return;
    }

    glm::vec2 L = ptsDerolled[LM_LEFT_MOUTH_CORNER];
    glm::vec2 R = ptsDerolled[LM_RIGHT_MOUTH_CORNER];

    float leftRaise  = (baseLeftCorner.y  - L.y) / std::max(1.f, iod);
    float rightRaise = (baseRightCorner.y - R.y) / std::max(1.f, iod);

    float intensity = std::max(0.0f, 0.5f * (std::abs(leftRaise) + std::abs(rightRaise)));
    float denom = 0.5f * (std::abs(leftRaise) + std::abs(rightRaise)) + 1e-6f;
    float asym  = std::abs(leftRaise - rightRaise) / denom;

    smileIntensity = (1.f - smoothingAlpha) * smileIntensity + smoothingAlpha * intensity;
    asymmetryScore = (1.f - smoothingAlpha) * asymmetryScore + smoothingAlpha * asym;
}

// Utility: draw 3D axes at a face landmark (e.g. nose tip) in camera coordinates
void ofApp::drawPoseAxesAtNose(float axisLen) {
    if (tracker.getInstances().empty()) return;
    auto &inst = tracker.getInstances().front();
    auto &lms = inst.getLandmarks();
    auto pts = lms.getImagePoints();
    if (pts.size() <= 30) return; // Ensure nose tip index exists

    glm::vec2 nose = pts[30];
    const auto &pose = inst.getPoseMatrix();
    glm::mat4 rot = pose;
    rot[3] = glm::vec4(0,0,0,1);

    glm::vec4 xAxis = rot * glm::vec4(axisLen, 0, 0, 0);
    glm::vec4 yAxis = rot * glm::vec4(0, axisLen, 0, 0);
    glm::vec4 zAxis = rot * glm::vec4(0, 0, axisLen, 0);

    ofSetLineWidth(1.5f);
    ofSetColor(255, 60, 60);     ofDrawLine(nose, nose + glm::vec2(xAxis));
    ofSetColor(60, 255, 60);     ofDrawLine(nose, nose + glm::vec2(yAxis));
    ofSetColor(60, 60, 255);     ofDrawLine(nose, nose + glm::vec2(zAxis));
    ofSetLineWidth(1.0f);
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
    // No action needed; function needed to match the declaration in the header.
}

// Reset to the initial state
//--------------------------------------------------------------
void ofApp::resetAll() {
    stage = STAGE_ALIGN;
    calibrated = false;
    abnormal = false;
    smileIntensity = 0.f;
    asymmetryScore = 0.f;
    smileHoldTime = 0.f;
    stableTime = 0.f;
    havePrevCenter = false;
}

//---------------- HOME SCREEN -----------------
void ofApp::drawHomeScreen() {
    ofSetColor(255);
    float cx = ofGetWidth()/2;
    float topY = ofGetHeight()/5;

    fontLarge.drawString("Facial Stroke Detector", cx - fontLarge.stringWidth("Facial Stroke Detector")/2, topY);

    std::vector<std::string> lines = {
        "Welcome!",
        "",
        "How it works:",
        "- Align your head inside the oval",
        "- Hold still until progress completes",
        "- Show your teeth (smile)",
        "- System checks for mouth asymmetry",
        "",
        "Press any key to begin"
    };
    for(size_t i=0; i<lines.size(); ++i) {
        fontMedium.drawString(lines[i], cx - fontMedium.stringWidth(lines[i])/2, topY + 80 + i*38);
    }
}
