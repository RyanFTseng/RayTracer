#include "ofApp.h"
#include <glm/gtx/intersect.hpp>
// Intersect Ray with Plane  (wrapper on glm::intersect*)
//
bool Plane::intersect(const Ray& ray, glm::vec3& point, glm::vec3& normalAtIntersect) {
	float dist;
	bool insidePlane = false;
	bool hit = glm::intersectRayPlane(ray.p, ray.d, position, this->normal, dist);
	if (hit) {
		Ray r = ray;
		point = r.evalPoint(dist);
		normalAtIntersect = this->normal;
		glm::vec2 xrange = glm::vec2(position.x - width / 2, position.x + width / 2);
		glm::vec2 yrange = glm::vec2(position.y - width / 2, position.y + width / 2);
		glm::vec2 zrange = glm::vec2(position.z - height / 2, position.z + height / 2);

		// horizontal 
		//
		if (normal == glm::vec3(0, 1, 0) || normal == glm::vec3(0, -1, 0)) {
			if (point.x < xrange[1] && point.x > xrange[0] && point.z < zrange[1] && point.z > zrange[0]) {
				insidePlane = true;
			}
		}
		// front or back
		//
		else if (normal == glm::vec3(0, 0, 1) || normal == glm::vec3(0, 0, -1)) {
			if (point.x < xrange[1] && point.x > xrange[0] && point.y < yrange[1] && point.y > yrange[0]) {
				insidePlane = true;
			}
		}
		// left or right
		//
		else if (normal == glm::vec3(1, 0, 0) || normal == glm::vec3(-1, 0, 0)) {
			if (point.y < yrange[1] && point.y > yrange[0] && point.z < zrange[1] && point.z > zrange[0]) {
				insidePlane = true;
			}
		}
	}
	return insidePlane;
}


// Convert (u, v) to (x, y, z) 
// We assume u,v is in [0, 1]
//
glm::vec3 ViewPlane::toWorld(float u, float v) {
	float w = width();
	float h = height();
	glm::vec3 yFlip = glm::vec3(1, -1, 1);
	return (yFlip * glm::vec3((u * w) + min.x, ((v * h) + min.y), position.z));
}

// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
//
Ray RenderCam::getRay(float u, float v) {
	glm::vec3 pointOnPlane = view.toWorld(u, v);
	return(Ray(position, glm::normalize(pointOnPlane - position)));
}

glm::vec3 RenderCam::getIntersectionPoint(float u, float v) {
	glm::vec3 pointOnPlane = view.toWorld(u, v);
	return(pointOnPlane);
}


glm::vec2 ofApp::map(float height, float width, float objX, float objY, float texWidth, float texHeight) {
	float u = ofMap(objX, 0, width, 0, numTiles);
	float v = ofMap(objY, 0, height, 0, numTiles);
	float i = u * texWidth - 0.5;
	float j = v * texHeight - 0.5;
	if (i < 0) {
		i = -fmod(i, texWidth);
	}
	else {
		i = fmod(i, texWidth);
	}
	if (j < 0) {
		j = -fmod(j, texHeight);
	}
	else {
		j = fmod(j, texHeight);
	}
	glm::vec2 mapPos = glm::vec2(i, j);
	return mapPos;
}

glm::vec2 ofApp::mapHorizontal(float height, float width, glm::vec3 objPos, glm::vec3 point, float hTexWidth, float hTexHeight) {
	float objX = (objPos - point).x;
	float objY = (objPos - point).z;
	return map(height, width, objX, objY, hTexWidth, hTexHeight);
}

glm::vec2 ofApp::mapVertical(float height, float width, glm::vec3 objPos, glm::vec3 point, float vTexWidth, float vTexHeight) {
	float objX = (objPos - point).x;
	float objY = (objPos - point).y;
	return map(height, width, objX, objY, vTexWidth, vTexHeight);
}


void ofApp::rayTrace() {
	cout << "rendering..." << endl;
	for (int x = 0; x < imageWidth; x++) {
		for (int y = 0; y < imageHeight; y++) {
			glm::vec3 intersectNormal;
			glm::vec3 rayDirection;
			//create ray
			float u = (x + 0.5) / imageWidth;
			float v = (y + 0.5) / imageHeight;
			Ray r = renderCam.getRay(u, v);
			glm::vec3 point = renderCam.getIntersectionPoint(u, v);

			rayHit = false;

			//default shortest distance
			float shortestDistance = std::numeric_limits<float>::infinity();

			SceneObject* closestObj = NULL;

			//check for intersections with scene objects and set pixel color
			for (int i = 0; i < scene.size() - 1; i++) {

				glm::vec3 objPosition = scene[i]->position;
				if (scene[i]->intersect(r, point, intersectNormal)) {
					if (glm::length(renderCam.position - objPosition) < shortestDistance) {
						shortestDistance = glm::length(renderCam.position - objPosition);
						closestObj = scene[i];

					}
				}
			}

			if (closestObj != NULL) {
				if (closestObj->intersect(r, point, intersectNormal)) {
					rayHit = true;
				}
			}

			if (rayHit) {
				if (closestObj != NULL) {
					ofColor diffuse = closestObj->diffuseColor;
					ofColor ambient = diffuse * 0.1;
					ofColor specular = closestObj->specularColor;
					ofColor shade = ambient;
					glm::vec2 mapCoord;
					if (closestObj->hasTexture()) {
						if (closestObj->getNormal() == glm::vec3(0, 1, 0)) {
							mapCoord = mapHorizontal(closestObj->getWidth(), closestObj->getHeight(), closestObj->position, point, hDiffuse.getWidth(), hDiffuse.getHeight());
							diffuse = hDiffuse.getColor(mapCoord.x, mapCoord.y);
							specular = hSpecular.getColor(mapCoord.x, mapCoord.y);
						}
						else if (closestObj->getNormal() == glm::vec3(0, 0, 1)) {
							mapCoord = mapVertical(closestObj->getWidth(), closestObj->getHeight(), closestObj->position, point, vDiffuse.getWidth(), vDiffuse.getHeight());
							diffuse = vDiffuse.getColor(mapCoord.x, mapCoord.y);
							specular = vSpecular.getColor(mapCoord.x, mapCoord.y);

						}
						ambient = diffuse * 0.1;
						shade = ambient;
					}

					for (int j = 0; j < lights.size(); j++) {
						ofLight* l = lights[j];
						Ray shadowRay = Ray(point + 0.0001 * glm::normalize(intersectNormal), glm::normalize(l->getPosition() - point));
						for (int i = 0; i < scene.size() - 1; i++) {
							if (!scene[i]->intersect(shadowRay, point, intersectNormal)) {
								shade += ofApp::lambert(point, intersectNormal, diffuse, l) +
									ofApp::phong(point, intersectNormal, diffuse, specular, phongPower, r.d, l);
							}
							else {
								shade += ofColor::black;
							}
						}
					}
					image.setColor(x, y, shade);
					image.update();
				}
			}
			else {
				image.setColor(x, y, backgroundColor);
				image.update();
			}
		}

	}
	cout << "render complete" << endl;
	image.save("render.png");
}

ofColor ofApp::lambert(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, ofLight* light) {
	ofColor lambert;
	glm::vec3 l;
	float r = glm::distance2(light->getPosition(), p);
	l = glm::normalize(light->getPosition() - p);
	glm::vec3 n = glm::normalize(norm);
	lambert = diffuse * (lightIntensity / (r * r)) * glm::max(0.0f, glm::dot(n, l));
	return lambert;
}


ofColor ofApp::phong(const glm::vec3& p, const glm::vec3& norm, const ofColor diffuse, const ofColor specular, float power, glm::vec3 rayDirection, ofLight* light) {
	ofColor phong;
	glm::vec3 l = glm::normalize(light->getPosition() - p);
	glm::vec3 n = glm::normalize(norm);
	glm::vec3 v = -glm::normalize(rayDirection);
	glm::vec3 h = glm::normalize(v + l);
	float r = glm::distance2(light->getPosition(), p);
	float max = glm::max(0.0f, glm::dot(n, h));
	phong = specular * (lightIntensity / (r * r)) * glm::pow(max, power);
	return phong;

}

void ofApp::drawGrid() {

}




void ofApp::drawAxis(glm::vec3 position) {

}


//--------------------------------------------------------------
void ofApp::setup() {
	image.allocate(imageWidth, imageHeight, OF_IMAGE_COLOR);
	hDiffuse.load("hDiffuse.jpg");
	hSpecular.load("hSpecular.jpg");
	vDiffuse.load("vDiffuse.jpg");
	vSpecular.load("vSpecular.jpg");

	gui.setup();
	gui.add(phongPower.setup("Phong Power", 1000.0, 10.0, 10000.0));
	gui.add(lightIntensity.setup("Light Intensity", 250, 0, 500));
	ofEnableDepthTest();

	//setup easycam
	theCam = &mainCam;
	mainCam.setPosition(renderCam.position);
	mainCam.setDistance(10);
	mainCam.setNearClip(.1);

	//setup preview cam
	thePreviewCam = &previewCam;
	thePreviewCam->setPosition(renderCam.position);
	thePreviewCam->setFov(50);
	previewCam.setNearClip(.1);

	//push scene objects
	scene.push_back(s1);
	scene.push_back(s2);
	scene.push_back(s3);
	scene.push_back(p1);
	scene.push_back(p2);

	//push view plane
	scene.push_back(view);

	//lights
	pointLight1->setPosition(5, 1, 4);
	pointLight2->setPosition(1, 2, 4);
	pointLight3->setPosition(-3, 2, 4);
	lights.push_back(pointLight1);
	lights.push_back(pointLight2);
	lights.push_back(pointLight3);
}

void ofApp::drawLights(vector<ofLight*> lights) {
	ofSetColor(ofColor::white);
	for (int i = 0; i < lights.size(); i++) {
		lights[i]->enable();
		ofDrawSphere(lights[i]->getPosition(), 0.2);
	}

};


//--------------------------------------------------------------
void ofApp::update() {

}

//--------------------------------------------------------------
void ofApp::draw() {
	ofSetColor(ofColor::white);
	if (!bGui) {
		ofDisableDepthTest();
		gui.draw();

	}

	ofEnableDepthTest();

	ofEnableLighting();


	ofSetColor(ofColor::white);


	//draw scene with preview cam
	if (bPreviewCam) {
		thePreviewCam->begin();

		//drawLights(lights);

		for (int i = 0; i < scene.size() - 1; i++) {
			ofSetColor(scene[i]->diffuseColor);
			scene[i]->draw();
		}

		thePreviewCam->end();
	}

	//draw scene with easycam
	if (!bPreviewCam) {
		theCam->begin();

		drawLights(lights);
		for (int i = 0; i < scene.size(); i++) {
			ofSetColor(scene[i]->diffuseColor);
			scene[i]->draw();
		}
		theCam->end();
	}
	ofDisableLighting();
	//display preview
	if (bPreview) {
		ofSetColor(ofColor::white);
		image.draw(0, 0);
	}

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {

		//raytrace
	case ' ':
		rayTrace();
		break;

		//toggle preview cam
	case 'p':
		if (bPreviewCam) {
			bPreviewCam = false;
		}
		else {
			bPreviewCam = true;
		};
		break;

		//toggle preview image
	case 'd':
		if (bPreview) {
			bPreview = false;
		}
		else {
			bPreview = true;
		}
		break;


	case 'g':
		if (bGui) {
			bGui = false;
		}
		else {
			bGui = true;
		}
		break;


		break;
	}


}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}