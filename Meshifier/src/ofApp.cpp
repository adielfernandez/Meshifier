#include "ofApp.h"

using namespace cv;
using namespace ofxCv;

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFullscreen(false);
	ofSetVerticalSync(false);
	ofSetFrameRate(200);


	screenFbo.allocate(viewWidth, viewHeight);
	screenFbo.begin();
	ofClear(50, 0, 0, 255);
	screenFbo.end();

	silhouette.load("images/henryford.png");
	//silhouettePos.set((viewWidth - silhouette.getWidth()) / 2, viewHeight - silhouette.getHeight());
	silhouettePos.set(0);


	//if (bDoHeadshots) {
	//	ofDirectory imgDir;
	//	imgDir.listDir("headshots");

	//	cout << "loading headshots..." << endl;
	//	for (int i = 0; i < (int)imgDir.size(); i++) {
	//		headshots.push_back(ofImage());
	//		cout << "loading " << imgDir.getPath(i) << endl;
	//		headshotFilenames.push_back(imgDir.getPath(i));
	//		headshots.back().load(imgDir.getPath(i));
	//	}

	//	cout << headshots.size() << " headshots loaded" << endl;
	//}


	aspect = viewWidth / viewHeight;

	viewScale = 1.0f;
	origin.set(300, 0);
	bIsFullRes = true;

	bShowCVPipeline = true;

	// GUI SETUP
	guiFilename = "settings.xml";
	gui.setup("settings", guiFilename);

	int guiWidth = 250;

	gui.setPosition(15, 60);
	gui.setSize(guiWidth, 60);

	gui.add(saveFbo.setup("Save FBO to File", guiWidth));
	saveFbo.addListener(this, &ofApp::saveFboToFile);

	gui.add(drawingLabel.setup("   DRAW TO SCREEN", ""));
	gui.add(bDrawRaw.setup("Draw Raw Image", true, guiWidth));
	gui.add(bDrawThreshold.setup("Draw Thresh Pixels", true, guiWidth));
	gui.add(bDrawContours.setup("Draw ContourFinder", false, guiWidth));
	gui.add(bDrawEdges.setup("Draw Canny Edges", false, guiWidth));
	gui.add(bDrawPolyline.setup("Draw Outline", false, guiWidth));
	gui.add(bDrawTriangles.setup("Draw Mesh Filled", false, guiWidth));
	gui.add(bDrawTriangleWires.setup("Draw Mesh Wireframe", false, guiWidth));
	gui.add(bDrawDelaunay.setup("Draw Delaunay", false, guiWidth));

	gui.add(meshCompositionLabel.setup("   MESH COMPOSITION", ""));
	gui.add(bAddOutlinePoints.setup("Add Outline Pts to Mesh", false));
	gui.add(outlineSpacing.setup("Outline Resample Spacing", 10, 1, 100, guiWidth));
	
	gui.add(bAddRandomPoints.setup("Add Random Pts to Mesh", false, guiWidth));
	gui.add(numRandomPoints.setup("Num Randomized Points", 2000, 10, 8000, guiWidth));
	
	gui.add(bAddCannyPoints.setup("Add Canny Edge Pts to Mesh", false, guiWidth));
	gui.add(cannyThresh1.setup("Canny Thresh 1", 100, 0, 500, guiWidth));
	gui.add(cannyThresh2.setup("Canny Thresh 2", 100, 0, 500, guiWidth));
	gui.add(cannyCullRes.setup("Canny Cull Resolution", 10, 1, 20, guiWidth));

	gui.add(bAddFeaturePoints.setup("Add CV Feature Pts to Mesh", false, guiWidth));
	gui.add(sift_numFeatures.setup("SIFT features", 2000, 1, 3000, guiWidth));
	gui.add(sift_octaveLayers.setup("SIFT octaves", 3, 1, 10, guiWidth));
	gui.add(sift_contrastThresh.setup("SIFT contrast", 0.04, 0, 0.4, guiWidth));
	gui.add(sift_edgeThresh.setup("SIFT edge", 10, 0, 100, guiWidth));
	gui.add(sift_sigma.setup("SIFT sigma", 1.6, 0, 10, guiWidth));

	gui.add(meshConstructionLabel.setup("   MESH CONSTRUCTION", ""));
	gui.add(bResetMesh.setup("Reset Mesh", guiWidth));
	bResetMesh.addListener(this, &ofApp::resetMesh);


	//formatting
	gui.setHeaderBackgroundColor(255);



	//color applies to gui title only
	gui.setDefaultTextColor(ofColor(255));

	//color of the labels
	drawingLabel.setDefaultTextColor(ofColor(0));
	drawingLabel.setBackgroundColor(255);
	meshCompositionLabel.setDefaultTextColor(ofColor(0));
	meshCompositionLabel.setBackgroundColor(255);
	meshConstructionLabel.setDefaultTextColor(ofColor(0));
	meshConstructionLabel.setBackgroundColor(255);

	gui.loadFromFile(guiFilename);
	cout << "SETTINGS LOADED" << endl;



	triangles = getTrisFromImg(silhouette);

}

void ofApp::resetMesh() {
	triangles = getTrisFromImg(silhouette);
}

deque<Tri> ofApp::getTrisFromImg(ofImage &img) {

	cout << "----------CREATING MESH----------" << endl;

	cout << "Image dimensions: (" << img.getWidth() << ", " << img.getHeight() << ")" << endl;

	meshPoints.clear();
	randFillPts.clear();
	featurePts.clear();
	culledCanny.clear();
	triangles.clear();
	outlines.clear();

	//go through the pixels of the image and get the points using the blob finder
	ofPixels grayPix = img.getPixels();

	//go through the pixels and drop any that are transparent
	if (img.getPixels().getNumChannels() == 4) {
		for (int i = 0; i < grayPix.getWidth() * grayPix.getHeight() * 4; i += 4) {

			//if alpha isnt zero, set to white so we can threshold it
			if (grayPix[i + 3] != 0) {

				//make any black regions 1 at the very least
				grayPix[i    ] =	MAX(1, grayPix[i    ]);
				grayPix[i + 1] =	MAX(1, grayPix[i + 1]);
				grayPix[i + 2] =	MAX(1, grayPix[i + 2]);
			} else {
				grayPix[i] = 0;
				grayPix[i + 1] = 0;
				grayPix[i + 2] = 0;
			}
		}

	}

	//convert to grayscale
	grayPix.setImageType(OF_IMAGE_GRAYSCALE);
	silhouetteGray.setFromPixels(grayPix);
	silhouetteGray.update();

	ofxCv::threshold(grayPix, threshPix, 0);

	//run through the contour finder
	contours.setThreshold(254);
	contours.setMinAreaRadius(10);
	contours.setMaxAreaRadius(10000);
	contours.setFindHoles(true);
	contours.findContours(threshPix);

	threshImg.setFromPixels(threshPix);

	//go through the blobs and add the points to the vector
	if(bAddOutlinePoints){
		int totalPoints = 0;
		for (int i = 0; i < contours.getContours().size(); i++) {
			auto &pts = contours.getContour(i);

			ofPolyline line;

			for (int j = 0; j < pts.size(); j += 1) {
				line.addVertex(toOf(pts[j]));
			}
			//cout << "Contour[" << i << "] is hole: " << contours.getHole(i) << endl;
			line = line.getResampledBySpacing(outlineSpacing);
			outlines.push_back(line);

			//add the outline points to the meshPoints vector
			meshPoints.insert(meshPoints.end(), outlines.back().getVertices().begin(), outlines.back().getVertices().end());
			totalPoints += outlines.back().getVertices().size();
		}
		cout << "Outline points added: " << totalPoints << endl;
	}

	

	//--------------------add a bunch of points inside the silhouette--------------------
	if (bAddRandomPoints) {
		
		int pointsAdded = 0;
		while(pointsAdded < numRandomPoints) {

			ofPoint p(ofRandom(img.getWidth()), ofRandom(img.getHeight()));

			if (threshPix[threshPix.getPixelIndex(p.x, p.y)] > 128) {
				meshPoints.push_back(p);
				randFillPts.push_back(p);
				pointsAdded++;

				//cout << "Random Point added: " << meshPoints.back() << endl;
			}

		}
		cout << "Random points added: " << meshPoints.size() << endl;
	}



	//--------------------add points using OpenCV feature detection--------------------
	if (bAddFeaturePoints) {

		auto pts = findFeaturePoints(silhouetteGray);
		for (auto &point : pts) {

			ofPoint p( (int)point.x, (int)point.y, 0 );

			bool validIndex = p.x >= 0 && p.x < threshPix.getWidth() && p.y >= 0 && p.y < threshPix.getHeight();

			if (validIndex) {
				if(threshPix[threshPix.getPixelIndex(p.x, p.y)] > 128) {
					featurePts.push_back(p);
				}
			}
		}

		//remove duplicate entries
		int pointsRemoved = 0;
		for (int i = 0; i < featurePts.size(); i++) {
			//loop from the last to current index
			for (int j = featurePts.size()-1; j > i; j--) {
				if (featurePts[i] == featurePts[j]) {
					//cout << i << " == " << j << ", Removing feature point index: " << j << endl;
					featurePts.erase(featurePts.begin() + j);
					pointsRemoved++;
				}
			}
		}


		meshPoints.insert(meshPoints.end(), featurePts.begin(), featurePts.end());
		
		cout << "Feature points added: " << featurePts.size() << " (removed "<<pointsRemoved<<" duplicates)" << endl;

	}

	//--------------------canny edge detection--------------------
	if (bAddCannyPoints) {

		Canny(silhouetteGray, canny, cannyThresh1, cannyThresh2);
		canny.update();

		//go through the canny image and black out pixels at a regular spacing
		//THIS COULD TAKE A WHILE
		ofPixels &cPix = canny.getPixels();
		int w = cPix.getWidth();
		int h = cPix.getHeight();
		int num = w * h;
		for(int i = 0; i < num; i++){
		
			int x = i % w;
			int y = (i - x)/w;

			if (x % cannyCullRes == 0 && y % cannyCullRes == 0) {

				//if this pixel is white
				if (cPix[i] > 0) {
			
					//add point to canny pts
					culledCanny.push_back(ofPoint(x,y));

				} else {
					cPix[i] = 0;
				}

			}

		}
		canny.update();
		cout << "Canny edge points added: " << culledCanny.size() <<   endl;;

		//add canny points to mesh for triangulation
		meshPoints.insert(meshPoints.end(), culledCanny.begin(), culledCanny.end());
	}

	cout << "Total Points added: " << meshPoints.size() << endl;

	//do delaunay triangulation on found points
	triangulation.reset();
	triangulation.addPoints(meshPoints);
	triangulation.triangulate();

	//triangulation is done. steal the triangles from the mesh
	ofMesh &mesh = triangulation.triangleMesh;
	deque<Tri> tris;

	for (int i = 0; i < mesh.getNumIndices(); i += 3) {

		if (i < mesh.getNumIndices() - 2) {

			ofPoint p1 = mesh.getVertex(mesh.getIndex(i    ));
			ofPoint p2 = mesh.getVertex(mesh.getIndex(i + 1));
			ofPoint p3 = mesh.getVertex(mesh.getIndex(i + 2));

			//only add triangles whose midpoint is 
			//INSIDE the silhouette of the image
			ofPoint mid = (p1 + p2 + p3) / 3.0f;

			bool validIndex = mid.x >= 0 && mid.x < threshPix.getWidth() && mid.y >= 0 && mid.y < threshPix.getHeight();

			if (validIndex){
				if(threshPix[threshPix.getPixelIndex(mid.x, mid.y)] > 128) {

					Tri tri;
					tri.setup(p1, p2, p3, img.getColor(mid.x, mid.y));
					tris.push_back(tri);
				}
			}
		}
	}

	cout << tris.size() << " triangles created" << endl;

	return tris;
}

vector<ofPoint> ofApp::findFeaturePoints(ofImage & image) {
	vector<ofPoint> points;
	cv::Mat toProcess = ofxCv::toCv(image);
	vector<cv::KeyPoint> objectKeypoints;

	// Extract key points
	//    The detector can be any of (see OpenCV features2d.hpp):
	//    ﻿cv::FeatureDetector * detector = new cv::DenseFeatureDetector();
	//    ﻿cv::FeatureDetector * detector = new cv::FastFeatureDetector();
	//    ﻿cv::FeatureDetector * detector = new cv::GFTTDetector();
	//    cv::FeatureDetector * detector = new cv::MSER();
	//    ﻿cv::FeatureDetector * detector = new cv::ORB();
	//    ﻿cv::FeatureDetector * detector = new cv::StarFeatureDetector();
	//    ﻿cv::FeatureDetector * detector = new cv::SURF(600.0);
	//const cv::SIFT::DetectorParams& detectorParams = cv::SIFT::DetectorParams(threshold, 10.0);
	//cv::FeatureDetector * detector = new cv::SiftFeatureDetector(sift_numFeatures, 
	//															sift_octaveLayers, 
	//															sift_contrastThresh, 
	//															sift_edgeThresh, 
	//															sift_sigma);
	//delete detector;

	auto detector = make_shared<cv::SiftFeatureDetector>(sift_numFeatures,
															sift_octaveLayers,
															sift_contrastThresh,
															sift_edgeThresh,
															sift_sigma);
	detector->detect(toProcess, objectKeypoints);

	// convert to ofPoint vector
	for (int i = 0, len = objectKeypoints.size(); i<len; i++) {
		points.push_back(ofPoint(objectKeypoints[i].pt.x, objectKeypoints[i].pt.y));
	}
	return points;
}

//--------------------------------------------------------------
void ofApp::update(){

	for (auto &tri : triangles) {
		tri.update();
	}

	if (bShowCVPipeline) {

	//	if (bDrawContours) {
	//		contours.setThreshold(thresh);
	//		contours.setMinAreaRadius(minArea);
	//		contours.setMaxAreaRadius(maxArea);
	//		contours.setFindHoles(bFindHoles);
	//		contours.findContours(threshPix);

	//	}

		if (bDrawEdges) {
			Canny(silhouetteGray, canny, cannyThresh1, cannyThresh2);
			canny.update();
		}

	}


}

void ofApp::drawContours() {
	ofSetColor(255, 0, 0);
	contours.draw();

	for (int i = 0; i < contours.getContours().size(); i++) {
		string s = "ID: " + ofToString(i) + "\nHole: " + ofToString(contours.getHole(i));
		cv::Rect rect = contours.getBoundingRect(i);
		ofVec2f pos = toOf(contours.getCentroid(i)) + ofVec2f(-rect.width / 2, rect.height / 2);
		ofDrawBitmapString(s, pos);
	}
}

void ofApp::renderToFbo(ofFbo &fbo) {

	fbo.begin();

	ofClear(0, 0, 0, 0);

	ofPushMatrix();{

		if (bDrawRaw) {
			ofSetColor(255);
			silhouette.draw(silhouettePos);
		}

		ofTranslate(silhouettePos);

		//draw thresholded pixel object
		if (bDrawThreshold) {
			ofSetColor(255);
			threshImg.draw(0, 0);
		}

		if (bDrawEdges) {
			ofSetColor(0, 150, 0);
			canny.draw(0,0);
		}

		if (bDrawPolyline) {
			ofSetColor(255, 0, 0);
			for (auto &line : outlines) {
				line.draw();
			}
		}

		if (bDrawContours) {
			drawContours();
		}

		if (bDrawDelaunay) {
			ofSetColor(0, 255, 0);
			ofNoFill();
			triangulation.draw();
		}

		if (bDrawTriangles) {
			for (auto &tri : triangles) {
				tri.draw();
			}
		}

		if (bDrawTriangleWires) {
			for (auto &tri : triangles) {
				tri.drawWireframe();
				//tri.drawMidPt();
			}
		}




	} ofPopMatrix();



	fbo.end();

}

void ofApp::saveFboToFile() {


	ofFbo exportFbo;
	exportFbo.allocate(silhouette.getWidth(), silhouette.getHeight());
	exportFbo.begin();
	ofClear(0,0,0,255);
	exportFbo.end();

	renderToFbo(exportFbo);

	ofPixels tempPix;
	tempPix.clear();
	exportFbo.readToPixels(tempPix);

	cout << "Saving FBO to file..." << endl;
	ofSaveImage(tempPix, "saved_images/" + ofGetTimestampString() + ".png");





}

//--------------------------------------------------------------
void ofApp::draw(){
	
	ofBackground(0);

	if (bShowCVPipeline) {

		//draw the different CV elements
		int leftMargin = 275;
		int w = silhouetteGray.getWidth();
		int h = silhouetteGray.getHeight();
		float scale = (ofGetWidth() - leftMargin)/(float)(w*5);

		int titleY = h + 20/scale;

		ofPushMatrix();{

			ofTranslate(leftMargin, 0);
			ofScale(scale, scale);

			//base image
			ofSetColor(255);
			silhouetteGray.draw(0, 0);
			ofDrawBitmapString("Raw Image", 0, titleY);


			//threshold + contours
			if (bAddCannyPoints) {
				ofSetColor(0, 255, 0);
				canny.draw(w, 0);
			}
			ofSetColor(255, 100);
			threshImg.draw(w, 0);
			ofPushMatrix();{
				ofTranslate(w, 0);
				drawContours();
			}ofPopMatrix();
			ofSetColor(255);
			string s = "Threshold + \nContours";
			if (bAddCannyPoints) s += " + \nCanny Edges";
			ofDrawBitmapString(s, w, titleY);

			//culled canny + polyline
			ofPushMatrix();{
				ofTranslate(w * 2, 0);
				ofSetColor(255, 0, 0);
				for (auto &line : outlines) {
					line.draw();
				}
				ofSetCircleResolution(3);
				ofSetColor(0, 128, 255);
				for (auto &pt : randFillPts) {
					ofDrawCircle(pt, 5);
				}
				ofSetColor(0, 255, 0);
				for (auto &pt : culledCanny) {
					ofDrawCircle(pt, 5);
				}
				ofSetColor(255, 200, 0);
				for (auto &pt : featurePts) {
					ofDrawCircle(pt, 5);
				}
			}ofPopMatrix();
			ofSetColor(255);
			string c = "";
			if (outlines.size())  c += "\nContour Outline";
			if (culledCanny.size())  c += "\nCulled Canny Pts (green)";
			if (featurePts.size())	c += "\nCV Feature Pts (yellow)";
			if (randFillPts.size()) c += "\nRandom Fill Pts (blue)";
			ofDrawBitmapString(c, w * 2, titleY);

			//delaunay triangulation
			ofPushMatrix();{
				ofTranslate(w * 3, 0);
					
				ofSetColor(0, 255, 0);
				ofNoFill();
				triangulation.draw();
			}ofPopMatrix();
			ofSetColor(255);
			ofDrawBitmapString("Delaunay Triangulation", w * 3, titleY);

			//Triangle Mesh
			ofPushMatrix();{
				ofTranslate(w * 4, 0);
				for (auto &tri : triangles) {
					tri.draw();
				}
			}ofPopMatrix();
			ofSetColor(255);
			ofDrawBitmapString("Culled Triangles + \nColor Sampling", w * 4, titleY);

		}ofPopMatrix();

	} else {

		renderToFbo(screenFbo);

		//draw frame around fbo
		ofSetColor(255);
		screenFbo.draw(origin, viewWidth * viewScale, viewHeight * viewScale);
		ofNoFill();
		ofSetLineWidth(1);
		ofDrawRectangle(origin, viewWidth * viewScale, viewHeight * viewScale);

	}


	ofSetColor(255);
	ofDrawBitmapString("Framerate: " + ofToString(ofGetFrameRate(), 2), 15, 20);
	ofDrawBitmapString("Num Triangles: " + ofToString(triangles.size()), 15, 35);

	gui.draw();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if (key == 'l') {
		gui.loadFromFile(guiFilename);
		cout << "SETTINGS LOADED" << endl;
	}
	if (key == 's') {
		gui.saveToFile(guiFilename);
		cout << "SETTING SAVED" << endl;
	}
	if (key == '-') {
		viewScale -= 0.05;
	}
	if (key == '=') {
		viewScale += 0.05;
	}
	if (key == '0') {
		if (bIsFullRes) {
			//fit full screen
			viewScale = ofGetHeight() / (float)viewHeight;
			origin.set(300,0);
			bIsFullRes = false;
		} else {
			viewScale = 1.0f;
			bIsFullRes = true;
		}
	}
	if (key == 'r') {
		resetMesh();
	}

	if (key == OF_KEY_LEFT) {
		origin.x += 50;
	}
	if (key == OF_KEY_RIGHT) {
		origin.x -= 50;
	}
	if (key == OF_KEY_UP) {
		origin.y += 50;
	}
	if (key == OF_KEY_DOWN) {
		origin.y -= 50;
	}
	if (key == ' ') {
		//ofToggleFullscreen();
		//origin.set(0);
		//viewScale = ofGetHeight() / (float)viewHeight;
		bShowCVPipeline = !bShowCVPipeline;
	}



	//if (bDoHeadshots && headshots.size() > 0 && key == 'b') {

	//	ofFbo headshotFbo;
	//	headshotFbo.allocate(headshots[0].getWidth(), headshots[0].getHeight());

	//	ofPixels tempPix;

	//	for (int i = 0; i < headshots.size(); i++) {

	//		cout << "Processing " << headshotFilenames[i] << endl;

	//		deque<Tri> tris = getTrisFromImg(headshots[i]);

	//		headshotFbo.begin();
	//		ofClear(0,0,0,255);

	//		for (auto &t : tris) {
	//			t.draw();
	//		}
	//		headshotFbo.end();

	//		tempPix.clear();
	//		headshotFbo.readToPixels(tempPix);

	//		ofSaveImage(tempPix, "meshes/" + headshotFilenames[i]);
	//	}

	//}



}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

	if (dragInfo.files.size() > 0) {
		ofImage newImg;
		newImg.load(dragInfo.files[0]);
		triangles = getTrisFromImg(newImg);

		silhouette = newImg;
	}

}
