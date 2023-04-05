#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/euler_angles.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// lib
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
