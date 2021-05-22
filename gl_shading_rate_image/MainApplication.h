//////////// Variable Rate Shading Sample ////////////
// 
// Look for ShadingRateSample to find all relevant parts of the sample related to GL_NV_shading_rate_image.
//
// This sample shows how to use the GL_NV_shading_rate_image API in OpenGL. To show this,
// a lot of the code is related to draw something and very little code is related to changing the 
// shading rate. To better find those locations, look for "ShadingRateSample" comments.
//
// First you might want to look at the begin() function of the MainApplication class. It will init the
// extension and create the OpenGL objects needed.
// Next look at the think() function of the MainApplication class to see the main rendering loop.
//

#pragma once

//#define __HTTP_PART //philip: ????????????.????????????????????????????????????��???????????? 2021-4-6? ???????��???????copy??????
#ifdef __HTTP_PART
#include "httpVisiting.h"
#endif

#include <gl/glew.h>

#include <nv_helpers/anttweakbar.hpp>
#include <nv_helpers/cameracontrol.hpp>
#include <nv_helpers/geometry.hpp>
#include <nv_helpers_gl/glresources.hpp>
#include <nv_helpers_gl/programmanager.hpp>
#include <nv_helpers_gl/WindowProfiler.hpp>
#include <nv_math/nv_math_glsltypes.h>

#define __VR_PART //lcj: openvr相关部分.打开-关闭这个宏以改变是否编译openvr相关部分。 2021-4-6
#ifdef __VR_PART
#include "../shared_sources/openvr/openvr.h"
#endif

//#define __RENDER_TORUS//画铁索还是mesh呢？这里提供了一个switch。

#include "glpp/Shader.h"
#include "glpp/model.h"

#include "stb_image/stb_image.h"

#include <Windows.h>

#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>

using namespace nv_math;
#include "common.h"

//////////// ShadingRateSample ////////////
//
// As this is a new extension, the first thing to do is to load all new function pointers and define all
// missing constants as they are being described in the extension spec.
//
typedef void (GLAPIENTRY * PFNGLBINDSHADINGRATEIMAGENVPROC) (GLuint texture);
typedef void (GLAPIENTRY * PFNGLSHADINGRATEIMAGEPALETTENVPROC) (GLuint viewport, GLuint first, GLuint count, const GLenum *rates);
typedef void (GLAPIENTRY * PFNGLGETSHADINGRATEIMAGEPALETTENVPROC) (GLuint viewport, GLuint entry, GLenum *rate);
typedef void (GLAPIENTRY * PFNGLSHADINGRATEIMAGEBARRIERNVPROC) (GLboolean synchronize);
typedef void (GLAPIENTRY * PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC)(GLenum rate, GLuint samples, const GLint *locations);
typedef void (GLAPIENTRY * PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC) (GLenum rate, GLuint index, GLint *location);

extern PFNGLBINDSHADINGRATEIMAGENVPROC				glBindShadingRateImageNV;
extern PFNGLSHADINGRATEIMAGEPALETTENVPROC			glShadingRateImagePaletteNV;
extern PFNGLGETSHADINGRATEIMAGEPALETTENVPROC		glGetShadingRateImagePaletteNV;
extern PFNGLSHADINGRATEIMAGEBARRIERNVPROC			glShadingRateImageBarrierNV;
extern PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC		glShadingRateSampleOrderCustomNV;
extern PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC	glGetShadingRateSampleLocationivNV;

#define GL_SHADING_RATE_IMAGE_NV							0x9563

#define GL_SHADING_RATE_NO_INVOCATIONS_NV					0x9564
#define GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV			0x9565
#define GL_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV		0x9566
#define GL_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV		0x9567
#define GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV		0x9568
#define GL_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV		0x9569
#define GL_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV		0x956A
#define GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV		0x956B
#define GL_SHADING_RATE_2_INVOCATIONS_PER_PIXEL_NV			0x956C
#define GL_SHADING_RATE_4_INVOCATIONS_PER_PIXEL_NV			0x956D
#define GL_SHADING_RATE_8_INVOCATIONS_PER_PIXEL_NV			0x956E
#define GL_SHADING_RATE_16_INVOCATIONS_PER_PIXEL_NV			0x956F

#define GL_SHADING_RATE_IMAGE_BINDING_NV					0x955B
#define GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV				0x955C
#define GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV				0x955D
#define GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV				0x955E
#define GL_MAX_COARSE_FRAGMENT_SAMPLES_NV					0x955F

#define GL_SHADING_RATE_SAMPLE_ORDER_DEFAULT_NV				0x95AE
#define GL_SHADING_RATE_SAMPLE_ORDER_PIXEL_MAJOR_NV			0x95AF
#define GL_SHADING_RATE_SAMPLE_ORDER_SAMPLE_MAJOR_NV		0x95B0

// Looking for the extension and loading the function pointers.
// Note that this sample will not work on older hardware and just quit.
bool loadShadingRateExtension();

// The window size:
#define SAMPLE_SIZE_WIDTH 640
#define SAMPLE_SIZE_HEIGHT 480
#ifdef __VR_PART
#define VR_SIZE_WIDTH 1852
#define VR_SIZE_HEIGHT 2056
#endif
#define SAMPLE_MAJOR_VERSION 4
#define SAMPLE_MINOR_VERSION 6


namespace render
{
	// the settings accessible via the AntTweakBar menu
	struct Tweak {
		Tweak()
			: m_loadFactor(20)
			, m_fragmentLoad(10)
			, m_torus_n(8)
			, m_torus_m(8)
			, m_visualizeShadingRate(false)
			, m_visualizeDepth(false)
			, m_fullShadingRateForGreenObjects(true)
			, m_activateShadingRate(true)
			, m_offset(0.)
		{ /* NOTHING HERE */ }

		typedef enum { 
			RATE_DEPTH_BASED, 
			RATE_ECCENTRICITY_BASED,
			RATE_ECCENTRICITY_BASED_VR, 
			RATE_1X1, 
			RATE_2X2, 
			RATE_4X4} ShadingMode;
		ShadingMode m_shadingMode = RATE_DEPTH_BASED;
		TwType m_ShadingModeType;

		int	m_loadFactor;
		int	m_fragmentLoad;
		int	m_torus_n;
		int	m_torus_m;
		int	m_offscreenPerEyeX;
		int	m_offscreenPerEyeY;

		bool m_visualizeShadingRate;
		bool m_visualizeDepth;
		bool m_fullShadingRateForGreenObjects;
		bool m_activateShadingRate;

		vec3 m_offset;
	};

	struct Vertex {
		Vertex(const nv_helpers::geometry::Vertex& vertex) {
			position = vertex.position;
			normal = vertex.normal;
			color = nv_math::vec4(1.0f);
		}

		nv_math::vec4	 position;
		nv_math::vec4	 normal;
		nv_math::vec4	 color;
	};

	struct Buffers
	{
		Buffers()
			: vbo(0)
			, ibo(0)
			, sceneUbo(0)
			, objectUbo(0)
			, numVertices(0)
			, numIndices(0)
		{ /* NOTHING HERE */ }

		GLuint	vbo;
		GLuint	ibo;
		GLuint	sceneUbo;
		GLuint	objectUbo;

		GLsizei numVertices;
		GLsizei numIndices;
	};

	struct Programs
	{
		nv_helpers_gl::ProgramManager::ProgramID sceneShader;
		//glpp::Shader sceneShader;
		glpp::Shader companionShader;	//lcj: 很抱歉，nv_helper里的ProgramManager我用不来，这里只好先用原始方式了
		GLuint companionVAO;	//只好先放在这里了
	};

	struct Textures
	{
		GLuint tex4test{0};//测试stb库正常运作的纹理

		GLuint colorTexArray{0};
		GLuint depthTexArray{0};
#ifdef __VR_PART
		GLuint colorTex4vr_Left{0};
		GLuint colorTex4vr_Right{0};
#endif
		GLuint shadingRateImageEccentricityBased{0};
		GLuint shadingRateImage1X1{0};
		GLuint shadingRateImage2X2{0};
		GLuint shadingRateImage4X4{0};

	};

	struct Data
	{
		Data()
#ifdef __VR_PART
			//: renderWidth(VR_SIZE_WIDTH)
			//, renderHeight(VR_SIZE_HEIGHT)
			: renderWidth(SAMPLE_SIZE_WIDTH)
			, renderHeight(SAMPLE_SIZE_HEIGHT)
#else
			: renderWidth(SAMPLE_SIZE_WIDTH)
			, renderHeight(SAMPLE_SIZE_HEIGHT)
#endif
			, offscreenScale(1)
			, lastOffscreenScale(-1)
			, shadingRateImageTexelHeight(16) // important: the value needs to get queried at runtime, future HW might use a different value!
			, shadingRateImageTexelWidth(16)	// same for the width
		{
			sceneData.projNear = 0.1f;
			sceneData.projFar =	10.0f;
		}

		Tweak		tweak;

		Buffers		buf;
		Textures	tex;
		Programs	prog;	// shaders

		glpp::Model	model;

		SceneData	sceneData;
		ObjectData	objectData;

		GLuint		fbo4render;	 // fbo -> frame buffer object
#ifdef __VR_PART
		GLuint		fbo4vr_Left;
		GLuint		fbo4vr_Right;
#endif

		nv_helpers_gl::ProgramManager pm;

		//////////// ShadingRateSample ////////////
		// 
		void updateShadingRateImageSize()
		{
			shadingRateImageWidth = (renderWidth + shadingRateImageTexelWidth - 1) / shadingRateImageTexelWidth;
			shadingRateImageHeight = (renderHeight + shadingRateImageTexelHeight - 1) / shadingRateImageTexelHeight;
		}

		// the resolution that rendering happens at
		int renderWidth;
		int renderHeight;

		int offscreenScale;	 //lcj: what does this mean?
		int lastOffscreenScale;

		// the size of SRI:
		// it should satisfies: (SRI_w, SRI_h) = (render_w, render_h) / SRI_texel_siz
		int shadingRateImageWidth;
		int shadingRateImageHeight;

		// Query this with glGetIntegerv and GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV / GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV
		// The value is hardware dependent.
		GLint shadingRateImageTexelHeight; //normally 16x16.
		GLint shadingRateImageTexelWidth;

		// Shading rate image data (Stored here to avoid frequent memory allocation)
		std::vector<uint8_t> shadingRateImageData;
	};

	bool initPrograms(Data& rd);

	bool initFBOs(Data& rd);


	// some geometry to render, the details are not important for this sample
	void initBuffers(Data& rd);

	void createFoveationTexture(
		int width, int height, //(SRI_w, SRI_h)
		float centerX, float centerY, 
		std::vector<uint8_t> &data,
		bool vr_specified=false
	);

	bool initTextures(Data& rd);

	// rendering the "scene", nothing specific to our extensions here
	void renderTori(
		Data& rd, 
		size_t numTori, 
		size_t width, size_t height, 
		mat4f view
	);
} //namespace render


void TW_CALL button_rebuild_geometry(void* c);

class MainApplication : public nv_helpers_gl::WindowProfiler {
public:
	/* constructor */
	MainApplication();

	/* key functions */
	bool begin();
	void think(double time);
	void resize(int width, int height);
	void end();
	// key function - extend
#ifdef __VR_PART
	void updateHMDMatrices();
#endif

	bool mouse_pos(int x, int y) {
		return !!TwEventMousePosGLFW(x, y);
	}
	bool mouse_button(int button, int action) {
		return !!TwEventMouseButtonGLFW(button, action);
	}
	bool mouse_wheel(int wheel) {
		return !!TwEventMouseWheelGLFW(wheel);
	}
	bool key_button(int button, int action, int mods) {
		return nv_helpers::handleTwKeyPressed(button, action, mods);
	}

	void rebuild_geometry()
	{
		render::initBuffers(m_rd);
		LOGOK("Scene data:\n");
		LOGOK("Vertices per torus:	%i\n", m_rd.buf.numVertices);
		LOGOK("Triangles per torus: %i\n", m_rd.buf.numIndices / 3);
	};

protected:
	nv_helpers::CameraControl m_control;
	size_t				m_frameCount;
	render::Data		m_rd;

	//begin() and think() 中调用的小模块
#ifdef __VR_PART
	bool initVR();
#endif
	bool initShadingRatePallete();
	bool initTW();
	//----------分割线------------//
#ifdef __VR_PART
	void updateSceneData(vr::Hmd_Eye nEye);
	void updateShadingRateImage(vr::Hmd_Eye nEye);//左右眼的SRI可能不同
	void predictGazePos();//预测眼位置，现在实现了的是简单算法

	void renderScene(vr::Hmd_Eye nEye);
	void submit2VR();//后面的这几个都是双眼一起操作
#else
	void updateSceneData();
	//void updateShadingRateImage();
	//void renderScene();
#endif
	void updateShadingRateImage();//实现双眼不同SRI之前先用这个
	void renderScene();			//这个同理
	void renderCompanionWindow();

	void drawCross(float rx, float ry);//在屏幕上NDC=(rx,ry)处画个小叉，用来标识注视点
#ifdef __VR_PART
	// main pointer for vr system
	vr::IVRSystem* m_pHMD{nullptr};

	// data structures for vr device(hmd, controllers)
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	mat4 m_mat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	char m_cDeviceType[vr::k_unMaxTrackedDeviceCount];

	mat4  m_mat4HMDProjLeft;
	mat4  m_mat4HMDProjRight;	//projection matrix not used so far
	mat4 m_mat4HMDPose;
	mat4 m_mat4HMDEyePosLeft;
	mat4 m_mat4HMDEyePosRight;

	

	unsigned m_uValidPoseCount {0};

	// pose: 位姿，方位 -> 旋转; pos->position: 位置 -> 平移
	void updateHMDMatrixPose();
	void updateHMDMatrixPosEye( vr::Hmd_Eye nEye );
	void updateHMDMatrixProjectionEye( vr::Hmd_Eye nEye );
	mat4 steamVRMat2mat4( const vr::HmdMatrix34_t &matPose );	//类型转换的工具函数
#endif
	//预测注视点用
	double m_tprev;//前一帧的时间
	//vec3 m_vec3Fwd;
	vec3 m_vec3FwdPrev;

	//FP: fixation point, 注视点
	//以屏幕中心为远点，xy区间为[-1,1]的坐标系中，注视点的位置为（极坐标）(ρ,η)
	//float m_FP_eta;
	//float m_FP_rho;
	//一种更trivial的描述方式，也是在上述定义的坐标中
	float m_FP_x{0.};
	float m_FP_y{0.};
};