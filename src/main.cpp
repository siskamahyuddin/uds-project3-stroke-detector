#include "ofMain.h"
#include "ofApp.h"

int main() {
    ofGLFWWindowSettings settings;
    settings.setSize(800, 1200); // Good default for vertical layout, but can be any size.
    settings.resizable = true;   // Allow maximizing/resizing.
    ofCreateWindow(settings);
    ofRunApp(new ofApp());
}
