#include "FaceTrackerAdapter.h"

void FaceTrackerAdapter::setup(const std::string& modelPath) {
    tracker_.setup(modelPath);
}

void FaceTrackerAdapter::update(ofVideoGrabber& grabber) {
    if (grabber.isFrameNew()) tracker_.update(grabber);
}

bool FaceTrackerAdapter::hasFace() const {
    return !tracker_.getInstances().empty();
}

bool FaceTrackerAdapter::getDerolled(DerolledData& out) const {
    out = DerolledData{};
    const auto& instances = tracker_.getInstances();
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

    out.points.resize(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        out.points[i] = math2d::rotateAround(pts[i], center, -roll);
    }
    out.leftEyeC   = math2d::rotateAround(leftEyeC,  center, -roll);
    out.rightEyeC  = math2d::rotateAround(rightEyeC, center, -roll);
    out.faceCenter = center;
    out.iod        = math2d::interOcular(out.leftEyeC, out.rightEyeC);
    out.insideGuide = guide::mostlyInsideGuide(out.points);
    out.valid = true;
    return true;
}
