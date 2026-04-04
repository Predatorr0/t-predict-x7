/* Copyright © 2026 BestProject Team */
#include "hud_editor.h"

#include "music_player.h"
#include "voice/voice.h"

#include <base/color.h>
#include <base/math.h>
#include <base/str.h>

#include <engine/graphics.h>
#include <engine/font_icons.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/client/bc_ui_animations.h>
#include <game/client/components/chat.h>
#include <game/client/components/voting.h>
#include <game/client/gameclient.h>
#include <game/localization.h>

namespace
{
constexpr float SNAP_THRESHOLD = 6.0f;
constexpr float SETTINGS_POPUP_WIDTH = 210.0f;
constexpr float SETTINGS_POPUP_HEIGHT = 164.0f;

bool IsMusicPlayerEnabled(const CGameClient *pGameClient)
{
	return g_Config.m_BcMusicPlayer != 0 &&
	       !pGameClient->m_BestClient.IsComponentDisabled(CBestClient::COMPONENT_VISUALS_MUSIC_PLAYER);
}

bool IsEditorModule(HudLayout::EModule Module)
{
	return Module == HudLayout::MODULE_MUSIC_PLAYER ||
	       Module == HudLayout::MODULE_CHAT ||
	       Module == HudLayout::MODULE_VOICE_TALKERS ||
	       Module == HudLayout::MODULE_VOICE_STATUS ||
	       Module == HudLayout::MODULE_VOTES ||
	       Module == HudLayout::MODULE_GAME_TIMER ||
	       Module == HudLayout::MODULE_LOCAL_TIME ||
	       Module == HudLayout::MODULE_KILLFEED ||
	       Module == HudLayout::MODULE_MOVEMENT_INFO ||
	       Module == HudLayout::MODULE_SPECTATOR_COUNT ||
	       Module == HudLayout::MODULE_SCORE;
}

bool IsLivePreviewModule(HudLayout::EModule Module)
{
	return Module == HudLayout::MODULE_MUSIC_PLAYER ||
		Module == HudLayout::MODULE_CHAT ||
		Module == HudLayout::MODULE_VOTES ||
		Module == HudLayout::MODULE_VOICE_TALKERS ||
		Module == HudLayout::MODULE_VOICE_STATUS ||
		Module == HudLayout::MODULE_GAME_TIMER ||
		Module == HudLayout::MODULE_LOCAL_TIME ||
		Module == HudLayout::MODULE_KILLFEED ||
		Module == HudLayout::MODULE_MOVEMENT_INFO ||
		Module == HudLayout::MODULE_SPECTATOR_COUNT ||
		Module == HudLayout::MODULE_SCORE;
}

bool PointInRect(vec2 Point, const CUIRect &Rect)
{
	return Point.x >= Rect.x && Point.x <= Rect.x + Rect.w &&
	       Point.y >= Rect.y && Point.y <= Rect.y + Rect.h;
}

CUIRect ClampToBounds(CUIRect Rect, float Width, float Height)
{
	Rect.x = std::clamp(Rect.x, 0.0f, maximum(0.0f, Width - Rect.w));
	Rect.y = std::clamp(Rect.y, 0.0f, maximum(0.0f, Height - Rect.h));
	return Rect;
}

float ChatInputBottomExtra(const CChat &Chat)
{
	const float ScaledFontSize = Chat.FontSize() * (8.0f / 6.0f);
	return maximum(2.25f * ScaledFontSize, maximum(ScaledFontSize + 4.0f, 16.0f));
}

int *ScaleConfigForModule(HudLayout::EModule Module)
{
	switch(Module)
	{
	case HudLayout::MODULE_MUSIC_PLAYER: return &g_Config.m_BcHudMusicPlayerScale;
	case HudLayout::MODULE_VOICE_TALKERS: return &g_Config.m_BcHudVoiceHudScale;
	case HudLayout::MODULE_VOICE_STATUS: return &g_Config.m_BcHudVoiceMuteIconsScale;
	case HudLayout::MODULE_CHAT: return &g_Config.m_BcHudChatScale;
	case HudLayout::MODULE_VOTES: return &g_Config.m_BcHudVotesScale;
	default: return nullptr;
	}
}
} // namespace

void CHudEditor::OnConsoleInit()
{
	Storage()->CreateFolder("BestClient", IStorage::TYPE_SAVE);
	HudLayout::OnConsoleInit(Console(), ConfigManager());
}

void CHudEditor::Activate()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	m_Active = true;
	m_MouseDownLast = false;
	m_RightMouseDownLast = false;
	m_Dragging = false;
	m_PressedModule = HudLayout::MODULE_COUNT;
	m_HoveredModule = HudLayout::MODULE_COUNT;
	m_SelectedModule = HudLayout::MODULE_COUNT;
	m_PressedOnReset = false;
	m_DrawerOpen = false;
	m_SettingsPanelOpen = false;
	m_DrawerPhase = 0.0f;
	m_SettingsPanelPhase = 0.0f;
	Ui()->ClosePopupMenus();
}

void CHudEditor::Deactivate()
{
	m_Active = false;
	m_MouseDownLast = false;
	m_RightMouseDownLast = false;
	m_Dragging = false;
	m_PressedModule = HudLayout::MODULE_COUNT;
	m_HoveredModule = HudLayout::MODULE_COUNT;
	m_SelectedModule = HudLayout::MODULE_COUNT;
	m_PressedOnReset = false;
	m_DrawerOpen = false;
	m_SettingsPanelOpen = false;
	m_DrawerPhase = 0.0f;
	m_SettingsPanelPhase = 0.0f;
	Ui()->ClosePopupMenus();
}

void CHudEditor::OnStateChange(int NewState, int OldState)
{
	(void)OldState;
	if(NewState != IClient::STATE_ONLINE && NewState != IClient::STATE_DEMOPLAYBACK)
		Deactivate();
}

bool CHudEditor::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!m_Active)
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	Ui()->OnCursorMove(x, y);
	return true;
}

bool CHudEditor::OnInput(const IInput::CEvent &Event)
{
	if(!m_Active)
		return false;
	if((Event.m_Flags & IInput::FLAG_PRESS) != 0 && Event.m_Key == KEY_ESCAPE)
	{
		Deactivate();
		return true;
	}
	return true;
}

float CHudEditor::HudWidth() const
{
	return HudLayout::CANVAS_HEIGHT * Graphics()->ScreenAspect();
}

float CHudEditor::HudHeight() const
{
	return HudLayout::CANVAS_HEIGHT;
}

bool CHudEditor::IsEditableModule(HudLayout::EModule Module) const
{
	return IsEditorModule(Module) && HudLayout::IsEditableModule(Module);
}

bool CHudEditor::IsModuleEnabled(HudLayout::EModule Module) const
{
	return HudLayout::IsEnabled(Module);
}

bool CHudEditor::IsLiveRenderedModule(HudLayout::EModule Module) const
{
	return IsLivePreviewModule(Module);
}

void CHudEditor::ResetEditorSettings()
{
	g_Config.m_BcHudEditorOverlayColor = DefaultConfig::BcHudEditorOverlayColor;
	g_Config.m_BcHudEditorOutlineColor = DefaultConfig::BcHudEditorOutlineColor;
	g_Config.m_BcHudEditorHoverColor = DefaultConfig::BcHudEditorHoverColor;
	g_Config.m_BcHudEditorAutoCorner = DefaultConfig::BcHudEditorAutoCorner;
	g_Config.m_BcHudEditorAutoSnap = DefaultConfig::BcHudEditorAutoSnap;
}

void CHudEditor::ResetModuleExtraSettings(HudLayout::EModule Module)
{
	if(Module == HudLayout::MODULE_VOTES)
		g_Config.m_TcMiniVoteHud = DefaultConfig::TcMiniVoteHud;
	else if(Module == HudLayout::MODULE_GAME_TIMER)
		g_Config.m_BcMusicPlayer = DefaultConfig::BcMusicPlayer;
}

CUIRect CHudEditor::GetDrawerToggleRect() const
{
	return {8.0f, 8.0f, 18.0f, 18.0f};
}

CUIRect CHudEditor::GetDrawerRect(float Phase) const
{
	const float DrawerWidth = 122.0f;
	const float DrawerHeight = 58.0f;
	const float ToggleWidth = GetDrawerToggleRect().w;
	const float Ease = BCUiAnimations::EaseInOutCubic(Phase);
	return {8.0f - (DrawerWidth - ToggleWidth) * (1.0f - Ease), 8.0f, DrawerWidth, DrawerHeight};
}

CUIRect CHudEditor::GetSettingsPanelRect() const
{
	return {12.0f, 34.0f, 214.0f, 142.0f};
}

CUIRect CHudEditor::GetFallbackModuleRect(HudLayout::EModule Module) const
{
	const float Width = HudWidth();
	const float Height = HudHeight();
	const auto Layout = HudLayout::Get(Module, Width, Height);
	CUIRect Rect{};

	switch(Module)
	{
	case HudLayout::MODULE_SCORE:
		Rect = {Layout.m_X, Layout.m_Y, 112.0f, 56.0f};
		break;
	case HudLayout::MODULE_MOVEMENT_INFO:
	{
		const bool ShowPos = g_Config.m_ClShowhudPlayerPosition != 0;
		const bool ShowSpeed = g_Config.m_ClShowhudPlayerSpeed != 0;
		const bool ShowAngle = g_Config.m_ClShowhudPlayerAngle != 0;
		float BoxHeight = 3.0f * 8.0f * ((ShowPos ? 1.0f : 0.0f) + (ShowSpeed ? 1.0f : 0.0f)) + 2.0f * 8.0f * (ShowAngle ? 1.0f : 0.0f);
		if(g_Config.m_BcShowhudDummyCoordIndicator)
			BoxHeight += 8.0f;
		if(ShowPos || ShowSpeed || ShowAngle)
			BoxHeight += 2.0f;
		BoxHeight = maximum(BoxHeight, 12.0f);
		Rect = {Layout.m_X, Layout.m_Y, 62.0f, BoxHeight};
		break;
	}
	case HudLayout::MODULE_GAME_TIMER:
		Rect = {Layout.m_X, Layout.m_Y, 64.0f, 12.0f};
		break;
	case HudLayout::MODULE_FPS:
		Rect = {Layout.m_X, Layout.m_Y, 26.0f, 9.0f};
		break;
	case HudLayout::MODULE_PING:
		Rect = {Layout.m_X, Layout.m_Y, 26.0f, 9.0f};
		break;
	case HudLayout::MODULE_LOCAL_TIME:
		Rect = {Layout.m_X, Layout.m_Y, 56.0f, 12.0f};
		break;
	case HudLayout::MODULE_SPECTATOR_COUNT:
		Rect = {Layout.m_X, Layout.m_Y, 118.0f, 16.0f};
		break;
	case HudLayout::MODULE_HOOK_COMBO:
	{
		const float Scale = std::clamp(Layout.m_Scale / 100.0f, 0.25f, 3.0f);
		const float FontSize = 13.0f * Scale;
		const float BoxWidth = TextRender()->TextWidth(FontSize, "fantastic (x7)", -1, -1.0f) + 8.0f * Scale;
		const float BoxHeight = FontSize + 4.0f * Scale;
		Rect = {Layout.m_X, Layout.m_Y, BoxWidth, BoxHeight};
		break;
	}
	case HudLayout::MODULE_MINI_VOTE:
		Rect = {Layout.m_X, Layout.m_Y, 70.0f, 35.0f};
		break;
	case HudLayout::MODULE_FROZEN_HUD:
		Rect = {Layout.m_X, Layout.m_Y, 176.0f, 34.0f};
		break;
	case HudLayout::MODULE_NOTIFY_LAST:
		Rect = {Layout.m_X, Layout.m_Y, 185.0f, 16.0f};
		break;
	case HudLayout::MODULE_LOCK_CAM:
		Rect = {Layout.m_X, Layout.m_Y, 16.0f, 16.0f};
		break;
	case HudLayout::MODULE_KILLFEED:
		Rect = {Layout.m_X, Layout.m_Y, 155.0f, 70.0f};
		break;
	default:
		Rect = {Layout.m_X, Layout.m_Y, 78.0f, 18.0f};
		break;
	}

	return ClampToBounds(Rect, Width, Height);
}

CHudEditor::SModuleVisual CHudEditor::GetModuleVisual(HudLayout::EModule Module) const
{
	SModuleVisual Visual;
	Visual.m_Module = Module;
	Visual.m_Editable = IsEditableModule(Module);
	Visual.m_Enabled = IsModuleEnabled(Module);
	Visual.m_IsFallbackPreview = false;

	const float Width = HudWidth();
	const float Height = HudHeight();

	switch(Module)
	{
	case HudLayout::MODULE_MUSIC_PLAYER:
		if(IsMusicPlayerEnabled(GameClient()))
		{
			Visual.m_Rect = GameClient()->m_MusicPlayer.GetHudEditorRect(false);
			if(Visual.m_Rect.w <= 0.0f || Visual.m_Rect.h <= 0.0f)
				Visual.m_Rect = GameClient()->m_MusicPlayer.GetHudEditorRect(true);
		}
		else
		{
			const auto Layout = HudLayout::Get(HudLayout::MODULE_MUSIC_PLAYER, Width, Height);
			Visual.m_Rect = {Layout.m_X, Layout.m_Y, 92.0f, 12.0f};
		}
		Visual.m_Rounding = 8.0f;
		break;
	case HudLayout::MODULE_VOICE_TALKERS:
		Visual.m_Rect = GameClient()->m_VoiceChat.GetHudTalkingIndicatorRect(Width, Height, true);
		Visual.m_Rounding = 4.0f;
		break;
	case HudLayout::MODULE_VOICE_STATUS:
		Visual.m_Rect = GameClient()->m_VoiceChat.GetHudMuteStatusIndicatorRect(Width, Height, true);
		Visual.m_Rounding = 3.0f;
		break;
	case HudLayout::MODULE_CHAT:
		Visual.m_Rect = GameClient()->m_Chat.GetHudRect(Width, Height, true);
		Visual.m_Rounding = 6.0f;
		Visual.m_UsesBottomAnchor = true;
		break;
	case HudLayout::MODULE_VOTES:
		Visual.m_Rect = GameClient()->m_Voting.GetHudRect(Width, Height, true);
		Visual.m_Rounding = 3.0f;
		break;
	case HudLayout::MODULE_GAME_TIMER:
	case HudLayout::MODULE_LOCAL_TIME:
	case HudLayout::MODULE_MOVEMENT_INFO:
	case HudLayout::MODULE_SPECTATOR_COUNT:
	case HudLayout::MODULE_SCORE:
		Visual.m_Rect = GameClient()->m_Hud.GetHudEditorRect(Module, true);
		Visual.m_Rounding = 5.0f;
		break;
	case HudLayout::MODULE_KILLFEED:
		Visual.m_Rect = GameClient()->m_InfoMessages.GetHudRect(Width, Height, true);
		Visual.m_Rounding = 4.0f;
		break;
	default:
		Visual.m_Rect = GetFallbackModuleRect(Module);
		Visual.m_Rounding = 4.0f;
		Visual.m_IsFallbackPreview = true;
		break;
	}

	if(Visual.m_Rect.w <= 0.0f || Visual.m_Rect.h <= 0.0f)
	{
		Visual.m_Rect = GetFallbackModuleRect(Module);
		Visual.m_Rounding = 4.0f;
		Visual.m_IsFallbackPreview = true;
	}

	Visual.m_Rounding = std::clamp(Visual.m_Rounding, 2.0f, 12.0f);
	Visual.m_Rect = ClampToBounds(Visual.m_Rect, Width, Height);
	return Visual;
}

void CHudEditor::CollectModuleVisuals(SModuleVisual *pOut, int &Count) const
{
	Count = 0;

	auto AddModule = [&](HudLayout::EModule Module) {
		if(Count >= MAX_MODULE_VISUALS)
			return;
		pOut[Count++] = GetModuleVisual(Module);
	};

	AddModule(HudLayout::MODULE_MUSIC_PLAYER);
	AddModule(HudLayout::MODULE_CHAT);
	AddModule(HudLayout::MODULE_VOTES);
	AddModule(HudLayout::MODULE_VOICE_TALKERS);
	AddModule(HudLayout::MODULE_VOICE_STATUS);
	AddModule(HudLayout::MODULE_GAME_TIMER);
	AddModule(HudLayout::MODULE_LOCAL_TIME);
	AddModule(HudLayout::MODULE_KILLFEED);
	AddModule(HudLayout::MODULE_MOVEMENT_INFO);
	AddModule(HudLayout::MODULE_SPECTATOR_COUNT);
	AddModule(HudLayout::MODULE_SCORE);
}

HudLayout::EModule CHudEditor::HitTestModule(vec2 MousePos) const
{
	SModuleVisual aVisuals[MAX_MODULE_VISUALS];
	int Count = 0;
	CollectModuleVisuals(aVisuals, Count);

	// Editable modules should always win hit-tests over locked preview modules.
	for(int i = Count - 1; i >= 0; --i)
	{
		if(!aVisuals[i].m_Editable)
			continue;
		const CUIRect &Rect = aVisuals[i].m_Rect;
		if(PointInRect(MousePos, Rect))
			return aVisuals[i].m_Module;
	}

	for(int i = Count - 1; i >= 0; --i)
	{
		const CUIRect &Rect = aVisuals[i].m_Rect;
		if(PointInRect(MousePos, Rect))
			return aVisuals[i].m_Module;
	}
	return HudLayout::MODULE_COUNT;
}

void CHudEditor::ApplyDraggedPosition(HudLayout::EModule Module, const CUIRect &Rect)
{
	if(!IsEditableModule(Module))
		return;

	const float CanvasX = Rect.x * (HudLayout::CANVAS_WIDTH / maximum(HudWidth(), 1.0f));
	if(Module == HudLayout::MODULE_CHAT)
	{
		const float BottomAnchor = Rect.y + Rect.h - ChatInputBottomExtra(GameClient()->m_Chat);
		HudLayout::SetPosition(Module, CanvasX, BottomAnchor);
	}
	else
		HudLayout::SetPosition(Module, CanvasX, Rect.y);
}

CUIRect CHudEditor::SnapRect(const CUIRect &Rect, HudLayout::EModule DraggedModule) const
{
	if(g_Config.m_BcHudEditorAutoSnap == 0)
		return ClampToBounds(Rect, HudWidth(), HudHeight());

	CUIRect Result = Rect;
	SModuleVisual aVisuals[MAX_MODULE_VISUALS];
	int Count = 0;
	CollectModuleVisuals(aVisuals, Count);

	auto TrySnap = [](float Candidate, float Target, float &BestDelta) {
		const float Delta = Target - Candidate;
		if(absolute(Delta) <= SNAP_THRESHOLD && absolute(Delta) < absolute(BestDelta))
			BestDelta = Delta;
	};

	float BestDeltaX = SNAP_THRESHOLD + 1.0f;
	float BestDeltaY = SNAP_THRESHOLD + 1.0f;
	const float Width = HudWidth();
	const float Height = HudHeight();

	TrySnap(Result.x, 0.0f, BestDeltaX);
	TrySnap(Result.x + Result.w, Width, BestDeltaX);
	TrySnap(Result.x + Result.w * 0.5f, Width * 0.5f, BestDeltaX);
	TrySnap(Result.y, 0.0f, BestDeltaY);
	TrySnap(Result.y + Result.h, Height, BestDeltaY);
	TrySnap(Result.y + Result.h * 0.5f, Height * 0.5f, BestDeltaY);

	for(int i = 0; i < Count; ++i)
	{
		if(aVisuals[i].m_Module == DraggedModule)
			continue;
		const CUIRect &Other = aVisuals[i].m_Rect;
		TrySnap(Result.x, Other.x, BestDeltaX);
		TrySnap(Result.x + Result.w, Other.x + Other.w, BestDeltaX);
		TrySnap(Result.x, Other.x + Other.w, BestDeltaX);
		TrySnap(Result.x + Result.w, Other.x, BestDeltaX);
		TrySnap(Result.x + Result.w * 0.5f, Other.x + Other.w * 0.5f, BestDeltaX);
		TrySnap(Result.y, Other.y, BestDeltaY);
		TrySnap(Result.y + Result.h, Other.y + Other.h, BestDeltaY);
		TrySnap(Result.y, Other.y + Other.h, BestDeltaY);
		TrySnap(Result.y + Result.h, Other.y, BestDeltaY);
		TrySnap(Result.y + Result.h * 0.5f, Other.y + Other.h * 0.5f, BestDeltaY);
	}

	if(absolute(BestDeltaX) <= SNAP_THRESHOLD)
		Result.x += BestDeltaX;
	if(absolute(BestDeltaY) <= SNAP_THRESHOLD)
		Result.y += BestDeltaY;

	return ClampToBounds(Result, Width, Height);
}

void CHudEditor::UpdateDragging(vec2 MousePos)
{
	if(!m_Dragging || m_PressedModule == HudLayout::MODULE_COUNT || !IsEditableModule(m_PressedModule))
		return;
	SModuleVisual Visual = GetModuleVisual(m_PressedModule);
	CUIRect NewRect = Visual.m_Rect;
	NewRect.x = MousePos.x - m_DragMouseOffset.x;
	NewRect.y = MousePos.y - m_DragMouseOffset.y;
	NewRect = SnapRect(NewRect, m_PressedModule);
	ApplyDraggedPosition(m_PressedModule, NewRect);
}

CUi::EPopupMenuFunctionResult CHudEditor::PopupModuleSettings(void *pContext, CUIRect View, bool Active)
{
	(void)Active;
	CHudEditor *pThis = static_cast<CHudEditor *>(pContext);
	if(pThis->m_SelectedModule == HudLayout::MODULE_COUNT)
		return CUi::POPUP_CLOSE_CURRENT;

	const bool Enabled = HudLayout::IsEnabled(pThis->m_SelectedModule);
	CUIRect Title, ToggleButton, ScaleLabel, ScaleSlider, ExtraToggleButton, ResetPositionButton, ResetSettingsButton;
	View.HSplitTop(16.0f, &Title, &View);
	pThis->Ui()->DoLabel(&Title, HudLayout::Name(pThis->m_SelectedModule), 10.0f, TEXTALIGN_MC);
	View.HSplitTop(8.0f, nullptr, &View);
	View.HSplitTop(16.0f, &ToggleButton, &View);
	if(pThis->GameClient()->m_Menus.DoButton_CheckBox(&pThis->m_ToggleModuleButton, Localize("Enabled"), Enabled ? 1 : 0, &ToggleButton))
		HudLayout::SetEnabled(pThis->m_SelectedModule, !Enabled);

	View.HSplitTop(8.0f, nullptr, &View);
	View.HSplitTop(12.0f, &ScaleLabel, &View);

	int *pScale = ScaleConfigForModule(pThis->m_SelectedModule);
	if(pScale != nullptr)
	{
		char aScale[32];
		str_format(aScale, sizeof(aScale), "%s %d%%", Localize("Scale"), *pScale);
		pThis->Ui()->DoLabel(&ScaleLabel, aScale, 8.0f, TEXTALIGN_ML);

		View.HSplitTop(14.0f, &ScaleSlider, &View);
		const float Relative = CUi::ms_LinearScrollbarScale.ToRelative(*pScale, 25, 300);
		const float NewRelative = pThis->Ui()->DoScrollbarH(pScale, &ScaleSlider, Relative);
		*pScale = CUi::ms_LinearScrollbarScale.ToAbsolute(NewRelative, 25, 300);
	}
	else
	{
		View.HSplitTop(14.0f, nullptr, &View);
	}

	if(pThis->m_SelectedModule == HudLayout::MODULE_VOTES || pThis->m_SelectedModule == HudLayout::MODULE_GAME_TIMER)
	{
		View.HSplitTop(8.0f, nullptr, &View);
		View.HSplitTop(16.0f, &ExtraToggleButton, &View);
		int *pValue = pThis->m_SelectedModule == HudLayout::MODULE_VOTES ? &g_Config.m_TcMiniVoteHud : &g_Config.m_BcMusicPlayer;
		const char *pLabel = pThis->m_SelectedModule == HudLayout::MODULE_VOTES ? Localize("Mini vote HUD") : Localize("Enable music player");
		if(pThis->GameClient()->m_Menus.DoButton_CheckBox(&pThis->m_ExtraModuleToggleButton, pLabel, *pValue, &ExtraToggleButton))
			*pValue ^= 1;
	}

	View.HSplitTop(10.0f, nullptr, &View);
	View.HSplitTop(16.0f, &ResetPositionButton, &View);
	if(pThis->Ui()->DoButton_PopupMenu(&pThis->m_ResetPositionButton, Localize("Reset position"), &ResetPositionButton, 8.0f, TEXTALIGN_MC))
		HudLayout::ResetPosition(pThis->m_SelectedModule);

	View.HSplitTop(5.0f, nullptr, &View);
	View.HSplitTop(16.0f, &ResetSettingsButton, &View);
	if(pThis->Ui()->DoButton_PopupMenu(&pThis->m_ResetSettingsButton, Localize("Reset settings"), &ResetSettingsButton, 8.0f, TEXTALIGN_MC))
	{
		HudLayout::ResetSettings(pThis->m_SelectedModule);
		pThis->ResetModuleExtraSettings(pThis->m_SelectedModule);
	}
	return CUi::POPUP_KEEP_OPEN;
}

void CHudEditor::OpenModuleSettings(const SModuleVisual &Visual)
{
	if(!IsEditableModule(Visual.m_Module))
		return;

	m_SelectedModule = Visual.m_Module;
	const float Width = HudWidth();
	const float Height = HudHeight();
	const float UiScaleX = Ui()->Screen()->w / maximum(Width, 1.0f);
	const float UiScaleY = Ui()->Screen()->h / maximum(Height, 1.0f);
	constexpr float PopupMargin = 5.0f;
	constexpr float PopupGap = 6.0f;
	const float PopupWidth = SETTINGS_POPUP_WIDTH * UiScaleX;
	const float PopupHeight = SETTINGS_POPUP_HEIGHT * UiScaleY;
	const CUIRect ModuleRectUi = {
		Visual.m_Rect.x * UiScaleX,
		Visual.m_Rect.y * UiScaleY,
		Visual.m_Rect.w * UiScaleX,
		Visual.m_Rect.h * UiScaleY};
	float PopupX = ModuleRectUi.x + ModuleRectUi.w + PopupGap;
	if(PopupX + PopupWidth > Ui()->Screen()->w - PopupMargin)
		PopupX = ModuleRectUi.x - PopupWidth - PopupGap;
	PopupX = std::clamp(PopupX, PopupMargin, maximum(PopupMargin, Ui()->Screen()->w - PopupWidth - PopupMargin));
	const float PopupY = std::clamp(ModuleRectUi.y, PopupMargin, maximum(PopupMargin, Ui()->Screen()->h - PopupHeight - PopupMargin));
	Ui()->ClosePopupMenus();
	Ui()->DoPopupMenu(&m_SettingsPopupId, PopupX, PopupY, PopupWidth, PopupHeight, this, PopupModuleSettings);
}

void CHudEditor::RenderModuleOutline(const SModuleVisual &Visual, bool Hovered, bool Selected) const
{
	const CUIRect &Rect = Visual.m_Rect;
	const ColorRGBA OutlineColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_BcHudEditorOutlineColor, true));
	const ColorRGBA HoverColor = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_BcHudEditorHoverColor, true));
	ColorRGBA Color = Selected ? OutlineColor : (Hovered ? HoverColor : OutlineColor);
	if(!Hovered && !Selected)
		Color.a *= 0.5f;
	if(!Visual.m_Editable)
		Color.a *= 0.72f;
	if(!Visual.m_Enabled)
		Color.a *= 0.82f;

	const float OuterPad = Selected ? 1.4f : (Hovered ? 1.0f : 0.7f);
	CUIRect OuterRect = {
		Rect.x - OuterPad,
		Rect.y - OuterPad,
		Rect.w + OuterPad * 2.0f,
		Rect.h + OuterPad * 2.0f};
	OuterRect.Draw(ColorRGBA(Color.r, Color.g, Color.b, Color.a * (Selected ? 0.26f : (Hovered ? 0.18f : 0.12f))), IGraphics::CORNER_ALL, Visual.m_Rounding + OuterPad);

	CUIRect HighlightRect = Rect;
	HighlightRect.Draw(ColorRGBA(Color.r, Color.g, Color.b, Color.a * (Selected ? 0.09f : 0.05f)), IGraphics::CORNER_ALL, Visual.m_Rounding);
}

void CHudEditor::RenderModuleLabel(const SModuleVisual &Visual) const
{
	char aLabel[96];
	if(Visual.m_Editable && !Visual.m_Enabled)
		str_format(aLabel, sizeof(aLabel), "%s (%s)", HudLayout::Name(Visual.m_Module), Localize("disabled"));
	else if(Visual.m_Editable)
		str_format(aLabel, sizeof(aLabel), "%s", HudLayout::Name(Visual.m_Module));
	else
		str_format(aLabel, sizeof(aLabel), "%s (%s)", HudLayout::Name(Visual.m_Module), Localize("preview"));

	const float Width = HudWidth();
	const float Height = HudHeight();
	const float FontSize = 6.4f;
	const float TextWidth = TextRender()->TextWidth(FontSize, aLabel, -1, -1.0f);
	const float LabelW = TextWidth + 8.0f;
	const float LabelH = 12.0f;
	float X = std::clamp(Visual.m_Rect.x + (Visual.m_Rect.w - LabelW) * 0.5f, 2.0f, Width - LabelW - 2.0f);
	float Y = Visual.m_Rect.y - LabelH - 3.0f;
	if(Y < 2.0f)
		Y = minimum(Height - LabelH - 2.0f, Visual.m_Rect.y + Visual.m_Rect.h + 3.0f);

	CUIRect LabelRect = {X, Y, LabelW, LabelH};
	Graphics()->DrawRect(LabelRect.x, LabelRect.y, LabelRect.w, LabelRect.h, ColorRGBA(0.0f, 0.0f, 0.0f, 0.78f), IGraphics::CORNER_ALL, 5.0f);
	Ui()->DoLabel(&LabelRect, aLabel, FontSize, TEXTALIGN_MC);
}

void CHudEditor::RenderChatExtraPreview(const SModuleVisual &Visual) const
{
	CUIRect Inner = Visual.m_Rect;
	Inner.Margin(3.0f, &Inner);
	if(Inner.w < 24.0f || Inner.h < 22.0f)
		return;

	const float SliderW = 3.0f;
	CUIRect Content, Slider;
	Inner.VSplitRight(SliderW, &Content, &Slider);
	Graphics()->DrawRect(Slider.x, Slider.y, Slider.w, Slider.h, ColorRGBA(1.0f, 1.0f, 1.0f, 0.16f), IGraphics::CORNER_ALL, 2.0f);
	Graphics()->DrawRect(Slider.x, Slider.y + Slider.h * 0.35f, Slider.w, Slider.h * 0.25f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.36f), IGraphics::CORNER_ALL, 2.0f);

	CUIRect InputRow;
	Content.HSplitBottom(11.0f, &Content, &InputRow);
	const float TranslateButtonSize = 9.0f;
	CUIRect InputRect, TranslateButtonRect;
	InputRow.VSplitRight(TranslateButtonSize + 2.0f, &InputRect, &TranslateButtonRect);
	Graphics()->DrawRect(InputRect.x, InputRect.y, InputRect.w, InputRect.h, ColorRGBA(1.0f, 1.0f, 1.0f, 0.12f), IGraphics::CORNER_ALL, 2.0f);
	Graphics()->DrawRect(TranslateButtonRect.x, TranslateButtonRect.y, TranslateButtonRect.w, TranslateButtonRect.h, ColorRGBA(0.35f, 0.68f, 1.0f, 0.36f), IGraphics::CORNER_ALL, 2.0f);
	Ui()->DoLabel(&TranslateButtonRect, "T", 6.0f, TEXTALIGN_MC);

	for(int i = 0; i < 4; ++i)
	{
		const float LineY = Content.y + 2.0f + i * 5.0f;
		const float LineW = std::clamp(Content.w - i * 6.0f, 14.0f, Content.w);
		Graphics()->DrawRect(Content.x + 1.0f, LineY, LineW, 2.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.18f), IGraphics::CORNER_ALL, 1.0f);
	}
}

void CHudEditor::RenderColorPickerRow(const char *pLabel, CButtonContainer *pButton, CUi::SColorPickerPopupContext *pPopup, unsigned int *pColorValue, bool Alpha, CUIRect &Row)
{
	(void)pButton;
	CUIRect LabelRect, ButtonRect;
	Row.VSplitRight(22.0f, &LabelRect, nullptr);
	Row.VSplitRight(18.0f, &LabelRect, &ButtonRect);
	Ui()->DoLabel(&LabelRect, pLabel, 6.5f, TEXTALIGN_ML);

	const ColorHSLA HslaColor = ColorHSLA(*pColorValue, Alpha);
	CUIRect Inner = ButtonRect;
	Inner.Margin(2.0f, &Inner);
	ButtonRect.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.2f), IGraphics::CORNER_ALL, 4.0f);
	Inner.Draw(color_cast<ColorRGBA>(HslaColor), IGraphics::CORNER_ALL, 4.0f);

	if(Ui()->IsPopupOpen(pPopup) && pPopup->m_pHslaColor == pColorValue)
	{
		*pColorValue = color_cast<ColorHSLA>(pPopup->m_HsvaColor).Pack(Alpha);
	}
}

void CHudEditor::RenderModulePreview(const SModuleVisual &Visual) const
{
	const CUIRect &Rect = Visual.m_Rect;
	if(Rect.w <= 0.0f || Rect.h <= 0.0f)
		return;
	if(IsLiveRenderedModule(Visual.m_Module))
		return;

	const bool LivePreview = IsLivePreviewModule(Visual.m_Module);
	ColorRGBA Fill = Visual.m_Editable ? ColorRGBA(0.22f, 0.37f, 0.56f, 0.26f) : ColorRGBA(0.25f, 0.25f, 0.25f, 0.22f);
	if(LivePreview)
		Fill = Visual.m_Editable ? ColorRGBA(0.22f, 0.37f, 0.56f, 0.10f) : ColorRGBA(0.25f, 0.25f, 0.25f, 0.08f);
	if(Visual.m_IsFallbackPreview)
		Fill = ColorRGBA(0.30f, 0.26f, 0.20f, 0.20f);
	if(!Visual.m_Enabled)
		Fill = ColorRGBA(0.12f, 0.14f, 0.18f, LivePreview ? 0.22f : 0.30f);
	Graphics()->DrawRect(Rect.x, Rect.y, Rect.w, Rect.h, Fill, IGraphics::CORNER_ALL, Visual.m_Rounding);
	if(LivePreview)
	{
		if(!Visual.m_Enabled)
			Graphics()->DrawRect(Rect.x, Rect.y, Rect.w, Rect.h, ColorRGBA(0.02f, 0.03f, 0.04f, 0.30f), IGraphics::CORNER_ALL, Visual.m_Rounding);
		return;
	}

	auto DrawPreviewRows = [&](const CUIRect &Area, int RowCount, float RowHeight, float RowGap, float Alpha) {
		CUIRect Inner = Area;
		Inner.Margin(3.0f, &Inner);
		for(int i = 0; i < RowCount; ++i)
		{
			const float y = Inner.y + i * (RowHeight + RowGap);
			if(y + RowHeight > Inner.y + Inner.h)
				break;
			const float RowWidth = maximum(10.0f, Inner.w - i * 6.0f);
			Graphics()->DrawRect(Inner.x + 1.0f, y, RowWidth, RowHeight, ColorRGBA(1.0f, 1.0f, 1.0f, Alpha), IGraphics::CORNER_ALL, 1.5f);
		}
	};

	if(Visual.m_Module == HudLayout::MODULE_CHAT)
	{
		RenderChatExtraPreview(Visual);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_VOTES)
	{
		CUIRect Inner = Rect;
		Inner.Margin(3.0f, &Inner);
		Graphics()->DrawRect(Inner.x, Inner.y, Inner.w, 6.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.16f), IGraphics::CORNER_ALL, 2.0f);
		Graphics()->DrawRect(Inner.x, Inner.y + 9.0f, Inner.w * 0.62f, 4.0f, ColorRGBA(0.4f, 0.9f, 0.5f, 0.4f), IGraphics::CORNER_ALL, 2.0f);
		Graphics()->DrawRect(Inner.x + Inner.w * 0.62f, Inner.y + 9.0f, Inner.w * 0.38f, 4.0f, ColorRGBA(0.9f, 0.45f, 0.45f, 0.4f), IGraphics::CORNER_ALL, 2.0f);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_GAME_TIMER)
	{
		CUIRect TimerRect = Rect;
		Ui()->DoLabel(&TimerRect, "00:37", 8.0f, TEXTALIGN_MC);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_SCORE)
	{
		CUIRect Inner = Rect;
		Inner.Margin(4.0f, &Inner);
		Graphics()->DrawRect(Inner.x, Inner.y, Inner.w, 18.0f, ColorRGBA(1.0f, 0.25f, 0.25f, 0.24f), IGraphics::CORNER_L, 3.0f);
		Graphics()->DrawRect(Inner.x, Inner.y + 22.0f, Inner.w, 18.0f, ColorRGBA(0.25f, 0.48f, 1.0f, 0.24f), IGraphics::CORNER_L, 3.0f);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_MOVEMENT_INFO)
	{
		DrawPreviewRows(Rect, 6, 2.2f, 4.8f, 0.20f);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_FPS)
	{
		CUIRect TextRect = Rect;
		TextRect.Margin(2.5f, &TextRect);
		Ui()->DoLabel(&TextRect, "144 FPS", 6.0f, TEXTALIGN_MC);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_PING)
	{
		CUIRect TextRect = Rect;
		TextRect.Margin(2.5f, &TextRect);
		Ui()->DoLabel(&TextRect, "24 ms", 6.0f, TEXTALIGN_MC);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_LOCAL_TIME)
	{
		CUIRect TextRect = Rect;
		TextRect.Margin(2.5f, &TextRect);
		Ui()->DoLabel(&TextRect, "18:42", 6.0f, TEXTALIGN_MC);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_SPECTATOR_COUNT)
	{
		CUIRect Inner = Rect;
		Inner.Margin(3.0f, &Inner);
		Graphics()->DrawRect(Inner.x, Inner.y + 1.5f, 6.0f, 6.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.24f), IGraphics::CORNER_ALL, 3.0f);
		CUIRect TextRect = Inner;
		TextRect.x += 8.5f;
		Ui()->DoLabel(&TextRect, "5 spectators", 5.8f, TEXTALIGN_ML);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_MINI_VOTE)
	{
		CUIRect Inner = Rect;
		Inner.Margin(3.0f, &Inner);
		Graphics()->DrawRect(Inner.x, Inner.y, Inner.w, 6.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.16f), IGraphics::CORNER_ALL, 2.0f);
		Graphics()->DrawRect(Inner.x, Inner.y + 10.0f, Inner.w * 0.52f, 4.0f, ColorRGBA(0.4f, 0.9f, 0.5f, 0.4f), IGraphics::CORNER_ALL, 2.0f);
		Graphics()->DrawRect(Inner.x + Inner.w * 0.52f, Inner.y + 10.0f, Inner.w * 0.48f, 4.0f, ColorRGBA(0.9f, 0.45f, 0.45f, 0.4f), IGraphics::CORNER_ALL, 2.0f);
		DrawPreviewRows(Inner, 3, 2.0f, 3.5f, 0.18f);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_NOTIFY_LAST)
	{
		CUIRect TextRect = Rect;
		TextRect.Margin(3.0f, &TextRect);
		Ui()->DoLabel(&TextRect, "You are last alive!", 5.8f, TEXTALIGN_MC);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_FROZEN_HUD)
	{
		CUIRect Inner = Rect;
		Inner.Margin(3.0f, &Inner);
		const float DotSize = minimum(Inner.h - 2.0f, 9.0f);
		for(int i = 0; i < 6; ++i)
		{
			const float x = Inner.x + i * (DotSize + 2.2f);
			if(x + DotSize > Inner.x + Inner.w)
				break;
			Graphics()->DrawRect(x, Inner.y + (Inner.h - DotSize) * 0.5f, DotSize, DotSize, ColorRGBA(1.0f, 1.0f, 1.0f, i % 2 == 0 ? 0.28f : 0.14f), IGraphics::CORNER_ALL, DotSize * 0.5f);
		}
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_LOCK_CAM)
	{
		CUIRect Inner = Rect;
		Inner.Margin(2.0f, &Inner);
		Graphics()->DrawRect(Inner.x + Inner.w * 0.15f, Inner.y + Inner.h * 0.35f, Inner.w * 0.70f, 1.8f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.24f), IGraphics::CORNER_ALL, 1.0f);
		Graphics()->DrawRect(Inner.x + Inner.w * 0.40f, Inner.y + Inner.h * 0.10f, Inner.w * 0.20f, Inner.h * 0.20f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.24f), IGraphics::CORNER_ALL, 1.0f);
		return;
	}
	if(Visual.m_Module == HudLayout::MODULE_KILLFEED)
	{
		CUIRect Inner = Rect;
		Inner.Margin(3.0f, &Inner);
		for(int i = 0; i < 3; ++i)
		{
			const float RowY = Inner.y + i * 7.0f;
			Graphics()->DrawRect(Inner.x, RowY, 16.0f, 2.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.22f), IGraphics::CORNER_ALL, 1.0f);
			Graphics()->DrawRect(Inner.x + 19.0f, RowY, 4.0f, 2.0f, ColorRGBA(0.95f, 0.55f, 0.55f, 0.36f), IGraphics::CORNER_ALL, 1.0f);
			Graphics()->DrawRect(Inner.x + 26.0f, RowY, minimum(Inner.w - 26.0f, 17.0f), 2.0f, ColorRGBA(1.0f, 1.0f, 1.0f, 0.22f), IGraphics::CORNER_ALL, 1.0f);
		}
		return;
	}

	DrawPreviewRows(Rect, 4, 2.2f, 4.0f, 0.20f);
	if(!Visual.m_Enabled)
		Graphics()->DrawRect(Rect.x, Rect.y, Rect.w, Rect.h, ColorRGBA(0.02f, 0.03f, 0.04f, 0.26f), IGraphics::CORNER_ALL, Visual.m_Rounding);
}

void CHudEditor::RenderOverlay(vec2 MousePos)
{
	const float Width = HudWidth();
	const float Height = HudHeight();
	Graphics()->MapScreen(0.0f, 0.0f, Width, Height);
	Graphics()->TextureClear();
	Graphics()->DrawRect(0.0f, 0.0f, Width, Height, color_cast<ColorRGBA>(ColorHSLA(g_Config.m_BcHudEditorOverlayColor, true)), IGraphics::CORNER_ALL, 0.0f);

	// Draw true HUD previews first, then add interactive editor overlays on top.
	GameClient()->m_MusicPlayer.RenderHudEditor(true);
	GameClient()->m_Chat.RenderHud(true);
	GameClient()->m_Voting.RenderHud(true);
	GameClient()->m_VoiceChat.RenderHudTalkingIndicator(Width, Height, true);
	GameClient()->m_VoiceChat.RenderHudMuteStatusIndicator(Width, Height, true);
	GameClient()->m_Hud.RenderHudEditorModule(HudLayout::MODULE_GAME_TIMER, true);
	GameClient()->m_Hud.RenderHudEditorModule(HudLayout::MODULE_LOCAL_TIME, true);
	GameClient()->m_Hud.RenderHudEditorModule(HudLayout::MODULE_MOVEMENT_INFO, true);
	GameClient()->m_Hud.RenderHudEditorModule(HudLayout::MODULE_SPECTATOR_COUNT, true);
	GameClient()->m_Hud.RenderHudEditorModule(HudLayout::MODULE_SCORE, true);
	GameClient()->m_InfoMessages.RenderHud(true);

	SModuleVisual aVisuals[MAX_MODULE_VISUALS];
	int Count = 0;
	CollectModuleVisuals(aVisuals, Count);
	for(int Pass = 0; Pass < 2; ++Pass)
	{
		const bool RenderEditable = Pass == 1;
		for(int i = 0; i < Count; ++i)
		{
			if(aVisuals[i].m_Editable != RenderEditable)
				continue;
			RenderModulePreview(aVisuals[i]);
		}
	}

	for(int Pass = 0; Pass < 2; ++Pass)
	{
		const bool RenderEditable = Pass == 1;
		for(int i = 0; i < Count; ++i)
		{
			if(aVisuals[i].m_Editable != RenderEditable)
				continue;
			const bool Hovered = aVisuals[i].m_Module == m_HoveredModule;
			const bool Selected = aVisuals[i].m_Module == m_SelectedModule || aVisuals[i].m_Module == m_PressedModule;
			if(!aVisuals[i].m_Enabled)
				Graphics()->DrawRect(aVisuals[i].m_Rect.x, aVisuals[i].m_Rect.y, aVisuals[i].m_Rect.w, aVisuals[i].m_Rect.h, ColorRGBA(0.02f, 0.03f, 0.04f, 0.26f), IGraphics::CORNER_ALL, aVisuals[i].m_Rounding);
			RenderModuleOutline(aVisuals[i], Hovered, Selected);
			if(Hovered)
				RenderModuleLabel(aVisuals[i]);
		}
	}

	const CUIRect DrawerRect = GetDrawerRect(m_DrawerPhase);
	const CUIRect DrawerToggleRect = GetDrawerToggleRect();
	const bool DrawerToggleHovered = PointInRect(MousePos, DrawerToggleRect);
	DrawerRect.Draw(ColorRGBA(0.03f, 0.04f, 0.06f, 0.78f), IGraphics::CORNER_ALL, 6.0f);
	DrawerToggleRect.Draw(DrawerToggleHovered ? ColorRGBA(1.0f, 1.0f, 1.0f, 0.20f) : ColorRGBA(1.0f, 1.0f, 1.0f, 0.12f), IGraphics::CORNER_ALL, 5.0f);
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	Ui()->DoLabel(&DrawerToggleRect, BCUiAnimations::EaseInOutCubic(m_DrawerPhase) > 0.5f ? FontIcon::CHEVRON_LEFT : FontIcon::CHEVRON_RIGHT, 6.5f, TEXTALIGN_MC);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

	if(m_DrawerPhase > 0.02f)
	{
		CUIRect Content = DrawerRect;
		Content.Margin(5.0f, &Content);
		Content.VSplitLeft(16.0f, nullptr, &Content);

		CUIRect ResetRect, Gap, SettingsRect;
		Content.HSplitTop(18.0f, &ResetRect, &Content);
		Content.HSplitTop(6.0f, &Gap, &Content);
		Content.HSplitTop(18.0f, &SettingsRect, &Content);

		const bool ResetHovered = PointInRect(MousePos, ResetRect);
		const ColorRGBA ResetColor = m_PressedOnReset ? ColorRGBA(0.95f, 0.48f, 0.48f, 0.90f) :
			(ResetHovered ? ColorRGBA(0.95f, 0.48f, 0.48f, 0.55f) : ColorRGBA(0.95f, 0.48f, 0.48f, 0.36f));
		ResetRect.Draw(ResetColor, IGraphics::CORNER_ALL, 4.0f);
		Ui()->DoLabel(&ResetRect, Localize("Reset All"), 6.5f, TEXTALIGN_MC);

		const bool SettingsHovered = PointInRect(MousePos, SettingsRect);
		SettingsRect.Draw(SettingsHovered ? ColorRGBA(1.0f, 1.0f, 1.0f, 0.22f) : ColorRGBA(1.0f, 1.0f, 1.0f, 0.14f), IGraphics::CORNER_ALL, 4.0f);
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		Ui()->DoLabel(&SettingsRect, FontIcon::GEAR, 7.0f, TEXTALIGN_MC);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	}

	if(m_SettingsPanelPhase > 0.01f)
	{
		const float Phase = BCUiAnimations::EaseInOutCubic(m_SettingsPanelPhase);
		Graphics()->DrawRect(0.0f, 0.0f, Width, Height, ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f * Phase), IGraphics::CORNER_ALL, 0.0f);
		CUIRect Panel = GetSettingsPanelRect();
		Panel.Draw(ColorRGBA(0.06f, 0.07f, 0.10f, 0.92f), IGraphics::CORNER_ALL, 6.0f);

		CUIRect Body = Panel;
		Body.Margin(7.0f, &Body);
		CUIRect Title;
		Body.HSplitTop(14.0f, &Title, &Body);
		Ui()->DoLabel(&Title, Localize("HUD editor settings"), 7.5f, TEXTALIGN_ML);
		Body.HSplitTop(6.0f, nullptr, &Body);

		CUIRect Row;
		Body.HSplitTop(16.0f, &Row, &Body);
		RenderColorPickerRow(Localize("Overlay"), &m_EditorOverlayColorButton, &m_OverlayColorPickerPopup, &g_Config.m_BcHudEditorOverlayColor, true, Row);
		Body.HSplitTop(4.0f, nullptr, &Body);
		Body.HSplitTop(16.0f, &Row, &Body);
		RenderColorPickerRow(Localize("Outline"), &m_EditorOutlineColorButton, &m_OutlineColorPickerPopup, &g_Config.m_BcHudEditorOutlineColor, true, Row);
		Body.HSplitTop(4.0f, nullptr, &Body);
		Body.HSplitTop(16.0f, &Row, &Body);
		RenderColorPickerRow(Localize("Hover"), &m_EditorHoverColorButton, &m_HoverColorPickerPopup, &g_Config.m_BcHudEditorHoverColor, true, Row);
		Body.HSplitTop(6.0f, nullptr, &Body);

		auto RenderCheck = [&](const char *pLabel, const CUIRect &CheckRow, bool Active) {
			CUIRect Box = CheckRow;
			CUIRect Label = CheckRow;
			Label.VSplitLeft(18.0f, &Box, &Label);
			CUIRect Inner = Box;
			Inner.w = 12.0f;
			Inner.h = 12.0f;
			Inner.y += 2.0f;
			Inner.Draw(Active ? ColorRGBA(0.38f, 0.82f, 0.52f, 0.95f) : ColorRGBA(1.0f, 1.0f, 1.0f, 0.16f), IGraphics::CORNER_ALL, 3.0f);
			Ui()->DoLabel(&Label, pLabel, 6.5f, TEXTALIGN_ML);
		};

		Body.HSplitTop(16.0f, &Row, &Body);
		RenderCheck(Localize("Autocorner"), Row, g_Config.m_BcHudEditorAutoCorner != 0);
		Body.HSplitTop(4.0f, nullptr, &Body);
		Body.HSplitTop(16.0f, &Row, &Body);
		RenderCheck(Localize("Auto snap"), Row, g_Config.m_BcHudEditorAutoSnap != 0);
	}

	Ui()->MapScreen();
	Ui()->RenderPopupMenus();
	Graphics()->MapScreen(0.0f, 0.0f, Width, Height);
	RenderTools()->RenderCursor(MousePos, 12.0f);
}

void CHudEditor::OnRender()
{
	if(!m_Active)
		return;
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		Deactivate();
		return;
	}

	Ui()->StartCheck();
	Ui()->Update();
	Ui()->MapScreen();

	const vec2 WindowSize(maximum(1.0f, (float)Graphics()->WindowWidth()), maximum(1.0f, (float)Graphics()->WindowHeight()));
	const vec2 UiMousePos = Ui()->UpdatedMousePos() * vec2(Ui()->Screen()->w, Ui()->Screen()->h) / WindowSize;
	const vec2 UiToHudScale(HudWidth() / maximum(Ui()->Screen()->w, 1.0f), HudHeight() / maximum(Ui()->Screen()->h, 1.0f));
	const vec2 MousePos = UiMousePos * UiToHudScale;
	const bool LeftDown = Input()->KeyIsPressed(KEY_MOUSE_1);
	const bool RightDown = Input()->KeyIsPressed(KEY_MOUSE_2);
	const bool LeftClicked = LeftDown && !m_MouseDownLast;
	const bool RightClicked = RightDown && !m_RightMouseDownLast;
	BCUiAnimations::UpdatePhase(m_DrawerPhase, m_DrawerOpen ? 1.0f : 0.0f, Client()->RenderFrameTime(), 0.18f);
	BCUiAnimations::UpdatePhase(m_SettingsPanelPhase, m_SettingsPanelOpen ? 1.0f : 0.0f, Client()->RenderFrameTime(), 0.18f);

	const bool PopupOpen = Ui()->IsPopupOpen(&m_SettingsPopupId);
	const bool ColorPopupOpen = Ui()->IsPopupOpen(&m_OverlayColorPickerPopup) || Ui()->IsPopupOpen(&m_OutlineColorPickerPopup) || Ui()->IsPopupOpen(&m_HoverColorPickerPopup);

	const CUIRect DrawerToggleRect = GetDrawerToggleRect();
	const CUIRect DrawerRect = GetDrawerRect(m_DrawerPhase);
	CUIRect DrawerContent = DrawerRect;
	DrawerContent.Margin(5.0f, &DrawerContent);
	DrawerContent.VSplitLeft(16.0f, nullptr, &DrawerContent);
	CUIRect ResetRect, SettingsRect, Tmp;
	DrawerContent.HSplitTop(18.0f, &ResetRect, &DrawerContent);
	DrawerContent.HSplitTop(6.0f, &Tmp, &DrawerContent);
	DrawerContent.HSplitTop(18.0f, &SettingsRect, &DrawerContent);

	CUIRect Panel = GetSettingsPanelRect();
	CUIRect PanelBody = Panel;
	PanelBody.Margin(7.0f, &PanelBody);
	CUIRect Title, OverlayRow, OutlineRow, HoverRow, AutoCornerRow, AutoSnapRow;
	PanelBody.HSplitTop(14.0f, &Title, &PanelBody);
	PanelBody.HSplitTop(6.0f, nullptr, &PanelBody);
	PanelBody.HSplitTop(16.0f, &OverlayRow, &PanelBody);
	PanelBody.HSplitTop(4.0f, nullptr, &PanelBody);
	PanelBody.HSplitTop(16.0f, &OutlineRow, &PanelBody);
	PanelBody.HSplitTop(4.0f, nullptr, &PanelBody);
	PanelBody.HSplitTop(16.0f, &HoverRow, &PanelBody);
	PanelBody.HSplitTop(6.0f, nullptr, &PanelBody);
	PanelBody.HSplitTop(16.0f, &AutoCornerRow, &PanelBody);
	PanelBody.HSplitTop(4.0f, nullptr, &PanelBody);
	PanelBody.HSplitTop(16.0f, &AutoSnapRow, &PanelBody);
	CUIRect OverlayButton = OverlayRow;
	CUIRect OutlineButton = OutlineRow;
	CUIRect HoverButton = HoverRow;
	OverlayButton.VSplitRight(22.0f, &OverlayButton, nullptr);
	OverlayButton.VSplitRight(18.0f, &OverlayButton, &OverlayButton);
	OutlineButton.VSplitRight(22.0f, &OutlineButton, nullptr);
	OutlineButton.VSplitRight(18.0f, &OutlineButton, &OutlineButton);
	HoverButton.VSplitRight(22.0f, &HoverButton, nullptr);
	HoverButton.VSplitRight(18.0f, &HoverButton, &HoverButton);

	auto OpenColorPopup = [&](CUi::SColorPickerPopupContext &Popup, unsigned int *pColorValue) {
		const ColorHSLA HslaColor(*pColorValue, true);
		Popup.m_pHslaColor = pColorValue;
		Popup.m_HslaColor = HslaColor;
		Popup.m_HsvaColor = color_cast<ColorHSVA>(HslaColor);
		Popup.m_RgbaColor = color_cast<ColorRGBA>(Popup.m_HsvaColor);
		Popup.m_Alpha = true;
		Ui()->ShowPopupColorPicker(Ui()->MouseX(), Ui()->MouseY(), &Popup);
	};

	const bool MouseOnDrawer = PointInRect(MousePos, DrawerToggleRect) || (m_DrawerPhase > 0.02f && PointInRect(MousePos, DrawerRect));
	const bool MouseOnSettingsPanel = m_SettingsPanelPhase > 0.01f && PointInRect(MousePos, Panel);
	const bool MouseOnUi = MouseOnDrawer || MouseOnSettingsPanel;

	m_HoveredModule = MouseOnUi ? HudLayout::MODULE_COUNT : HitTestModule(MousePos);

	if(RightClicked && !MouseOnUi && m_HoveredModule != HudLayout::MODULE_COUNT && IsEditableModule(m_HoveredModule))
	{
		m_SelectedModule = m_HoveredModule;
		OpenModuleSettings(GetModuleVisual(m_HoveredModule));
	}

	if(PopupOpen || ColorPopupOpen)
	{
		m_Dragging = false;
		m_PressedModule = HudLayout::MODULE_COUNT;
		m_PressedOnReset = false;
	}
	else if(LeftClicked && PointInRect(MousePos, DrawerToggleRect))
	{
		m_DrawerOpen = !m_DrawerOpen;
		m_PressedOnReset = false;
	}
	else if(LeftClicked && m_SettingsPanelPhase > 0.01f)
	{
		m_PressedOnReset = false;
		if(PointInRect(MousePos, OverlayButton))
			OpenColorPopup(m_OverlayColorPickerPopup, &g_Config.m_BcHudEditorOverlayColor);
		else if(PointInRect(MousePos, OutlineButton))
			OpenColorPopup(m_OutlineColorPickerPopup, &g_Config.m_BcHudEditorOutlineColor);
		else if(PointInRect(MousePos, HoverButton))
			OpenColorPopup(m_HoverColorPickerPopup, &g_Config.m_BcHudEditorHoverColor);
		else if(PointInRect(MousePos, AutoCornerRow))
			g_Config.m_BcHudEditorAutoCorner ^= 1;
		else if(PointInRect(MousePos, AutoSnapRow))
			g_Config.m_BcHudEditorAutoSnap ^= 1;
		else if(!PointInRect(MousePos, Panel))
			m_SettingsPanelOpen = false;
	}
	else if(LeftClicked && m_DrawerPhase > 0.02f && PointInRect(MousePos, ResetRect))
	{
		SModuleVisual aVisuals[MAX_MODULE_VISUALS];
		int Count = 0;
		CollectModuleVisuals(aVisuals, Count);
		for(int i = 0; i < Count; ++i)
		{
			if(IsEditableModule(aVisuals[i].m_Module))
			{
				HudLayout::ResetSettings(aVisuals[i].m_Module);
				ResetModuleExtraSettings(aVisuals[i].m_Module);
			}
		}
		ResetEditorSettings();
		m_Dragging = false;
		m_PressedModule = HudLayout::MODULE_COUNT;
		m_SelectedModule = HudLayout::MODULE_COUNT;
		m_PressedOnReset = true;
		Ui()->ClosePopupMenus();
	}
	else if(LeftClicked && m_DrawerPhase > 0.02f && PointInRect(MousePos, SettingsRect))
	{
		m_SettingsPanelOpen = !m_SettingsPanelOpen;
		m_PressedOnReset = false;
	}
	else if(LeftClicked && !MouseOnUi)
	{
		m_PressedOnReset = false;
		m_PressMousePos = MousePos;
		m_SelectedModule = m_HoveredModule;
		m_PressedModule = (m_HoveredModule != HudLayout::MODULE_COUNT && IsEditableModule(m_HoveredModule)) ? m_HoveredModule : HudLayout::MODULE_COUNT;
		if(m_PressedModule != HudLayout::MODULE_COUNT)
		{
			const SModuleVisual Visual = GetModuleVisual(m_PressedModule);
			m_DragMouseOffset = MousePos - vec2(Visual.m_Rect.x, Visual.m_Rect.y);
		}
	}
	else if(LeftDown && m_MouseDownLast && m_PressedModule != HudLayout::MODULE_COUNT)
	{
		if(!m_Dragging && distance(m_PressMousePos, MousePos) > 2.0f)
			m_Dragging = true;
		UpdateDragging(MousePos);
	}
	else if(!LeftDown && m_MouseDownLast)
	{
		m_Dragging = false;
		m_PressedModule = HudLayout::MODULE_COUNT;
		m_PressedOnReset = false;
	}

	m_MouseDownLast = LeftDown;
	m_RightMouseDownLast = RightDown;

	RenderOverlay(MousePos);
	Ui()->FinishCheck();
	Ui()->ClearHotkeys();
}
