#include "ViewCamera.h"

#include "Device.h"


const cml::vec3f WorldUp{ 0, 1, 0 };

cml::mat4f ViewCameraManager::GetViewMat (ViewCameraID id)
{
	ViewCameraData& cam = camera_data.at (id);
	if (isViewMatDirty)
	{
		isViewMatDirty = false;
		cml::vec3f front = cml::vec3f::forward;
		// cml::vec3f front = cml::normalize (rotation * cml::vec3f{ 0, 0, 1 });

		cml::vec3f right = cml::normalize (cml::cross (front, WorldUp));
		cml::vec3f up = cml::normalize (cml::cross (right, front));

		mat_view = cml::lookAt (position, position + front, up);
	}
	return mat_view;
}
cml::mat4f ViewCameraManager::GetProjMat ()
{
	if (isProjMatDirty)
	{
		isProjMatDirty = false;
		if (type == CamType::perspective)
		{
			mat_proj = cml::perspective (fov, aspect, clip_near, clip_far);
		}
		else
		{
			mat_proj = cml::ortho (-size, size, -size, size, clip_near, clip_far);
		}
	}
	return mat_proj;
}

cml::mat4f ViewCameraData::GetProjViewMat (ViewCameraID id)
{
	if (isProjMatDirty || isViewMatDirty)
	{
		mat_projView = GetProjMat () * GetViewMat ();
	}
	return mat_projView;
}

cml::vec3f ViewCameraData::GetPosition () { return position; }
cml::quatf ViewCameraData::GetRotation () { return rotation; }

void ViewCameraData::SetPosition (cml::vec3f position)
{
	isViewMatDirty = true;
	position = position;
}
void ViewCameraData::SetRotation (cml::quatf rotation)
{
	isViewMatDirty = true;
	rotation = rotation;
}

void ViewCameraData::SetFOV (float fov)
{
	isProjMatDirty = true;
	fov = fov;
}
void ViewCameraData::SetAspectRatio (float aspect)
{
	isProjMatDirty = true;
	aspect = aspect;
}
void ViewCameraData::SetViewSize (float size)
{
	isProjMatDirty = true;
	size = size;
}

void ViewCameraData::SetClipNear (float near)
{
	isProjMatDirty = true;
	clip_near = near;
}
void ViewCameraData::SetClipFar (float far)
{
	isProjMatDirty = true;
	clip_far = far;
}

ViewCameraManager::ViewCameraManager (VulkanDevice& device)
: device (device),
  data_buffers (device, uniform_details (sizeof (CameraGPUData) * MaxCameraCount)){


  };

ViewCameraID ViewCameraManager::Create (CameraType type, cml::vec3f position, cml::quatf rotation)
{
	std::lock_guard lg (lock);
	for (ViewCameraID i = 0; i < camera_data.size (); i++)
	{
		if (camera_data.at (i).is_active == false)
		{
			camera_data.at (i).is_active = true;
			SetupViewCamera (id, type, position, rotation);
			return i;
		}
	}
	return -1; // assume theres enough cameras available
}
void ViewCameraManager::Delete (ViewCameraID id)
{
	std::lock_guard lg (lock);

	camera_data.at (id).is_active = false;
}


void ViewCameraData::SetPosition (ViewCameraID id, cml::vec3f position)
{
	camera_data.at (i).SetPosition (position);
}
void ViewCameraData::SetRotation (ViewCameraID id, cml::quatf rotation)
{
	camera_data.at (i).SetRotation (position);
}
void ViewCameraData::SetFOV (ViewCameraID id, float fov) { camera_data.at (i).SetFOV (fov); }
void ViewCameraData::SetAspectRatio (ViewCameraID id, float aspect)
{
	camera_data.at (i).SetAspectRatio (aspect);
}
void ViewCameraData::SetViewSize (ViewCameraID id, float size)
{
	camera_data.at (i).SetViewSize (size);
}
void ViewCameraData::SetClipNear (ViewCameraID id, float near)
{
	camera_data.at (i).SetClipNear (near);
}
void ViewCameraData::SetClipFar (ViewCameraID id, float far)
{
	camera_data.at (i).SetClipFar (far);
}


void ViewCameraManager::UpdateGPUBuffer (int index)
{
	std::vector<CameraGPUData> data;
	data.reserve (MaxCameraCount);
	for (auto& cam : camera_data)
	{
		CameraGPUData gpu_cam;
		gpu_cam.projView = cam.GetViewProjMat ();
		gpu_cam.view = cam.GetViewMat ();
		gpu_cam.cameraPos = cam.GetPosition ();
		gpu_cam.cameraDir = cam.GetRotation ();

		data.push_back (gpu_cam);
	}
	data_buffers.Write ().CopyToBuffer (data);
}

DescriptorResource GetDescriptorSet (int index, ViewCameraID id) {}


void ViewCameraManager::SetupViewCamera (ViewCameraID id, CameraType type, cml::vec3f position, cml::quatf rotation)
{
	camera_data.at (id).type = type;
	camera_data.at (id).position = position;
	camera_data.at (id).rotation = rotation;
	camera_data.at (id).isProjMatDirty = true;
	camera_data.at (id).isViewMatDirty = true;
}