#pragma once

#include <stdexcept>
#include <string>
#include <volk.h>

class VulkanException : public std::runtime_error {
public:
    VulkanException(const std::string& message, VkResult result)
        : std::runtime_error(message + " (VkResult: " + std::to_string(result) + ")"), error_code(result) {}

    VkResult error_code;
};