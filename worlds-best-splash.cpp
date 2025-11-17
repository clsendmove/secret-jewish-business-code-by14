Vector c_projectile::splash(c_base_entity* ent, const Vector& pos) {

  if (!g_vars->projectile.m_splash)
    return pos;

  ray_t traceRay = {};
  c_game_trace trace = {};
  static c_trace_filter_world_and_props_only traceFilter = {};
  traceFilter.m_skip = ent;

  Vector mins{ent->mins()};
  Vector maxs{ent->maxs()};
  auto center{pos + Vector(0.0f, 0.0f, (mins.z + maxs.z) * 0.5f)};
  std::vector<Vector> end;

  f32 range = 146 * g_vars->projectile.m_scale;
  i32 sample = g_vars->projectile.m_sample;
  auto sphere = points(range, sample);

  for (auto& p : sphere) {
    auto poss = center + p;

    traceRay.init(center, poss);
    g_tf2.m_trace->trace_ray(traceRay, CONTENTS_SOLID, &traceFilter, &trace); 
    auto test = g_tf2.m_trace->point_contents(poss);   

    if (!(test & CONTENTS_SOLID)) 
      continue;

    end.emplace_back(trace.endpos);
  }

  if (sphere.size() > end.size()) {
    sphere.resize(end.size());
  }

 return Vector();
}
