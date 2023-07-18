cmake_minimum_required( VERSION 3.23.2 ) # Latest version of CMake when this file was created.

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    set(PHYSX_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Libs/PhysX/physx/debug")
    set(NVSDK_LIB_VERSION nvsdk_ngx_d_dbg.lib)
else ()
    set(PHYSX_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Libs/PhysX/physx/release")
    set(NVSDK_LIB_VERSION nvsdk_ngx_d.lib)
ENDIF()

add_compile_definitions(DEBUG_CMD)
add_compile_definitions(ImTextureID=ImU64)
add_compile_definitions(NRD_COMPILER_DXC)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(Libs/assimp)

find_package(Vulkan REQUIRED FATAL_ERROR)

set(NRD_STATIC_LIBRARY OFF)
add_subdirectory(Libs/RayTracingDenoiser)

add_subdirectory(Libs/NRI)

# Include Google Test
add_subdirectory(Libs/googletest)

set( HEADER_FILES 
    "core/KeyCodes.h"
    "core/Helpers.h"
    "core/Events.h"
    "core/Game.h"
    "core/HighResolutionClock.h"
    "core/EnginePCH.h"
    "core/ThreadSafeQueue.h"
    "core/DX12Helper.h"
    "core/Window.h"
    "core/Camera.h"
    "core/Components.h"
    "core/Transform.h"
    "core/GameMode.h"
    "core/World.h"
    "core/Entity.h" 
    "core/AssimpLoader.h"
    "core/Logger.h"
    "DX12/Application.h"
    "DX12/Buffer.h"
    "DX12/ByteAddressBuffer.h"
    "DX12/CommandList.h"
    "DX12/CommandQueue.h"
    "DX12/ConstantBuffer.h"
    "DX12/DescriptorAllocation.h"
    "DX12/DescriptorAllocator.h"
    "DX12/DescriptorAllocatorPage.h"
    "DX12/DynamicDescriptorHeap.h"   
    "DX12/GenerateMipsPSO.h"    
    "DX12/IndexBuffer.h"  
    "AssetManager/Mesh.h"
    "DX12/PanoToCubemapPSO.h"
    "DX12/RenderTarget.h"
    "DX12/Resource.h"
    "DX12/ResourceStateTracker.h"
    "DX12/RootSignature.h"
    "DX12/StructuredBuffer.h"
    "AssetManager/Texture.h"
    "DX12/TextureUsage.h"   
    "DX12/UploadBuffer.h"
    "DX12/VertexBuffer.h"
    "DX12/d3dx12.h"   
    "DX12/StaticDescriptorHeap.h"
    "Editor/GUI.h" 
    "Renderer/DeferredRenderer.h"
    "AssetManager/AssetManager.h" 
    "Physics/PhysicsSystem.h"
    "Physics/PhysicsActor.h"
    "Editor/Editor.h"   
    "AssetManager/Material.h"
    "Raytracing/Raytracing.h"
    "AssetManager/MeshManager.h"
    "AssetManager/TextureManager.h"
    "AssetManager/MaterialManager.h"
    "Libs/eventcpp/event.hpp"           
    "Lights/DirectionalLight.h"
    "Renderer/CompositionPass/CompositionPass.h"
    "Renderer/Debug/DebugTexture.h"
    "Renderer/Denoiser/NvidiaDenoiser.h"    
    "Renderer/Skybox/Skybox.h" 
    "Renderer/HDR/HDR.h" 
    "Renderer/DLSS/DLSS.h" 
    "Renderer/GBuffer/GBuffer.h" 
    "Renderer/Lightpass/LightPass.h" 
    "Renderer/Noise/BlueNoise.h"    
    )

set( SOURCE_FILES
    "core/EnginePCH.cpp"
    "DX12/Application.cpp"
    "DX12/Buffer.cpp"
    "DX12/ByteAddressBuffer.cpp"
    "DX12/CommandQueue.cpp"
    "DX12/CommandList.cpp"
    "DX12/ConstantBuffer.cpp"
    "DX12/DescriptorAllocation.cpp"
    "DX12/DescriptorAllocator.cpp"
    "DX12/DescriptorAllocatorPage.cpp"
    "DX12/DynamicDescriptorHeap.cpp"
    "core/Game.cpp"
    "DX12/GenerateMipsPSO.cpp"
    "Editor/GUI.cpp"
    "core/HighResolutionClock.cpp"
    "DX12/IndexBuffer.cpp"
    "AssetManager/Mesh.cpp"
    "DX12/PanoToCubemapPSO.cpp"
    "DX12/RenderTarget.cpp"
    "DX12/Resource.cpp"
    "DX12/ResourceStateTracker.cpp"
    "DX12/RootSignature.cpp"
    "DX12/StructuredBuffer.cpp"
    "AssetManager/Texture.cpp"
    "DX12/UploadBuffer.cpp"
    "DX12/VertexBuffer.cpp"
    "core/Window.cpp"
    "core/Camera.cpp"
    "Renderer/DeferredRenderer.cpp"
    "AssetManager/AssetManager.cpp"    
    "Physics/PhysicsSystem.cpp"
    "core/GameMode.cpp"
    "core/main.cpp"
    "Editor/Editor.cpp"  
    "core/World.cpp"   
    "core/Entity.cpp" 
    "core/AssimpLoader.cpp"
    "Raytracing/Raytracing.cpp"
    "AssetManager/MeshManager.cpp"
    "AssetManager/TextureManager.cpp"
    "AssetManager/MaterialManager.cpp"
    "AssetManager/Material.cpp"
    "Renderer/GBuffer/GBuffer.cpp" 
    "DX12/StaticDescriptorHeap.cpp" 
    "Renderer/CompositionPass/CompositionPass.cpp"
    "Renderer/Debug/DebugTexture.cpp" 
    "Lights/DirectionalLight.cpp"  
    "Renderer/Denoiser/NvidiaDenoiser.cpp" 
    "Renderer/Skybox/Skybox.cpp" 
    "Renderer/HDR/HDR.cpp" 
    "Renderer/DLSS/DLSS.cpp" 
    "Physics/PhysicsActor.cpp"
    "Renderer/Noise/BlueNoise.cpp"
    "Renderer/Lightpass/Lightpass.cpp")

set( IMGUI_HEADERS
    Libs/ImGui/imgui.h
    Libs/ImGui/imconfig.h
    Libs/ImGui/imgui_impl_win32.h
    Libs/ImGui/imgui_impl_dx12.h
)

source_group( "Header Files\\ImGUI" FILES ${IMGUI_HEADERS} )

set( IMGUI_SOURCE
    "Libs/ImGui/imgui.cpp"
    "Libs/ImGui/imgui_demo.cpp"
    "Libs/ImGui/imgui_draw.cpp"
    "Libs/ImGui/imgui_widgets.cpp"
    "Libs/ImGui/imgui_impl_win32.cpp"
    Libs/ImGui/imgui_impl_dx12.cpp
    Libs/ImGui/imgui_tables.cpp
)

set( ENTT_HEADERS
    Libs/entt/entt.h
)

source_group( "Source Files\\ImGUI" FILES ${IMGUI_SOURCE} )

set( SHADER_FILES
    Resources/Shaders/Div/GenerateMips_CS.hlsl
    Resources/Shaders/Div/PanoToCubemap_CS.hlsl
    Resources/Shaders/GBuffer/GeometryPass_VS.hlsl
    Resources/Shaders/GBuffer/GeometryPass_PS.hlsl
    Resources/Shaders/GBuffer/DepthPrePass_VS.hlsl
    Resources/Shaders/LightPass/LightPass_VS.hlsl
    Resources/Shaders/LightPass/LightPass_PS.hlsl
    Resources/Shaders/CompositionPass/CompositionPass_VS.hlsl
    Resources/Shaders/CompositionPass/CompositionPass_PS.hlsl
    Resources/Shaders/Debug/DebugTexture_VS.hlsl
    Resources/Shaders/Debug/DebugTexture_PS.hlsl
    Resources/Shaders/HDR/HDR_VS.hlsl
    Resources/Shaders/HDR/HDR_PS.hlsl
    Resources/Shaders/HDR/Exposure_CS.hlsl
    Resources/Shaders/HDR/Histogram_CS.hlsl
)

set( RESOURCE_FILES 
    Engine.rc
    Resources/Icon/app_icon.ico
	${SHADER_FILES}
)

source_group( "Resources" FILES Engine.rc resource.h )
source_group( "Resources\\Icon" FILES Resources/Icon/app_icon.ico )
source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

 set ( TARGET_INCLUDES
    PUBLIC DX12
    PUBLIC Libs
    PUBLIC Libs/ImGui
    PUBLIC Libs/assimp/include
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/Libs/assimp/include
    PUBLIC Libs/RayTracingDenoiser/include
    PUBLIC Libs/RayTracingDenoiser/Integration
    PUBLIC Libs/RayTracingDenoiser/Shaders/Include
    PUBLIC Libs/NRI/include
    PUBLIC core
    PUBLIC Editor
    Public Physics
    PUBLIC Libs/physx/include 
    PUBLIC AssetManager
    PUBLIC Resources/Shaders
    PUBLIC Resources/Shaders/Common
    PUBLIC Libs/eventcpp
    PUBLIC Libs/WinPixEventRuntime/include
    PUBLIC Raytracing
    PUBLIC Renderer
    PUBLIC Renderer/CompositionPass
    PUBLIC Renderer/GBuffer
    PUBLIC Renderer/Debug
    PUBLIC Renderer/Lightpass
    PUBLIC Renderer/Denoiser
    PUBLIC Libs/DLSS/include
    PUBLIC Lights
    PUBLIC Physics
    PUBLIC Libs/PhysX/physx/include
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Shaders"
    PUBLIC Libs/googletest/googletest/include
    PUBLIC Tests/AssetManager
 )

 set ( TARGET_LINK_LIBS
    PUBLIC assimp
    PUBLIC NRD
    PUBLIC NRI
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libs/DLSS/Lib/Windows_x86_64/x86_64/${NVSDK_LIB_VERSION}
    PUBLIC DirectXTex
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libs/WinPixEventRuntime/Lib/WinPixEventRuntime.lib
 )

 set (PHYSX_LIBS
    PUBLIC ${PHYSX_LIB_PATH}/LowLevel_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/LowLevelAABB_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/LowLevelDynamics_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysX_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXCharacterKinematic_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXCommon_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXCooking_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXExtensions_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXFoundation_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXPvdSDK_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXTask_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXVehicle_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PhysXVehicle2_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/PVDRuntime_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/SceneQuery_static_64.lib
    PUBLIC ${PHYSX_LIB_PATH}/SimulationController_static_64.lib
 )

add_library( Engine STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
    ${IMGUI_HEADERS} ${IMGUI_SOURCE} "Tests/AssetManager/TextureManagerTest.h" )

 add_executable( EngineBin WIN32
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
    ${IMGUI_HEADERS} ${IMGUI_SOURCE} "Tests/AssetManager/TextureManagerTest.h" )

target_link_libraries( Engine
	${TARGET_LINK_LIBS}   
    ${PHYSX_LIBS} 
   # PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/Engine.dir/${CMAKE_CFG_INTDIR}/Engine.res # This is the only way I could figure out how to link the compiled resource file.
)

target_link_libraries( EngineBin
	${TARGET_LINK_LIBS}
    ${PHYSX_LIBS} 
   # PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/Engine.dir/${CMAKE_CFG_INTDIR}/Engine.res # This is the only way I could figure out how to link the compiled resource file.
)

target_include_directories( Engine
    ${TARGET_INCLUDES}
)


target_include_directories( EngineBin
    ${TARGET_INCLUDES}
)

# Enable precompiled header files.
#set_source_files_properties( ${SOURCE_FILES} 
#    PROPERTIES 
#        COMPILE_FLAGS /Yu"EnginePCH.h"
#)

#set_source_files_properties( src/EnginePCH.cpp
#    PROPERTIES
#        COMPILE_FLAGS /Yc"EnginePCH.h"
#)

#install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Shaders ${directory})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Shaders)

add_custom_target(shaders)
set_source_files_properties( ${SHADER_FILES}
    PROPERTIES
        VS_SHADER_MODEL 6_5
        VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/Shaders/%(Filename).h"
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

set_source_files_properties( Resources/Shaders/Div/GenerateMips_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_GenerateMips_CS
        ShaderType "cs"
)

set_source_files_properties( Resources/Shaders/HDR/Exposure_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_Exposure_CS
        ShaderType "cs"
)

set_source_files_properties( Resources/Shaders/HDR/Histogram_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_Histogram_CS
        ShaderType "cs"
)

set_source_files_properties( Resources/Shaders/Div/PanoToCubemap_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_PanoToCubemap_CS
        ShaderType "cs"
)

set_source_files_properties( Resources/Shaders/GBuffer/GeometryPass_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_GeometryPass_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/GBuffer/GeometryPass_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_GeometryPass_PS
        ShaderType "ps"
)

set_source_files_properties( Resources/Shaders/GBuffer/DepthPrePass_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_DepthPrePass_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/LightPass/LightPass_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_LightPass_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/LightPass/LightPass_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_LightPass_PS
        ShaderType "ps"
)

set_source_files_properties( Resources/Shaders/CompositionPass/CompositionPass_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_CompositionPass_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/CompositionPass/CompositionPass_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_CompositionPass_PS
        ShaderType "ps"
)

set_source_files_properties( Resources/Shaders/Debug/DebugTexture_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_DebugTexture_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/Debug/DebugTexture_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_DebugTexture_PS
        ShaderType "ps"
)

set_source_files_properties( Resources/Shaders/HDR/HDR_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_HDR_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/HDR/HDR_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_HDR_PS
        ShaderType "ps"
)

foreach(FILE ${SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  get_source_file_property(shadermodel ${FILE} VS_SHADER_MODEL)
  get_source_file_property(var ${FILE} VS_SHADER_VARIABLE_NAME)
  add_custom_command(TARGET shaders
                     COMMAND dxc.exe /nologo /Emain /T ${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fh ${CMAKE_CURRENT_BINARY_DIR}/Shaders/${FILE_WE}.h /Vn ${var} ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

add_dependencies(Engine shaders)
add_dependencies(EngineBin shaders)

add_custom_command(TARGET Engine POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Assets
    ${CMAKE_BINARY_DIR}/bin/Assets
)

add_custom_command(TARGET EngineBin POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Assets
    ${CMAKE_BINARY_DIR}/bin/Assets
)

add_custom_command(TARGET Engine POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Libs/WinPixEventRuntime/Bin
    ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET EngineBin POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Libs/WinPixEventRuntime/Bin
    ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET Engine POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Libs/DLSS/lib/Windows_x86_64/dev
    ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET EngineBin POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Libs/DLSS/lib/Windows_x86_64/dev
    ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET Engine POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Libs/PhysX/physx/debug
    ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET EngineBin POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/Libs/PhysX/physx/debug
    ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET EngineBin POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/Config/imgui.ini
        ${CMAKE_BINARY_DIR}/bin/imgui.ini
)

add_custom_command(TARGET Engine POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/Config/imgui.ini
        ${CMAKE_BINARY_DIR}/bin/imgui.ini
)

target_link_libraries(EngineBin PRIVATE gtest gtest_main)
target_link_libraries(Engine PRIVATE gtest gtest_main)
