
#include "Tri.h"

Tri::Tri() {

}

void Tri::setup(ofVec3f p0, ofVec3f p1, ofVec3f p2, ofColor c) {

	pt0 = p0;
	pt1 = p1;
	pt2 = p2;

	mid = (pt0 + pt1 + pt2)/3.0f;

	mesh.addVertex(pt0);
	mesh.addVertex(pt1);
	mesh.addVertex(pt2);

	mesh.setMode(OF_PRIMITIVE_TRIANGLES);

	col = c;
	mesh.addColor(col);
	mesh.addColor(col);
	mesh.addColor(col);


	debugCol.setHsb( ofRandom(255), 200, 200);
	debugMesh.addVertex(pt0);
	debugMesh.addVertex(pt1);
	debugMesh.addVertex(pt2);

	//debugMesh.addColor(debugCol);
	//debugMesh.addColor(debugCol);
	//debugMesh.addColor(debugCol);

}

ofVec3f Tri::getMidPt() {
	return mid;
}

void Tri::update() {



}

void Tri::draw() {
	mesh.draw();
}

void Tri::drawWireframe() {
	ofSetColor(255);
	debugMesh.drawWireframe();
}

void Tri::drawMidPt() {
	ofSetCircleResolution(6);
	ofSetColor(debugCol);
	ofDrawCircle(mid, 2);
}



