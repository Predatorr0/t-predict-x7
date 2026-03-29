#include "hud_layout.h"

#include <base/math.h>

#include <engine/shared/config.h>

#include <algorithm>

namespace HudLayout
{

namespace
{

static const SModuleLayout gs_aModuleLayouts[MODULE_COUNT] = {
	{0.0f, 60.0f, 100, 0, true, 0x66000000U},
	{304.0f, 0.0f, 100, 0, true, 0x66000000U},
	{470.0f, 205.0f, 100, 0, true, 0x66000000U},
	{100.0f, 3.0f, 100, 0, false, 0x66000000U},
	{500.0f, 5.0f, 100, 0, false, 0x66000000U},
	{500.0f, 20.0f, 100, 0, false, 0x66000000U},
	{233.0f, 64.0f, 100, 0, false, 0x66000000U},
	{220.0f, 240.0f, 100, 0, false, 0x66000000U},
	{110.0f, 2.0f, 100, 0, true, 0x66000000U},
	{500.0f, 141.0f, 100, 0, true, 0x66000000U},
	{460.0f, 229.0f, 100, 0, true, 0x40000000U},
	{184.0f, 2.0f, 100, 0, false, 0x1E59A36BU},
	{4.0f, 122.0f, 100, 0, false, 0x66000000U},
	{160.0f, 0.0f, 100, 0, false, 0x66000000U},
	{5.0f, 273.0f, 100, 0, false, 0x66000000U},
	{0.0f, 60.0f, 100, 0, true, 0x66000000U},
	{250.0f, 200.0f, 65, 0, true, 0x66000000U},
	{490.0f, 5.0f, 100, 0, false, 0x66000000U},
};

static const char *gs_apModuleNames[MODULE_COUNT] = {
	"Mini Vote",
	"Frozen HUD",
	"Movement Info",
	"Notify Last",
	"FPS",
	"Ping",
	"Game Timer",
	"Hook Combo",
	"Local Time",
	"Spectator Count",
	"Score",
	"Music Player",
	"Voice HUD",
	"Voice Mute Icons",
	"Ingame Chat",
	"Votes",
	"Lock Cam",
	"Killfeed",
};

SModuleLayout ConfigLayout(EModule Module)
{
	switch(Module)
	{
	case MODULE_MUSIC_PLAYER:
		return {(float)g_Config.m_BcHudMusicPlayerX, (float)g_Config.m_BcHudMusicPlayerY, g_Config.m_BcHudMusicPlayerScale, 0, gs_aModuleLayouts[Module].m_BackgroundEnabled, gs_aModuleLayouts[Module].m_BackgroundColor};
	case MODULE_VOICE_TALKERS:
		return {(float)g_Config.m_BcHudVoiceHudX, (float)g_Config.m_BcHudVoiceHudY, g_Config.m_BcHudVoiceHudScale, 0, gs_aModuleLayouts[Module].m_BackgroundEnabled, gs_aModuleLayouts[Module].m_BackgroundColor};
	case MODULE_VOICE_STATUS:
		return {(float)g_Config.m_BcHudVoiceMuteIconsX, (float)g_Config.m_BcHudVoiceMuteIconsY, g_Config.m_BcHudVoiceMuteIconsScale, 0, gs_aModuleLayouts[Module].m_BackgroundEnabled, gs_aModuleLayouts[Module].m_BackgroundColor};
	case MODULE_CHAT:
		return {(float)g_Config.m_BcHudChatX, (float)g_Config.m_BcHudChatY, g_Config.m_BcHudChatScale, 0, gs_aModuleLayouts[Module].m_BackgroundEnabled, gs_aModuleLayouts[Module].m_BackgroundColor};
	case MODULE_VOTES:
		return {(float)g_Config.m_BcHudVotesX, (float)g_Config.m_BcHudVotesY, g_Config.m_BcHudVotesScale, 0, gs_aModuleLayouts[Module].m_BackgroundEnabled, gs_aModuleLayouts[Module].m_BackgroundColor};
	default:
		return gs_aModuleLayouts[Module];
	}
}

void WriteConfigLayout(EModule Module, const SModuleLayout &Layout)
{
	switch(Module)
	{
	case MODULE_MUSIC_PLAYER:
		g_Config.m_BcHudMusicPlayerX = round_to_int(Layout.m_X);
		g_Config.m_BcHudMusicPlayerY = round_to_int(Layout.m_Y);
		g_Config.m_BcHudMusicPlayerScale = Layout.m_Scale;
		break;
	case MODULE_VOICE_TALKERS:
		g_Config.m_BcHudVoiceHudX = round_to_int(Layout.m_X);
		g_Config.m_BcHudVoiceHudY = round_to_int(Layout.m_Y);
		g_Config.m_BcHudVoiceHudScale = Layout.m_Scale;
		break;
	case MODULE_VOICE_STATUS:
		g_Config.m_BcHudVoiceMuteIconsX = round_to_int(Layout.m_X);
		g_Config.m_BcHudVoiceMuteIconsY = round_to_int(Layout.m_Y);
		g_Config.m_BcHudVoiceMuteIconsScale = Layout.m_Scale;
		break;
	case MODULE_CHAT:
		g_Config.m_BcHudChatX = round_to_int(Layout.m_X);
		g_Config.m_BcHudChatY = round_to_int(Layout.m_Y);
		g_Config.m_BcHudChatScale = Layout.m_Scale;
		break;
	case MODULE_VOTES:
		g_Config.m_BcHudVotesX = round_to_int(Layout.m_X);
		g_Config.m_BcHudVotesY = round_to_int(Layout.m_Y);
		g_Config.m_BcHudVotesScale = Layout.m_Scale;
		break;
	default:
		break;
	}
}

bool IsLegacyModule(EModule Module)
{
	return Module >= MODULE_MINI_VOTE && Module <= MODULE_GAME_TIMER;
}

SModuleLayout ResolveBaseLayout(EModule Module, float HudWidth, float HudHeight)
{
	SModuleLayout Layout = gs_aModuleLayouts[Module];
	if(IsLegacyModule(Module))
	{
		switch(Module)
		{
		case MODULE_MINI_VOTE:
			Layout.m_X = 0.0f;
			Layout.m_Y = 60.0f;
			break;
		case MODULE_FROZEN_HUD:
			Layout.m_X = (float)round_to_int(HudWidth * 0.595f);
			Layout.m_Y = 0.0f;
			break;
		case MODULE_MOVEMENT_INFO:
			Layout.m_X = (float)round_to_int(HudWidth - 62.0f);
			Layout.m_Y = 205.0f;
			break;
		case MODULE_NOTIFY_LAST:
			Layout.m_X = (float)round_to_int(HudWidth * 0.2f);
			Layout.m_Y = (float)round_to_int(HudHeight * 0.01f);
			break;
		case MODULE_FPS:
			Layout.m_X = (float)round_to_int(HudWidth - 26.0f);
			Layout.m_Y = 5.0f;
			break;
		case MODULE_PING:
			Layout.m_X = (float)round_to_int(HudWidth - 26.0f);
			Layout.m_Y = 20.0f;
			break;
		case MODULE_GAME_TIMER:
			Layout.m_X = (float)round_to_int(HudWidth * 0.5f - 22.0f);
			Layout.m_Y = -2.0f;
			break;
		default:
			break;
		}
	}
	else
	{
		Layout = ConfigLayout(Module);
		Layout.m_X = CanvasXToHud(Layout.m_X, HudWidth);
		if(Module == MODULE_MUSIC_PLAYER)
		{
			Layout.m_X = std::clamp(Layout.m_X, 0.0f, HudWidth);
		}
	}
	return Layout;
}

} // namespace

SModuleLayout Get(EModule Module, float HudWidth, float HudHeight)
{
	SModuleLayout Layout = ResolveBaseLayout(Module, HudWidth, HudHeight);
	if(Module == MODULE_HOOK_COMBO)
	{
		const float UserScale = std::clamp(g_Config.m_BcHookComboSize / 100.0f, 0.5f, 2.0f);
		Layout.m_Scale = round_to_int(Layout.m_Scale * UserScale);
	}
	return Layout;
}

bool IsEditableModule(EModule Module)
{
	return Module == MODULE_MUSIC_PLAYER || Module == MODULE_VOICE_TALKERS || Module == MODULE_VOICE_STATUS || Module == MODULE_CHAT || Module == MODULE_VOTES;
}

const char *Name(EModule Module)
{
	return Module >= 0 && Module < MODULE_COUNT ? gs_apModuleNames[Module] : "HUD Module";
}

void SetPosition(EModule Module, float X, float Y)
{
	SModuleLayout Layout = ConfigLayout(Module);
	Layout.m_X = X;
	Layout.m_Y = Y;
	WriteConfigLayout(Module, Layout);
}

void SetScale(EModule Module, int Scale)
{
	SModuleLayout Layout = ConfigLayout(Module);
	Layout.m_Scale = std::clamp(Scale, 25, 300);
	WriteConfigLayout(Module, Layout);
}

void Reset(EModule Module)
{
	WriteConfigLayout(Module, gs_aModuleLayouts[Module]);
}

void ResetEditableModules()
{
	for(int Module = 0; Module < MODULE_COUNT; ++Module)
	{
		if(IsEditableModule((EModule)Module))
			Reset((EModule)Module);
	}
}

SModuleRect ClampRectToScreen(const SModuleRect &Rect, float HudWidth, float HudHeight)
{
	SModuleRect Result = Rect;
	Result.m_X = std::clamp(Result.m_X, 0.0f, maximum(0.0f, HudWidth - Result.m_W));
	Result.m_Y = std::clamp(Result.m_Y, 0.0f, maximum(0.0f, HudHeight - Result.m_H));
	return Result;
}

float CanvasXToHud(float CanvasX, float HudWidth)
{
	return CanvasX * (HudWidth / CANVAS_WIDTH);
}

int BackgroundCorners(int DefaultCorners, float RectX, float RectY, float RectW, float RectH, float CanvasWidth, float CanvasHeight)
{
	(void)RectX;
	(void)RectY;
	(void)RectW;
	(void)RectH;
	(void)CanvasWidth;
	(void)CanvasHeight;
	return DefaultCorners;
}

} // namespace HudLayout
