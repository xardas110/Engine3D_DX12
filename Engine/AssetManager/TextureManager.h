#pragma once
#include <Texture.h>
#include <StaticDescriptorHeap.h>
#include <shared_mutex>

// The registry that holds texture data
struct TextureRegistry 
{
    friend class TextureManager;

private:
    // Private constructor to prevent external instantiation
    TextureRegistry() = default;

    // Disable copy and move operations
    TextureRegistry(const TextureRegistry&) = delete;
    TextureRegistry& operator=(const TextureRegistry&) = delete;
    TextureRegistry(TextureRegistry&&) = delete;
    TextureRegistry& operator=(TextureRegistry&&) = delete;

    // Texture data containers
    std::unordered_map<std::wstring, TextureInstance> textureInstanceMap;
    std::vector<Texture> textures;
    std::vector<TextureRefCount> refCounts;
    std::vector<TextureGPUHandle> gpuHandles;

    // Mutexes for thread safety
    mutable std::shared_mutex textureInstanceMapMutex;
    mutable std::shared_mutex texturesMutex;
    mutable std::shared_mutex refCountsMutex;
    mutable std::shared_mutex gpuHandlesMutex;
};

class TextureManager 
{
    friend class AssetManager;
    friend struct TextureInstance;

    explicit TextureManager(const SRVHeapData& srvHeapData);

public:

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    // Public interface for managing textures
    const TextureInstance& LoadTexture(const std::wstring& path);
    bool IsTextureInstanceValid(const TextureInstance& textureInstance) const;
    const std::vector<Texture>& GetTexturesNotThreadSafe() const;
    const std::vector<Texture> GetTexturesThreadSafe() const;
    const std::vector<TextureGPUHandle>& GetTextureGPUHandlesNotThreadSafe() const;
    const std::vector<TextureGPUHandle> GetTextureGPUHandlesThreadSafe() const;
    const Texture* GetTexture(const TextureInstance& textureInstance) const;
    const std::optional<TextureGPUHandle> GetTextureGPUHandle(const TextureInstance& textureInstance) const;
    const std::optional<TextureRefCount> GetTextureRefCount(const TextureInstance& textureInstance) const;

private:
    // Methods for internal use
    const TextureInstance& CreateTexture(const std::wstring& path);
    void IncreaseRefCount(const TextureID textureID);
    void DecreaseRefCount(const TextureID textureID);

    // Member variables
    std::vector<TextureID> releasedTextureIDs;
    mutable std::shared_mutex releasedTextureIDsMutex;

    TextureRegistry textureRegistry;

    const SRVHeapData& m_SrvHeapData;
};
