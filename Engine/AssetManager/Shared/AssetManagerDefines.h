#pragma once

#include <shared_mutex>
#include <atomic>
#include <array>

#define ASSET_MANAGER_THREAD_SAFE true

#define TEXTURE_MANAGER_MAX_TEXTURES 50000
#define MATERIAL_MANAGER_MAX_MATERIALS 10000

#if ASSET_MANAGER_THREAD_SAFE
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