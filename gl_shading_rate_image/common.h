// This file is included by
// both SHADERs and CPP sources
// so treat it CAREFULLY		-lcj

#pragma once

// general behavior defines
// benchmark mode - change render load and print perf data
#define BENCHMARK_MODE 0		// default 0
// measure times of the different stages (currently not mgpu-safe!)
#define DEBUG_MEASURETIME 0 // default 0
// exit after a set time in seconds
#define DEBUG_EXITAFTERTIME 0 // default 0



// scene data defines
#define VERTEX_POS			0
#define VERTEX_NORMAL		1
#define OFFSET_LOC			2

#define UBO_SCENE			0
#define UBO_OBJECT			1
#define ATOMICS				2

#define TEX_VOL_DATA		0
#define TEX_VOL_DEPTH		1

// compose data defines
#define UBO_COMP			0
#define TEX_COMPOSE_COLOR 	0
#define TEX_COMPOSE_DEPTH 	8

struct SceneData
{
	mat4 viewMatrix;		// view matrix: world->view
	mat4 projMatrix;		// proj matrix: view ->proj
	mat4 viewProjMatrix;	// viewproj	 : world->proj
	vec3 lightPos_world;	// light position in world space
	vec3 eyepos_world;		// eye position in world space
	vec3 eyePos_view;		// eye position in view space

	vec3 offset;

	int	loadFactor;
	int	fragmentLoadFactor;

	float projNear;
	float projFar;
	float depth_stare;

	int visualizeShadingRate;
	int visualizeDepth;

	int fullShadingRateForGreenObjects;

	int applyDepthBasedFoveatedRendering;
};

struct ObjectData
{
	mat4 model;			// model -> world
	mat4 modelView;		// model -> view
	mat4 modelViewIT;	// model -> view for normals
	mat4 modelViewProj;	// model -> proj
	vec3 color;			// model color
};

struct ComposeData 
{
	uint numTextures;
};

#if defined(GL_core_profile) || defined(GL_compatibility_profile) || defined(GL_es_profile)
// prevent this to be used by c++

#if defined(USE_SCENE_DATA)
layout(std140,binding=UBO_SCENE) uniform sceneBuffer {
	SceneData scene;
};
layout(std140,binding=UBO_OBJECT) uniform objectBuffer {
	ObjectData object;
};
#endif

#endif
