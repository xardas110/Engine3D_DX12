#pragma once

/*
 *  Copyright(c) 2018 Jeremiah van Oosten
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files(the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions :
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

/**
 *  @file GUI.h
 *  @date October 24, 2018
 *  @author Jeremiah van Oosten
 *
 *  @brief Wrapper for ImGui. This class is used internally by the Window class.
 */

#include <imgui.h>

#include <d3dx12.h>
#include <wrl.h>
#include <DynamicDescriptorHeap.h>
#include <Texture.h>

class CommandList;
class Texture;
class RenderTarget;
class RootSignature;
class Window;

#define DEBUG_IMGUI

class GUI
{
    friend class World;
    friend class Editor;
public:
    GUI();
    virtual ~GUI();

    // Initialize the ImGui context.
    virtual bool Initialize( std::shared_ptr<Window> window );

    // Begin a new frame.
    virtual void NewFrame();
    virtual void Render( std::shared_ptr<CommandList> commandList, const RenderTarget& renderTarget);

    // Destroy the ImGui context.
    virtual void Destroy();

protected:

private:
    void GetNextHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE &cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle);
    void ResetHeapHandle();

    ImGuiContext* m_pImGuiCtx;
    std::shared_ptr<Window> m_Window;

    //Max 24 windows for now(assigned inside virtual bool Initialize( std::shared_ptr<Window> window );)
    ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
    int nextHeapHandle = 0; //First handle is for the main window
};