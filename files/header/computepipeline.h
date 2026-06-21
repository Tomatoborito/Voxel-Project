#pragma once
#include <dawn/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/noise.hpp>

#include "helpers.hpp"

using namespace wgpu;

class Computepipeline {
public:

	struct Buffers {
		Buffer VoxeldataBuffer;
	};

	void onCompute(Device device, uint32_t workgroupsx, uint32_t workgroupsy, uint32_t workgroupsz, Buffers& buffers);
	void initcomputepipeline(Device device, Buffers& buffers, const std::string& shaderfile, std::vector<int> whichbuffers);
	void initcomputeBindgrouplayout(Device device, std::vector<int> whichbuffers);
	void initcomputeBindgroup(Device device, Buffers& bufferes, std::vector<int> whichbuffers);

	BindGroup bindGroup;
	BindGroupLayout bindGroupLayout;
	ComputePipeline computePipeline;
	PipelineLayout pipelinelayout;

	ShaderModule computeShaderModule;
	PipelineLayout pipelineLayout;

};