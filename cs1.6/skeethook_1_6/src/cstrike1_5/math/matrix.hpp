#pragma once

#include <cstrike1_5/math/vector.hpp>

struct view_matrix_t
{
    float data[4][4];

    view_matrix_t()
    {
        for (int row = 0; row < 4; row++)
            for (int column = 0; column < 4; column++)
                data[row][column] = (row == column) ? 1.0f : 0.0f;
    }

    bool wts(const vec3_t& world_pos, vec2_t& screen_pos, float screen_width, float screen_height) const
    {
        const float w = data[3][0] * world_pos.x + data[3][1] * world_pos.y + data[3][2] * world_pos.z + data[3][3];
        if (w < 0.001f)
            return false;

        const float ndc_x = data[0][0] * world_pos.x + data[0][1] * world_pos.y + data[0][2] * world_pos.z + data[0][3];
        const float ndc_y = data[1][0] * world_pos.x + data[1][1] * world_pos.y + data[1][2] * world_pos.z + data[1][3];

        screen_pos.x = (screen_width * 0.5f) + (ndc_x * screen_width * 0.5f);
        screen_pos.y = (screen_height * 0.5f) - (ndc_y * screen_height * 0.5f);

        return true;
    }
};

struct matrix3x4_t
{
    matrix3x4_t() = default;

    constexpr matrix3x4_t(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23)
    {
        array_data[0][0] = m00;
        array_data[0][1] = m01;
        array_data[0][2] = m02;
        array_data[0][3] = m03;
        array_data[1][0] = m10;
        array_data[1][1] = m11;
        array_data[1][2] = m12;
        array_data[1][3] = m13;
        array_data[2][0] = m20;
        array_data[2][1] = m21;
        array_data[2][2] = m22;
        array_data[2][3] = m23;
    }

    float* operator[](int index) { return array_data[index]; }
    const float* operator[](int index) const { return array_data[index]; }

    float array_data[3][4];
};
