#pragma once
#include <Texture.h>
#include <StaticDescriptorHeap.h>
#include <AssetManagerDefines.h>
#include <event.hpp>

class TextureManager;

// The TextureManagerAccess class serves as an exclusive access point for creating the TextureManager class. 
// It restricts the creation of TextureManager instances to its friend classes, specifically the AssetManager class in this context.
class TextureManagerAccess
{
    friend class AssetManager;
private:
    static std::unique_ptr<TextureManager> CreateTextureManager(const SRVHeapData& srvHeapData);
};

// The TextureManager class utilizes the Factory and Flyweight design patterns, and follows a data-driven design for 
// cache efficiency, this is better for wait free multithreading and CPU-GPU synchronization. It is responsible for creating Textures & Texture instances, 
// managing their lifetimes. The TextureManager also uses mutexes for thread safety, which can be turned off for a wait-free multi threaded system.
class TextureManager
{  
    friend struct TextureInstance;
    friend class TextureManagerAccess;
    friend class TextureManagerTest;

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
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetTextures() const;

    // Returns a reference to the GPU handles if thread safety is off, otherwise returns a copy.
    // This is because returning a reference would allow for potential race conditions.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetTextureGPUHandles() const;

    const Texture* GetTexture(const TextureInstance& textureInstance) const;
    const std::optional<TextureGPUHandle> GetTextureGPUHandle(const TextureInstance& textureInstance) const;
    const std::optional<TextureRefCount> GetTextureRefCount(const TextureInstance& textureInstance) const;

    // Attach to events.
    template<typename TClass, typename TRet, typename ...Args>
    void AttachToTextureCreatedEvent(TRet(TClass::* func) (Args...), TClass* obj)
    {
        textureInstanceCreatedEvent.attach(func, *obj);
    }

    template<typename TClass, typename TRet, typename ...Args>
    void AttachToTextureDeletedEvent(TRet(TClass::* func) (Args...), TClass* obj)
    {
        textureInstanceDeletedEvent.attach(func, *obj);
    }

private:
    // Methods for internal use
    const TextureInstance& CreateTexture(const std::wstring& path);
    void IncreaseRefCount(const TextureID textureID);
    void DecreaseRefCount(const TextureID textureID);
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
        std::vector<TextureGPUHandle> gpuHandles;
        std::array<std::atomic<TextureRefCount>, TEXTURE_MANAGER_MAX_TEXTURES> refCounts;

        // Mutexes for thread safety
        CREATE_MUTEX(textureInstanceMap);
        CREATE_MUTEX(textures);
        CREATE_MUTEX(gpuHandles);
    } textureRegistry;

    event::event<void(const TextureInstance&)> textureInstanceCreatedEvent;
    event::event<void(const TextureID&)> textureInstanceDeletedEvent;

    // Released textureIDs, this is used to re-populate the released memory in the Texture Registry
    std::vector<TextureID> releasedTextureIDs;
    CREATE_MUTEX(releasedTextureIDs);

    const SRVHeapData& m_SrvHeapData;
};

template <>
inline auto TextureManager::GetTextures<true>() const
{
    SHARED_LOCK(Textures, textureRegistry.texturesMutex);
    return textureRegistry.textures; //TODO: return a copy. This isn't thread safe.
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