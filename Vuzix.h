#ifndef VUZIX_H
#define VUZIX_H

#include "IWRsdk.h"

enum							{ MONO, PROGRESSIVE_SCAN, SIDE_X_SIDE };
enum							{ LEFT_EYE=0, RIGHT_EYE, MONO_EYES };
#define DEFAULT_SEPARATION		0.25f
#define DEFAULT_FOCAL_LENGTH	10.0f
float		g_EyeSeparation = DEFAULT_SEPARATION;// Intraocular Distance: aka, Distance between left and right cameras.
float		g_FocalLength	= DEFAULT_FOCAL_LENGTH;	// Screen projection plane: aka, focal length(distance to front of virtual screen).
int g_IWRStereoscopyMode;

bool initHMD() {
	// Open the VR920's tracker driver.
	long iwr_status = IWRLoadDll();
	if( iwr_status != IWR_OK ) {
		MessageBox( NULL, "NO VR920 iwrdriver support", "VR920 Driver ERROR", MB_OK );
		IWRFreeDll();
		return false;
	}

	// Handle stereo setup
	HANDLE g_StereoHandle = IWRSTEREO_Open();
	if( g_StereoHandle == INVALID_HANDLE_VALUE ) {
		if (GetLastError() == ERROR_INVALID_FUNCTION) 
			MessageBox( NULL, "Your VR920 firmware does not support stereoscopy.  Please update your firmware.", "VR920 Stereo ERROR", MB_OK );
		else 
			MessageBox( NULL, "NO VR920 Stereo Driver handle", "VR920 STEREO ERROR", MB_OK );
		IWRFreeDll();
		return false;
	}

	// Look for proc address...Ask for device installed on users system, set/clear sidexside
	int Pid;
	if( IWRGetProductID ) 
		Pid = IWRGetProductID();

	if (Pid == IWR_PROD_WRAP920)
		g_IWRStereoscopyMode = SIDE_X_SIDE;
	else
		g_IWRStereoscopyMode = PROGRESSIVE_SCAN;

	return true;
}


#endif