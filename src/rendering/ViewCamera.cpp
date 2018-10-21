#include "ViewCamera.h"

GPU_CameraData ViewCamera::CameraData ()
{


	// glm::mat4 proj = depthReverserMatrix * glm::perspective(glm::radians(45.0f),
	//	renderer.vulkanSwapChain.swapChainExtent.width /
	//(float)renderer.vulkanSwapChain.swapChainExtent.height, 	0.05f, 10000000.0f); proj[1][1] *= -1;

	// CameraData cd;
	// cd.view = camera->GetViewMatrix();
	// cd.projView = proj * cd.view;
	// cd.cameraDir = camera->Front;
	// cd.cameraPos = camera->Position;
	return GPU_CameraData{};
}