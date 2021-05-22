#version 460

#extension GL_ARB_shading_language_include : enable

//////////// ShadingRateSample ////////////
//
// The extension needs to get enabled in the Fragment Shader
//
#extension GL_NV_shading_rate_image : enable

#include "common.h"
#include "noise.glsl"

// inputs in view space
in Interpolants {
	centroid vec3 model_pos;
	centroid vec3 normal;
	centroid vec3 eyeDir;
	centroid vec3 lightDir;
	vec2 texCoord;
} IN;

layout(location=0, index=0) out vec4 out_Color;
uniform sampler2D texture_diffuse1;

float calcNoise(vec3 modelPos, int iterations)
{	
	float val = 0;
	for ( int i = 0; i < iterations; ++i )
	{
		val += SimplexPerlin3D(modelPos*20) / iterations;
	}
	val = smoothstep(-0.1, 0.1, val);
	return val;
}

vec4 calculateLight(vec3 normal, vec3 eyeDir, vec3 lightDir, vec3 objColor) 
{
	// ambient term
	vec4 ambient_color = vec4( objColor * 0.15, 1.0 );
	
	// diffuse term
	float diffuse_intensity = max(dot(normal, lightDir), 0.0);
	vec4 diffuse_color = diffuse_intensity * vec4(objColor, 1.0);
	
	// specular term
	vec3 R = reflect( -lightDir, normal );
	float specular_intensity = max( dot( eyeDir, R ), 0.0 );
	vec4 specular_color = pow(specular_intensity, 4) * vec4(0.8,0.8,0.8,1);
	
	return ambient_color + diffuse_color + specular_color;
}


float LinearizeDepth(float depth) 
{
	float zNear=scene.projNear;//这样更robust一点
	float zFar=scene.projFar;
	//float z_n = depth * 2.0 - 1.0; // back to NDC 
	//return (2.0 * near * far) / (far + near - z * (far - near));		
	return 2*zFar*zNear / (zFar + zNear - (zFar - zNear)*(2*depth -1));
}

void main()
{
	// interpolated inputs in view space
	vec3 normal	 = normalize(IN.normal);
	vec3 eyeDir	 = normalize(IN.eyeDir);
	vec3 lightDir = normalize(IN.lightDir);

	//float noiseVal = calcNoise(IN.model_pos, scene.fragmentLoadFactor * 100);
	//vec3 objColor = object.color * (1 - noiseVal * 0.3f);
	out_Color = calculateLight(normal, eyeDir, lightDir, texture(texture_diffuse1, IN.texCoord).xyz);
		
	//可视化shading rate
	if (scene.visualizeShadingRate == 1)
	{
		//out_Color = vec4(1,0,0,1);
		int maxCoarse = max( gl_FragmentSizeNV.x, gl_FragmentSizeNV.y );
		vec4 shadingColor;
		if (maxCoarse == 1)
			shadingColor = vec4(1,0,0,1);
		else if (maxCoarse == 2)
			shadingColor = vec4(1,1,0,1);
		else if (maxCoarse == 4)
			shadingColor = vec4(0,1,0,1);
		else
			shadingColor = vec4(1,1,1,1);
		out_Color = mix(out_Color, shadingColor, 0.7);
		//out_Color = shadingColor;
	}
	
	//可视化深度缓冲
	if(scene.visualizeDepth == 1){
		float z = LinearizeDepth(gl_FragCoord.z); // 线性化深度；为了演示除以 far
		z = z/scene.projFar;
		out_Color = vec4(vec3(z), 1.0);
	}
}