#include <windows.h>
#include <GL/glut.h>
#include <math.h>

//#include "Vuzix.h"
#include "NoHMD.h"

#include <opencv2\highgui\highgui.hpp>
#include <opencv2\core\core.hpp>

#include "OPIRALibrary.h"
#include "OPIRALibraryMT.h"
#include "CaptureLibrary.h"
#include "RegistrationAlgorithms\OCVSurf.h"

void initGLTextures();

void idleRenderScene();
void SetStereoViewport(int eye);
void SetViewingFrustum( int Eye, CvMat *captureParams);
void reshape(int w, int h);
void RenderWorld(IplImage *frame, std::vector<MarkerTransform> transform, int Eye);
void idle();
void keyboard(unsigned char key, int x, int y);

#define DEGREESTORADIANS		0.0174532925f
#define RADIANSTODEGREES		57.29577951308f

GLint		g_screenWidth	= 640, g_screenHeight = 480, g_bpp = 32;

Capture *capLeft, *capRight;

GLuint GLTextureID;

const bool stereoRegistration = true;

OPIRALibrary::Registration *regLeft, *regRight;

int main(int argc, char** argv) {
	if (!initHMD()) return 1;

	//Initialise our Camera
	capLeft = new Camera(0, cvSize(320,240), "camera.yml");
	capRight = new Camera(1, cvSize(320,240), "camera.yml");

	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB); // Double Buffered and RGB color model
	glutInitWindowSize( g_screenWidth, g_screenHeight );
	glutInitWindowPosition( 0, 0 );

	glutCreateWindow("Example");
	glutDisplayFunc(idleRenderScene);// assign the main drawing function
	glutReshapeFunc(reshape);		// Hook reshape calls for proper perspective.
	glutKeyboardFunc(keyboard);

	regLeft = new OPIRALibrary::RegistrationOPIRA(new OCVSurf()); regLeft->addMarker("CelicaSmall.bmp");
	regRight = new OPIRALibrary::RegistrationOPIRA(new OCVSurf()); regRight->addMarker("CelicaSmall.bmp");

	initGLTextures();

	glutIdleFunc(idle);
	glutFullScreen();

	glutMainLoop();
}

void idle() {
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
	if (key==27) exit(0);
	if (key=='a') {g_EyeSeparation-=0.01; printf("%f\n", g_EyeSeparation); }
	if (key=='d') {g_EyeSeparation+=0.01; printf("%f\n", g_EyeSeparation); }
}


void idleRenderScene()
{
	//clear color and depth buffers before 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	IplImage *frame_input; std::vector<MarkerTransform> mt;
		if (g_IWRStereoscopyMode == SIDE_X_SIDE){
			// Render left eye frame
			{
				frame_input = capLeft->getFrame(); 
				mt = regLeft->performRegistration(frame_input, capLeft->getParameters(), capLeft->getDistortion());
				SetStereoViewport( LEFT_EYE );
				RenderWorld( frame_input, mt, LEFT_EYE );
				glFinish();
				//cvReleaseImage(&frame_input);
			}

			if (stereoRegistration) { for (int i=0; i<mt.size(); i++) mt.at(i).clear(); mt.clear(); }
			
			// Render MATCHING right eye frame; (No tracking or animation movement from the left eyes frame.)	
			{
				frame_input = capRight->getFrame();
				if (stereoRegistration) mt = regRight->performRegistration(frame_input, capRight->getParameters(), capRight->getDistortion());
				SetStereoViewport( RIGHT_EYE );
				RenderWorld( frame_input, mt, RIGHT_EYE );
				glFinish();
				cvReleaseImage(&frame_input);
			}

			for (int i=0; i<mt.size(); i++) mt.at(i).clear(); mt.clear();

			// Force GPU to finish, the rendered frame must scan out on the next VSYNC.
			glutSwapBuffers();
		}
}

void SetStereoViewport(int eye){
	if (eye == LEFT_EYE){
		glViewport( 0, 0, g_screenWidth/2, g_screenHeight );
	} else if (eye == RIGHT_EYE) {
		glViewport( g_screenWidth/2, 0, g_screenWidth/2, g_screenHeight );
	} else {
		glViewport( 0, 0, g_screenWidth, g_screenHeight );
	}
}

void SetViewingFrustum( int Eye, CvMat *captureParams)
{
//GLfloat		fovy	= 2 * atan(g_screenHeight/(2*captureParams->data.db[4])) * RADIANSTODEGREES;	//field of view in y-axis
//GLfloat		aspect	= ((double)g_screenWidth/(double)g_screenHeight) * ((double)captureParams->data.db[0] / (double)captureParams->data.db[4]);		//screen aspect ratio
GLfloat		fovy	= 45.0;                                          //field of view in y-axis
GLfloat		aspect	= float(g_screenWidth)/float(g_screenHeight);    //screen aspect ratio

GLfloat		nearZ	= 1.0;                                        //near clipping plane
GLfloat		farZ	= 1000.0;                                      //far clipping plane
GLfloat		top		= nearZ * tan( DEGREESTORADIANS * fovy / 2);     
GLdouble	right	= aspect * top;                             
GLdouble	frustumshift= (g_EyeSeparation / 2) * nearZ / g_FocalLength;
GLdouble	leftfrustum, rightfrustum, bottomfrustum, topfrustum;
GLfloat		modeltranslation;

	switch( Eye ) {
		case LEFT_EYE:
			// Setup left viewing frustum.
			topfrustum		= top;
			bottomfrustum	= -top;
			leftfrustum		= -right + frustumshift;
			rightfrustum	= right + frustumshift;
			modeltranslation= g_EyeSeparation / 2;
		break;
		case RIGHT_EYE:
			// Setup right viewing frustum.
			topfrustum		= top;
			bottomfrustum	= -top;
			leftfrustum		= -right - frustumshift;
			rightfrustum	= right - frustumshift;
			modeltranslation= -g_EyeSeparation / 2;
		break;
		case MONO_EYES:
			// Setup mono viewing frustum.
			topfrustum		= top;
			bottomfrustum	= -top;
			leftfrustum		= -right;
			rightfrustum	= right;
			modeltranslation= 0.0f;
		break;
	}
	// Initialize off-axis projection if in stereoscopy mode.
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();                  
	glFrustum(leftfrustum, rightfrustum, bottomfrustum, topfrustum,	nearZ, farZ);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// Translate Camera to cancel parallax.
	glTranslated(modeltranslation, 0.0f, 0.0f);

}

void reshape(int w, int h)
{
    if (h==0)
        h=1; //prevent divide by 0
	g_screenWidth  = w;
	g_screenHeight = h;
    glViewport(0, 0, w, h);
}

//
// Render the geometry of the world, for this Eye.
//  Including status output, if enabled.
//
void RenderWorld(IplImage *frame, std::vector<MarkerTransform> transform, int Eye)
{
	// Select appropriate back buffer.
	if( g_IWRStereoscopyMode != SIDE_X_SIDE ){
		switch( Eye ) {
			case LEFT_EYE:
				glDrawBuffer(GL_BACK_LEFT);                              
			break;
			case RIGHT_EYE:
				glDrawBuffer(GL_BACK_RIGHT);                             
			break;
			case MONO_EYES:
				glDrawBuffer(GL_BACK);                             
			break;
		}
	} else {
		glDrawBuffer(GL_BACK);
	}

	//Render Background
	{
		glMatrixMode(GL_PROJECTION); glLoadIdentity();
		int left, right;
		if (Eye == LEFT_EYE){
			left = 0; right = g_screenWidth/2;
		} else if (Eye == RIGHT_EYE) {
			left = g_screenWidth/2; right = g_screenWidth;
		} else {
			left = 0; right = g_screenWidth;
		}
		glOrtho(left, right, g_screenHeight, 0.0, 1.0, -1.0);
		glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	
		//Turn off Light and enable a texture
		glDisable(GL_LIGHTING);	glEnable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST);

		glBindTexture(GL_TEXTURE_2D, GLTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, frame->width, frame->height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, frame->imageData);

		//Draw the background
		glPushMatrix();
			glColor3f(255, 255, 255);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2f(0.0, 0.0);	glVertex2f(left+10, 10.0);
				glTexCoord2f(1.0, 0.0);	glVertex2f(right-10, 10.0);
				glTexCoord2f(0.0, 1.0);	glVertex2f(left+10, g_screenHeight-10);
				glTexCoord2f(1.0, 1.0);	glVertex2f(right-10, g_screenHeight-10);
			glEnd();
		glPopMatrix();

		//Turn off Texturing
		glBindTexture(GL_TEXTURE_2D, 0);
		glEnable(GL_LIGHTING);	glDisable(GL_TEXTURE_2D); glEnable(GL_DEPTH_TEST);
	
	}

	if (transform.size()>0) {
		if (stereoRegistration) {
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixd(transform.at(0).projMat);
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixd(transform.at(0).transMat);
		} else {
			// Translate Camera to cancel parallax.
			SetViewingFrustum(Eye, capLeft->getParameters());
			glMultMatrixd(transform.at(0).transMat);
		}

		
		glEnable(GL_LIGHTING);
			glColor3f(1,0,0);
			glTranslatef(transform.at(0).marker.size.width/2.0, transform.at(0).marker.size.height/2.0,-25);
			glRotatef(-90, 1.0,0,0.0);
			glutSolidTeapot(50.0);
		glDisable(GL_LIGHTING);

	}

}

void initGLTextures() {
	//Set up Materials 
	GLfloat mat_specular[] = { 0.4, 0.4, 0.4, 1.0 };
	GLfloat mat_diffuse[] = { .8,.8,.8, 1.0 };
	GLfloat mat_ambient[] = { .4,.4,.4, 1.0 };

	glShadeModel(GL_SMOOTH);//smooth shading
	glMatrixMode(GL_MODELVIEW);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialf(GL_FRONT, GL_SHININESS, 100.0);//define the material
	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);//enable the material
	glEnable(GL_NORMALIZE);

	//Set up Lights
	GLfloat light0_ambient[] = {0.1, 0.1, 0.1, 0.0};
	float light0_diffuse[] = { 0.8f, 0.8f, 0.8, 1.0f };
	glLightfv (GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glEnable(GL_LIGHT0);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_NORMALIZE);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable (GL_LINE_SMOOTH);	

	//Initialise the OpenCV Image for GLRendering
	glGenTextures(1, &GLTextureID); 	// Create a Texture object
    glBindTexture(GL_TEXTURE_2D, GLTextureID);  //Use this texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);	
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);	
	glBindTexture(GL_TEXTURE_2D, 0);

}