#ifndef NOHMD_H
#define NOHMD_H

enum							{ MONO, PROGRESSIVE_SCAN, SIDE_X_SIDE };
enum							{ LEFT_EYE=0, RIGHT_EYE, MONO_EYES };
#define DEFAULT_SEPARATION		0.25f
#define DEFAULT_FOCAL_LENGTH	10.0f
float		g_EyeSeparation = DEFAULT_SEPARATION;// Intraocular Distance: aka, Distance between left and right cameras.
float		g_FocalLength	= DEFAULT_FOCAL_LENGTH;	// Screen projection plane: aka, focal length(distance to front of virtual screen).
int g_IWRStereoscopyMode;

bool initHMD() { g_IWRStereoscopyMode = SIDE_X_SIDE; return true; }

#endif