#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

    // NOTE: Make sure your model files are under bin/data/model/
    ofSetDataPathRoot(ofFile(__BASE_FILE__).getEnclosingDirectory() + "../bin/data/model/");

    // Camera
    grabber.setup(1280, 720);

    // Face tracker
    tracker.setup(); // using default detector is fine/stable

    // Guide oval sized for a typical head region in the camera view
    guideCenter = { grabber.getWidth() * 0.5f, grabber.getHeight() * 0.45f };
    guideA = grabber.getWidth()  * 0.18f;  // horizontal semi-axis
    guideB = grabber.getHeight() * 0.28f;  // vertical   semi-axis

    ofSetFrameRate(60);
    ofBackground(15);
}

//--------------------------------------------------------------
void ofApp::update() {
    float now = ofGetElapsedTimef();
    static float last = now;
    float dt = now - last;
    last = now;

    grabber.update();

    // Update tracker when there are new frames
    if (grabber.isFrameNew()) {
        tracker.update(grabber);
    }

    // If no face, decay metrics and go back to ALIGN (unless we're showing the result)
    if (!hasFace()) {
        smileIntensity = ofLerp(smileIntensity, 0.0f, 0.08f);
        asymmetryScore = ofLerp(asymmetryScore, 0.0f, 0.08f);
        havePrevCenter = false;
        if (stage != STAGE_EVALUATE) {
            stage = STAGE_ALIGN;
        }
        return;
    }

    // Get derolled landmarks and eye centers
    std::vector<glm::vec2> ptsDerolled;
    glm::vec2 leftEyeC, rightEyeC, faceC;
    if (!getDerolledKeypoints(ptsDerolled, leftEyeC, rightEyeC, faceC)) {
        return;
    }
    float iod = interOcular(leftEyeC, rightEyeC);
    bool inside = mostlyInsideGuide(ptsDerolled);

    // State machine
    switch (stage) {
        case STAGE_ALIGN: {
            // Wait until face is mostly inside the oval guide
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
            // Require stability for a few seconds while staying inside the oval
            updateStability(faceC, iod, inside, dt);
            if (!inside) {
                stage = STAGE_ALIGN;
                break;
            }
            if (stableTime >= HOLD_STILL_SECONDS) {
                // Capture baseline neutral mouth corners + size
                baseLeftCorner  = ptsDerolled[LM_LEFT_MOUTH_CORNER];
                baseRightCorner = ptsDerolled[LM_RIGHT_MOUTH_CORNER];
                baseIOD         = iod;
                calibrated      = true;
                stage           = STAGE_PROMPT_SMILE;
            }
            break;
        }
        case STAGE_PROMPT_SMILE: {
            // Track smile and asymmetry live; decide once a strong smile is held briefly
            if (!inside) {
                // If user moves away, go back to ALIGN
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
            // Nothing to update; user can press 'R' to restart
            break;
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw() {
    // --- Mirrored drawing for camera + overlays (selfie-like) ---
    ofPushMatrix();
    if (mirrorView) {
        ofTranslate(grabber.getWidth(), 0);
        ofScale(-1, 1);
    }

    // Camera image
    grabber.draw(0, 0);

    // Oval guide (green if aligned / red otherwise)
    ofPushStyle();
    bool isOkColor = false;
    {
        // Quick "inside" check colorization using most recent tracker result
        bool inside = false;
        if (!tracker.getInstances().empty()) {
            auto& inst = tracker.getInstances().front();
            auto& lms  = inst.getLandmarks();
            auto pts   = lms.getImagePoints();
            // NOTE: these are NOT derolled, but the guide is just a rough visual aid.
            // We still use derolled points in update() for logic.
            int insideCount = 0;
            for (const auto& p : pts) {
                float nx = (p.x - guideCenter.x) / guideA;
                float ny = (p.y - guideCenter.y) / guideB;
                if (nx*nx + ny*ny <= 1.f) insideCount++;
            }
            inside = (pts.empty() ? false : (insideCount >= (int)(0.92f * pts.size())));
        }
        isOkColor = inside && (stage != STAGE_ALIGN);
    }
    ofNoFill();
    ofSetLineWidth(3.f);
    ofSetColor(isOkColor ? ofColor(40, 220, 120) : ofColor(230, 70, 70));
    ofDrawEllipse(guideCenter.x, guideCenter.y, 2.f * guideA, 2.f * guideB);
    ofPopStyle();

    // Tracker’s built-in overlay (landmarks / pose)
    tracker.drawDebug();
    tracker.drawDebugPose();

    ofPopMatrix();
    // --- End mirrored section ---

    // HUD / instructions (not mirrored)
    ofPushStyle();
    ofSetColor(255);

    std::string line1, line2, line3;
    switch (stage) {
        case STAGE_ALIGN:
            line1 = "Align your head inside the oval.";
            line2 = "Keep your head roughly upright.";
            break;
        case STAGE_HOLD_STILL:
            line1 = "Hold still...";
            line2 = "Capturing neutral baseline.";
            line3 = "Progress: " + ofToString(std::min(1.f, stableTime / HOLD_STILL_SECONDS) * 100.f, 0) + "%";
            break;
        case STAGE_PROMPT_SMILE:
            line1 = "Show your teeth (smile)!";
            line2 = "Hold for a moment...";
            line3 = "Smile intensity: " + ofToString(smileIntensity, 3) + "  |  Asym: " + ofToString(asymmetryScore, 3);
            break;
        case STAGE_EVALUATE:
            line1 = "Result:";
            line2 = (abnormal ? "ABNORMALITY DETECTED" : "NORMAL");
            line3 = "Press [R] to restart";
            break;
    }
    ofDrawBitmapStringHighlight(line1, 20, 26);
    ofDrawBitmapStringHighlight(line2, 20, 46);
    if (!line3.empty()) ofDrawBitmapStringHighlight(line3, 20, 66);

    // Big banner for the final result
    if (stage == STAGE_EVALUATE) {
        ofPushMatrix();
        ofPushStyle();
        // Scale up the bitmap font to look big
        ofTranslate(20, 120);
        ofScale(2.4f, 2.4f);
        ofSetColor(abnormal ? ofColor(230, 60, 60) : ofColor(60, 200, 120));
        ofDrawBitmapStringHighlight(abnormal ? "ABNORMALITY DETECTED" : "NORMAL", 0, 0,
                                    ofColor(0,0), ofColor(255,255,255,40));
        ofPopStyle();
        ofPopMatrix();
    }

    // Small FPS line
    ofDrawBitmapStringHighlight("Framerate : " + ofToString(ofGetFrameRate()), 20, ofGetHeight() - 30);

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

    // Non-const because getLandmarks() is not const in the addon API
    auto& inst = const_cast<ofxFaceTracker2Instance&>(instances.front());
    auto& lms  = inst.getLandmarks();
    auto pts   = lms.getImagePoints(); // image-space

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

    // Roll from eye line
    glm::vec2 d = rightEyeC - leftEyeC;
    float roll = std::atan2(d.y, d.x);

    // De-roll around eye center
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
    int insideCount = 0;
    for (const auto& p : pts) {
        float nx = (p.x - guideCenter.x) / guideA;
        float ny = (p.y - guideCenter.y) / guideB;
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
        // Decay while not ready
        smileIntensity = ofLerp(smileIntensity, 0.f, 0.2f);
        asymmetryScore = ofLerp(asymmetryScore, 0.f, 0.2f);
        return;
    }

    glm::vec2 L = ptsDerolled[LM_LEFT_MOUTH_CORNER];
    glm::vec2 R = ptsDerolled[LM_RIGHT_MOUTH_CORNER];

    float leftRaise  = (baseLeftCorner.y  - L.y) / std::max(1.f, iod);  // Y up = negative in image → use baseline-current
    float rightRaise = (baseRightCorner.y - R.y) / std::max(1.f, iod);

    // Smile intensity = mean of absolute raises
    float intensity = std::max(0.0f, 0.5f * (std::abs(leftRaise) + std::abs(rightRaise)));

    // Relative imbalance (0 = perfectly symmetric)
    float denom = 0.5f * (std::abs(leftRaise) + std::abs(rightRaise)) + 1e-6f;
    float asym  = std::abs(leftRaise - rightRaise) / denom;

    // Smooth with EMA
    smileIntensity = (1.f - smoothingAlpha) * smileIntensity + smoothingAlpha * intensity;
    asymmetryScore = (1.f - smoothingAlpha) * asymmetryScore + smoothingAlpha * asym;
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
