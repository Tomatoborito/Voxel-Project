#include "render.h"

void Render::onInit() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	//GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	//const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	//
	//glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
	//glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	//glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	//glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	//glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

	//window = glfwCreateWindow(mode->width, mode->height, "du opfer", monitor, nullptr);
	window = glfwCreateWindow(width, height, "du opfer", nullptr, nullptr);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	InstanceDescriptor desc = {};
	desc.nextInChain = nullptr;
	instance = CreateInstance(&desc);

	surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

	RequestAdapterOptions adapterOpts = { };
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;
	//adapterOpts.backendType = BackendType::Vulkan;
	adapterOpts.powerPreference = wgpu::PowerPreference::HighPerformance;
	adapter = requestAdapterSync(instance, &adapterOpts);

	
	DeviceDescriptor deviceDesc = {};

	deviceDesc.SetUncapturedErrorCallback([](const wgpu::Device& device, wgpu::ErrorType type, wgpu::StringView message) {
		std::cout << "Device error: " << message.data << std::endl;
		});

	const char* allowUnsafeToggles[] = { "allow_unsafe_apis" };

	wgpu::DawnTogglesDescriptor togglesDesc{};
	togglesDesc.nextInChain = nullptr;
	togglesDesc.enabledToggles = allowUnsafeToggles;
	togglesDesc.enabledToggleCount = 1;

	deviceDesc.nextInChain = togglesDesc.nextInChain;

	deviceDesc.label = "My Device";
	deviceDesc.requiredFeatureCount = 3;

	wgpu::FeatureName requiredFeature[] = { wgpu::FeatureName::TimestampQuery, wgpu::FeatureName::MultiDrawIndirect, wgpu::FeatureName::Depth32FloatStencil8 };
	deviceDesc.requiredFeatures = requiredFeature;
	Limits requiredLimits = GetRequiredLimits(adapter);
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	deviceDesc.nextInChain = &togglesDesc;

	device = requestDeviceSync(instance, adapter, &deviceDesc);

	queue = device.GetQueue();

	initSwapchain();
	initDepthBuffer();

	wgpu::SurfaceCapabilities capabilities;
	surface.GetCapabilities(adapter, &capabilities);
	initRenderpipeline();
	initcomputepipeline();

	initBuffers();

	using glm::vec3, glm::mat4x4; {

		float angle1 = (float)glfwGetTime();

		float angle2 = 3.0 * PI / 4.0;
		vec3 focalPoint(0.0, 0.0, -2.0);


		glm::mat4x4 S = glm::scale(mat4x4(1.0), vec3(1.0f));
		glm::mat4x4 T1 = glm::translate(mat4x4(1.0), vec3(0.0, 0.0, 0.0));
		glm::mat4x4 R3 = glm::rotate(mat4x4(1.0), glm::radians(-90.0f), vec3(1.0, 0.0, 0.0));
		glm::mat4x4 R1 = glm::rotate(mat4x4(1.0), angle1, vec3(1.0, 1.0, 1.0));

		//	uniforms.modelMatrix = R1 * T1 * S * R3;

			//	glm::mat4x4 R2 = glm::rotate(mat4x4(1.0), -angle2, vec3(1.0, 0.0, 0.0));
				// In InitializeBuffers:
		glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, -2.0f); // Etwas erhöht und zurückgesetzt
		glm::vec3 target = glm::vec3(32.0f, 32.0f, 32.0f);  // Blick auf den Ursprung
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // Oben ist Y

		matuniforms.viewMatrix = glm::lookAt(cameraPos, target, up);
		//glm::mat4x4 T2 = glm::translate(mat4x4(1.0), -focalPoint);
		//uniforms.viewMatrix = T2 * R2;

		float ratio = (float)width / (float)height;

		float f = 1.0f / tan(fov / 2.0f);
		mat4x4 projection = mat4x4(0.0f);

		projection[0][0] = f / ratio;
		projection[1][1] = f;
		projection[2][2] = 0.0f;
		projection[2][3] = -1.0f;
		projection[3][2] = nearplane;
		matuniforms.projectionMatrix = projection;
	}

	uniforms.viewprojectionmodelMatrix = matuniforms.projectionMatrix * matuniforms.viewMatrix * matuniforms.modelMatrix;
	queue.WriteBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

	initBindGroups();




	// 1. ImGui Kontext erstellen
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark(); // Oder StyleColorsLight();

	// 2. GLFW Backend initialisieren
	ImGui_ImplGlfw_InitForOther(window, true);

	
	// 3. WebGPU Backend initialisieren
	ImGui_ImplWGPU_InitInfo init_info = {};
	init_info.Device = device.Get();
	init_info.NumFramesInFlight = 3; // Meistens 3 bei WebGPU (Triple Buffering)
	init_info.RenderTargetFormat = (WGPUTextureFormat)surfaceFormat; // Z.B. WGPUTextureFormat_BGRA8Unorm
	init_info.DepthStencilFormat = WGPUTextureFormat_Undefined; // Falls du kein Depth-Buffer nutzt

	ImGui_ImplWGPU_Init(&init_info);



	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int, int) {
		auto instance = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
		if (instance && instance->render) {
			instance->render->onResize();
		}
		});

}

void Render::prepareimgui(){
	float currentFrame = static_cast<float>(glfwGetTime());
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	glfwPollEvents();
	timeAccumulator += deltaTime;
	fpsFrames++;

	ImGui_ImplWGPU_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

bool Render::prepareFrame(){
	// ---- 3. IMGUI RENDERN VORBEREITEN ----
	ImGui::Render();

	targetView = GetNextSurfaceViewData();

	if(!targetView){
		device.Tick();
		return false;
	}

	CommandEncoderDescriptor encoderDesc = {};
	encoderDesc.nextInChain = nullptr;
	encoderDesc.label = "My command encoder";

	encoder = device.CreateCommandEncoder(&encoderDesc);
	
	uniforms.viewprojectionmodelMatrix = matuniforms.projectionMatrix * matuniforms.viewMatrix * matuniforms.modelMatrix;
	queue.WriteBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));
	return true;
}

void Render::computeframe(){
	// ------------------------------------------------------------------
	// 1. COMPUTE PASS (Wird zuerst ausgeführt, z.B. für Culling)
	// ------------------------------------------------------------------
	ComputePassDescriptor computePassDesc = {};

	if(chunkcount > 0){
		ComputePassDescriptor computePassDesc = {};
		computepass = encoder.BeginComputePass(&computePassDesc);
		computepass.SetPipeline(computepipeline);
		computepass.SetBindGroup(0, computeBindGroup, 0, nullptr);

		uint32_t workgroupCount = (chunkcount + 63) / 64;
		
		if(cull){
			computepass.DispatchWorkgroups(workgroupCount, 1, 1);
		}

		computepass.End();
	}
}

void Render::beginrenderpass(){

	// ------------------------------------------------------------------
	// 2. RENDER PASS (Zeichnet das Ergebnis auf den Bildschirm)
	// ------------------------------------------------------------------
	RenderPassDescriptor renderPassDesc = {};
	RenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = msaaTextureView;
	renderPassColorAttachment.resolveTarget = targetView;
	renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Discard;
	renderPassColorAttachment.clearValue = wgpu::Color{ 0.1, 0.7, 1.0, 1.0 };
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

	renderPassDesc.nextInChain = nullptr;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = depthTextureView;
	depthStencilAttachment.depthClearValue = 0.0f;
	depthStencilAttachment.depthLoadOp = LoadOp::Clear;
	depthStencilAttachment.depthStoreOp = StoreOp::Store;
	depthStencilAttachment.depthReadOnly = false;

	depthStencilAttachment.stencilClearValue = 0;
	depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
	depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
	depthStencilAttachment.stencilReadOnly = true;
	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

	renderPass = encoder.BeginRenderPass(&renderPassDesc);
	renderPass.SetViewport(0, 0, width, height, 0.0f, 1.0f);
	renderPass.SetPipeline(pipeline);
	renderPass.SetBindGroup(0, renderBindGroup, 0, nullptr);
	renderPass.SetIndexBuffer(IndexBuffer, IndexFormat::Uint32, 0, 4 * sizeof(uint32_t));
	// Normalerweise würde hier in deiner Game-Loop noch ein renderPass.Draw(...) 
	// oder renderPass.DrawIndexed(...) folgen, das kannst du machen, bevor du renderPass.End() rufst.
}

void Render::onFrameend(){
	renderPass.End();


	// ------------------------------------------------------------------
	// 2. NEUER IMGUI RENDER PASS (Zeichnet über das fertige 3D Bild)
	// ------------------------------------------------------------------
	RenderPassColorAttachment imguiColorAttachment = {};
	imguiColorAttachment.view = targetView; // Direkt in den Swapchain schreiben!
	imguiColorAttachment.loadOp = wgpu::LoadOp::Load; // WICHTIG: Bild behalten, nicht clearen!
	imguiColorAttachment.storeOp = wgpu::StoreOp::Store;
	imguiColorAttachment.clearValue = wgpu::Color{ 0.0, 0.0, 0.0, 1.0 }; // Wird durch Load ignoriert

	RenderPassDescriptor imguiPassDesc = {};
	imguiPassDesc.nextInChain = nullptr;
	imguiPassDesc.colorAttachmentCount = 1;
	imguiPassDesc.colorAttachments = &imguiColorAttachment;
	imguiPassDesc.depthStencilAttachment = nullptr; // Kein Depth Buffer für UI

	// UI zeichnen
	wgpu::RenderPassEncoder imguiPass = encoder.BeginRenderPass(&imguiPassDesc);
	ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), imguiPass.Get());
	imguiPass.End();
	// ------------------------------------------------------------------



	CommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.nextInChain = nullptr;
	cmdBufferDescriptor.label = "Command buffer";
	CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);

	queue.Submit(1, &command);

	if(timeAccumulator >= 0.5f){
		float fps = fpsFrames / timeAccumulator;
		glfwSetWindowTitle(window, (std::to_string(fps) + " FPS").c_str());

		fpsFrames = 0;
		timeAccumulator = 0.0f;
	}

	surface.Present();
	device.Tick();

	frameCount++;
}

void Render::onFinish()
{
	glfwDestroyWindow(window);
	glfwTerminate();

	ImGui_ImplWGPU_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Render::initSwapchain() {
	glfwGetFramebufferSize(window, &width, &height);

	SurfaceConfiguration config = {};
	config.nextInChain = nullptr;

	config.width = width;
	config.height = height;
	config.usage = wgpu::TextureUsage::RenderAttachment;

	wgpu::SurfaceCapabilities capabilities;
	surface.GetCapabilities(adapter, &capabilities);
	surfaceFormat = capabilities.formats[0];
	config.format = surfaceFormat;

	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	config.device = device;
	config.presentMode = wgpu::PresentMode::Mailbox;
	config.alphaMode = wgpu::CompositeAlphaMode::Auto;

	surface.Configure(&config);

	wgpu::TextureDescriptor msaaDesc = {};
	msaaDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
	msaaDesc.sampleCount = 4;
	msaaDesc.format = surfaceFormat;
	msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
	msaaDesc.dimension = wgpu::TextureDimension::e2D;
	msaaDesc.mipLevelCount = 1;

	msaaTexture = device.CreateTexture(&msaaDesc);
	msaaTextureView = msaaTexture.CreateView();
}

void Render::initRenderpipeline() {
	ShaderModuleDescriptor shaderDesc;

	std::string shaderCode = readFile((std::filesystem::path(SOURCE_DIR) / "files" / "shader" / "shader.wgsl").string());

	ShaderSourceWGSL shaderCodeDesc;
	shaderCodeDesc.nextInChain = nullptr;
	shaderDesc.nextInChain = &shaderCodeDesc;
	shaderCodeDesc.code = shaderCode.c_str();
	ShaderModule shaderModule = device.CreateShaderModule(&shaderDesc);
	
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	pipelineDesc.primitive.topology = PrimitiveTopology::TriangleStrip;
	pipelineDesc.primitive.stripIndexFormat = IndexFormat::Uint32;
	pipelineDesc.primitive.frontFace = FrontFace::CCW;
	pipelineDesc.primitive.cullMode = CullMode::Back;

	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	blendState.color.srcFactor = BlendFactor::SrcAlpha;
	blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
	blendState.color.operation = BlendOperation::Add;
	blendState.alpha.srcFactor = BlendFactor::Zero;
	blendState.alpha.dstFactor = BlendFactor::One;
	blendState.alpha.operation = BlendOperation::Add;

	colorTarget.format = surfaceFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = ColorWriteMask::All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	pipelineDesc.fragment = &fragmentState;

	pipelineDesc.multisample.count = 4;
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = true;

	pipelineDesc.depthStencil = &depthStencilState;

	initBindGroupLayouts();

	pipelineDesc.layout = renderLayout;

	pipeline = device.CreateRenderPipeline(&pipelineDesc);


}

void Render::initcomputepipeline(){
	ShaderModuleDescriptor shaderDesc;

	std::string shaderCode = readFile((std::filesystem::path(SOURCE_DIR) / "files" / "shader" / "culling.wgsl").string());

	ShaderSourceWGSL shaderCodeDesc;
	shaderCodeDesc.nextInChain = nullptr;
	shaderDesc.nextInChain = &shaderCodeDesc;
	shaderCodeDesc.code = shaderCode.c_str();
	ShaderModule shaderModule = device.CreateShaderModule(&shaderDesc);

	// Compute Pipelines verwenden einen eigenen Descriptor (keine Vertex/Fragment-States!)
	ComputePipelineDescriptor computePipelineDesc = {};
	computePipelineDesc.layout = computeLayout;
	computePipelineDesc.compute.module = shaderModule;

	// WICHTIG: Schau in deiner culling.wgsl nach, wie die @compute Funktion heißt.
	// Häufig nennt man sie "cs_main" für Compute. Wenn sie bei dir "vs_main" heißt, 
	// solltest du das im Shader und hier anpassen.
	computePipelineDesc.compute.entryPoint = "cs_main";

	computepipeline = device.CreateComputePipeline(&computePipelineDesc);
}

void Render::initBindGroupLayouts(){
	// --- LAYOUT FÜR RENDERING ---
	BindGroupLayoutEntry renderEntries[2] = {};
	// Binding 0: Uniforms
	renderEntries[0].binding = 0;
	renderEntries[0].visibility = ShaderStage::Vertex | ShaderStage::Fragment;
	renderEntries[0].buffer.type = BufferBindingType::Uniform;
	renderEntries[0].buffer.minBindingSize = sizeof(MyUniforms);

	// Binding 1: Vertex Data (Hier reicht ReadOnly für den Render-Pass!)
	renderEntries[1].binding = 1;
	renderEntries[1].visibility = ShaderStage::Vertex;
	renderEntries[1].buffer.type = BufferBindingType::ReadOnlyStorage;
	renderEntries[1].buffer.minBindingSize = sizeof(shaderdata);

	BindGroupLayoutDescriptor renderLayoutDesc{};
	renderLayoutDesc.entryCount = 2;
	renderLayoutDesc.entries = renderEntries;
	renderBindGroupLayout = device.CreateBindGroupLayout(&renderLayoutDesc);

	// --- LAYOUT FÜR COMPUTE ---
	BindGroupLayoutEntry computeEntries[5] = {};
	// Binding 0 & 1 wie oben (aber für Compute sichtbar)
	computeEntries[0] = renderEntries[0];
	computeEntries[0].visibility = ShaderStage::Compute;
	computeEntries[1] = renderEntries[1];
	computeEntries[1].visibility = ShaderStage::Compute;
	computeEntries[1].buffer.type = BufferBindingType::Storage; // Compute darf schreiben

	// Binding 2: Indirect Buffer
	computeEntries[2].binding = 2;
	computeEntries[2].visibility = ShaderStage::Compute;
	computeEntries[2].buffer.type = BufferBindingType::Storage;
	computeEntries[2].buffer.minBindingSize = 20;

	// Binding 3: Draw Count
	computeEntries[3].binding = 3;
	computeEntries[3].visibility = ShaderStage::Compute;
	computeEntries[3].buffer.type = BufferBindingType::Storage;
	computeEntries[3].buffer.minBindingSize = sizeof(uint32_t);

	computeEntries[4].binding = 4;
	computeEntries[4].visibility = ShaderStage::Compute;
	computeEntries[4].buffer.type = BufferBindingType::Storage;
	computeEntries[4].buffer.minBindingSize = sizeof(positions);

	BindGroupLayoutDescriptor computeLayoutDesc{};
	computeLayoutDesc.entryCount = 5;
	computeLayoutDesc.entries = computeEntries;
	computeBindGroupLayout = device.CreateBindGroupLayout(&computeLayoutDesc);

	// --- PIPELINE LAYOUTS ---
	PipelineLayoutDescriptor renderPipelineLayoutDesc{};
	renderPipelineLayoutDesc.bindGroupLayoutCount = 1;
	renderPipelineLayoutDesc.bindGroupLayouts = (wgpu::BindGroupLayout*)&renderBindGroupLayout;
	renderLayout = device.CreatePipelineLayout(&renderPipelineLayoutDesc);

	PipelineLayoutDescriptor computePipelineLayoutDesc{};
	computePipelineLayoutDesc.bindGroupLayoutCount = 1;
	computePipelineLayoutDesc.bindGroupLayouts = (wgpu::BindGroupLayout*)&computeBindGroupLayout;
	computeLayout = device.CreatePipelineLayout(&computePipelineLayoutDesc);
}

void Render::initBuffers() {
	BufferDescriptor bufferDesc;

	bufferDesc.size = sizeof(MyUniforms);
	bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
	bufferDesc.mappedAtCreation = false;
	bufferDesc.label = "UniformBuffer";
	uniformBuffer = device.CreateBuffer(&bufferDesc);

	bufferDesc.size = 410000000;
	bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage;
	bufferDesc.label = "VertexBuffer";
	VertexBuffer = device.CreateBuffer(&bufferDesc);

	bufferDesc.size = 10000000;
	bufferDesc.usage = wgpu::BufferUsage::Indirect | BufferUsage::CopyDst | BufferUsage::Storage;
	bufferDesc.label = "IndirectBuffer";
	IndirectBuffer = device.CreateBuffer(&bufferDesc);

	uint32_t indices[4] = { 0, 1, 2, 3 };

	bufferDesc.size = sizeof(indices);
	bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
	bufferDesc.label = "IndexBuffer";
	IndexBuffer = device.CreateBuffer(&bufferDesc);
	queue.WriteBuffer(IndexBuffer, 0, indices, sizeof(indices));

	bufferDesc.size = sizeof(uint32_t);
	bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Indirect;
	bufferDesc.label = "DrawCountBuffer";
	DrawCountBuffer = device.CreateBuffer(&bufferDesc);

	bufferDesc.size = sizeof(positions) * 10000000;
	bufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
	bufferDesc.label = "positionsBuffer";
	positionBuffer = device.CreateBuffer(&bufferDesc);

	bufferDesc.size = 10000000;
	bufferDesc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
	bufferDesc.label = "StagingIndirectBuffer";
	StagingIndirectBuffer = device.CreateBuffer(&bufferDesc);

	bufferDesc.size = 410000000;
	bufferDesc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
	bufferDesc.label = "StagingVertexBuffer";
	StagingVertexBuffer = device.CreateBuffer(&bufferDesc);

	bufferDesc.size = sizeof(positions) * 10000000;
	bufferDesc.usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
	bufferDesc.label = "StagingpositionsBuffer";
	StagingpositionBuffer = device.CreateBuffer(&bufferDesc);
}

void Render::initBindGroups(){
	// --- RENDER BIND GROUP ---
	BindGroupEntry renderBindings[2] = {};
	renderBindings[0].binding = 0;
	renderBindings[0].buffer = uniformBuffer;
	renderBindings[0].size = sizeof(MyUniforms);

	renderBindings[1].binding = 1;
	renderBindings[1].buffer = VertexBuffer;
	renderBindings[1].size = 410000000;

	BindGroupDescriptor renderGroupDesc{};
	renderGroupDesc.layout = renderBindGroupLayout;
	renderGroupDesc.entryCount = 2;
	renderGroupDesc.entries = renderBindings;
	renderBindGroup = device.CreateBindGroup(&renderGroupDesc);

	// --- COMPUTE BIND GROUP ---
	BindGroupEntry computeBindings[5] = {};
	computeBindings[0] = renderBindings[0]; // Uniforms
	computeBindings[1] = renderBindings[1]; // Vertex

	computeBindings[2].binding = 2;
	computeBindings[2].buffer = IndirectBuffer;
	computeBindings[2].size = 10000000;

	computeBindings[3].binding = 3;
	computeBindings[3].buffer = DrawCountBuffer;
	computeBindings[3].size = sizeof(uint32_t);

	computeBindings[4].binding = 4;
	computeBindings[4].buffer = positionBuffer;
	computeBindings[4].size = sizeof(positions) * 10000000;

	BindGroupDescriptor computeGroupDesc{};
	computeGroupDesc.layout = computeBindGroupLayout;
	computeGroupDesc.entryCount = 5;
	computeGroupDesc.entries = computeBindings;
	computeBindGroup = device.CreateBindGroup(&computeGroupDesc);
}

void Render::initDepthBuffer() {
	glfwGetFramebufferSize(window, &width, &height);

	depthStencilState = {};
	depthStencilState.depthCompare = CompareFunction::Greater;
	depthStencilState.depthWriteEnabled = true;
	TextureFormat depthTextureFormat = TextureFormat::Depth32FloatStencil8;
	depthStencilState.format = depthTextureFormat;
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;

	// Create the depth texture
	TextureDescriptor depthTextureDesc;
	depthTextureDesc.dimension = TextureDimension::e2D;
	depthTextureDesc.format = depthTextureFormat;
	depthTextureDesc.mipLevelCount = 1;
	depthTextureDesc.sampleCount = 4;
	depthTextureDesc.size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
	depthTextureDesc.usage = TextureUsage::RenderAttachment;
	depthTextureDesc.viewFormatCount = 1;
	depthTextureDesc.viewFormats = (wgpu::TextureFormat*)&depthTextureFormat;
	depthTexture = device.CreateTexture(&depthTextureDesc);

	// Create the view of the depth texture manipulated by the rasterizer
	TextureViewDescriptor depthTextureViewDesc;
	depthTextureViewDesc.aspect = TextureAspect::All;
	depthTextureViewDesc.baseArrayLayer = 0;
	depthTextureViewDesc.arrayLayerCount = 1;
	depthTextureViewDesc.baseMipLevel = 0;
	depthTextureViewDesc.mipLevelCount = 1;
	depthTextureViewDesc.dimension = TextureViewDimension::e2D;
	depthTextureViewDesc.format = depthTextureFormat;
	depthTextureView = depthTexture.CreateView(&depthTextureViewDesc);

}

Adapter Render::requestAdapterSync(Instance instance, RequestAdapterOptions const* options)
{
	wgpu::Adapter adapter = nullptr;
	bool requestEnded = false;

	instance.RequestAdapter(
		options,
		wgpu::CallbackMode::AllowProcessEvents,
		[&](wgpu::RequestAdapterStatus status, wgpu::Adapter respAdapter, wgpu::StringView message) {
			if (status == wgpu::RequestAdapterStatus::Success) {
				adapter = std::move(respAdapter);
			}
			else {
				std::cout << "Could not get WebGPU adapter: " << std::string_view(message.data, message.length) << std::endl;
			}
			requestEnded = true;
		}
	);

	while (!requestEnded) {
		instance.ProcessEvents();
	}

	return adapter;
}

Device Render::requestDeviceSync(Instance instance, Adapter adapter, DeviceDescriptor const* descriptor)
{
	wgpu::Device device = nullptr;
	bool requestEnded = false;

	adapter.RequestDevice(
		descriptor,
		wgpu::CallbackMode::AllowProcessEvents,
		[&](wgpu::RequestDeviceStatus status, wgpu::Device respDevice, wgpu::StringView message) {
			if (status == wgpu::RequestDeviceStatus::Success) {
				device = std::move(respDevice);
			}
			else {
				std::cout << "Could not get WebGPU device: " << std::string_view(message.data, message.length) << std::endl;
			}
			requestEnded = true;
		}
	);

	while (!requestEnded) {
		instance.ProcessEvents(); // Die Instanz verarbeitet die Callbacks
	}

	return device;
}

Limits Render::GetRequiredLimits(Adapter adapter) {
	Limits supportedLimits;
	adapter.GetLimits(&supportedLimits);
	return supportedLimits;
}

TextureView Render::GetNextSurfaceViewData()
{
	SurfaceTexture surfaceTexture;
	surface.GetCurrentTexture(&surfaceTexture);

	if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal) {
		return nullptr;
	}

	TextureViewDescriptor viewDescriptor;
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.label = "Surface texture view";
	wgpu::SurfaceCapabilities capabilities;
	viewDescriptor.format = surfaceTexture.texture.GetFormat();
	viewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = wgpu::TextureAspect::All;
	TextureView targetVieww = surfaceTexture.texture.CreateView(&viewDescriptor);

	return targetVieww;
}

void Render::updateProjectionMatrix() {
	glfwGetFramebufferSize(window, &width, &height);
	float ratio = (float)width / (float)height;

	float f = 1.0f / tan(fov / 2.0f);

	glm::mat4x4 projection =glm:: mat4x4(0.0f);

	projection[0][0] = f / ratio;
	projection[1][1] = f;
	projection[2][2] = 0.0f;
	projection[2][3] = -1.0f;
	projection[3][2] = nearplane;
	matuniforms.projectionMatrix = projection;
}

void Render::onResize() {

	glfwGetFramebufferSize(window, &width, &height);

	if (width == 0 || height == 0) {
		return;
	}
	depthTexture.Destroy();
	msaaTexture.Destroy();

	initSwapchain();
	initDepthBuffer();
	updateProjectionMatrix();
	//onFrame();
}

bool Render::isrunning()
{
	return !glfwWindowShouldClose(window);
}

bool Render::mapstagingbuffer(){
	bool mappedsucess = true;
	StagingIndirectBuffer.MapAsync(wgpu::MapMode::Write, 0, StagingIndirectBuffer.GetSize(), wgpu::CallbackMode::AllowProcessEvents, [&](wgpu::MapAsyncStatus status, wgpu::StringView message){
		if(status == wgpu::MapAsyncStatus::Success){
			void* raw = StagingIndirectBuffer.GetMappedRange();
			indirectdata = static_cast<DrawIndirectArgs*>(raw);
		}
		else{
			mappedsucess = false;
		}
										  });

	StagingVertexBuffer.MapAsync(wgpu::MapMode::Write, 0, StagingVertexBuffer.GetSize(), wgpu::CallbackMode::AllowProcessEvents, [&](wgpu::MapAsyncStatus status, wgpu::StringView message){
		if(status == wgpu::MapAsyncStatus::Success){
			void* raw = StagingVertexBuffer.GetMappedRange();
			vertexdata = static_cast<shaderdata*>(raw);
		}
		else{
			mappedsucess = false;
		}
										});

	StagingpositionBuffer.MapAsync(wgpu::MapMode::Write, 0, StagingpositionBuffer.GetSize(), wgpu::CallbackMode::AllowProcessEvents, [&](wgpu::MapAsyncStatus status, wgpu::StringView message){
		if(status == wgpu::MapAsyncStatus::Success){
			void* raw = StagingpositionBuffer.GetMappedRange();
			positiondata = static_cast<positions*>(raw);
		}
		else{
			mappedsucess = false;
		}

		

										  });
	return mappedsucess;
}

void Render::unmapandcopy(){

	StagingIndirectBuffer.Unmap();
	StagingVertexBuffer.Unmap();
	StagingpositionBuffer.Unmap();
	indirectdata = nullptr;
	vertexdata = nullptr;
	positiondata = nullptr;

	encoder.CopyBufferToBuffer(StagingIndirectBuffer, 0, IndirectBuffer, 0, StagingIndirectBuffer.GetSize());
	encoder.CopyBufferToBuffer(StagingVertexBuffer, 0, VertexBuffer, 0, StagingVertexBuffer.GetSize());
	encoder.CopyBufferToBuffer(StagingpositionBuffer, 0, positionBuffer, 0, StagingpositionBuffer.GetSize());

	ismapped = false;
}
