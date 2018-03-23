#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "ofxDelaunay.h"
#include "ofxGui.h"
#include "Tri.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);


		void resetMesh();
		deque<Tri> getTrisFromImg(ofImage &img);
		void renderToFbo( ofFbo &fbo );
		void drawContours();
		vector<ofPoint> findFeaturePoints(ofImage & image);
		void saveFboToFile();

		const int viewWidth = 2160;
		const int viewHeight = 3840;
		float aspect;
		float viewScale;
		bool bIsFullRes;
		ofVec2f origin;
		ofFbo screenFbo;

		bool bShowCVPipeline;

		ofVec2f silhouettePos;
		ofImage silhouette;
		ofImage silhouetteGray;
		ofImage threshImg;
		ofPixels threshPix;
		ofImage canny;

		vector<ofPoint> meshPoints;
		vector<ofPoint> culledCanny;
		vector<ofPoint> randFillPts;
		vector<ofPoint> featurePts;
		vector<ofPolyline> outlines;

		ofxDelaunay triangulation;

		deque<Tri> triangles;

		ofxCv::ContourFinder contours;

		ofxPanel gui;
		string guiFilename;
		ofxLabel drawingLabel;
		ofxLabel meshCompositionLabel;
		ofxLabel meshConstructionLabel;
		ofxToggle bDrawRaw;
		ofxToggle bDrawThreshold;
		ofxToggle bDrawContours;
		ofxToggle bDrawEdges;
		ofxToggle bDrawPolyline;
		ofxToggle bDrawTriangles;
		ofxToggle bDrawTriangleWires;
		ofxToggle bDrawDelaunay;
		ofxButton bResetMesh;
		ofxToggle bAddOutlinePoints;
		ofxIntSlider outlineSpacing;
		ofxToggle bAddRandomPoints;
		ofxIntSlider numRandomPoints;
		ofxToggle bAddCannyPoints;
		ofxIntSlider cannyThresh1;
		ofxIntSlider cannyThresh2;
		ofxIntSlider cannyCullRes;

		ofxToggle bAddFeaturePoints;
		ofxIntSlider sift_numFeatures;
		ofxIntSlider sift_octaveLayers;
		ofxFloatSlider sift_contrastThresh;
		ofxFloatSlider sift_edgeThresh;
		ofxFloatSlider sift_sigma;

		ofxButton saveFbo;

		const bool bDoHeadshots = false;
		vector<ofImage> headshots;
		vector<string> headshotFilenames;

};
