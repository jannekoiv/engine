#define STB_IMAGE_IMPLEMENTATION
//#include "../Include/stb_image.h"
#include "../Include/Model.h"
#include "../Include/Device.h"
#include "stb_image.h"
#include <fstream>
#include <iostream>

uint32_t readInt(std::ifstream& file)
{
    uint32_t v = 0;
    file.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

float readFloat(std::ifstream& file)
{
    float v = 0;
    file.read(reinterpret_cast<char*>(&v), sizeof(v));
    return v;
}

std::string readString(std::ifstream& file)
{
    uint32_t len = readInt(file);
    char tmp[100];
    memset(tmp, 0, sizeof(tmp));
    file.read(tmp, len);
    return std::string(tmp);
}

Image createTextureImage(Device& device, std::string filename)
{
    const int bytesPerPixel = 4;
    int width = 0;
    int height = 0;
    int channelCount = 0;

    stbi_uc* pixels = stbi_load(filename.data(), &width, &height, &channelCount, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }

    vk::DeviceSize imageSize = width * height * bytesPerPixel;

    Buffer stagingBuffer(
        device,
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = nullptr;
    static_cast<vk::Device>(device).mapMemory(stagingBuffer.memory(), 0, imageSize, {}, &data);
    memcpy(data, pixels, imageSize);
    static_cast<vk::Device>(device).unmapMemory(stagingBuffer.memory());
    stbi_image_free(pixels);

    Image image(
        device,
        vk::Extent3D(width, height, 1),
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    image.transitionLayout(
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);

    stagingBuffer.copyToImage(image);

    image.transitionLayout(
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    return image;
}

Model createModelFromFile(
    Device& device,
    DescriptorManager& descriptorManager,
    SwapChain& swapChain,
    Image& depthImage,
    std::string filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open model file!");
    }

    std::string header = readString(file);
    if (header != "paskaformaatti 1.0") {
        throw std::runtime_error("Header file not matching!");
    }

    glm::mat4 worldMatrix;
    file.read(reinterpret_cast<char*>(&worldMatrix), sizeof(glm::mat4));

    std::string material = readString(file);
    //Image image = createTextureImage(device, material);
    Image image = createTextureImage(device, "d:/texture3.jpg");

    uint32_t vertexCount = readInt(file);
    std::vector<Vertex> vertices(vertexCount);
    for (Vertex& vertex : vertices) {
        vertex.position.x = readFloat(file);
        vertex.position.y = readFloat(file);
        vertex.position.z = readFloat(file);
        vertex.normal.x = readFloat(file);
        vertex.normal.y = readFloat(file);
        vertex.normal.z = readFloat(file);
        vertex.texCoord.x = readFloat(file);
        vertex.texCoord.y = readFloat(file);
    }

    uint32_t indexCount = readInt(file);
    std::vector<uint32_t> indices(indexCount);
    for (uint32_t& index : indices) {
        index = readInt(file);
    }

    file.close();

    return Model(
        device,
        descriptorManager,
        swapChain,
        depthImage,
        worldMatrix,
        image,
        vertices,
        indices);
}

Buffer createUniformBuffer(Device& device)
{
    std::cout << "UB created\n";
    return Buffer(
        device,
        sizeof(UniformBufferObject),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

Buffer createVertexBuffer(Device& device, std::vector<Vertex>& vertices)
{
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    Buffer stagingBuffer(
        device,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = nullptr;
    static_cast<vk::Device>(device).mapMemory(stagingBuffer.memory(), 0, bufferSize, {}, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    static_cast<vk::Device>(device).unmapMemory(stagingBuffer.memory());

    Buffer vertexBuffer(
        device,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    stagingBuffer.copy(vertexBuffer);

    std::cout << "VB created\n";
    return vertexBuffer;
}

Buffer createIndexBuffer(Device& device, std::vector<uint32_t>& indices)
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    Buffer stagingBuffer(
        device,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = nullptr;
    static_cast<vk::Device>(device).mapMemory(stagingBuffer.memory(), 0, bufferSize, {}, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    static_cast<vk::Device>(device).unmapMemory(stagingBuffer.memory());

    Buffer indexBuffer(
        device,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    stagingBuffer.copy(indexBuffer);

    std::cout << "IB created\n";
    return indexBuffer;
}

Model::Model(
    Device& device,
    DescriptorManager& descriptorManager,
    SwapChain& swapChain,
    Image& depthImage,
    glm::mat4 worldMatrix,
    Image texture,
    std::vector<Vertex> vertices,
    std::vector<uint32_t> indices)
    : mDevice(device),
      mWorldMatrix(worldMatrix),
      mTexture(texture),
      mVertexBuffer(createVertexBuffer(mDevice, vertices)),
      mIndexBuffer(createIndexBuffer(mDevice, indices)),
      mIndexCount(indices.size()),
      mUniformBuffer(createUniformBuffer(mDevice)),
      mTextureSampler(mDevice, vk::SamplerAddressMode::eClampToEdge),
      mDescriptorManager(descriptorManager),
      mDescriptorSet(createDescriptorSet(mUniformBuffer, mTexture.view(), mTextureSampler)),
      mMaterial{
          mDevice,
          swapChain,
          depthImage,
          Vertex::getBindingDescription(),
          Vertex::getAttributeDescriptions(),
          mDescriptorSet.layout(),
          "d:/Shaders/vert.spv",
          "d:/Shaders/frag.spv"}
{
}

void Model::updateUniformBuffer()
{
    void* data = static_cast<vk::Device>(mDevice).mapMemory(
        mUniformBuffer.memory(), 0, sizeof(mUniformBufferObject), {});
    memcpy(data, &mUniformBufferObject, sizeof(UniformBufferObject));
    static_cast<vk::Device>(mDevice).unmapMemory(mUniformBuffer.memory());
}

DescriptorSet Model::createDescriptorSet(
    vk::Buffer uniformBuffer, vk::ImageView textureView, vk::Sampler textureSampler)
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        {0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
        {1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}};

    DescriptorSet descriptorSet = mDescriptorManager.createDescriptorSet(bindings);

    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = textureView;
    imageInfo.sampler = textureSampler;

    descriptorSet.writeDescriptors({{0, 0, 1, &bufferInfo}, {1, 0, 1, &imageInfo}});

    return descriptorSet;
}
