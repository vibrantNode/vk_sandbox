// player_input.h

#pragma once
#include "interfaces/window_input_i.h"
#include "transform_component.h"
#include <glm/glm.hpp>
#include <memory>

class SandboxMNKController {
public:
    SandboxMNKController(float moveSpeed = 5.f, float mouseSensitivity = 0.1f);

    void update(float dt, std::shared_ptr<IWindowInput> input, TransformComponent& transform);
    void mouseCallback(glm::vec2 delta);
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }

private:
    float m_moveSpeed;
    float m_mouseSensitivity;
    float m_yaw;
    float m_pitch;

    glm::vec2 m_mouseDelta{ 0.f };
};