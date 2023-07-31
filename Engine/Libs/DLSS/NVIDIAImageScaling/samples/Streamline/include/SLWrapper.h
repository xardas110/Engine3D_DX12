// The MIT License(MIT)
//
// Copyright(c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files(the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include <climits>
#include <sl.h>
#include <sl_nis.h>
#include <d3d12.h>

class SLWrapper
{
private:
    static bool m_sl_initialized;
    static bool m_nis_available;
    sl::NISConstants m_nis_consts;
    static void LogFunctionCallback(sl::LogType type, const char* msg);
public:
    SLWrapper();
    static void Initialize();
    static void Shutdown();
    bool CheckSupportNIS();
    void SetNISConsts(const sl::NISConstants& consts, int frameNumber);
    bool IsNISAvailable() { return m_nis_available; }
    bool IsSLInitialized() { return m_sl_initialized; }
    void EvaluateNIS(ID3D12GraphicsCommandList* context, ID3D12Resource* unresolvedColor, ID3D12Resource* resolvedColor);
};