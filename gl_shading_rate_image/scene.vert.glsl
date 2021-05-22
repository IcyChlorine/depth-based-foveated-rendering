#version 450

#extension GL_ARB_shading_language_include : enable
#extension GL_NV_viewport_array2: require

#include "common.h"

// inputs in model space
//in layout(location=VERTEX_POS)		vec3 vertex_pos_model;
in layout(location=0) vec3 vertex_pos_model;
in layout(location=VERTEX_NORMAL) vec3 normal;
in layout(location=2) vec2 texCoord_;

layout(location=OFFSET_LOC) uniform float offset;

// outputs in view space
out Interpolants {
	centroid vec3 model_pos;
	centroid vec3 normal;
	centroid vec3 eyeDir;
	centroid vec3 lightDir;
	vec2 texCoord;
} OUT;


float LinearizeDepth(float depth) 
{
	float zNear=0.1f;
	float zFar=10.0f;
	//float z_n = depth * 2.0 - 1.0; // back to NDC 
	//return (2.0 * near * far) / (far + near - z * (far - near));		
	return 2*zFar*zNear / (zFar + zNear - (zFar - zNear)*(2*depth -1));
}

void main()
{
	// proj space calculations
	//vec4 proj_pos = object.modelViewProj * vec4( vertex_pos_model + scene.offset, 1 );
	/* !!! 由于英伟达的库给出的投影矩阵形式与标准形式不一样，这里需要手动修改，否则无法算出正确的线性深度 !!! */
	mat4 my_proj = scene.projMatrix;
	my_proj[3].z = -2*scene.projFar*scene.projNear / (scene.projFar - scene.projNear);
	vec4 pos_ = object.modelView * vec4(vertex_pos_model + scene.offset, 1);
	vec4 proj_pos = my_proj * pos_;
	gl_Position	 = proj_pos + vec4(offset, 0,0,0);

	gl_Layer = 0;

	// view space calculations
	vec3 pos		= (object.modelView	 * vec4(vertex_pos_model,1)).xyz;
	vec3 lightPos	= (scene.viewMatrix	 * vec4(scene.lightPos_world,1)).xyz;
	OUT.normal		= ( (object.modelViewIT * vec4(normal,0)).xyz );
	OUT.eyeDir		= (scene.eyePos_view - pos);
	OUT.lightDir	= (lightPos - pos);
	OUT.model_pos	= vertex_pos_model;
	OUT.texCoord	= texCoord_;//simply passing it


	if(scene.applyDepthBasedFoveatedRendering>0){
		// 人眼聚焦的焦平面的z距离
		float z_stare = LinearizeDepth(scene.depth_stare);
		float z = -pos_.z;
		/*	
			float D_pupil=4;//参数：瞳孔大小，单位为mm
			float delta=0.3;//参数：人眼能分辨的最小角度，单位为mRad，数值为1角分左右
			float A_R=13.3;//=D_pupil/delta
			
		*/
		float A_R=25;//基于OOI和认知机制，可以考虑更激进的C
		float factor = abs(1/z-1/z_stare)*A_R;
		if (factor>4)//4x4像素块
			gl_ViewportMask[0] = 8;
		else if (factor>2)//2x2像素块
			gl_ViewportMask[0] = 4;
		else //1x1像素块
			gl_ViewportMask[0] = 2;
	}else{
		gl_ViewportMask[0]=1;
	}
}