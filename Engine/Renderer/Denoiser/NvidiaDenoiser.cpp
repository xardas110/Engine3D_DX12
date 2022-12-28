#include <EnginePCH.h>
#include <NvidiaDenoiser.h>
#include <Application.h>
#include <CommandQueue.h>

//#include <NRIDescs.hpp>
#include <NRDIntegration.hpp>

#include <Helpers.h>
#include <imgui.h>

static void ShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}


std::string errorResult[] =
{
	"Success", "FAILURE", "INVALID ARGUMENT",
	"OUT_OF_MEMORY",
	"UNSUPPORTED",
	"DEVICE_LOST",
	"SWAPCHAIN_RESIZE",
	"MAX_NUM"
};

void NRDERRORCHECK(nri::Result result)
{
	if (nri::Result::SUCCESS == result) return;

	throw std::exception(errorResult[(int)result].c_str());
}

NvidiaDenoiser::NvidiaDenoiser(int width, int height, DenoiserTextures& texs)
	:width(width), height(height)
{
	auto device = Application::Get().GetDevice();
	auto cq = Application::Get().GetCommandQueue();

	m_Adapter = Application::Get().GetAdapter(false).Get();

	nri::DeviceCreationD3D12Desc deviceDesc = {};
	deviceDesc.d3d12Device = device.Get();

	deviceDesc.d3d12PhysicalAdapter = m_Adapter.Get();
	deviceDesc.d3d12GraphicsQueue = cq->GetD3D12CommandQueue().Get();

	deviceDesc.enableNRIValidation = true;
	
	m_NRIDevice = nullptr;
	nri::Result nriResult = nri::CreateDeviceFromD3D12Device(deviceDesc, m_NRIDevice);

	// Get core functionality
	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::CoreInterface), (nri::CoreInterface*)&NRI);

	NRDERRORCHECK(nriResult);

	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface*)&NRI);

	NRDERRORCHECK(nriResult);

	// Get appropriate "wrapper" extension (XXX - can be D3D11, D3D12 or VULKAN)
	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::WrapperD3D12Interface), (nri::WrapperD3D12Interface*)&NRI);

	NRDERRORCHECK(nriResult);

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));

	const nrd::MethodDesc methodDescs[] =
	{
		// Put neeeded methods here, like:
		{ nrd::Method::REBLUR_DIFFUSE_SPECULAR, width, height },
	};

	nrd::DenoiserCreationDesc denoiserCreationDesc = {};
	denoiserCreationDesc.requestedMethods = methodDescs;
	denoiserCreationDesc.requestedMethodsNum = 1;

	bool result = NRD.Initialize(denoiserCreationDesc, *m_NRIDevice, NRI, NRI);

	if (!result)
	{
		throw std::exception("Failed to create NRI interface");
	}

	{//Normalrough
		nri::TextureD3D12Desc normalRoughDesc = {};

		normalRoughDesc.d3d12Resource = texs.inNormalRough.GetD3D12Resource().Get();

		auto& current = nriTextures[nriTypes::inNormalRoughness];

		nriTextures[nriTypes::inNormalRoughness].entryFormat =
			nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_UNORM);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, normalRoughDesc, current.texture));

		current.entryDescs.texture = current.texture;

		current.entryDescs.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		current.entryDescs.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Motion Vector
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inMV.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inMV].entryDescs;
		auto& current = nriTextures[nriTypes::inMV];

		nriTextures[nriTypes::inMV].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));

		entryDesc.texture = current.texture;

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
		entryDesc.nextLayout = nri::TextureLayout::GENERAL;
	};
	{//viewZ
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inViewZ.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inViewZ].entryDescs;
		auto& current = nriTextures[nriTypes::inViewZ];

		nriTextures[nriTypes::inViewZ].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R32_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));
		entryDesc.texture = current.texture;

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Indirect Diffuse
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inIndirectDiffuse.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inIndirectDiffuse].entryDescs;
		auto& current = nriTextures[nriTypes::inIndirectDiffuse];

		nriTextures[nriTypes::inIndirectDiffuse].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));
		entryDesc.texture = current.texture;

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Indirect Specular
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inIndirectSpecular.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inIndirectSpecular].entryDescs;
		auto& current = nriTextures[nriTypes::inIndirectSpecular];

		nriTextures[nriTypes::inIndirectSpecular].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));
		entryDesc.texture = current.texture;

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//OUT Indirect Diffuse
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.outIndirectDiffuse.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::outIndirectDiffuse].entryDescs;
		auto& current = nriTextures[nriTypes::outIndirectDiffuse];

		nriTextures[nriTypes::outIndirectDiffuse].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));
		
		entryDesc.texture = current.texture;
		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
		entryDesc.nextLayout = nri::TextureLayout::GENERAL;
	};
	{//OUT Indirect Specular
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.outIndirectSpecular.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::outIndirectSpecular].entryDescs;
		auto& current = nriTextures[nriTypes::outIndirectSpecular];

		nriTextures[nriTypes::outIndirectSpecular].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));

		entryDesc.texture = current.texture;
		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
		entryDesc.nextLayout = nri::TextureLayout::GENERAL;
	};

	{//IN SHADOW DATA
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inShadowData.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inShadowData].entryDescs;
		auto& current = nriTextures[nriTypes::inShadowData];

		nriTextures[nriTypes::inShadowData].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));

		entryDesc.texture = current.texture;
		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};

	{//OUT SHADOW
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.outShadow.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::outShadow].entryDescs;
		auto& current = nriTextures[nriTypes::outShadow];

		nriTextures[nriTypes::outShadow].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R8_UNORM);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, current.texture));

		entryDesc.texture = current.texture;
		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};

	std::cout << "-------------------------" << std::endl;
	std::cout << "Denoiser Normal Encoding: "  << (int)nrd::GetLibraryDesc().normalEncoding << std::endl;
	std::cout << "Denoiser roughness Encoding: " << (int)nrd::GetLibraryDesc().roughnessEncoding << std::endl;
	std::cout << "-------------------------" << std::endl;
}

NvidiaDenoiser::~NvidiaDenoiser()
{
	m_CommandAllocator->Reset();

	NRD.Destroy();	

	for (uint32_t i = 0; i < nriTypes::size; i++)
		NRI.DestroyTexture(*nriTextures[i].texture);

	nri::DestroyDevice(*m_NRIDevice);
}

void NvidiaDenoiser::RenderFrame(CommandList& commandList, const CameraCB &cam, int currentBackbufferIndex, int width, int height)
{
	auto& commonSettings = denoiserSettings.commonSettings;
	auto& settings = denoiserSettings.settings;

	nri::CommandBufferD3D12Desc commandBufferDesc = {};
	commandBufferDesc.d3d12CommandList = (ID3D12GraphicsCommandList*)commandList.GetGraphicsCommandList().Get();
	commandBufferDesc.d3d12CommandAllocator = m_CommandAllocator.Get();
	
	NRI.CreateCommandBufferD3D12(*m_NRIDevice, commandBufferDesc, nriCommandBuffer);

	memcpy(commonSettings.worldToViewMatrix, &cam.view, sizeof(cam.view));
	memcpy(commonSettings.viewToClipMatrix, &cam.proj, sizeof(cam.proj));
	
	memcpy(commonSettings.worldToViewMatrixPrev, &cam.prevView, sizeof(cam.prevView));
	memcpy(commonSettings.viewToClipMatrixPrev, &cam.prevProj, sizeof(cam.prevProj));

	commonSettings.isMotionVectorInWorldSpace = true;
	commonSettings.frameIndex = Application::GetFrameCount();

	NRD.SetMethodSettings(nrd::Method::REBLUR_DIFFUSE_SPECULAR, &settings);

	{ //Indirect light denoising
		NrdUserPool userPool = {};
	
		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_MV,
			{ &nriTextures[nriTypes::inMV].entryDescs,
			nriTextures[nriTypes::inMV].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS,
			{ &nriTextures[nriTypes::inNormalRoughness].entryDescs, 
			nriTextures[nriTypes::inNormalRoughness].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_VIEWZ,
			{ &nriTextures[nriTypes::inViewZ].entryDescs, 
			nriTextures[nriTypes::inViewZ].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::inIndirectDiffuse].entryDescs, 
			nriTextures[nriTypes::inIndirectDiffuse].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::outIndirectDiffuse].entryDescs, 
			nriTextures[nriTypes::outIndirectDiffuse].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::inIndirectSpecular].entryDescs,
			nriTextures[nriTypes::inIndirectSpecular].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::outIndirectSpecular].entryDescs,
			nriTextures[nriTypes::outIndirectSpecular].entryFormat });
	

		NRD.Denoise(currentBackbufferIndex, *nriCommandBuffer, commonSettings, userPool, true);
	}

	NRD.SetMethodSettings(nrd::Method::SIGMA_SHADOW, &settings);

	{ //Shadow denoising
		NrdUserPool userPool = {};

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS,
			{ &nriTextures[nriTypes::inNormalRoughness].entryDescs,
			nriTextures[nriTypes::inNormalRoughness].entryFormat });
	
		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_SHADOWDATA,
			{ &nriTextures[nriTypes::inShadowData].entryDescs,
			nriTextures[nriTypes::inShadowData].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_SHADOW_TRANSLUCENCY,
			{ &nriTextures[nriTypes::outShadow].entryDescs,
			nriTextures[nriTypes::outShadow].entryFormat });

		NRD.Denoise(currentBackbufferIndex, *nriCommandBuffer, commonSettings, userPool, true);
	}

	NRI.DestroyCommandBuffer(*nriCommandBuffer);
}

void NvidiaDenoiser::OnGUI()
{
	ImGui::Begin("Denoiser Settings");

	auto* settings = &denoiserSettings.settings;
	auto* commonSettings = &denoiserSettings.commonSettings;

	ImGui::Text("Frame History Settings --------------------");

	ShowHelpMarker("[0; REBLUR_MAX_HISTORY_FRAME_NUM] - maximum number of linearly accumulated frames (= FPS * time of accumulation)");
	ImGui::SameLine();

	static int i1 = settings->maxAccumulatedFrameNum;
	ImGui::SliderInt("maxAccumulatedFrameNum", &i1, 0, 60);
	settings->maxAccumulatedFrameNum = i1;

	ShowHelpMarker("[0; REBLUR_MAX_HISTORY_FRAME_NUM] - maximum number of linearly accumulated frames in fast history (less than maxAccumulatedFrameNum)");
	ImGui::SameLine();

	static int i2 = settings->maxFastAccumulatedFrameNum;
	ImGui::SliderInt("maxFastAccumulatedFrameNum", &i2, 0, 60);
	settings->maxFastAccumulatedFrameNum = i2;

	ShowHelpMarker("[0; REBLUR_MAX_HISTORY_FRAME_NUM] - number of reconstructed frames after history reset (less than maxFastAccumulatedFrameNum)");
	ImGui::SameLine();

	static int i3 = settings->historyFixFrameNum;
	ImGui::SliderInt("historyFixFrameNum", &i3, 0, 60);
	settings->historyFixFrameNum = i3;

	ImGui::Text("-----------------------------------------");

	ImGui::Text("Anti-Lag Hit Settings --------------------");

	ImGui::Checkbox("Enabled", &settings->antilagHitDistanceSettings.enable);

	ShowHelpMarker("(0; 1] - hit distances are normalized");
	ImGui::SameLine();
	ImGui::SliderFloat("sensitivityToDarkness", &settings->antilagHitDistanceSettings.sensitivityToDarkness, 0.f, 1.f);

	ShowHelpMarker("(> 0) - real delta is reduced by local variance multiplied by this value");
	ImGui::SameLine();
	ImGui::SliderFloat("sigmaScale", &settings->antilagHitDistanceSettings.sigmaScale, 0.f, 2.f);

	ShowHelpMarker("(normalized %) - max > min, usually 2-4x times greater than min");
	ImGui::SameLine();
	ImGui::SliderFloat("thresholdMax", &settings->antilagHitDistanceSettings.thresholdMax, 0.f, 1.f);

	ShowHelpMarker("(normalized %) - must almost ignore residual noise (boiling), default is tuned for 0.5rpp for the worst case");
	ImGui::SameLine();
	ImGui::SliderFloat("thresholdMin", &settings->antilagHitDistanceSettings.thresholdMin, 0.f, 1.f);

	ImGui::Text("-----------------------------------------");

	ImGui::NewLine();

	ImGui::Text("Anti-Lag Itensity Settings --------------------");

	ImGui::Checkbox("Enabled", &settings->antilagIntensitySettings.enable);

	ShowHelpMarker("(0; 1] - hit distances are normalized");
	ImGui::SameLine();
	ImGui::SliderFloat("sensitivityToDarkness", &settings->antilagIntensitySettings.sensitivityToDarkness, 0.f, 1.f);

	ShowHelpMarker("(> 0) - real delta is reduced by local variance multiplied by this value");
	ImGui::SameLine();
	ImGui::SliderFloat("sigmaScale", &settings->antilagIntensitySettings.sigmaScale, 0.f, 2.f);

	ShowHelpMarker("(normalized %) - max > min, usually 2-4x times greater than min");
	ImGui::SameLine();
	ImGui::SliderFloat("thresholdMax", &settings->antilagIntensitySettings.thresholdMax, 0.f, 1.f);

	ShowHelpMarker("(normalized %) - must almost ignore residual noise (boiling), default is tuned for 0.5rpp for the worst case");
	ImGui::SameLine();
	ImGui::SliderFloat("thresholdMin", &settings->antilagIntensitySettings.thresholdMin, 0.f, 1.f);

	ImGui::Text("-----------------------------------------");

	ImGui::NewLine();

	ImGui::Text("HIT DISTANCE PARAMETERS --------------------");

	ShowHelpMarker("(units) - constant value IMPORTANT: if your unit is not meter, you must convert it from meters to units manually!");
	ImGui::SameLine();
	ImGui::SliderFloat("hitDistanceParameters.A", &settings->hitDistanceParameters.A, 0.1f, 100.f);

	ShowHelpMarker("(> 0) - viewZ based linear scale (1 m - 10 cm, 10 m - 1 m, 100 m - 10 m)");
	ImGui::SameLine();
	ImGui::SliderFloat("hitDistanceParameters.B", &settings->hitDistanceParameters.B, 0.f, 1.f);

	ShowHelpMarker("(>= 1) - roughness based scale, use values > 1 to get bigger hit distance for low roughness");
	ImGui::SameLine();
	ImGui::SliderFloat("hitDistanceParameters.C", &settings->hitDistanceParameters.C, 1.f, 100.f);

	ShowHelpMarker("(<= 0) - absolute value should be big enough to collapse exp2(D * roughness ^ 2) to ~0 for roughness = 1");
	ImGui::SameLine();
	ImGui::SliderFloat("hitDistanceParameters.D", &settings->hitDistanceParameters.D, -100.f, 100.f);

	ImGui::Text("-----------------------------------------");

	ImGui::NewLine();

	ShowHelpMarker("(pixels) - pre-accumulation spatial reuse pass blur radius (0 = disabled, must be used in case of probabilistic sampling)");
	ImGui::SameLine();
	ImGui::SliderFloat("Diffuse Prepass Blur Radius", &settings->diffusePrepassBlurRadius, 0.f, 100.f);

	ShowHelpMarker("(pixels) - pre-accumulation spatial reuse pass blur radius (0 = disabled, must be used in case of probabilistic sampling)");
	ImGui::SameLine();
	ImGui::SliderFloat("Specular Prepass Blur Radius", &settings->specularPrepassBlurRadius, 0.f, 100.f);
	
	ShowHelpMarker("(pixels) - base denoising radius (30 is a baseline for 1440p)");
	ImGui::SameLine();
	ImGui::SliderFloat("Blur radius", &settings->blurRadius, 0.f, 100.f);

	ShowHelpMarker("(pixels) - base stride between samples in history reconstruction pass");
	ImGui::SameLine();
	ImGui::SliderFloat("historyFixStrideBetweenSamples", &settings->historyFixStrideBetweenSamples, 0.f, 100.f);

	ShowHelpMarker("(normalized %) - base fraction of diffuse or specular lobe angle used to drive normal based rejection");
	ImGui::SameLine();
	ImGui::SliderFloat("lobeAngleFraction", &settings->lobeAngleFraction, 0.f, 1.f);

	ShowHelpMarker("(normalized %) - base fraction of center roughness used to drive roughness based rejection");
	ImGui::SameLine();
	ImGui::SliderFloat("roughnessFraction", &settings->roughnessFraction, 0.f, 1.f);

	ShowHelpMarker("[0; 1] - if roughness < this, temporal accumulation becomes responsive and driven by roughness (useful for animated water)");
	ImGui::SameLine();
	ImGui::SliderFloat("responsiveAccumulationRoughnessThreshold", &settings->responsiveAccumulationRoughnessThreshold, 0.f, 1.f);

	ShowHelpMarker("(normalized %) - stabilizes output, more stabilization improves antilag (clean signals can use lower values)");
	ImGui::SameLine();
	ImGui::SliderFloat("stabilizationStrength", &settings->stabilizationStrength, 0.f, 1.f);

	ShowHelpMarker("(normalized %) - represents maximum allowed deviation from local tangent plane");
	ImGui::SameLine();
	ImGui::SliderFloat("planeDistanceSensitivity", &settings->planeDistanceSensitivity, 0.f, 1.f);

	ShowHelpMarker("(normalized %) - if relative distance difference is greater than threshold, history gets reset (0.5-2.5% works well)");
	ImGui::SameLine();
	ImGui::SliderFloat("disocclusionThreshold", &commonSettings->disocclusionThreshold, 0.f, 1.f);

	ShowHelpMarker("(normalized %) - alternative disocclusion threshold, which is mixed to based on IN_DISOCCLUSION_THRESHOLD_MIX");
	ImGui::SameLine();
	ImGui::SliderFloat("disocclusionThresholdAlternate", &commonSettings->disocclusionThresholdAlternate, 0.f, 1.f);

	ShowHelpMarker("[0; 1] - enables noisy input / denoised output comparison");
	ImGui::SameLine();
	ImGui::SliderFloat("splitScreen", &commonSettings->splitScreen, 0.f, 1.f);

	ShowHelpMarker("(units) > 0 - use TLAS or tracing range (max value = NRD_FP16_MAX / NRD_FP16_VIEWZ_SCALE - 1 = 524031)");
	ImGui::SameLine();
	ImGui::SliderFloat("denoisingRange", &commonSettings->denoisingRange, 0.f, 500000.f);

	ShowHelpMarker("Adds bias in case of badly defined signals, but tries to fight with fireflies");
	ImGui::SameLine();
	ImGui::Checkbox("enableAntiFirefly", &settings->enableAntiFirefly);

	ShowHelpMarker("Turns off spatial filtering and virtual motion based specular tracking");
	ImGui::SameLine();
	ImGui::Checkbox("enableReferenceAccumulation", &settings->enableReferenceAccumulation);

	ShowHelpMarker("Boosts performance by sacrificing IQ");
	ImGui::SameLine();
	ImGui::Checkbox("enablePerformanceMode", &settings->enablePerformanceMode);

	ShowHelpMarker("Spatial passes do optional material index comparison as: ( materialEnabled ? material[ center ] == material[ sample ] : 1 )");
	ImGui::SameLine();
	ImGui::Checkbox("enableMaterialTestForDiffuse", &settings->enableMaterialTestForDiffuse);

	ShowHelpMarker("Spatial passes do optional material index comparison as: ( materialEnabled ? material[ center ] == material[ sample ] : 1 )");
	ImGui::SameLine();
	ImGui::Checkbox("enableMaterialTestForSpecular", &settings->enableMaterialTestForSpecular);

	if (ImGui::Button("Reset to default"))
		denoiserSettings = DenoiserSettings();

	ImGui::End();
}