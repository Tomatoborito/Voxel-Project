#include "computepipeline.h"

void Computepipeline::onCompute(Device device, uint32_t workgroupsx, uint32_t workgroupsy, uint32_t workgroupsz, Buffers& buffers){
        Queue queue = device.GetQueue();
        CommandEncoderDescriptor encoderDesc = {};
        CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);
        ComputePassDescriptor computePassDesc;
        computePassDesc.timestampWrites = nullptr;
        ComputePassEncoder computePass = encoder.BeginComputePass(&computePassDesc);

        computePass.SetPipeline(computePipeline);
        computePass.SetBindGroup(0, bindGroup, 0, nullptr);
        computePass.DispatchWorkgroups(workgroupsx, workgroupsy, workgroupsz);
        
        computePass.End();

        CommandBufferDescriptor cmdbufferdesc = {};
        CommandBuffer commands = encoder.Finish(&cmdbufferdesc);
        queue.Submit(1, &commands);
     
}

void Computepipeline::initcomputepipeline(Device device, Buffers& buffers, const std::string& shaderfile, std::vector<int> whichbuffers) {

    initcomputeBindgrouplayout(device, whichbuffers);
    PipelineLayoutDescriptor pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayoutCount = 1;
    pipelineLayoutDesc.bindGroupLayouts = (wgpu::BindGroupLayout*)&bindGroupLayout;
    pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);


    ShaderModuleDescriptor shaderDesc;
    std::string shaderCode = readFile(shaderfile); //"E:/.codingshit/newcppshit/files/shaders/gencomputeshader.wgsl"

    ShaderSourceWGSL shaderCodeDesc;
    shaderCodeDesc.nextInChain = nullptr;
    shaderDesc.nextInChain = &shaderCodeDesc;
    shaderCodeDesc.code = shaderCode.c_str();
    computeShaderModule = device.CreateShaderModule(&shaderDesc);

    computeShaderModule.GetCompilationInfo(
        wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::CompilationInfoRequestStatus status, const wgpu::CompilationInfo* info) { // Wichtig: Pointer!
            if (status != wgpu::CompilationInfoRequestStatus::Success || !info) {
                std::cerr << "Shader error" << std::endl;
                return;
            }

            for (uint32_t i = 0; i < info->messageCount; ++i) {
                const wgpu::CompilationMessage& msg = info->messages[i];

                const char* typeStr = "[INFO]";
                if (msg.type == wgpu::CompilationMessageType::Error) typeStr = "[ERROR]";
                else if (msg.type == wgpu::CompilationMessageType::Warning) typeStr = "[WARNING]";

                std::cout << typeStr << " Line " << msg.lineNum << ": "
                    << std::string_view(msg.message.data, msg.message.length) << std::endl;
            }
        }
    );
    
    ComputePipelineDescriptor computePipelineDesc = {};
    computePipelineDesc.compute.entryPoint = "computeStuff";
    computePipelineDesc.compute.module = computeShaderModule;

    computePipelineDesc.layout = pipelineLayout;

    computePipeline = device.CreateComputePipeline(&computePipelineDesc); 
    //initcomputeBuffers(device, buffers, whichbuffers);
    initcomputeBindgroup(device, buffers, whichbuffers);
}

void Computepipeline::initcomputeBindgrouplayout(Device device, std::vector<int> whichbuffers) {

    const int size = whichbuffers.size();
    std::vector<BindGroupLayoutEntry> bindingLayout(size);

    for (int i = 0; i < whichbuffers.size(); i++) {
        switch (whichbuffers[i]) {
        case 1:
            bindingLayout[i].binding = i;
            bindingLayout[i].visibility = ShaderStage::Compute;
            bindingLayout[i].buffer.type = BufferBindingType::ReadOnlyStorage;
            bindingLayout[i].buffer.minBindingSize = sizeof(glm::ivec4);
            break;
     

        default:
            break;
        }

    }
    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = size;
    bindGroupLayoutDesc.entries = bindingLayout.data();;
    bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);
}

void Computepipeline::initcomputeBindgroup(Device device, Buffers& buffers, std::vector<int> whichbuffers) {

    const int size = whichbuffers.size();
    std::vector<BindGroupEntry> binding(size);

    for (int i = 0; i < whichbuffers.size(); i++) {
        switch (whichbuffers[i]) {
        case 1:
            binding[i].binding = i;
            binding[i].buffer = buffers.VoxeldataBuffer;
            binding[i].offset = 0;
            binding[i].size = sizeof(glm::ivec4) * 10000;
            break;
    
        }
    }
    

    BindGroupDescriptor bindGroupDesc{};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = size;
    bindGroupDesc.entries = binding.data();
    bindGroup = device.CreateBindGroup(&bindGroupDesc);
}
