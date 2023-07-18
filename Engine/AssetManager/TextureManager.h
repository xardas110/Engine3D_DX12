#pragma once
#include <Texture.h>
#include <StaticDescriptorHeap.h>
#include <shared_mutex>
#include <atomic>
#include <array>

#define TEXTURE_MANAGER_THREAD_SAFE true
#define TEXTURE_MANAGER_MAX_TEXTURES 50000

#if TEXTURE_MANAGER_THREAD_SAFE
#define CREATE_MUTEX(type) mutable std::shared_mutex type##Mutex
#define UNIQUE_LOCK(type, mutex) std::unique_lock<std::shared_mutex> lock##type(mutex)
#define SHARED_LOCK(type, mutex) std::shared_lock<std::shared_mutex> lock##type(mutex)
#define UNIQUE_UNLOCK(type) lock##type.unlock();
#define SHARED_UNLOCK(type) lock##type.unlock_shared();
#define SCOPED_UNIQUE_LOCK(...) std::scoped_lock lock##type(__VA_ARGS__)
#else
#define UNLOCK_MUTEX(mutex)
#define UNIQUE_UNLOCK(type);
#define SHARED_UNLOCK(type);
#define UNIQUE_LOCK(type, mutex)
#define SHARED_LOCK(type, mutex)
#define CREATE_MUTEX(type)
#define SCOPED_UNIQUE_LOCK(...)
#endif

// The TextureManager class utilizes the Factory and Flyweight design patterns, and follows a data-driven design for 
// cache efficiency, easier impl for wait free multithreading and CPU-GPU synchronization. It is responsible for creating Textures & Texture instances, 
// managing their lifetimes. The TextureManager also uses mutexes for thread safety, which can be turned off for a wait-free system.

class TextureManager;

class TextureManagerAccess
{
    friend class AssetManager;

    static std::unique_ptr<TextureManager> CreateTextureManager(const SRVHeapData& srvHeapData);
};

class TextureManager
{  
    friend struct TextureInstance;
    friend class TextureManagerAccess;

    explicit TextureManager(const SRVHeapData& srvHeapData);

public:
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    // Public interface for managing textures
    const TextureInstance& LoadTexture(const std::wstring& path);
    bool IsTextureInstanceValid(const TextureInstance& textureInstance) const;

    // Returns a reference to the textures if thread safety is off, otherwise returns a copy.
    // This is because returning a reference would allow for potential race conditions.
    template <bool ThreadSafe = TEXTURE_MANAGER_THREAD_SAFE>
    auto GetTextures() const;

    // Returns a reference to the GPU handles if thread safety is off, otherwise returns a copy.
    // This is because returning a reference would allow for potential race conditions.
    template <bool ThreadSafe = TEXTURE_MANAGER_THREAD_SAFE>
    auto GetTextureGPUHandles() const;

    const Texture* GetTexture(const TextureInstance& textureInstance) const;
    const std::optional<TextureGPUHandle> GetTextureGPUHandle(const TextureInstance& textureInstance) const;
    const std::optional<TextureRefCount> GetTextureRefCount(const TextureInstance& textureInstance) const;

private:
    // Methods for internal use
    const TextureInstance& CreateTexture(const std::wstring& path);
    void IncreaseRefCount(const TextureID textureID);
    void DecreaseRefCount(const TextureID textureID);
    //TODO: Free the heap on Texture Release
    void ReleaseTexture(const TextureID textureID);

    // The registry that holds texture data
    struct TextureRegistry
    {
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
        std::array<std::atomic<TextureRefCount>, TEXTURE_MANAGER_MAX_TEXTURES> refCounts;
        std::vector<TextureGPUHandle> gpuHandles;

        // Mutexes for thread safety
        CREATE_MUTEX(textureInstanceMap);
        CREATE_MUTEX(textures);
        CREATE_MUTEX(gpuHandles);
    } textureRegistry;

    // Member variables
    std::vector<TextureID> releasedTextureIDs;
    CREATE_MUTEX(releasedTextureIDs);

    const SRVHeapData& m_SrvHeapData;
};

template <>
inline auto TextureManager::GetTextures<true>() const
{
    SHARED_LOCK(Textures, textureRegistry.texturesMutex);
    return std::ref(textureRegistry.textures); //TODO: return a copy. This isn't thread safe.
}

template <>
inline auto TextureManager::GetTextures<false>() const
{
    return std::ref(textureRegistry.textures);
}

template <>
inline auto TextureManager::GetTextureGPUHandles<true>() const
{
    SHARED_LOCK(GPUHandles, textureRegistry.gpuHandlesMutex);
    return textureRegistry.gpuHandles;
}

template <>
inline auto TextureManager::GetTextureGPUHandles<false>() const
{
    return std::ref(textureRegistry.gpuHandles); 
}