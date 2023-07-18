#pragma once
#include <Assetmanager.h>
#include <gtest/gtest.h>
#include <Application.h>

class TextureManagerTest : public ::testing::Test
{
protected:
    TextureManager* textureManager_;

    TextureManagerTest()
    {
        textureManager_ = Application::Get().GetAssetManager()->m_TextureManager.get();
    }
};

TEST_F(TextureManagerTest, UnloadTexture_ValidTextureInstance_RefCountDecreased)
{
    // Arrange
    const std::wstring path = L"Assets/Textures/BlueNoise/sobol_256_4d.png";
    TextureInstance textureInstance = textureManager_->LoadTexture(path);

    // Act
    const std::optional<TextureRefCount> initialRefCount = textureManager_->GetTextureRefCount(textureInstance);
    textureInstance = TextureInstance();
    const std::optional<TextureRefCount> updatedRefCount = textureManager_->GetTextureRefCount(textureInstance);

    const auto textures = textureManager_->GetTextures();

    bool bHasValidTexture = false;
    for (const auto& texture : textures.get())
    {
        if (texture.IsValid())
        {
            bHasValidTexture = true;
            break;
        }
    }

    // Assert
    EXPECT_EQ(bHasValidTexture, false);
}

TEST_F(TextureManagerTest, IncreaseRefCount_ValidTextureInstance_RefCountIncreased)
{
    // Arrange
    const std::wstring path = L"Assets/Textures/BlueNoise/sobol_256_4d.png";
    TextureInstance textureInstance = textureManager_->LoadTexture(path);

    // Act
    const std::optional<TextureRefCount> initialRefCount = textureManager_->GetTextureRefCount(textureInstance);
    TextureInstance textureInstance1 = textureManager_->LoadTexture(path);
    const std::optional<TextureRefCount> updatedRefCount = textureManager_->GetTextureRefCount(textureInstance);

    // Assert
    EXPECT_TRUE(initialRefCount.has_value());
    EXPECT_TRUE(updatedRefCount.has_value());
    EXPECT_EQ(updatedRefCount.value(), initialRefCount.value() + 1);
    EXPECT_EQ(updatedRefCount.value(), 2);
}

TEST_F(TextureManagerTest, IncreaseAndDecreaseRefCount_ValidTextureInstance_RefCountIncreasedAndDecreased)
{
    // Arrange
    const std::wstring path = L"Assets/Textures/BlueNoise/sobol_256_4d.png";
    TextureInstance textureInstance = textureManager_->LoadTexture(path);

    // Act
    const std::optional<TextureRefCount> initialRefCount = textureManager_->GetTextureRefCount(textureInstance);
    TextureInstance textureInstance1 = textureInstance;
    const std::optional<TextureRefCount> increasedRefCount = textureManager_->GetTextureRefCount(textureInstance);
    textureInstance1 = TextureInstance();
    const std::optional<TextureRefCount> decreasedRefCount = textureManager_->GetTextureRefCount(textureInstance);

    // Assert
    EXPECT_TRUE(initialRefCount.has_value());
    EXPECT_TRUE(increasedRefCount.has_value());
    EXPECT_TRUE(decreasedRefCount.has_value());
    EXPECT_EQ(increasedRefCount.value(), initialRefCount.value() + 1);
    EXPECT_EQ(decreasedRefCount.value(), initialRefCount.value());
}

TEST_F(TextureManagerTest, MultithreadingRaceCondition_RefCountConsistency)
{
    // Arrange
    const std::wstring path = L"Assets/Textures/BlueNoise/sobol_256_4d.png";
    TextureInstance textureInstance = textureManager_->LoadTexture(path);

    std::vector<std::thread> threads;
    const int numThreads = 10;
    const int numIterations = 1000;
    std::atomic<int> counter{ 0 };

    // Act
    for (int i = 0; i < numThreads; ++i)
    {
        threads.emplace_back([&]() {
            for (int j = 0; j < numIterations; ++j)
            {
                TextureInstance textureInstance1 = textureInstance;
                counter.fetch_add(1);
            }
            });
    }

    // Wait for all threads to finish
    for (auto& thread : threads)
    {
        thread.join();
    }

    const std::optional<TextureRefCount> finalRefCount = textureManager_->GetTextureRefCount(textureInstance);

    // Assert
    EXPECT_TRUE(finalRefCount.has_value());
    EXPECT_EQ(finalRefCount.value(), 1);
    EXPECT_EQ(counter.load(), numThreads * numIterations);
}

TEST_F(TextureManagerTest, LoadTexture_Multithreading_CheckRaceCondition)
{
    // Number of threads to create
    const int numThreads = 10;

    // Path to the texture file
    const std::wstring path = L"Assets/Textures/BlueNoise/sobol_256_4d.png";

    // Function to be executed by each thread
    auto threadFunc = [&]() {
        for (int i = 0; i < 100; ++i) {
            const TextureInstance& textureInstance = textureManager_->LoadTexture(path);
            EXPECT_TRUE(textureManager_->IsTextureInstanceValid(textureInstance));
        }
    };

    // Create multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(threadFunc);
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
}
