#include "../Include/Engine.h"
#include <fstream>

GLFWwindow* initWindow(const int width, const int height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window =
        glfwCreateWindow(width, height, "Totaalinen Yliruletus Rendering Engine", nullptr, nullptr);
    //glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    return window;
}

int initPaskaa()
{
    std::cout << "PASKAA\n";
    return 0;
}

Engine::Engine(const int width, const int height, const bool enableValidationLayers)
    : mWindow{initWindow(width, height)},
      mDevice{mWindow, enableValidationLayers},
      mSwapChain{mDevice},
      mDepthTexture{
          mDevice,
          vk::ImageViewType::e2D,
          1,
          vk::Extent3D{mSwapChain.extent().width, mSwapChain.extent().height, 1},
          findDepthAttachmentFormat(mDevice),
          vk::ImageTiling::eOptimal,
          vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
          vk::MemoryPropertyFlagBits::eDeviceLocal,
          vk::SamplerAddressMode::eClampToEdge},
      mDescriptorManager{mDevice},
      mTextureManager{mDevice},
      mRenderer{mDevice, mSwapChain, mDepthTexture},
      mSkybox{mDevice, mDescriptorManager, mTextureManager, mSwapChain, mDepthTexture},
      mLight{mDevice, mDescriptorManager, mTextureManager, mSwapChain},
      mQuad{mDevice, mDescriptorManager, mTextureManager, mSwapChain, mLight.depthTexture()}
{
    std::cout << "Engine initialized\n";
}

Engine::~Engine()
{
}

Mesh Engine::createModelFromFile(std::string filename)
{
    return ::createMeshFromFile(
        mDevice,
        mDescriptorManager,
        mTextureManager,
        mSwapChain,
        mDepthTexture,
        &mLight.depthTexture(),
        filename);
}

void Engine::drawFrame(std::vector<Mesh>& models)
{
    mCamera.update();
    const glm::mat4& world = mLight.worldMatrix();
    for (Mesh& model : models) {
        model.updateUniformBuffer(
            mCamera.viewMatrix(),
            mCamera.projMatrix(),
            mLight.projMatrix() * mLight.viewMatrix(),
            {world[2][0], world[2][1], world[2][2]});
    }

    mSkybox.updateUniformBuffer(glm::mat4(glm::mat3(mCamera.viewMatrix())), mCamera.projMatrix());
    mQuad.updateUniformBuffer();

    mLight.drawFrame(models, mSwapChain.extent());
    mRenderer.drawFrame(models, mSkybox, mQuad, mLight);
}
