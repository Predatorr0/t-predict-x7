// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Flags, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) ;
#endif

// Camera drift
MACRO_CONFIG_INT(BcCameraDrift, bc_camera_drift, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable smooth camera drift that lags behind player movement")
MACRO_CONFIG_INT(BcCameraDriftAmount, bc_camera_drift_amount, 50, 1, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Amount of camera drift (1 = minimal drift, 200 = maximum drift)")
MACRO_CONFIG_INT(BcCameraDriftSmoothness, bc_camera_drift_smoothness, 20, 1, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Smoothness of camera drift (1 = near instant, 20 = very smooth)")
MACRO_CONFIG_INT(BcCameraDriftReverse, bc_camera_drift_reverse, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Reverse camera drift direction (camera drifts opposite to movement)")
MACRO_CONFIG_INT(BcDynamicFov, bc_dynamic_fov, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Increase FOV dynamically based on movement speed")
MACRO_CONFIG_INT(BcDynamicFovAmount, bc_dynamic_fov_amount, 50, 1, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Amount of dynamic FOV boost (1 = minimal boost, 200 = maximum boost)")
MACRO_CONFIG_INT(BcDynamicFovSmoothness, bc_dynamic_fov_smoothness, 20, 1, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Smoothness of dynamic FOV boost (1 = near instant, 20 = very smooth)")
MACRO_CONFIG_INT(BcCustomAspectRatioMode, bc_custom_aspect_ratio_mode, -1, -1, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Aspect ratio mode (-1=legacy auto, 0=off, 1=preset, 2=custom)")
MACRO_CONFIG_INT(BcCustomAspectRatio, bc_custom_aspect_ratio, 0, 0, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Aspect ratio value x100 (0=off, presets: 125=5:4, 133=4:3, 150=3:2, custom: 100-300)")
MACRO_CONFIG_INT(BcCrystalLaser, bc_crystal_laser, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render rifle and shotgun lasers with crystal shards and icy glow")

// Media background
MACRO_CONFIG_INT(BcMenuMediaBackground, bc_menu_media_background, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable custom media background in offline menus")
MACRO_CONFIG_INT(BcGameMediaBackground, bc_game_media_background, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable custom media background in game background rendering")
MACRO_CONFIG_STR(BcMenuMediaBackgroundPath, bc_menu_media_background_path, IO_MAX_PATH_LENGTH, "", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Path to the custom menu media background file")
MACRO_CONFIG_INT(BcGameMediaBackgroundOffset, bc_game_media_background_offset, 0, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How much the custom media background is fixed to the map when rendering the in-game background")

// Afterimage
MACRO_CONFIG_INT(BcAfterimage, bc_afterimage, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render afterimage layers of your tee")
MACRO_CONFIG_INT(BcAfterimageFrames, bc_afterimage_frames, 6, 2, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How many previous frames to keep for the afterimage")
MACRO_CONFIG_INT(BcAfterimageAlpha, bc_afterimage_alpha, 40, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum alpha of afterimage layers (1-100)")
MACRO_CONFIG_INT(BcAfterimageSpacing, bc_afterimage_spacing, 18, 1, 64, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Distance between afterimage samples")

// Chat bubbles
MACRO_CONFIG_INT(BcChatBubbles, bc_chat_bubbles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle Chatbubbles")
MACRO_CONFIG_INT(BcChatBubblesSelf, bc_chat_bubbles_self, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Chatbubbles above you")
MACRO_CONFIG_INT(BcChatBubblesDemo, bc_chat_bubbles_demo, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show Chatbubbles in demoplayer")
MACRO_CONFIG_INT(BcChatBubbleSize, bc_chat_bubble_size, 20, 20, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Size of the chat bubble")
MACRO_CONFIG_INT(BcChatBubbleShowTime, bc_chat_bubble_showtime, 200, 200, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long to show the bubble for")
MACRO_CONFIG_INT(BcChatBubbleFadeOut, bc_chat_bubble_fadeout, 35, 15, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long it fades out")
MACRO_CONFIG_INT(BcChatBubbleFadeIn, bc_chat_bubble_fadein, 15, 15, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How long it fades in")

// Magic particles
MACRO_CONFIG_INT(BcMagicParticles, bc_magic_particles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle magic particles")
MACRO_CONFIG_INT(BcMagicParticlesRadius, bc_magic_particles_radius, 10, 1, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Radius of magic particles")
MACRO_CONFIG_INT(BcMagicParticlesSize, bc_magic_particles_size, 8, 1, 50, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Size of magic particles")
MACRO_CONFIG_INT(BcMagicParticlesAlphaDelay, bc_magic_particles_alpha_delay, 3, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha delay of magic particles")
MACRO_CONFIG_INT(BcMagicParticlesType, bc_magic_particles_type, 1, 1, 4, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Type of magic particles. 1 = Slice, 2 = Ball, 3 = Smoke, 4 = Shell")
MACRO_CONFIG_INT(BcMagicParticlesCount, bc_magic_particles_count, 10, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Count of magic particles")

// Orbit aura
MACRO_CONFIG_INT(BcOrbitAura, bc_orbit_aura, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle orbit aura around the local player")
MACRO_CONFIG_INT(BcOrbitAuraRadius, bc_orbit_aura_radius, 32, 8, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura radius")
MACRO_CONFIG_INT(BcOrbitAuraParticles, bc_orbit_aura_particles, 14, 2, 120, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura particle count")
MACRO_CONFIG_INT(BcOrbitAuraAlpha, bc_orbit_aura_alpha, 70, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura alpha")
MACRO_CONFIG_INT(BcOrbitAuraSpeed, bc_orbit_aura_speed, 100, 10, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Orbit aura speed")
MACRO_CONFIG_INT(BcOrbitAuraIdle, bc_orbit_aura_idle, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Only show orbit aura after standing idle")
MACRO_CONFIG_INT(BcOrbitAuraIdleTimer, bc_orbit_aura_idle_timer, 2, 1, 30, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Idle delay before orbit aura appears in seconds")

// 3D particles
MACRO_CONFIG_INT(Bc3dParticles, bc_3d_particles, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Toggle 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesType, bc_3d_particles_type, 1, 1, 3, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Type of 3D particles. 1 = Cube, 2 = Heart, 3 = Mixed")
MACRO_CONFIG_INT(Bc3dParticlesCount, bc_3d_particles_count, 60, 1, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Count of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesSizeMin, bc_3d_particles_size_min, 3, 2, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Minimum size of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesSizeMax, bc_3d_particles_size_max, 8, 2, 200, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum size of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesSpeed, bc_3d_particles_speed, 18, 1, 500, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Base speed of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesDepth, bc_3d_particles_depth, 300, 10, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Depth range for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesAlpha, bc_3d_particles_alpha, 35, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Alpha of 3D particles (1-100)")
MACRO_CONFIG_INT(Bc3dParticlesFadeInMs, bc_3d_particles_fade_in_ms, 400, 1, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fade-in time in ms for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesFadeOutMs, bc_3d_particles_fade_out_ms, 400, 1, 5000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Fade-out time in ms for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesPushRadius, bc_3d_particles_push_radius, 120, 0, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Player push radius for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesPushStrength, bc_3d_particles_push_strength, 120, 0, 2000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Player push strength for 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesCollide, bc_3d_particles_collide, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "3D particles collide with each other")
MACRO_CONFIG_INT(Bc3dParticlesViewMargin, bc_3d_particles_view_margin, 120, 0, 1000, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Margin outside view before 3D particles disappear")
MACRO_CONFIG_INT(Bc3dParticlesColorMode, bc_3d_particles_color_mode, 1, 1, 2, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Color mode for 3D particles. 1 = Custom, 2 = Random")
MACRO_CONFIG_COL(Bc3dParticlesColor, bc_3d_particles_color, 4294967295, CFGFLAG_CLIENT | CFGFLAG_SAVE | CFGFLAG_COLALPHA, "Color of 3D particles")
MACRO_CONFIG_INT(Bc3dParticlesGlow, bc_3d_particles_glow, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable 3D particles glow")
MACRO_CONFIG_INT(Bc3dParticlesGlowAlpha, bc_3d_particles_glow_alpha, 35, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Glow alpha of 3D particles (1-100)")
MACRO_CONFIG_INT(Bc3dParticlesGlowOffset, bc_3d_particles_glow_offset, 2, 1, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Glow offset for 3D particles")
