void c_move_sim::strafe() {
    i32 idx = m_player->entidx();
    auto &record = m_velocity_map[idx];
    i32 size = m_velocity_map[idx].size() - 1;

    auto init = m_move_data.m_vecVelocity;
    init.x /= init.length2d();
    init.y /= init.length2d();

    f32 correct{0.f};

    for (auto &strafe: record) {

        strafe.x /= strafe.length2d();
        strafe.y /= strafe.length2d();

        f32 sin_theta = init.x * strafe.y - init.y * strafe.x; // cross product
        f32 cos_theta = init.x * strafe.x + init.y * strafe.y; // dot product

        f32 delta = atan2(sin_theta, cos_theta);
        correct += delta;
    }

    correct /= size;
    strafe_ang = correct;
}
