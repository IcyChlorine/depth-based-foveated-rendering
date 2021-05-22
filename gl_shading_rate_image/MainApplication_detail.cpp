#include "MainApplication.h"

#ifdef __HTTP_PART
clock_t stamp = 0;
clock_t GAP = 1000.0f;
#endif

//
// Extenson and function loading
//
PFNGLBINDSHADINGRATEIMAGENVPROC				glBindShadingRateImageNV			{nullptr};
PFNGLSHADINGRATEIMAGEPALETTENVPROC			glShadingRateImagePaletteNV			{nullptr};
PFNGLGETSHADINGRATEIMAGEPALETTENVPROC		glGetShadingRateImagePaletteNV		{nullptr};
PFNGLSHADINGRATEIMAGEBARRIERNVPROC			glShadingRateImageBarrierNV			{nullptr};
PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC		glShadingRateSampleOrderCustomNV	{nullptr};
PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC	glGetShadingRateSampleLocationivNV	{nullptr};

bool loadShadingRateExtension()
{
	//
	// Make sure the extension is supported
	//
	GLint numExtensions;
	bool shadingRateImageExtensionFound = false;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
	for (GLint i = 0; i < numExtensions && !shadingRateImageExtensionFound; ++i)
	{
		std::string name((const char*)glGetStringi(GL_EXTENSIONS, i));
		shadingRateImageExtensionFound = (name == "GL_NV_shading_rate_image");
	}

	if (!shadingRateImageExtensionFound)
	{
		LOGOK("\nGL_NV_shading_rate_image extension NOT found!\n\n");
		return false;
	}

	glBindShadingRateImageNV = (PFNGLBINDSHADINGRATEIMAGENVPROC)wglGetProcAddress("glBindShadingRateImageNV");
	glShadingRateImagePaletteNV = (PFNGLSHADINGRATEIMAGEPALETTENVPROC)wglGetProcAddress("glShadingRateImagePaletteNV");
	glGetShadingRateImagePaletteNV = (PFNGLGETSHADINGRATEIMAGEPALETTENVPROC)wglGetProcAddress("glGetShadingRateImagePaletteNV");
	glShadingRateImageBarrierNV = (PFNGLSHADINGRATEIMAGEBARRIERNVPROC)wglGetProcAddress("glShadingRateImageBarrierNV");
	glShadingRateSampleOrderCustomNV = (PFNGLSHADINGRATESAMPLEORDERCUSTOMNVPROC)wglGetProcAddress("glShadingRateSampleOrderCustomNV");
	glGetShadingRateSampleLocationivNV = (PFNGLGETSHADINGRATESAMPLELOCATIONIVNVPROC)wglGetProcAddress("glGetShadingRateSampleLocationivNV");

	if ((glBindShadingRateImageNV != nullptr)
		&& (glShadingRateImagePaletteNV != nullptr)
		&& (glGetShadingRateImagePaletteNV != nullptr)
		&& (glShadingRateImageBarrierNV != nullptr)
		&& (glShadingRateSampleOrderCustomNV != nullptr)
		&& (glGetShadingRateSampleLocationivNV != nullptr))
	{
		LOGOK("\nAll functions for GL_NV_shading_rate_image extension found!\n");
	}
	else
	{
		LOGOK("\nSome functions for GL_NV_shading_rate_image extension are missing!\n");
		return false;
	}

	return true;
}



//
// The window size:
//
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
	bool initPrograms(Data& rd)
	{
		nv_helpers_gl::ProgramManager& pm = rd.pm;
		Programs& programs = rd.prog;

		int validated(true);
		pm.addDirectory(std::string(PROJECT_NAME));
		pm.addDirectory(NVPWindow::sysExePath() + std::string(PROJECT_RELDIRECTORY));
		pm.addDirectory(std::string(PROJECT_ABSDIRECTORY));

		pm.registerInclude("common.h", "common.h");
		pm.registerInclude("noise.glsl", "noise.glsl");
#ifndef __RENDER_TORUS
		programs.sceneShader = pm.createProgram(
			nv_helpers_gl::ProgramManager::Definition(GL_VERTEX_SHADER, "#define USE_SCENE_DATA", "scene.vert.glsl"),
			nv_helpers_gl::ProgramManager::Definition(GL_FRAGMENT_SHADER, "#define USE_SCENE_DATA", "scene.frag.glsl"));
#else
		programs.sceneShader = pm.createProgram(
			nv_helpers_gl::ProgramManager::Definition(GL_VERTEX_SHADER, "#define USE_SCENE_DATA", "torus.vert.glsl"),
			nv_helpers_gl::ProgramManager::Definition(GL_FRAGMENT_SHADER, "#define USE_SCENE_DATA", "torus.frag.glsl"));
#endif

		validated = pm.areProgramsValid();

		if(!validated)// 如果已经出错了，那就直接返回吧
			return validated;//return false*/
		//programs.sceneShader = glpp::Shader("scene.vert.glsl", "scene.frag.glsl");

		/* lcj: 很抱歉，nv_helper里的ProgramManager我用不来，只好先用原始方式了
			这里会有一大坨
			有待改进 */
		// 以下是companion window 用的shader的初始化 [代码读取 -> 编译 -> 链接]
		programs.companionShader = glpp::Shader("companion.vert.glsl", "companion.frag.glsl");

		// 为companionShader创建vao - 因为companionWindow的渲染顶点是高度确定的
		float vert_data[]={
		//   x, y,z,	s,t
		//  左眼左上的三角形
			-1,-1,0,	0,0,
			 0,+1,0,	1,1,
			-1,+1,0,	0,1,
		//  左眼右下的三角形
			-1,-1,0,	0,0,
			 0,-1,0,	1,0,
			 0,+1,0,	1,1,
		//  右眼左上的三角形
			 0,-1,0,	0,0,
			+1,+1,0,	1,1,
			 0,+1,0,	0,1,
		//  右眼右下的三角形
			 0,-1,0,	0,0,
			+1,-1,0,	1,0,
			+1,+1,0,	1,1
		};
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glGenVertexArrays(1, &programs.companionVAO);

		glBindVertexArray(programs.companionVAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vert_data), vert_data, GL_STATIC_DRAW);

		glVertexAttribPointer(
			0, 3, //layout=0, len('xyz')=3
			GL_FLOAT, GL_FALSE, //meaningless here
			sizeof(float)*5, //stride of record, in bytes
			(void*)0
		);
		glVertexAttribPointer(
			1, 2, //layout=1, len('st')=2
			GL_FLOAT, GL_FALSE, //meaningless here
			sizeof(float)*5, //stride of record, in bytes
			(void*)(3*sizeof(float))//offset
		);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		//用后解绑好习惯
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		return validated;
	}

	bool initFBOs(Data& rd)
	{
		nv_helpers_gl::newFramebuffer(rd.fbo4render);
#ifdef __VR_PART
		nv_helpers_gl::newFramebuffer(rd.fbo4vr_Left);
		nv_helpers_gl::newFramebuffer(rd.fbo4vr_Right);
#endif
		return true;
	}

	// build geometry(torus, more specivically) and bind with VBO
	void initBuffers(Data& rd)
	{
		Buffers& buffers = rd.buf;

		// Torus geometry
		{
			unsigned int m = rd.tweak.m_torus_m;
			unsigned int n = rd.tweak.m_torus_n;
			float innerRadius = 0.8f;
			float outerRadius = 0.2f;

			std::vector< nv_math::vec3 > vertices;
			std::vector< nv_math::vec3 > tangents;
			std::vector< nv_math::vec3 > binormals;
			std::vector< nv_math::vec3 > normals;
			std::vector< nv_math::vec2 > texcoords;
			std::vector<unsigned int> indices;

			unsigned int size_v = (m + 1) * (n + 1);

			vertices.reserve(size_v);
			tangents.reserve(size_v);
			binormals.reserve(size_v);
			normals.reserve(size_v);
			texcoords.reserve(size_v);
			indices.reserve(6 * m * n);

			float mf = (float)m;
			float nf = (float)n;

			float phi_step = 2.0f * nv_pi / mf;
			float theta_step = 2.0f * nv_pi / nf;

			// Setup vertices and normals
			// Generate the Torus exactly like the sphere with rings around the origin along the latitudes.
			for (unsigned int latitude = 0; latitude <= n; latitude++) // theta angle
			{
				float theta = (float)latitude * theta_step;
				float sinTheta = sinf(theta);
				float cosTheta = cosf(theta);

				float radius = innerRadius + outerRadius * cosTheta;

				for (unsigned int longitude = 0; longitude <= m; longitude++) // phi angle
				{
					float phi = (float)longitude * phi_step;
					float sinPhi = sinf(phi);
					float cosPhi = cosf(phi);

					vertices.push_back(nv_math::vec3(radius			*	cosPhi,
						outerRadius *	sinTheta,
						radius			* -sinPhi));

					tangents.push_back(nv_math::vec3(-sinPhi, 0.0f, -cosPhi));

					binormals.push_back(nv_math::vec3(cosPhi * -sinTheta,
						cosTheta,
						sinPhi * sinTheta));

					normals.push_back(nv_math::vec3(cosPhi * cosTheta,
						sinTheta,
						-sinPhi * cosTheta));

					texcoords.push_back(nv_math::vec2((float)longitude / mf, (float)latitude / nf));
				}
			}

			const unsigned int columns = m + 1;

			// Setup indices
			for (unsigned int latitude = 0; latitude < n; latitude++)
			{
				for (unsigned int longitude = 0; longitude < m; longitude++)
				{
					// two triangles
					indices.push_back(latitude			* columns + longitude);	// lower left
					indices.push_back(latitude			* columns + longitude + 1);	// lower right
					indices.push_back((latitude + 1) * columns + longitude);	// upper left

					indices.push_back((latitude + 1) * columns + longitude);	// upper left
					indices.push_back(latitude			* columns + longitude + 1);	// lower right
					indices.push_back((latitude + 1) * columns + longitude + 1);	// upper right
				}
			}

			buffers.numVertices = static_cast<GLsizei>(vertices.size());
			GLsizeiptr const sizePositionAttributeData = vertices.size() * sizeof(vertices[0]);
			GLsizeiptr const sizeNormalAttributeData = normals.size() * sizeof(normals[0]);

			buffers.numIndices = static_cast<GLsizei>(indices.size());
			GLsizeiptr sizeIndexData = indices.size() * sizeof(indices[0]);

			nv_helpers_gl::newBuffer(buffers.vbo);
			glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);
			glBufferData(GL_ARRAY_BUFFER, sizePositionAttributeData + sizeNormalAttributeData, nullptr, GL_STATIC_DRAW);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizePositionAttributeData, &vertices[0]);
			glBufferSubData(GL_ARRAY_BUFFER, sizePositionAttributeData, sizeNormalAttributeData, &normals[0]);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			nv_helpers_gl::newBuffer(buffers.ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIndexData, &indices[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);
			glVertexAttribPointer(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
			glVertexAttribPointer(VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)(sizePositionAttributeData));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.ibo);

			glEnableVertexAttribArray(VERTEX_POS);
			glEnableVertexAttribArray(VERTEX_NORMAL);
		}


		nv_helpers_gl::newBuffer(buffers.sceneUbo);
		glNamedBufferDataEXT(buffers.sceneUbo, sizeof(SceneData), &rd.sceneData, GL_DYNAMIC_DRAW);

		nv_helpers_gl::newBuffer(buffers.objectUbo);
		glNamedBufferDataEXT(buffers.objectUbo, sizeof(ObjectData), &rd.objectData, GL_DYNAMIC_DRAW);
	}

	void createFoveationTexture(int width, int height,
								float centerX, float centerY,
								std::vector<uint8_t> &data,
								bool vr_specified)
	{
		//////////// ShadingRateSample ////////////
		// 
		// Creates the data for a 'foveation' shading rate imaage. It will have a
		// high resolution at the given center and a lower rate further away from
		// the center. At the edges we also use the SHADE_NO_PIXELS_NV rate
		// which will discard the full block. This is useful for areas in HMDs
		// which won't end up on the screen anyway due to the lens distortions.
		//
		for (size_t y = 0; y < height; ++y)
		{
			for (size_t x = 0; x < width; ++x)
			{
				float fx = x / (float)width;
				float fy = y / (float)height;

				
				float d = std::sqrt((fx - centerX)*(fx - centerX) + (fy - centerY)*(fy - centerY));//到注视点的距离
				
				if(!vr_specified){
					if (d < 0.15f)
						data[x + y * width] = 1;
					else if (d < 0.35f)
						data[x + y * width] = 2;
					else 
						data[x + y * width] = 3;
				}else{
					float d0 = std::sqrt((fx - 0.5)*(fx - 0.5) + (fy - 0.5)*(fy - 0.5));//到屏幕中心的距离
					if (d0 >0.28f){//在vr可视范围之外，直接置零，不渲染
						data[x+y*width]=0;
						continue;
					}
					if (d < 0.15f)
						data[x + y * width] = 1;
					else if (d < 0.22f)
						data[x + y * width] = 2;
					else
						data[x + y * width] = 3;
				}
			}
		}
	}

	bool initTextures(Data& rd)
	{
		// allocate color and depth texture arrays for fbos
		auto& ctex = rd.tex.colorTexArray;
		nv_helpers_gl::newTexture(ctex);
	
		glBindTexture(GL_TEXTURE_2D_ARRAY, ctex);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, rd.renderWidth, rd.renderHeight, 1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

		auto& dtex = rd.tex.depthTexArray;
		nv_helpers_gl::newTexture(dtex);
		glBindTexture(GL_TEXTURE_2D_ARRAY, dtex);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, rd.renderWidth, rd.renderHeight, 1);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
#ifdef __VR_PART
		/*
		* 初始化fbo4vr_{Left|Right}的附着color texture
		*/
		//left eye
		auto& ctex4vr_Left = rd.tex.colorTex4vr_Left;
		nv_helpers_gl::newTexture(ctex4vr_Left);
		glBindTexture(GL_TEXTURE_2D, ctex4vr_Left);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2D(GL_TEXTURE_2D, 
			0,//mipmaplevel
			GL_RGBA8,//target format
			rd.renderWidth, rd.renderHeight,
			0,//legacy stuff
			GL_RGBA, GL_UNSIGNED_BYTE,//source format
			nullptr
		 );
		glBindFramebuffer(GL_FRAMEBUFFER, rd.fbo4vr_Left);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex4vr_Left, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER::fbo4vr_Left is not complete!" << std::endl;
		//right eye
		auto& ctex4vr_Right = rd.tex.colorTex4vr_Right;
		nv_helpers_gl::newTexture(ctex4vr_Right);
		glBindTexture(GL_TEXTURE_2D, ctex4vr_Right);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2D(GL_TEXTURE_2D, 
			0,//mipmaplevel
			GL_RGBA8,//target format
			rd.renderWidth, rd.renderHeight,
			0,//legacy stuff
			GL_RGBA, GL_UNSIGNED_BYTE,//source format
			nullptr
		);
		glBindFramebuffer(GL_FRAMEBUFFER, rd.fbo4vr_Right);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex4vr_Right, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER::fbo4vr_Right is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0); 
#endif
		// SRI textures
		// The shading rate image has to be immutable and of type R8UI!
		std::vector<uint8_t> &data = rd.shadingRateImageData;
		data.resize(rd.shadingRateImageWidth * rd.shadingRateImageHeight);
		nv_helpers_gl::newTexture(rd.tex.shadingRateImageEccentricityBased);
		nv_helpers_gl::newTexture(rd.tex.shadingRateImage1X1);
		nv_helpers_gl::newTexture(rd.tex.shadingRateImage2X2);
		nv_helpers_gl::newTexture(rd.tex.shadingRateImage4X4);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		//
		// The first shading rate image will have the full resolution
		// in the center and a lower rate towards the edges.
		//
		clock_t time = clock();
		//createFoveationTexture(rd.shadingRateImageWidth, rd.shadingRateImageHeight, float((sin(time)+2)/4), float((cos(time)+2)/4), data);
		createFoveationTexture(rd.shadingRateImageWidth, rd.shadingRateImageHeight, 0.5f, 0.5f, data);
		glBindTexture(GL_TEXTURE_2D, rd.tex.shadingRateImageEccentricityBased);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, rd.shadingRateImageWidth, rd.shadingRateImageHeight);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rd.shadingRateImageWidth, rd.shadingRateImageHeight, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &data[0]);

		//
		// The remaining three images have a constant value. This could also simply be
		// done by different palettes but we wanted to show how to change to a
		// completely different shading rate image here.
		//
		for (size_t y = 0; y < rd.shadingRateImageHeight; ++y)
		{
			for (size_t x = 0; x < rd.shadingRateImageWidth; ++x)
			{
				data[x + y * rd.shadingRateImageWidth] = 1;
			}
		}
		glBindTexture(GL_TEXTURE_2D, rd.tex.shadingRateImage1X1);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, rd.shadingRateImageWidth, rd.shadingRateImageHeight);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rd.shadingRateImageWidth, rd.shadingRateImageHeight, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &data[0]);

		for (size_t y = 0; y < rd.shadingRateImageHeight; ++y)
		{
			for (size_t x = 0; x < rd.shadingRateImageWidth; ++x)
			{
				data[x + y * rd.shadingRateImageWidth] = 2;
			}
		}
		glBindTexture(GL_TEXTURE_2D, rd.tex.shadingRateImage2X2);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, rd.shadingRateImageWidth, rd.shadingRateImageHeight);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rd.shadingRateImageWidth, rd.shadingRateImageHeight, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &data[0]);

		for (size_t y = 0; y < rd.shadingRateImageHeight; ++y)
		{
			for (size_t x = 0; x < rd.shadingRateImageWidth; ++x)
			{
				data[x + y * rd.shadingRateImageWidth] = 3;
			}
		}
		glBindTexture(GL_TEXTURE_2D, rd.tex.shadingRateImage4X4);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8UI, rd.shadingRateImageWidth, rd.shadingRateImageHeight);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rd.shadingRateImageWidth, rd.shadingRateImageHeight, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &data[0]);

		{ GLenum errorCode = glGetError(); assert(errorCode == GL_NO_ERROR); } // verify there are no errors during development

		glBindTexture(GL_TEXTURE_2D, 0);

		return true;
	}

	// rendering the "scene", nothing specific to our extensions here
	void renderTori(Data& rd, size_t numTori, size_t width, size_t height, mat4f view)
	{
		float num = (float)numTori;

		// bind geometry
		glBindBuffer(GL_ARRAY_BUFFER, rd.buf.vbo);
		glVertexAttribPointer(VERTEX_POS, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
		glVertexAttribPointer(VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)(rd.buf.numVertices * 3 * sizeof(float)));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rd.buf.ibo);

		glEnableVertexAttribArray(VERTEX_POS);
		glEnableVertexAttribArray(VERTEX_NORMAL);

		// distribute num tori into an numX x numY pattern
		// with numX * numY > num, numX = aspect * numY

		float aspect = (float)width / (float)height;

		size_t numX = static_cast<size_t>(ceil(sqrt(num * aspect)));
		size_t numY = static_cast<size_t>((float)numX / aspect);
		if (numX * numY < num)
		{
			++numY;
		}
		//size_t numY = static_cast<size_t>( ceil(sqrt(num / aspect)) );
		float rx = 1.0f;										 // radius of ring
		float ry = 1.0f;
		float dx = 1.0f;										 // ring distance
		float dy = 1.5f;
		float sx = (numX - 1) * dx + 2 * rx; // array size 
		float sy = (numY - 1) * dy + 2 * ry;

		float x0 = -sx / 2.0f + rx;
		float y0 = -sy / 2.0f + ry;

		float scale = std::min(1.f / sx, 1.f / sy) * 0.8f;

		size_t torusIndex = 0;
		for (size_t i = 0; i < numY && torusIndex < num; ++i)
		{
			for (size_t j = 0; j < numX && torusIndex < num; ++j)
			{
				float y = y0 + i * dy;
				float x = x0 + j * dx;
				rd.objectData.model = nv_math::scale_mat4(nv_math::vec3(scale)) * nv_math::translation_mat4(nv_math::vec3(x, y, 4.0f)) * nv_math::rotation_mat4_x((j % 2 ? -1.0f : 1.0f) * 45.0f * nv_pi / 180.0f);

				rd.objectData.modelView = view * rd.objectData.model;
				rd.objectData.modelViewIT = nv_math::transpose(nv_math::invert(rd.objectData.modelView));
				rd.objectData.modelViewProj = rd.sceneData.viewProjMatrix * rd.objectData.model;

				// Use colors light blue and green
				int colorIndex = torusIndex % 5;
				nv_math::vec3f color(0, .7f, 1);
				if (colorIndex == 4) {
					color = nv_math::vec3f(0, 1, 0);
				}
				rd.objectData.color = color;

				// set model UBO
				glNamedBufferSubDataEXT(rd.buf.objectUbo, 0, sizeof(ObjectData), &rd.objectData);
				glBindBufferBase(GL_UNIFORM_BUFFER, UBO_OBJECT, rd.buf.objectUbo);

				glDrawElements(GL_TRIANGLES, rd.buf.numIndices, GL_UNSIGNED_INT, NV_BUFFER_OFFSET(0));

				++torusIndex;
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glDisableVertexAttribArray(VERTEX_POS);
		glDisableVertexAttribArray(VERTEX_NORMAL);
	}
} //namespace render

MainApplication::MainApplication()
	: nv_helpers_gl::WindowProfiler( /*singleThreaded=*/true, /*doSwap=*/true)
	, m_frameCount(0)
{ /* NOTHING HERE*/ }

#ifdef __VR_PART
bool MainApplication::initVR(){
	// Loading the SteamVR Interface Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		std::cerr << buf << std::endl;
		return false;
	}

	// get recommended render size for vr
	m_pHMD->GetRecommendedRenderTargetSize((uint32_t*)&m_rd.renderWidth, (uint32_t*)&m_rd.renderHeight);
	std::cout << "recomended vr render size is (width, height) = (" << m_rd.renderWidth << ", " << m_rd.renderHeight << ")" << std::endl;
	
	eError = vr::VRInitError_None;
	if (!vr::VRCompositor())
	{
			std::cerr << "Compositor initialization failed. See log file for details." << std::endl;
			return false;
	}
}
#endif
bool MainApplication::initShadingRatePallete(){	//
	// The shading rate texel size is relevant to create the shading rate image at
	// the correct resolution. It is constant for a certain hardware but might change
	// with future hardware generations.
	//
	glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV, &m_rd.shadingRateImageTexelHeight);
	LOGOK("\nGL_SHADING_RATE_IMAGE_TEXEL_HEIGHT_NV = %d\n", m_rd.shadingRateImageTexelHeight);
	glGetIntegerv(GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV, &m_rd.shadingRateImageTexelWidth);
	LOGOK("GL_SHADING_RATE_IMAGE_TEXEL_WIDTH_NV = %d\n", m_rd.shadingRateImageTexelWidth);
	m_rd.updateShadingRateImageSize();

	GLint palSize;
	glGetIntegerv(GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV, &palSize);
	LOGOK("GL_SHADING_RATE_IMAGE_PALETTE_SIZE_NV = %d\n\n", palSize);

	//
	// We only set the first four entries explicitly, so check that the
	// GPU supports at least 4 entries. Actual hardware supports more.
	//
	assert(palSize >= 4);
	if(palSize < 4)//TODO: better exception information return
		return false;

	//////////// ShadingRateSample ////////////
	//
	// setting the palettes
	// The second palette is used to send geometry in the Vertex Shader 
	// to an alternative shading rate.
	//
	GLenum *palette = new GLenum[palSize];
	GLenum *paletteFullRate = new GLenum[palSize];
	GLenum *palette2x2 = new GLenum[palSize];
	GLenum *palette4x4 = new GLenum[palSize];

	palette[0] = GL_SHADING_RATE_NO_INVOCATIONS_NV;
	palette[1] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
	palette[2] = GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV;
	palette[3] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;

	// fill the rest
	for (size_t i = 4; i < palSize; ++i)
		palette[i] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
	for (size_t i = 0; i < palSize; ++i)
	{
		paletteFullRate[i] = GL_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV;
		palette2x2[i] = GL_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV;
		palette4x4[i] = GL_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV;
	}

	glShadingRateImagePaletteNV(0, 0, palSize, palette);
	glShadingRateImagePaletteNV(1, 0, palSize, paletteFullRate);
	glShadingRateImagePaletteNV(2, 0, palSize, palette2x2);
	glShadingRateImagePaletteNV(3, 0, palSize, palette4x4);

	delete[] palette;
	delete[] paletteFullRate;
	delete[] palette2x2;
	delete[] palette4x4;

	return true;
}
bool MainApplication::initTW(){
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(m_window.m_viewsize[0], m_window.m_viewsize[1]);
	
	TwEnumVal ShadingModeEV[] = {
		{ render::Tweak::RATE_DEPTH_BASED, "depth(z) based (general)" },
		{ render::Tweak::RATE_ECCENTRICITY_BASED, "eccentricity(xy) based (general)" },
		{ render::Tweak::RATE_ECCENTRICITY_BASED_VR, "eccentricity(xy) based (vr_specified)" },
		{ render::Tweak::RATE_1X1, "1x1" },
		{ render::Tweak::RATE_2X2, "2x2" },
		{ render::Tweak::RATE_4X4, "4x4" } };
	m_rd.tweak.m_ShadingModeType = TwDefineEnum("shading mode", ShadingModeEV, 6);

	TwBar *bar = TwNewBar("mainbar");
	TwDefine(" GLOBAL contained=true help='OpenGL samples.\nCopyright NVIDIA Corporation 2014-2018' ");
	TwDefine(" mainbar position='0 0' size='350 250' color='0 0 0' alpha=128 valueswidth=170 ");
	TwDefine(" mainbar label='Shading Rate Image'");
	TwAddVarRW(bar, "fragment load", TW_TYPE_UINT32, &m_rd.tweak.m_fragmentLoad, nullptr);
	TwAddVarRW(bar, "tori count", TW_TYPE_UINT16, &m_rd.tweak.m_loadFactor, nullptr);
	TwAddVarRW(bar, "torus M", TW_TYPE_UINT16, &m_rd.tweak.m_torus_m, "min=3 max=32 step=1");
	TwAddVarRW(bar, "torus N", TW_TYPE_UINT16, &m_rd.tweak.m_torus_n, "min=3 max=32 step=1");
	TwAddButton(bar, "rebuild geometry", button_rebuild_geometry, this, " label='Rebuild Geometry'");
	TwAddVarRW(bar, "pixel zoom", TW_TYPE_INT32, &m_rd.offscreenScale, "min=1 max=8 step=1");
	TwAddVarRW(bar, "visualize shading rate", TW_TYPE_BOOLCPP, &m_rd.tweak.m_visualizeShadingRate, nullptr);
	TwAddVarRW(bar, "visualize depth", TW_TYPE_BOOLCPP, &m_rd.tweak.m_visualizeDepth, nullptr);
	TwAddVarRW(bar, "full rate for green tori", TW_TYPE_BOOLCPP, &m_rd.tweak.m_fullShadingRateForGreenObjects, nullptr);
	TwAddVarRW(bar, "activate shading rate", TW_TYPE_BOOLCPP, &m_rd.tweak.m_activateShadingRate, nullptr);
	TwAddVarRW(bar, "shading mode", m_rd.tweak.m_ShadingModeType, &m_rd.tweak.m_shadingMode, NULL);

	TwAddVarRW(bar, "offset.x", TW_TYPE_FLOAT, &m_rd.tweak.m_offset.x, nullptr);
	TwAddVarRW(bar, "offset.y", TW_TYPE_FLOAT, &m_rd.tweak.m_offset.y, nullptr);
	TwAddVarRW(bar, "offset.z", TW_TYPE_FLOAT, &m_rd.tweak.m_offset.z, nullptr);
	return true;
}

#ifdef __VR_PART
void MainApplication::updateSceneData(vr::Hmd_Eye nEye){
	auto width = m_window.m_viewsize[0];
	auto height = m_window.m_viewsize[1];


	nv_math::mat4 view;
	if(nEye == vr::Eye_Left)
		view = m_mat4HMDEyePosLeft * m_mat4HMDPose;
	else
		view = m_mat4HMDEyePosRight * m_mat4HMDPose;
	auto iview = invert(view);

	/*
	auto proj = perspective(
		45.f, 
		float(width) / float(height), 
		m_rd.sceneData.projNear, m_rd.sceneData.projFar
	);
	*/
	nv_math::mat4 proj;
	if(nEye == vr::Eye_Left)
		proj = m_mat4HMDProjLeft;
	else
		proj = m_mat4HMDProjRight;

	vec3f eyePos_world = vec3f(iview(0, 3), iview(1, 3), iview(2, 3));
	vec3f eyePos_view = view * vec4f(eyePos_world, 1);

	// send scene data to shader via UBO technique
	m_rd.sceneData.viewMatrix = view;
	m_rd.sceneData.projMatrix = proj;
	m_rd.sceneData.viewProjMatrix = proj * view;
	m_rd.sceneData.lightPos_world = eyePos_world;
	m_rd.sceneData.eyepos_world = eyePos_world;
	m_rd.sceneData.eyePos_view = eyePos_view;
	m_rd.sceneData.loadFactor = m_rd.tweak.m_loadFactor;
	m_rd.sceneData.fragmentLoadFactor = m_rd.tweak.m_fragmentLoad;
	m_rd.sceneData.visualizeShadingRate = m_rd.tweak.m_visualizeShadingRate ? 1 : 0;
	//m_rd.sceneData.visualizeShadingRate = nEye==vr::Eye_Right ? 1 : 0;//截图用
	m_rd.sceneData.visualizeDepth = m_rd.tweak.m_visualizeDepth ? 1 : 0;
	m_rd.sceneData.fullShadingRateForGreenObjects = m_rd.tweak.m_fullShadingRateForGreenObjects ? 1 : 0;
	m_rd.sceneData.offset = m_rd.tweak.m_offset;

	//m_rd.sceneData.offset = m_rd.tweak.m_offset;
	// set scene UBO
	glNamedBufferSubDataEXT(m_rd.buf.sceneUbo, 0, sizeof(SceneData), &m_rd.sceneData);
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, m_rd.buf.sceneUbo);
}
void MainApplication::updateShadingRateImage(vr::Hmd_Eye nEye){
	this -> updateShadingRateImage(); 
	// 实现双眼不同SRI之前先统一用这个
}
void MainApplication::renderScene(vr::Hmd_Eye nEye){
	this -> renderScene();
	// 将fbo中渲染好的内容传送到fbo4vr中
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rd.fbo4render);
	if(nEye==vr::Eye_Left)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_rd.fbo4vr_Left);
	else
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_rd.fbo4vr_Right);

	//实验中: 提取注视点纵向depth信息,并在下一帧送给shader
	float depth;
	glReadPixels(m_rd.renderWidth*((m_FP_x+1)/2),//x坐标
                    m_rd.renderHeight*((m_FP_y+1)/2),//y坐标
                    1,1,//读取一个像素
                    GL_DEPTH_COMPONENT,//获得深度信息
                    GL_FLOAT,//数据类型为浮点型
                    &depth);//获得的深度值保存在winZ中
	//transform depth to z
	float& zNear=m_rd.sceneData.projNear;
	float& zFar=m_rd.sceneData.projFar;	
	float z = 2*zFar*zNear / (zFar + zNear - (zFar - zNear)*(2*depth -1));
	//std::cout<<"distance z of center is: "<<z<<std::endl;
	m_rd.sceneData.depth_stare = depth;

	//glNamedFramebufferTextureLayerEXT(m_rd.fbo4render, GL_COLOR_ATTACHMENT0, m_rd.tex.colorTexArray, 0, 0);
	//glNamedFramebufferTextureLayerEXT(m_rd.fbo4render, GL_DEPTH_ATTACHMENT, m_rd.tex.depthTexArray, 0, 0);
	glBlitFramebuffer(
		0, 0, m_rd.renderWidth, m_rd.renderHeight,
		0, 0, m_rd.renderWidth, m_rd.renderHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

}
void MainApplication::submit2VR(){
	// 将fbo4vr的关联texture传送给VRCompositor
	vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)m_rd.tex.colorTex4vr_Left, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);

	vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)m_rd.tex.colorTex4vr_Right, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	/*vr::EVRCompositorError submitErr = */
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	
	/*if (submitErr != vr::VRCompositorError_None) {
			std::cout << "VRCompositor() -> Submit *ERROR*:" << (int)submitErr << std::endl;
	}*/

	glFlush();	
}
#else
void MainApplication::updateSceneData(){
	m_control.processActions(
		m_window.m_viewsize,
		nv_math::vec2f(m_window.m_mouseCurrent[0], m_window.m_mouseCurrent[1]),
		m_window.m_mouseButtonFlags, 
		m_window.m_wheel
	);

	auto width = m_window.m_viewsize[0];
	auto height = m_window.m_viewsize[1];

	//
	auto view = m_control.m_viewMatrix;
	auto iview = invert(view);

	auto proj = perspective(45.f, float(width) / float(height), m_rd.sceneData.projNear, m_rd.sceneData.projFar);

	float depth = 1.0f;
	vec4f background = vec4f(118.f / 255.f, 185.f / 255.f, 0.f / 255.f, 0.f / 255.f);
	vec3f eyePos_world = vec3f(iview(0, 3), iview(1, 3), iview(2, 3));
	vec3f eyePos_view = view * vec4f(eyePos_world, 1);

	// send scene data to shader via UBO technique
	m_rd.sceneData.viewMatrix = view;
	m_rd.sceneData.projMatrix = proj;
	m_rd.sceneData.viewProjMatrix = proj * view;
	m_rd.sceneData.lightPos_world = eyePos_world;
	m_rd.sceneData.eyepos_world = eyePos_world;
	m_rd.sceneData.eyePos_view = eyePos_view;
	m_rd.sceneData.loadFactor = m_rd.tweak.m_loadFactor;
	m_rd.sceneData.fragmentLoadFactor = m_rd.tweak.m_fragmentLoad;
	m_rd.sceneData.visualizeShadingRate = m_rd.tweak.m_visualizeShadingRate ? 1 : 0;
	m_rd.sceneData.visualizeDepth = m_rd.tweak.m_visualizeDepth ? 1 : 0;
	m_rd.sceneData.fullShadingRateForGreenObjects = m_rd.tweak.m_fullShadingRateForGreenObjects ? 1 : 0;
	m_rd.sceneData.offset = m_rd.tweak.m_offset;

	// set scene UBO
	glNamedBufferSubDataEXT(m_rd.buf.sceneUbo, 0, sizeof(SceneData), &m_rd.sceneData);
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_SCENE, m_rd.buf.sceneUbo);
}


#endif
void MainApplication::updateShadingRateImage(){
	if (m_rd.tweak.m_activateShadingRate)
	{
		glEnable(GL_SHADING_RATE_IMAGE_NV);
	}else{
		glDisable(GL_SHADING_RATE_IMAGE_NV);
		return;
	}

	m_rd.sceneData.applyDepthBasedFoveatedRendering = m_rd.tweak.m_shadingMode==render::Tweak::RATE_DEPTH_BASED;

	if (m_rd.tweak.m_shadingMode == render::Tweak::RATE_ECCENTRICITY_BASED ||
		m_rd.tweak.m_shadingMode == render::Tweak::RATE_ECCENTRICITY_BASED_VR )
	{
		// Buffer to store image data in. 
		std::vector<uint8_t>& data = m_rd.shadingRateImageData;

		// Create shading rate image data. This will have a higher resolution at the point you stare at
		// and a lower rate further away, which is part of "Foveated-Rendering" technique
		render::createFoveationTexture(
			m_rd.shadingRateImageWidth, m_rd.shadingRateImageHeight, 
			(m_FP_x+1)/2, (m_FP_y+1)/2, // 得到归一化的相对位置 i.e. (x,y) in [-1,1] -> [0,1
			data,
			m_rd.tweak.m_shadingMode == render::Tweak::RATE_ECCENTRICITY_BASED_VR);

		glBindTexture(GL_TEXTURE_2D, m_rd.tex.shadingRateImageEccentricityBased);
		glTexSubImage2D(GL_TEXTURE_2D,
			0,						// #level of detial, i.e. #LoD
			0, 0,				 // xoffset, yoffset -> 0, 0 -> "Sub"特性并没有被用上
			m_rd.shadingRateImageWidth, m_rd.shadingRateImageHeight,	// 宽高，老生常谈
			GL_RED_INTEGER, GL_UNSIGNED_BYTE, //由于 data的格式是vector<uint8_t>，故使用UNSIGNED_BYTE;RED_INTEGER没搞懂
			&data[0]			// 真正的数据
		);
	} else if(m_rd.tweak.m_shadingMode == render::Tweak::RATE_DEPTH_BASED){ 
		// need do nothing here.
		// shading rate is set in VERTEX SHADER.
	}
#ifdef __HTTP_PART
	else {
		// Buffer to store image data in. 
		std::vector<uint8_t>& data = m_rd.shadingRateImageData;

		// clock_t time = clock() / 50.0f;
		// render::createFoveationTexture(m_rd.shadingRateImageWidth, m_rd.shadingRateImageHeight, float((sin(time) + 2) / 4), float((cos(time) + 2) / 4), data);

		clock_t time = clock();
		if (time - stamp > GAP) {
			float http_x, http_y;
			if (send_http( 13050, &http_x, &http_y) < 0) { http_x = 0.3; http_y = 0.3; }
			render::createFoveationTexture(m_rd.shadingRateImageWidth, m_rd.shadingRateImageHeight, http_x, http_y, data);
			stamp = time;
		}
		glBindTexture(GL_TEXTURE_2D, m_rd.tex.shadingRateImageVarying);
		glTexSubImage2D(GL_TEXTURE_2D,
			0,						// #level of detial, i.e. #LoD
			0, 0,				 // xoffset, yoffset -> 0, 0 -> "Sub"特性并没有被用上
			m_rd.shadingRateImageWidth, m_rd.shadingRateImageHeight,	// 宽高，老生常谈
			GL_RED_INTEGER, GL_UNSIGNED_BYTE, //由于 data的格式是vector<uint8_t>，故使用UNSIGNED_BYTE;RED_INTEGER没搞懂
			&data[0]			// 真正的数据
		);
	}
#endif
	/* 根据不同情形设置决定vrs的shadingRateImage*/

	if (m_rd.tweak.m_shadingMode == render::Tweak::RATE_DEPTH_BASED)
	{
		//actually needless to bind here
		glBindShadingRateImageNV(m_rd.tex.shadingRateImageEccentricityBased);
	}
	else if (m_rd.tweak.m_shadingMode == render::Tweak::RATE_ECCENTRICITY_BASED ||
			m_rd.tweak.m_shadingMode == render::Tweak::RATE_ECCENTRICITY_BASED_VR)
	{
		glBindShadingRateImageNV(m_rd.tex.shadingRateImageEccentricityBased);
	}
	else if (m_rd.tweak.m_shadingMode == render::Tweak::RATE_1X1)
	{
		glBindShadingRateImageNV(m_rd.tex.shadingRateImage1X1);
	}
	else if (m_rd.tweak.m_shadingMode == render::Tweak::RATE_2X2)
	{
		glBindShadingRateImageNV(m_rd.tex.shadingRateImage2X2);
	}
	else
	{
		glBindShadingRateImageNV(m_rd.tex.shadingRateImage4X4);
	}

	

}


void MainApplication::renderScene(){
	//profile
	float t[12];
	t[0]=sysGetTime();
	// remember: shading rate image only works offscreen, not on framebuffer 0
	glBindFramebuffer(GL_FRAMEBUFFER, m_rd.fbo4render);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_rd.tex.colorTexArray, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_rd.tex.depthTexArray, 0);

	float depth = 1.0f;
	vec4f background = vec4f(118.f / 255.f, 185.f / 255.f, 0.f / 255.f, 0.f / 255.f);
	glViewport(0, 0, m_rd.renderWidth, m_rd.renderHeight);
	glClearBufferfv(GL_COLOR, 0, &background[0]);
	glClearBufferfv(GL_DEPTH, 0, &depth);
	t[1]=sysGetTime();

	// renders shaded tori
	// 真正渲染的地方
	glUseProgram(m_rd.pm.get(m_rd.prog.sceneShader));
#ifdef __RENDER_TORUS
	renderTori(m_rd, m_rd.sceneData.loadFactor, m_rd.renderWidth, m_rd.renderHeight, m_rd.sceneData.viewMatrix);
#else
	//下面这些东西最好能统一塞到一个函数renderMesh里
	m_rd.objectData.model = nv_math::scale_mat4(nv_math::vec3(0.01f));//先设成单位矩阵

	m_rd.objectData.modelView = m_rd.sceneData.viewMatrix * m_rd.objectData.model;
	m_rd.objectData.modelViewIT = nv_math::transpose(nv_math::invert(m_rd.objectData.modelView));
	m_rd.objectData.modelViewProj = m_rd.sceneData.viewProjMatrix * m_rd.objectData.model;

	// set model UBO
	glNamedBufferSubDataEXT(m_rd.buf.objectUbo, 0, sizeof(ObjectData), &m_rd.objectData);
	glBindBufferBase(GL_UNIFORM_BUFFER, UBO_OBJECT, m_rd.buf.objectUbo);
	t[2]=sysGetTime();
	m_rd.model.Draw(m_rd.pm.get(m_rd.prog.sceneShader));
#endif
	t[3]=sysGetTime();
	//this -> drawCross(0,0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//printf("renerScene profile: (%.4f,%.4f,%.4f)\n",(t[1]-t[0])*1000,(t[2]-t[1])*1000,(t[3]-t[2])*1000);
}
void MainApplication::renderCompanionWindow(){
	// Disable the variable rate shading before rendering the window and menu.
	// (in this case it's rendered onto FB0 which isn't affected)
	//
	glViewport(0,0,m_window.m_viewsize[0],m_window.m_viewsize[1]);
	glDisable(GL_SHADING_RATE_IMAGE_NV);

	// clear the FB0 because the blit below is blitting a low resolution
	// image onto a higher resolution framebuffer ("pixel zoom" in the menu)
	// if the window size is not an integer multiple of the rendering resolution, 
	// some old framebuffer content might stay visible otherwise.
	vec4f background = vec4f(118.f / 255.f, 185.f / 255.f, 0.f / 255.f, 0.f / 255.f);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearBufferfv(GL_COLOR, 0, &background[0]);

	/*
	* lcj:
	*	这里有必要解释一下逻辑。
	*	sample里本来的逻辑是，将fbo中的内容blit到品目fbo(FB 0)中，从而将离屏渲染得到的画面放到窗口中
	*	但如果要进行vr渲染，vr渲染势必有两幅画面，而电脑上的窗口只能作为companion window。
	*	为了让companion window能够一左一右同时展示两张渲染画面，就只能用纹理贴图（而不是fbo传送），
	*	用shader和vao画左右眼渲染的两张纹理贴图上去——可能还要进行拉伸。
	*/
#ifndef __VR_PART
	// blit the rendering to FB 0 as shading rate image only works offscreen:
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rd.fbo4render);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glNamedFramebufferTextureLayerEXT(m_rd.fbo4render, GL_COLOR_ATTACHMENT0, m_rd.tex.colorTexArray, 0, 0);
	glNamedFramebufferTextureLayerEXT(m_rd.fbo4render, GL_DEPTH_ATTACHMENT, m_rd.tex.depthTexArray, 0, 0);
	glBlitFramebuffer(
		0, 0, m_rd.renderWidth, m_rd.renderHeight,
		0, 0, m_rd.renderWidth * m_rd.offscreenScale, m_rd.renderHeight * m_rd.offscreenScale, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
	// 以下往窗口上渲染
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	//glViewport( 0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight );

	glBindVertexArray( m_rd.prog.companionVAO );
	//glUseProgram( m_rd.prog.companionShader.m_ID );
	m_rd.prog.companionShader.activate();

	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, m_rd.tex.colorTex4vr_Left);
	//glBindTexture(GL_TEXTURE_2D, m_rd.tex.tex4test);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glDrawArrays(GL_TRIANGLES, 0, 6);
	

	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, m_rd.tex.colorTex4vr_Right);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glDrawArrays(GL_TRIANGLES, 6, 6);
	//右边的顶点在vert_data的后半边 | 左右各两个三角形拼成长方形，各6个顶点，共12个

	glBindVertexArray( 0 );
	glUseProgram( 0 );	
#endif
	

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


#ifdef __VR_PART
void MainApplication::updateHMDMatrices(){
	if(m_frameCount%2) return;
	updateHMDMatrixPose();
	updateHMDMatrixPosEye(vr::Eye_Left);
	updateHMDMatrixPosEye(vr::Eye_Right);
	updateHMDMatrixProjectionEye(vr::Eye_Left);
	updateHMDMatrixProjectionEye(vr::Eye_Right);
	
}
void MainApplication::updateHMDMatrixPose(){
	if ( !m_pHMD )
		return;

	
	vr::VRCompositor()->WaitGetPoses(
		m_rTrackedDevicePose, 			// array pointer and its (max) length
		vr::k_unMaxTrackedDeviceCount, 
		NULL, 0 // another array pointer and its length, unnecessary here
	);

	m_uValidPoseCount = 0;
	for ( int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice )
	{
		if ( m_rTrackedDevicePose[nDevice].bPoseIsValid )
		{
			m_uValidPoseCount++;
			m_mat4DevicePose[nDevice] = steamVRMat2mat4( m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking );
			if (m_cDeviceType[nDevice]==0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_cDeviceType[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_cDeviceType[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_cDeviceType[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_cDeviceType[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_cDeviceType[nDevice] = 'T'; break;
				default:                                       m_cDeviceType[nDevice] = '?'; break;
				}
			}
			//m_strPoseClasses += m_rDevClassChar[nDevice]; //不知道干什么用的
		}
	}

	// extract HMD's pose matrix
	if ( m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
	{
		m_mat4HMDPose = m_mat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose = invert(m_mat4HMDPose);
	}
}
mat4 MainApplication::steamVRMat2mat4( const vr::HmdMatrix34_t &matPose ){
	mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0f,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0f,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0f,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}
void MainApplication::updateHMDMatrixPosEye( vr::Hmd_Eye nEye )
{
	if ( !m_pHMD ){ 
		if(nEye == vr::Eye_Left) m_mat4HMDEyePosLeft = mat4(); 
		if(nEye == vr::Eye_Right) m_mat4HMDEyePosRight = mat4(); 
		return; 
	}

	vr::HmdMatrix34_t mat = m_pHMD->GetEyeToHeadTransform( nEye );
	mat4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	);

	//if(nEye == vr::Eye_Left) m_mat4HMDEyePosLeft = invert(matrixObj); 
	//if(nEye == vr::Eye_Right) m_mat4HMDEyePosRight = invert(matrixObj);
	if(nEye == vr::Eye_Left) m_mat4HMDEyePosLeft = matrixObj; 
	if(nEye == vr::Eye_Right) m_mat4HMDEyePosRight = matrixObj;
}
void MainApplication::updateHMDMatrixProjectionEye( vr::Hmd_Eye nEye )
{
	if ( !m_pHMD ){ 
		if(nEye == vr::Eye_Left) m_mat4HMDProjLeft = mat4(); 
		if(nEye == vr::Eye_Right) m_mat4HMDProjRight = mat4(); 
		return; 
	}

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix( 
		nEye, m_rd.sceneData.projNear, m_rd.sceneData.projFar );

	mat4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], 
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);

	if(nEye == vr::Eye_Left) m_mat4HMDProjLeft = matrixObj; 
	if(nEye == vr::Eye_Right) m_mat4HMDProjRight = matrixObj;
}
#endif

void TW_CALL button_rebuild_geometry(void* c)
{
	MainApplication* s = reinterpret_cast<MainApplication*>(c);
	s->rebuild_geometry();
}
void MainApplication::drawCross(float rx, float ry){//现在画不出来，我也不知道为什么
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SHADING_RATE_IMAGE_NV);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glBegin(GL_LINES);
		glColor3f(1.,1.,1.);
		glVertex2f(rx-0.1f,ry);
		glVertex2f(rx+0.1f,ry);
		glVertex2f(rx,ry-0.1f);
		glVertex2f(rx,ry+0.1f);
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_SHADING_RATE_IMAGE_NV);
}

void MainApplication::predictGazePos(){
	//预测注视点位置
	if(m_frameCount%2) return;//每两帧预测一次，与updateHMDmatrix保持一致；否则会出现闪烁。

	//naive算法
	double t=sysGetTime();
	float dt = t - m_tprev;
	m_tprev = t;
	vec3 forward(m_rd.sceneData.viewMatrix.a20,
				m_rd.sceneData.viewMatrix.a21,
				m_rd.sceneData.viewMatrix.a22);
	vec3 up(m_rd.sceneData.viewMatrix.a10,
			m_rd.sceneData.viewMatrix.a11,
			m_rd.sceneData.viewMatrix.a12);
	vec3 right(m_rd.sceneData.viewMatrix.a00,
			m_rd.sceneData.viewMatrix.a01,
			m_rd.sceneData.viewMatrix.a02);
	forward.normalize();
	up.normalize();
	right.normalize();

	vec3 dFwd = m_vec3FwdPrev-forward;
	m_vec3FwdPrev = forward;

	float C=0.23;//常系数
	float smooth_coef = 0.7;//加上一点上一帧的自己，让变化缓和一些
	m_FP_x = smooth_coef*m_FP_x + (1-smooth_coef)*C*dot(right, (dFwd/dt));
	m_FP_y = smooth_coef*m_FP_y + (1-smooth_coef)*C*dot(up, (dFwd/dt));
	//debug output
	//printf("FP coord = ( %.2f, %.2f )\n",m_FP_x, m_FP_y);
}