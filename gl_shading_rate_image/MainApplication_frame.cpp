#include "MainApplication.h"

bool MainApplication::begin()
{
	vsync(false);

	bool validated(true);

#ifdef __VR_PART
	this -> initVR();
#endif
	//////////// ShadingRateSample ////////////
	// 
	// This sample can not run if the extension is not present.
	//
	if (!loadShadingRateExtension())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		return false;
	}

	this -> initShadingRatePallete();

	const GLubyte *string = glGetString(GL_RENDERER);
	LOGOK("Renderer: %s\n", string);
	string = glGetString(GL_VENDOR);
	LOGOK("Vendor: %s\n", string);
	string = glGetString(GL_VERSION);
	LOGOK("Version: %s\n\n", string);

	// control setup
	m_control.m_sceneOrbit = nv_math::vec3(0.0f);
	m_control.m_sceneDimension = 1.0f;
	m_control.m_viewMatrix = nv_math::look_at(m_control.m_sceneOrbit - vec3(0, 0, -m_control.m_sceneDimension), m_control.m_sceneOrbit, vec3(0, 1, 0));

	render::initPrograms(m_rd);
	render::initFBOs(m_rd);
	render::initBuffers(m_rd);
	render::initTextures(m_rd);
	
	//m_rd.model.loadModel("assets/model/nanosuit/nanosuit.obj");
	m_rd.model.loadModel("assets/model/Sponza/SponzaNoFlag.obj");

	std::cout << "Scene data: \n";
	std::cout << "Vertices per torus:	" << m_rd.buf.numVertices << "\n";
	std::cout << "Triangles per torus: " << m_rd.buf.numIndices / 3 << "\n";

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	this -> initTW();
	
	return validated;
}

void MainApplication::think(double time)
{
	if (m_rd.lastOffscreenScale != m_rd.offscreenScale)// recognize and handle window size change
		resize(m_window.m_viewsize[0], m_window.m_viewsize[1]);

	m_rd.tweak.m_offscreenPerEyeX = m_window.m_viewsize[0] / 2;
	m_rd.tweak.m_offscreenPerEyeY = m_window.m_viewsize[1];

	++m_frameCount;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SHADING_RATE_IMAGE_NV);
#ifndef __VR_PART
	this -> updateSceneData();
	this -> updateShadingRateImage();
	this -> renderScene();
#else
	//lcj: 2021-5-22 performance profile
		double t[12];
		int ti=0;
		t[ti++] = this->sysGetTime();
	updateHMDMatrices();
		t[ti++] = this->sysGetTime();
	this -> updateSceneData(vr::Eye_Left);
		t[ti++] = this->sysGetTime();
	this -> updateShadingRateImage(vr::Eye_Left);
		t[ti++] = this->sysGetTime();
	this -> renderScene(vr::Eye_Left);
		t[ti++] = this->sysGetTime();

	this -> updateSceneData(vr::Eye_Right);
	this -> updateShadingRateImage(vr::Eye_Right);
	this -> renderScene(vr::Eye_Right);
	
		t[ti++] = this->sysGetTime();
	this -> submit2VR();
		t[ti++] = this->sysGetTime();
#endif
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SHADING_RATE_IMAGE_NV);

	this -> predictGazePos();

	this -> renderCompanionWindow();
		t[ti++] = this->sysGetTime();

	// render GUI
	TwDraw();
		t[ti++] = this->sysGetTime();

		printf("profile: [%.4f,%.4f,%.4f](%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f) ms\n",
			(t[8]-t[0])*1000, (t[4]-t[1])*1000, (t[5]-t[4])*1000,
			(t[1]-t[0])*1000, (t[2]-t[1])*1000, (t[3]-t[2])*1000, (t[4]-t[3])*1000, (t[5]-t[4])*1000, (t[6]-t[5])*1000, (t[7]-t[6])*1000, (t[8]-t[7])*1000);
}

void MainApplication::resize(int width, int height)
{
	m_window.m_viewsize[0] = width;
	m_window.m_viewsize[1] = height;

	glViewport(0,0,width,height);	
#ifndef __VR_PART // resize window parameters only
	m_rd.renderWidth = width / m_rd.offscreenScale;
	m_rd.renderHeight = height / m_rd.offscreenScale;
	m_rd.lastOffscreenScale = m_rd.offscreenScale;

	//////////// ShadingRateSample ////////////
	// 
	// Remember to update the shading rate image each
	// time the window gets resized:
	m_rd.updateShadingRateImageSize();

	initTextures(m_rd);
#endif
	TwWindowSize(width, height);
}

void MainApplication::end()
{
#ifdef __VR_PART
	if( m_pHMD )
{
	vr::VR_Shutdown();
	m_pHMD = NULL;
}
#endif
	nv_helpers_gl::deleteBuffer(m_rd.buf.vbo);
	nv_helpers_gl::deleteBuffer(m_rd.buf.ibo);
	nv_helpers_gl::deleteBuffer(m_rd.buf.sceneUbo);
	nv_helpers_gl::deleteBuffer(m_rd.buf.objectUbo);
	m_rd.pm.deletePrograms();
}

