#include "vt_render_pass.hpp"

namespace vt 
{
	VkRenderPass vt::VtRenderPass::getRenderPass()
	{
		return renderPass;
	}

	VkFramebuffer vt::VtRenderPass::getFramebuffer(int index)
	{
		return framebuffers[index];
	}

	void vt::VtRenderPass::cleanFramebuffer()
	{
		for (auto framebuffer : framebuffers) {
			vkDestroyFramebuffer(device.device(), framebuffer, nullptr);
		}
	}

	VtRenderPass::VtRenderPass(VtDevice& deviceRef, std::shared_ptr<VtSwapChain> swapchainRef) : device{deviceRef},
		swapchain{swapchainRef}
	{

	}

	vt::VtRenderPass::~VtRenderPass()
	{
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
		cleanFramebuffer();
		vkDestroyRenderPass(device.device(), renderPass, nullptr);
	}

}

