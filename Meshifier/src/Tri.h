#pragma once

#include "ofMain.h"

class Tri {

public:

	Tri();
	~Tri() {};

	void setup( ofVec3f p0, ofVec3f p1, ofVec3f p2, ofColor c);

	void update();
	void draw();
	void drawWireframe();
	void drawMidPt();

	ofVec3f getMidPt();

	ofVec3f pt0, pt1, pt2;
	ofVec3f mid;

	ofColor col;
	ofColor debugCol;
	ofVboMesh mesh;

	ofVboMesh debugMesh;

};