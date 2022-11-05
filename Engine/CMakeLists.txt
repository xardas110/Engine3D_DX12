cmake_minimum_required( VERSION 3.12.1 ) # Latest version of CMake when this file was created.

add_compile_definitions(DEBUG_CMD)

set( HEADER_FILES 
    "core/EnginePCH.h"
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
    "core/Events.h"
    "core/Game.h"
    "DX12/GenerateMipsPSO.h"
    "core/GUI.h"
    "core/Helpers.h"
    "core/HighResolutionClock.h"
    "DX12/IndexBuffer.h"
    "core/KeyCodes.h"
    "DX12/Mesh.h"
    "DX12/PanoToCubemapPSO.h"
    "DX12/RenderTarget.h"
    "DX12/Resource.h"
    "DX12/ResourceStateTracker.h"
    "DX12/RootSignature.h"
    "DX12/StructuredBuffer.h"
    "DX12/Texture.h"
    "DX12/TextureUsage.h"
    "core/ThreadSafeQueue.h"
    "DX12/UploadBuffer.h"
    "DX12/VertexBuffer.h"
    "core/Window.h"
    "DX12/d3dx12.h"
    "core/Camera.h"
    "DX12/DeferredRenderer.h"
    "core/AssetManager.h"
    "DX12/PipelineManager.h"
    "core/Components.h"
    "core/Transform.h"
    "Physics/PhysicsSystem.h"
)

set( SOURCE_FILES
    "core/EnginePCH.cpp"
    "core/Application.cpp"
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
    "DX12/GUI.cpp"
    "core/HighResolutionClock.cpp"
    "DX12/IndexBuffer.cpp"
    "DX12/Mesh.cpp"
    "DX12/PanoToCubemapPSO.cpp"
    "DX12/RenderTarget.cpp"
    "DX12/Resource.cpp"
    "DX12/ResourceStateTracker.cpp"
    "DX12/RootSignature.cpp"
    "DX12/StructuredBuffer.cpp"
    "DX12/Texture.cpp"
    "DX12/UploadBuffer.cpp"
    "DX12/VertexBuffer.cpp"
    "core/Window.cpp"
    "core/Camera.cpp"
    "DX12/DeferredRenderer.cpp"
    "core/AssetManager.cpp"
    "DX12/PipelineManager.cpp"
    "Physics/PhysicsSystem.cpp"
)

set( IMGUI_HEADERS
    external/ImGui/imgui.h
    external/ImGui/imconfig.h
    external/ImGui/imgui_impl_win32.h
)

source_group( "Header Files\\ImGUI" FILES ${IMGUI_HEADERS} )

set( IMGUI_SOURCE
    "external/ImGui/imgui.cpp"
    "external/ImGui/imgui_demo.cpp"
    "external/ImGui/imgui_draw.cpp"
    "external/ImGui/imgui_widgets.cpp"
    "external/ImGui/imgui_impl_win32.cpp"
)

set( ENTT_HEADERS
    external/entt/entt.h
)

source_group( "Source Files\\ImGUI" FILES ${IMGUI_SOURCE} )

set( SHADER_FILES
    Resources/Shaders/GenerateMips_CS.hlsl
    Resources/Shaders/PanoToCubemap_CS.hlsl
    Resources/Shaders/ImGUI_VS.hlsl
    Resources/Shaders/ImGUI_PS.hlsl
    Resources/Shaders/GeometryMesh_VS.hlsl
    Resources/Shaders/GeometryMesh_PS.hlsl
)

set( RESOURCE_FILES 
    Engine.rc
    Resources/Icon/app_icon.ico
	${SHADER_FILES}
)

source_group( "Resources" FILES Engine.rc resource.h )
source_group( "Resources\\Icon" FILES Resources/Icon/app_icon.ico )
source_group( "Resources\\Shaders" FILES ${SHADER_FILES} )

add_library( Engine STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${RESOURCE_FILES}
    ${IMGUI_HEADERS} ${IMGUI_SOURCE}
)

target_include_directories( Engine
    PUBLIC DX12
    PUBLIC external
    PUBLIC external/ImGui
    PUBLIC core
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/Shaders"
)

target_link_libraries( Engine 
	PUBLIC DirectXTex
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
   # PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/Engine.dir/${CMAKE_CFG_INTDIR}/Engine.res # This is the only way I could figure out how to link the compiled resource file.
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
        VS_SHADER_MODEL 5_1
        VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/Shaders/%(Filename).h"
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

set_source_files_properties( Resources/Shaders/GenerateMips_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_GenerateMips_CS
        ShaderType "cs"
)

set_source_files_properties( Resources/Shaders/PanoToCubemap_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_PanoToCubemap_CS
        ShaderType "cs"
)

set_source_files_properties( Resources/Shaders/ImGUI_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_ImGUI_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/ImGUI_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_ImGUI_PS
        ShaderType "ps"
)

set_source_files_properties( Resources/Shaders/GeometryMesh_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_GeometryMesh_VS
        ShaderType "vs"
)

set_source_files_properties( Resources/Shaders/GeometryMesh_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_GeometryMesh_PS
        ShaderType "ps"
)

foreach(FILE ${SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  get_source_file_property(shadermodel ${FILE} VS_SHADER_MODEL)
  get_source_file_property(var ${FILE} VS_SHADER_VARIABLE_NAME)
  add_custom_command(TARGET shaders
                     COMMAND fxc.exe /nologo /Emain /T ${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fh ${CMAKE_CURRENT_BINARY_DIR}/Shaders/${FILE_WE}.h /Vn ${var} ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

add_dependencies(Engine shaders)