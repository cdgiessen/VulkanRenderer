// #include "ViewCamera.h"
// #include <glm/gtc/matrix_transform.hpp>

// const glm::mat4 depthReverserMatrix{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 1, 1 };


// glm::mat4 ViewCamera::UpdateProjMatrix (ProjectionType projectionType)
// {
// 	this->projectionType = projectionType;
// 	if (projectionType == ProjectionType::perspective)
// 	{
// 		// glm::lookAt(Position, Position + Front, Up);
// 	}
// 	else
// 	{
// 		// ProjectionType::orthographic
// 		// glm::ortho()
// 	}
// 	return glm::mat4 (1.0f);
// }

// glm::mat4 ViewCamera::CalcViewMatrix ()
// {
// 	// cd.at(0).view = camera->GetViewMatrix();
// 	// cd.at(0).projView = proj * cd.at(0).view;
// 	// cd.at(0).cameraDir = camera->Front;
// 	// cd.at(0).cameraPos = camera->Position;
// 	return glm::mat4 (1.0f);
// }
// glm::mat4 ViewCamera::CalcViewFrustum () { return glm::mat4 (1.0f); }

// GPU_CameraData ViewCamera::CameraData ()
// {


// 	// glm::mat4 proj = depthReverserMatrix * glm::perspective(glm::radians(45.0f),
// 	//	renderer.vulkanSwapChain.swapChainExtent.width /
// 	//(float)renderer.vulkanSwapChain.swapChainExtent.height, 	0.05f, 10000000.0f); proj[1][1] *= -1;

// 	// CameraData cd;
// 	// cd.view = camera->GetViewMatrix();
// 	// cd.projView = proj * cd.view;
// 	// cd.cameraDir = camera->Front;
// 	// cd.cameraPos = camera->Position;
// 	return GPU_CameraData{ projectionMatrix, CalcViewMatrix () };
// }