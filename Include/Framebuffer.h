#pragma once

#include "../Include/Base.h"

class Device;
class SwapChain;
class Image;

class FramebufferSet {
public:
    FramebufferSet(
        Device& device, SwapChain& swapChain, Image& depthImage, vk::AttachmentLoadOp loadOp);

    ~FramebufferSet();

    vk::RenderPass renderPass()
    {
        return mRenderPass;
    }

    vk::Framebuffer frameBuffer(int index)
    {
        return mFramebuffers[index];
    }

    size_t framebufferCount()
    {
        return mFramebuffers.size();
    }

private:
    vk::RenderPass mRenderPass;
    std::vector<vk::Framebuffer> mFramebuffers;
};
