#pragma once
#include <dawn/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include "webgpu/webgpu_glfw.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/noise.hpp>
#include <cstddef>
#include <iostream>
#include <filesystem>
#include "common.h"

#include "helpers.hpp"
#include "appcontext.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_wgpu.h"

using namespace wgpu;

class Render {
public:
	struct MyUniforms {
		glm::mat4x4 viewprojectionmodelMatrix;
		glm::vec4 color;
		glm::vec3 camerapos;
		float time;
		float _pad[3];
	};

	struct MatrixUniforms {
		glm::mat4x4 projectionMatrix;
		glm::mat4x4 viewMatrix;
		glm::mat4x4 modelMatrix;
	};

	void onInit();
	bool prepareFrame();
	void computeframe();
	void beginrenderpass();
	void onFrameend();
	void onFinish();

	void initSwapchain();
	void initRenderpipeline();
	void initBuffers();
	void initBindGroupLayouts();
	void initBindGroups();
	void initDepthBuffer();

	Device requestDeviceSync(Instance instance, Adapter adapter, DeviceDescriptor const* descriptor);
	Adapter requestAdapterSync(Instance syncinstance, RequestAdapterOptions const* options);
	Limits GetRequiredLimits(Adapter adapter);
	TextureView GetNextSurfaceViewData();
	void updateProjectionMatrix();
	void onResize();

	bool isrunning();
public:
	const float PI = 3.14159265358979323846f;

	int width = 800;
	int height = 800;

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;

	float focalLength = 2.0;
	float farplane = 10000.0f;
	float nearplane = 0.01f;
	float fov = 2 * glm::atan(1 / focalLength);

	RenderPassEncoder renderPass;
	TextureView targetView;

	int frameCount = 0;
	int fpsFrames = 0;
	float timeAccumulator = 0.0f;

	GLFWwindow* window = nullptr;
	Device device;
	Queue queue;
	Adapter adapter;
	Surface surface;
	Instance instance;
	CommandEncoder encoder;
	TextureFormat surfaceFormat = TextureFormat::Undefined;
	BindGroup computeBindGroup;
	BindGroup renderBindGroup;
	RenderPipeline pipeline;
	PipelineLayout renderLayout;
	PipelineLayout computeLayout;
	MyUniforms uniforms = {};
	MatrixUniforms matuniforms = {};

	Buffer VertexBuffer;
	Buffer uniformBuffer;
	Buffer IndirectBuffer;
	Buffer IndexBuffer;
	Buffer positionBuffer;
	Buffer StagingIndirectBuffer;
	Buffer StagingpositionBuffer;
	Buffer StagingVertexBuffer;

	Texture depthTexture;
	TextureView depthTextureView;
	DepthStencilState depthStencilState;

	FragmentState fragmentState;
	BlendState blendState;
	ColorTargetState colorTarget;

	BindGroupLayout renderBindGroupLayout;
	BindGroupLayout computeBindGroupLayout;
	VertexBufferLayout vertexBufferLayout;
	std::vector<VertexAttribute> vertexAttribs;
	RenderPipelineDescriptor pipelineDesc;

	wgpu::Texture msaaTexture;
	wgpu::TextureView msaaTextureView;

	ComputePipeline computepipeline;
	ComputePassEncoder computepass;

	void initcomputepipeline();

	bool mapstagingbuffer();
	void unmapandcopy();
	void prepareimgui();

	bool ismapped = false;
	DrawIndirectArgs* indirectdata = nullptr;
	shaderdata* vertexdata = nullptr;
	positions* positiondata = nullptr;

	int chunkcount = 0;
	bool cull = true;
	Buffer DrawCountBuffer;
};