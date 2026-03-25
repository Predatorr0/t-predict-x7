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

// Afterimage
MACRO_CONFIG_INT(BcAfterimage, bc_afterimage, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Render afterimage layers of your tee")
MACRO_CONFIG_INT(BcAfterimageFrames, bc_afterimage_frames, 6, 2, 20, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How many previous frames to keep for the afterimage")
MACRO_CONFIG_INT(BcAfterimageAlpha, bc_afterimage_alpha, 40, 1, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Maximum alpha of afterimage layers (1-100)")
MACRO_CONFIG_INT(BcAfterimageSpacing, bc_afterimage_spacing, 18, 1, 64, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Distance between afterimage samples")
