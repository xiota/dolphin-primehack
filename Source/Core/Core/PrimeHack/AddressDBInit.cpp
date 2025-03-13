#include "Core/PrimeHack/AddressDB.h"

namespace prime {

void init_db(AddressDB& addr_db) {
  using rt = AddressDB::region_triple;
  const rt rt0 = rt(0, 0, 0);
  const auto mrt1 = [](u32 offset_all) -> rt {
    return rt(offset_all, offset_all, offset_all);
  };

  addr_db.register_address(Game::PRIME_1, "arm_cannon_matrix", 0x804c10ec, 0x804c502c, 0x804c136c);
  addr_db.register_address(Game::PRIME_1, "state_manager", 0x804bf420, 0x804c3360, 0x804bf6a0);
  addr_db.register_address(Game::PRIME_1, "static_fov_fp", 0x805c0e38, 0x805c5178, 0x80641138);
  addr_db.register_address(Game::PRIME_1, "static_fov_tp", 0x805c0e3c, 0x805c517c, 0x8064113c);
  addr_db.register_address(Game::PRIME_1, "gun_pos", 0x804ddae4, 0x804e1a24, 0x804d398c);
  addr_db.register_address(Game::PRIME_1, "control_flag", 0x8052e9b8, 0x80532b38, 0x805aecb8);
  addr_db.register_address(Game::PRIME_1, "tweakplayer", 0x804ddff8, 0x804e1f38, 0x804de278);
  addr_db.register_address(Game::PRIME_1, "beamvisor_menu_base", 0x805c28b0, 0x805c6c34, 0x80642b98);
  addr_db.register_address(Game::PRIME_1, "cursor_base", 0x805c28a8, 0x805c6c2c, 0x80642b90);
  addr_db.register_address(Game::PRIME_1, "powerups_array_base", 0x804bfcd4, 0x804c3c14, 0x804bff54);
  addr_db.register_address(Game::PRIME_1, "powerups_size", 8, 8, 8);
  addr_db.register_address(Game::PRIME_1, "powerups_offset", 0x30, 0x30, 0x30);
  addr_db.register_address(Game::PRIME_1, "holster_timer_offset", 0x4, 0x4, 0x4);
  addr_db.register_address(Game::PRIME_1, "transform_offset", 0x2c, 0x2c, 0x2c);
  addr_db.register_dynamic_address(Game::PRIME_1, "object_list", "state_manager", {mrt1(0x810), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1, "menu_state", "state_manager",  {mrt1(0x117C)});
  addr_db.register_dynamic_address(Game::PRIME_1, "player", "state_manager", {mrt1(0x84c), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1, "firstperson_pitch", "player", {mrt1(0x3dc)});
  addr_db.register_dynamic_address(Game::PRIME_1, "angular_momentum", "player", {mrt1(0x118)});
  addr_db.register_dynamic_address(Game::PRIME_1, "angular_vel", "player", {mrt1(0x154)});
  addr_db.register_dynamic_address(Game::PRIME_1, "ball_state", "player", {mrt1(0x2f4)});
  addr_db.register_dynamic_address(Game::PRIME_1, "orbit_state", "player", {mrt1(0x300)});
  addr_db.register_dynamic_address(Game::PRIME_1, "lockon_state", "state_manager", {mrt1(0xc93)});
  addr_db.register_dynamic_address(Game::PRIME_1, "powerups_list", "state_manager", {mrt1(0x8b4), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1, "camera_manager", "state_manager", {mrt1(0x868), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1, "cursor", "cursor_base", {rt0, rt(0xc54, 0xd04, 0xc54), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1, "beamvisor_menu_state", "beamvisor_menu_base", {rt0, rt(0x32c, 0x32c, 0x338)});
  addr_db.register_dynamic_address(Game::PRIME_1, "beamvisor_menu_mode", "beamvisor_menu_base", {rt0, rt(0x334, 0x334, 0x340)});
  addr_db.register_dynamic_address(Game::PRIME_1, "powerups_array", "powerups_array_base", {rt0, rt0});
  addr_db.register_dynamic_address(Game::PRIME_1, "active_visor", "powerups_array", {mrt1(0x1c)});
  addr_db.register_dynamic_address(Game::PRIME_1, "gun_holster_state", "player", {mrt1(0x488)});

  // angular momentum z = player+x118
  // pitch = player+x3dc
  // ignore pitch goal for now
  // angular velocity z = player+x154
  // orbit state = player+x300
  // lockon state = state_manager+xc93
  // camera UID = state mgr + 0x868
  // powerups = state mgr + 0x8b4

  addr_db.register_address(Game::PRIME_1_GCN, "state_manager", 0x8045a1a8, 0x803e2088); // camera +x870
  addr_db.register_address(Game::PRIME_1_GCN, "fov_fp_offset", -0x7ff0, -0x7fe8);
  addr_db.register_address(Game::PRIME_1_GCN, "fov_tp_offset", -0x7fec, -0x7fe4);
  addr_db.register_address(Game::PRIME_1_GCN, "gun_pos", 0x8045bce8, 0x803e3c14);
  addr_db.register_address(Game::PRIME_1_GCN, "tweak_player", 0x8045c208, 0x803e4134);
  addr_db.register_address(Game::PRIME_1_GCN, "grapple_swing_speed_offset", 0x2b0, 0x2b0);
  addr_db.register_address(Game::PRIME_1_GCN, "crosshair_color", 0x8045b678, 0x803e35a4);
  addr_db.register_address(Game::PRIME_1_GCN, "transform_offset", 0x34, 0x34);
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "world", "state_manager", {mrt1(0x850), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "player", "state_manager", {mrt1(0x84c), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "camera_manager", "state_manager", {mrt1(0x86c), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "object_list", "state_manager", {mrt1(0x810), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "menu_state", "state_manager",  {mrt1(0xf90)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "player_xf", "player", {mrt1(0x34)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "orbit_state", "player", {rt(0x304, 0x314, 0)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "angular_vel", "player", {rt(0x14c, 0x15c, 0)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "firstperson_pitch", "player", {rt(0x3ec, 0x3fc, 0)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "camera_state", "player", {rt(0x2f4, 0x304, 0)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "move_state", "player", {rt(0x258, 0x268, 0)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "freelook_rotation_speed", "tweak_player", { mrt1(0x280) });
  addr_db.register_dynamic_address(Game::PRIME_1_GCN, "air_transitional_friction", "tweak_player", { mrt1(0x180) });

  addr_db.register_address(Game::PRIME_1_GCN_R1, "state_manager", 0x8045a388);
  addr_db.register_address(Game::PRIME_1_GCN_R1, "fov_fp_offset", -0x7ff0);
  addr_db.register_address(Game::PRIME_1_GCN_R1, "fov_tp_offset", -0x7fec);
  addr_db.register_address(Game::PRIME_1_GCN_R1, "gun_pos", 0x8045bec8); // [r13-5ecc]+4c
  addr_db.register_address(Game::PRIME_1_GCN_R1, "tweak_player", 0x8045c3e8);
  addr_db.register_address(Game::PRIME_1_GCN_R2, "grapple_swing_speed_offset", 0x2b0);
  addr_db.register_address(Game::PRIME_1_GCN_R1, "crosshair_color", 0x8045b698); // [r13 - 5ec0]+1c0
  addr_db.register_address(Game::PRIME_1_GCN_R1, "transform_offset", 0x34);
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "world", "state_manager", {mrt1(0x850), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "player", "state_manager", {mrt1(0x84c), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "camera_manager", "state_manager", {mrt1(0x86c), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "object_list", "state_manager", {mrt1(0x810), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "menu_state", "state_manager",  {mrt1(0xf90)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "player_xf", "player", {mrt1(0x34)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "orbit_state", "player", {mrt1(0x304)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "angular_vel", "player", {mrt1(0x14c)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "firstperson_pitch", "player", {mrt1(0x3ec)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "camera_state", "player", {mrt1(0x2f4)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "move_state", "player", {mrt1(0x258)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "freelook_rotation_speed", "tweak_player", {mrt1(0x280)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R1, "air_transitional_friction", "tweak_player", {mrt1(0x180)});

  addr_db.register_address(Game::PRIME_1_GCN_R2, "state_manager", 0x8045b208);
  addr_db.register_address(Game::PRIME_1_GCN_R2, "fov_fp_offset", -0x7ff0);
  addr_db.register_address(Game::PRIME_1_GCN_R2, "fov_tp_offset", -0x7fec);
  addr_db.register_address(Game::PRIME_1_GCN_R2, "gun_pos", 0x8045cd48); // [r13-5ecc]+4c
  addr_db.register_address(Game::PRIME_1_GCN_R2, "tweak_player", 0x8045d268);
  addr_db.register_address(Game::PRIME_1_GCN_R2, "grapple_swing_speed_offset", 0x2b0);
  addr_db.register_address(Game::PRIME_1_GCN_R2, "crosshair_color", 0x8045c6d8); // [r13 - 5ec0]+1c0
  addr_db.register_address(Game::PRIME_1_GCN_R2, "transform_offset", 0x34);
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "world", "state_manager", {mrt1(0x850), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "player", "state_manager", {mrt1(0x84c), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "camera_manager", "state_manager", {mrt1(0x86c), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "object_list", "state_manager", {mrt1(0x810), rt0});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "menu_state", "state_manager",  {mrt1(0xf90)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "player_xf", "player", {mrt1(0x34)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "orbit_state", "player", {mrt1(0x314)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "angular_vel", "player", {mrt1(0x15c)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "firstperson_pitch", "player", {mrt1(0x3fc)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "camera_state", "player", {mrt1(0x304)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "move_state", "player", {mrt1(0x268)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "freelook_rotation_speed", "tweak_player", {mrt1(0x280)});
  addr_db.register_dynamic_address(Game::PRIME_1_GCN_R2, "air_transitional_friction", "tweak_player", {mrt1(0x180)});

  // object list = state mgr + x810
  // camera mgr = state mgr + x870
  // camera uid = camera mgr
  // player = state mgr + x84c

  addr_db.register_address(Game::PRIME_2, "state_manager", 0x804e72e8, 0x804ee738, 0x804e94a0); // +1514 = camera mgr, +153c load state
  addr_db.register_address(Game::PRIME_2, "tweakgun", 0x805cb274, 0x805d2cdc, 0x805cba54);
  addr_db.register_address(Game::PRIME_2, "world_id_ptr", 0x805081cc, 0x8050f76c);
  addr_db.register_address(Game::PRIME_2, "control_flag", 0x805373f8, 0x8053ebf8, 0x80537bb8);
  addr_db.register_address(Game::PRIME_2, "beamvisor_menu_base", 0x805cb314, 0x805d2d80, 0x805cbaec);
  addr_db.register_address(Game::PRIME_2, "cursor_base", 0x805cb2c8, 0x805d2d30, 0x805cbaa0);
  addr_db.register_address(Game::PRIME_2, "tweak_player_offset", -0x6410, -0x6368, -0x63f8);
  addr_db.register_address(Game::PRIME_2, "powerups_size", 12, 12, 12);
  addr_db.register_address(Game::PRIME_2, "powerups_offset", 0x5c, 0x5c, 0x5c);
  addr_db.register_address(Game::PRIME_2, "conn_vec_offset", 0x10, 0x10, 0x10);
  addr_db.register_address(Game::PRIME_2, "seq_timer_vec_offset", 0x34, 0x34, 0x34);
  addr_db.register_address(Game::PRIME_2, "seq_timer_fire_size", 0x14, 0x14, 0x14);
  addr_db.register_address(Game::PRIME_2, "seq_timer_time_offset", 0xc, 0xc, 0xc);
  addr_db.register_address(Game::PRIME_2, "holster_timer_offset", -0x20, -0x20, -0x20);
  addr_db.register_address(Game::PRIME_2, "transform_offset", 0x20, 0x20, 0x20);
  addr_db.register_dynamic_address(Game::PRIME_2, "player", "state_manager", {mrt1(0x14f4), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2, "object_list", "state_manager", {mrt1(0x810), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2, "menu_state", "state_manager", {mrt1(0x2C0C)});
  addr_db.register_dynamic_address(Game::PRIME_2, "camera_manager", "state_manager", {mrt1(0x1514), mrt1(0x10)});
  addr_db.register_dynamic_address(Game::PRIME_2, "load_state", "state_manager", {mrt1(0x153c)});
  addr_db.register_dynamic_address(Game::PRIME_2, "beamvisor_menu_state", "beamvisor_menu_base", {rt0, mrt1(0x340)});
  addr_db.register_dynamic_address(Game::PRIME_2, "beamvisor_menu_mode", "beamvisor_menu_base", {rt0, mrt1(0x34c)});
  addr_db.register_dynamic_address(Game::PRIME_2, "orbit_state", "player", {mrt1(0x390)});
  addr_db.register_dynamic_address(Game::PRIME_2, "lockon_state", "state_manager", {mrt1(0x1667)});
  addr_db.register_dynamic_address(Game::PRIME_2, "cursor", "cursor_base", {rt0, rt(0xc54, 0xd04, 0xc54), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2, "angular_momentum", "player", {mrt1(0x178)});
  addr_db.register_dynamic_address(Game::PRIME_2, "firstperson_pitch", "player", {mrt1(0x5f0)});
  addr_db.register_dynamic_address(Game::PRIME_2, "armcannon_matrix", "player", {mrt1(0xea8), mrt1(0x3b0)});
  addr_db.register_dynamic_address(Game::PRIME_2, "ball_state", "player", {mrt1(0x374)});
  addr_db.register_dynamic_address(Game::PRIME_2, "powerups_array", "player", {mrt1(0x12ec), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2, "active_visor", "powerups_array", {mrt1(0x34)});
  addr_db.register_dynamic_address(Game::PRIME_2, "world_id", "world_id_ptr", {rt0, rt0});
  addr_db.register_dynamic_address(Game::PRIME_2, "area_id", "state_manager", {mrt1(0x1e44)});
  addr_db.register_dynamic_address(Game::PRIME_2, "area_layers_vector", "state_manager", {mrt1(0x1e38), mrt1(0x8), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2, "gun_holster_state", "player", {mrt1(0xEA8), mrt1(0x3A4)});

  // Strangely while the holster state is stored in CPlayer in MP1, in MP2 it is placed into a new class.

  addr_db.register_address(Game::PRIME_2_GCN, "state_manager", 0x803db6e0, 0x803dc900, 0x803de690);
  addr_db.register_address(Game::PRIME_2_GCN, "tweakgun_offset", -0x6e1c, -0x6e14, -0x6ddc);
  addr_db.register_address(Game::PRIME_2_GCN, "tweak_player_offset", -0x6e3c, -0x6e34, -0x6dfc);
  addr_db.register_address(Game::PRIME_2_GCN, "tweakgui_offset", -0x6e20, -0x6e18, -0x6de0);
  addr_db.register_address(Game::PRIME_2_GCN, "conn_vec_offset", 0x14, 0x14, 0x14);
  addr_db.register_address(Game::PRIME_2_GCN, "seq_timer_vec_offset", 0x3c, 0x3c, 0x3c);
  addr_db.register_address(Game::PRIME_2_GCN, "seq_timer_fire_size", 0x18, 0x18, 0x18);
  addr_db.register_address(Game::PRIME_2_GCN, "seq_timer_time_offset", 0x10, 0x10, 0x10);
  addr_db.register_address(Game::PRIME_2_GCN, "transform_offset", 0x24, 0x24, 0x24);
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "camera_manager", "state_manager", {mrt1(0x151c), mrt1(0x14)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "object_list", "state_manager", {mrt1(0x810), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "world", "state_manager", {mrt1(0x1604), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "player", "state_manager", {mrt1(0x14fc), rt0});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "menu_state", "state_manager", {mrt1(0x2470)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "player_xf", "player", {mrt1(0x24)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "orbit_state", "player", {mrt1(0x3a4)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "firstperson_pitch", "player", {mrt1(0x604)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "ball_state", "player", {mrt1(0x38c)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "angular_vel", "player", {mrt1(0x1bc)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "world_id", "world", {mrt1(0x8)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "area_id", "state_manager", {mrt1(0x16a0)});
  addr_db.register_dynamic_address(Game::PRIME_2_GCN, "area_layers_vector", "state_manager", {mrt1(0x1694), mrt1(0xc), rt0});


  addr_db.register_address(Game::PRIME_3, "state_manager", 0x805c6c68, 0x805ca0e8); // +0x1010 object list [+0x10]+0xc]+0x16
  addr_db.register_address(Game::PRIME_3, "tweakgun", 0x8066f87c, 0x806730fc);
  addr_db.register_address(Game::PRIME_3, "motion_vf", 0x802e0dac, 0x802e0a88);
  addr_db.register_address(Game::PRIME_3, "dna_scanner_vftable", 0x8059b700, 0x8059e160);
  addr_db.register_address(Game::PRIME_3, "cursor_base", 0x8066fd08, 0x80673588);
  addr_db.register_address(Game::PRIME_3, "cursor_dlg_enabled", 0x805c8d77, 0x805cc1d7);
  addr_db.register_address(Game::PRIME_3, "boss_info_base", 0x8066e1ec, 0x80671a6c);
  addr_db.register_address(Game::PRIME_3, "beamvisor_menu_base", 0x8066fcfc, 0x8067357c);
  addr_db.register_address(Game::PRIME_3, "lockon_state", 0x805c6db7, 0x805ca237);
  addr_db.register_address(Game::PRIME_3, "gun_lag_toc_offset", -0x5ff0, -0x6000);
  addr_db.register_address(Game::PRIME_3, "powerups_size", 12, 12, 12);
  addr_db.register_address(Game::PRIME_3, "powerups_offset", 0x58, 0x58, 0x58);
  addr_db.register_address(Game::PRIME_3, "bloom_offset", 0x8058b018, 0x8058da58);
  addr_db.register_dynamic_address(Game::PRIME_3, "camera_manager", "state_manager", {mrt1(0x10), mrt1(0xc), mrt1(0x16)});
  addr_db.register_dynamic_address(Game::PRIME_3, "perspective_info", "camera_manager", {mrt1(0x2), mrt1(0x14), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3, "object_list", "state_manager", {rt0, mrt1(0x1010), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3, "menu_state", "state_manager", {mrt1(0x32C)});
  addr_db.register_dynamic_address(Game::PRIME_3, "player", "state_manager", {rt0, mrt1(0x2184), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3, "cursor", "cursor_base", {rt0, rt(0xc54, 0xd04, 0xc54), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3, "powerups_array", "player", {mrt1(0x35a8), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3, "boss_name", "boss_info_base", {rt0, mrt1(0x6e0), mrt1(0x24), mrt1(0x150), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3, "boss_status", "boss_info_base", {rt0, mrt1(0x6e0), mrt1(0x24), mrt1(0xb3)});
  addr_db.register_dynamic_address(Game::PRIME_3, "firstperson_pitch", "player", {mrt1(0x784)});
  addr_db.register_dynamic_address(Game::PRIME_3, "active_visor", "powerups_array", {mrt1(0x34)});
  addr_db.register_dynamic_address(Game::PRIME_3, "beamvisor_menu_state", "beamvisor_menu_base", {rt0, mrt1(0x300)});
  addr_db.register_dynamic_address(Game::PRIME_3, "angular_momentum", "player", {mrt1(0x174)});
  addr_db.register_dynamic_address(Game::PRIME_3, "ball_state", "player", {mrt1(0x358)});
  addr_db.register_dynamic_address(Game::PRIME_3, "lockon_type", "player", {mrt1(0x370)});
  addr_db.register_dynamic_address(Game::PRIME_3, "audio_manager", "state_manager", {mrt1(0x250), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3, "audio_fadein_time", "audio_manager", {mrt1(0x308)});
  addr_db.register_dynamic_address(Game::PRIME_3, "audio_fade_mode", "audio_manager", {mrt1(0x310)});

  addr_db.register_address(Game::PRIME_3_STANDALONE, "state_manager", 0x805c4f98, 0x805c7598, 0x805caa58); // +0x1010 object list
  addr_db.register_address(Game::PRIME_3_STANDALONE, "tweakgun", 0x8067d78c, 0x8067fdb4, 0x806835fc);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "motion_vf", 0x802e2508, 0x802e3be4, 0x802e5ed8);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "dna_scanner_vftable", 0x80599b50, 0x8059c140, 0x8059f580);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "cursor_base", 0x8067dc18, 0x80680240, 0x80683a88);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "cursor_dlg_enabled", 0x805c70c7, 0x805c96df, 0x805ccbd7);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "boss_info_base", 0x8067c0e4, 0x8067e70c, 0x80681f54);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "beamvisor_menu_base", 0x8067dc0c, 0x80680234, 0x80683a7c);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "lockon_state", 0x805c50e7, 0x805c76e7, 0x805caba7);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "gun_lag_toc_offset", -0x5fb0, -0x5f98, -0x5f68);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "powerups_size", 12, 12, 12);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "powerups_offset", 0x58, 0x58, 0x58);
  addr_db.register_address(Game::PRIME_3_STANDALONE, "bloom_offset", 0x80589410, 0x8058b9d8, 0x8058edd8);
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "camera_manager", "state_manager", {mrt1(0x10), mrt1(0xc), mrt1(0x16)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "perspective_info", "camera_manager", {mrt1(0x2), mrt1(0x14), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "object_list", "state_manager", {rt0, mrt1(0x1010), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "menu_state", "state_manager", {mrt1(0x32C)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "player", "state_manager", {rt0, mrt1(0x2184), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "cursor", "cursor_base", {rt0, mrt1(0xc54), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "powerups_array", "player", {rt(0x35a0, 0x35a0, 0x35a8), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "boss_name", "boss_info_base", {rt0, mrt1(0x6e0), mrt1(0x24), mrt1(0x150), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "boss_status", "boss_info_base", {rt0, mrt1(0x6e0), mrt1(0x24), mrt1(0xb3)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "firstperson_pitch", "player", {rt(0x77c, 0x77c, 0x784)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "beamvisor_menu_state", "beamvisor_menu_base", {rt0, mrt1(0x1708)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "active_visor", "powerups_array", {mrt1(0x34)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "angular_momentum", "player", {mrt1(0x174)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "ball_state", "player", {mrt1(0x358)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "lockon_type", "player", {mrt1(0x370)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "audio_manager", "state_manager", {mrt1(0x250), rt0});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "audio_fadein_time", "audio_manager", {mrt1(0x308)});
  addr_db.register_dynamic_address(Game::PRIME_3_STANDALONE, "audio_fade_mode", "audio_manager", {mrt1(0x310)});
}
}
