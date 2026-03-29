#include "hud_editor.h"

#include "music_player.h"
#include "voice/voice.h"

#include <base/color.h>
#include <base/math.h>
#include <base/str.h>

#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#include <game/client/components/chat.h>
#include <game/client/components/voting.h>
#include <game/client/gameclient.h>
#include <game/localization.h>

namespace
{
constexpr float SCREEN_W = HudLayout::CANVAS_WIDTH;
constexpr float SCREEN_H = HudLayout::CANVAS_HEIGHT;
constexpr float SNAP_THRESHOLD = 6.0f;

float AutoRounding(const CUIRect &Rect, float Preferred)
{
	if(Preferred > 0.0f)
		return Preferred;
	return std::clamp(minimum(Rect.w, Rect.h) * 0.18f, 3.0f, 12.0f);
}
}

void CHudEditor::Activate()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	m_Active = true;
	m_Dragging = false;
	m_PressedModule = HudLayout::MODULE_COUNT;
	m_HoveredModule = HudLayout::MODULE_COUNT;
	m_SelectedModule = HudLayout::MODULE_COUNT;
	Ui()->ClosePopupMenus();
}

void CHudEditor::Deactivate()
{
	m_Active = false;
	m_Dragging = false;
	m_PressedModule = HudLayout::MODULE_COUNT;
	m_HoveredModule = HudLayout::MODULE_COUNT;
	m_SelectedModule = HudLayout::MODULE_COUNT;
	m_PressedOnReset = false;
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
	(void)x;
	(void)y;
	(void)CursorType;
	return m_Active;
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

CHudEditor::SModuleVisual CHudEditor::GetModuleVisual(HudLayout::EModule Module) const
{
	SModuleVisual Visual;
	Visual.m_Module = Module;
	switch(Module)
	{
	case HudLayout::MODULE_MUSIC_PLAYER:
		Visual.m_Rect = GameClient()->m_MusicPlayer.GetHudEditorRect(!GameClient()->m_MusicPlayer.HudReservation().m_Visible);
		Visual.m_Rounding = 8.0f;
		break;
	case HudLayout::MODULE_VOICE_TALKERS:
		Visual.m_Rect = GameClient()->m_VoiceChat.GetHudTalkingIndicatorRect(SCREEN_W, SCREEN_H, GameClient()->m_VoiceChat.GetHudTalkingIndicatorRect(SCREEN_W, SCREEN_H, false).h <= 0.0f);
		Visual.m_Rounding = 4.0f;
		break;
	case HudLayout::MODULE_VOICE_STATUS:
		Visual.m_Rect = GameClient()->m_VoiceChat.GetHudMuteStatusIndicatorRect(SCREEN_W, SCREEN_H, GameClient()->m_VoiceChat.GetHudMuteStatusIndicatorRect(SCREEN_W, SCREEN_H, false).h <= 0.0f);
		Visual.m_Rounding = 3.0f;
		break;
	case HudLayout::MODULE_CHAT:
		Visual.m_Rect = GameClient()->m_Chat.GetHudRect(SCREEN_W, SCREEN_H, true);
		Visual.m_Rounding = 6.0f;
		Visual.m_UsesBottomAnchor = true;
		break;
	case HudLayout::MODULE_VOTES:
		Visual.m_Rect = GameClient()->m_Voting.GetHudRect(SCREEN_W, SCREEN_H, true);
		Visual.m_Rounding = 3.0f;
		break;
	default:
		break;
	}
	return Visual;
}

void CHudEditor::CollectModuleVisuals(SModuleVisual *pOut, int &Count) const
{
	Count = 0;
	const HudLayout::EModule aModules[] = {
		HudLayout::MODULE_MUSIC_PLAYER,
		HudLayout::MODULE_VOICE_TALKERS,
		HudLayout::MODULE_VOICE_STATUS,
		HudLayout::MODULE_CHAT,
		HudLayout::MODULE_VOTES,
	};
	for(const auto Module : aModules)
		pOut[Count++] = GetModuleVisual(Module);
}

HudLayout::EModule CHudEditor::HitTestModule(vec2 MousePos) const
{
	SModuleVisual aVisuals[5];
	int Count = 0;
	CollectModuleVisuals(aVisuals, Count);
	for(int i = Count - 1; i >= 0; --i)
	{
		const CUIRect &Rect = aVisuals[i].m_Rect;
		if(MousePos.x >= Rect.x && MousePos.x <= Rect.x + Rect.w &&
			MousePos.y >= Rect.y && MousePos.y <= Rect.y + Rect.h)
			return aVisuals[i].m_Module;
	}
	return HudLayout::MODULE_COUNT;
}

void CHudEditor::ApplyDraggedPosition(HudLayout::EModule Module, const CUIRect &Rect)
{
	if(Module == HudLayout::MODULE_CHAT)
		HudLayout::SetPosition(Module, Rect.x, Rect.y + Rect.h);
	else
		HudLayout::SetPosition(Module, Rect.x, Rect.y);
}

CUIRect CHudEditor::SnapRect(const CUIRect &Rect, HudLayout::EModule DraggedModule) const
{
	CUIRect Result = Rect;
	SModuleVisual aVisuals[5];
	int Count = 0;
	CollectModuleVisuals(aVisuals, Count);

	auto TrySnap = [](float Candidate, float Target, float &BestDelta) {
		const float Delta = Target - Candidate;
		if(absolute(Delta) <= SNAP_THRESHOLD && absolute(Delta) < absolute(BestDelta))
			BestDelta = Delta;
	};

	float BestDeltaX = SNAP_THRESHOLD + 1.0f;
	float BestDeltaY = SNAP_THRESHOLD + 1.0f;

	TrySnap(Result.x, 0.0f, BestDeltaX);
	TrySnap(Result.x + Result.w, SCREEN_W, BestDeltaX);
	TrySnap(Result.x + Result.w * 0.5f, SCREEN_W * 0.5f, BestDeltaX);
	TrySnap(Result.y, 0.0f, BestDeltaY);
	TrySnap(Result.y + Result.h, SCREEN_H, BestDeltaY);
	TrySnap(Result.y + Result.h * 0.5f, SCREEN_H * 0.5f, BestDeltaY);

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

	Result.x = std::clamp(Result.x, 0.0f, maximum(0.0f, SCREEN_W - Result.w));
	Result.y = std::clamp(Result.y, 0.0f, maximum(0.0f, SCREEN_H - Result.h));
	return Result;
}

void CHudEditor::UpdateDragging(vec2 MousePos)
{
	if(!m_Dragging || m_PressedModule == HudLayout::MODULE_COUNT)
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

	CUIRect Title, Slider;
	View.HSplitTop(16.0f, &Title, &View);
	char aTitle[128];
	str_format(aTitle, sizeof(aTitle), "\"%s\" settings", HudLayout::Name(pThis->m_SelectedModule));
	pThis->Ui()->DoLabel(&Title, aTitle, 10.0f, TEXTALIGN_ML);
	View.HSplitTop(6.0f, nullptr, &View);
	View.HSplitTop(20.0f, &Slider, &View);

	int *pScale = nullptr;
	switch(pThis->m_SelectedModule)
	{
	case HudLayout::MODULE_MUSIC_PLAYER: pScale = &g_Config.m_BcHudMusicPlayerScale; break;
	case HudLayout::MODULE_VOICE_TALKERS: pScale = &g_Config.m_BcHudVoiceHudScale; break;
	case HudLayout::MODULE_VOICE_STATUS: pScale = &g_Config.m_BcHudVoiceMuteIconsScale; break;
	case HudLayout::MODULE_CHAT: pScale = &g_Config.m_BcHudChatScale; break;
	case HudLayout::MODULE_VOTES: pScale = &g_Config.m_BcHudVotesScale; break;
	default: break;
	}
	if(pScale != nullptr)
		pThis->Ui()->DoScrollbarOption(pScale, pScale, &Slider, Localize("Scale"), 25, 300, &CUi::ms_LinearScrollbarScale, 0u, "%");
	return CUi::POPUP_KEEP_OPEN;
}

void CHudEditor::OpenModuleSettings(const SModuleVisual &Visual)
{
	m_SelectedModule = Visual.m_Module;
	Ui()->DoPopupMenu(&m_SettingsPopupId, minimum(Visual.m_Rect.x + Visual.m_Rect.w + 8.0f, SCREEN_W - 180.0f), maximum(2.0f, Visual.m_Rect.y), 180.0f, 42.0f, this, PopupModuleSettings);
}

void CHudEditor::RenderModuleOutline(const SModuleVisual &Visual, bool Hovered, bool Selected) const
{
	const CUIRect &Rect = Visual.m_Rect;
	const float Thickness = Selected ? 1.6f : 1.0f;
	const ColorRGBA Color = Selected ? ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f) : (Hovered ? ColorRGBA(1.0f, 1.0f, 1.0f, 0.92f) : ColorRGBA(1.0f, 1.0f, 1.0f, 0.65f));
	Graphics()->DrawRect(Rect.x, Rect.y, Rect.w, Thickness, Color, IGraphics::CORNER_NONE, 0.0f);
	Graphics()->DrawRect(Rect.x, Rect.y + Rect.h - Thickness, Rect.w, Thickness, Color, IGraphics::CORNER_NONE, 0.0f);
	Graphics()->DrawRect(Rect.x, Rect.y, Thickness, Rect.h, Color, IGraphics::CORNER_NONE, 0.0f);
	Graphics()->DrawRect(Rect.x + Rect.w - Thickness, Rect.y, Thickness, Rect.h, Color, IGraphics::CORNER_NONE, 0.0f);
}

void CHudEditor::RenderModuleLabel(const SModuleVisual &Visual) const
{
	char aLabel[64];
	str_copy(aLabel, HudLayout::Name(Visual.m_Module), sizeof(aLabel));
	const float FontSize = 6.5f;
	const float TextWidth = TextRender()->TextWidth(FontSize, aLabel, -1, -1.0f);
	const float LabelW = TextWidth + 8.0f;
	const float LabelH = 12.0f;
	float X = std::clamp(Visual.m_Rect.x + (Visual.m_Rect.w - LabelW) * 0.5f, 2.0f, SCREEN_W - LabelW - 2.0f);
	float Y = Visual.m_Rect.y - LabelH - 3.0f;
	if(Y < 2.0f)
		Y = minimum(SCREEN_H - LabelH - 2.0f, Visual.m_Rect.y + Visual.m_Rect.h + 3.0f);
	CUIRect LabelRect = {X, Y, LabelW, LabelH};
	Graphics()->DrawRect(LabelRect.x, LabelRect.y, LabelRect.w, LabelRect.h, ColorRGBA(0.0f, 0.0f, 0.0f, 0.78f), IGraphics::CORNER_ALL, 5.0f);
	Ui()->DoLabel(&LabelRect, aLabel, FontSize, TEXTALIGN_MC);
}

void CHudEditor::RenderOverlay(vec2 MousePos)
{
	Graphics()->MapScreen(0.0f, 0.0f, SCREEN_W, SCREEN_H);
	Graphics()->TextureClear();
	Graphics()->DrawRect(0.0f, 0.0f, SCREEN_W, SCREEN_H, ColorRGBA(0.0f, 0.0f, 0.0f, 0.5f), IGraphics::CORNER_ALL, 0.0f);

	const bool MusicPreview = !GameClient()->m_MusicPlayer.HudReservation().m_Visible;
	GameClient()->m_MusicPlayer.RenderHudEditor(MusicPreview);
	GameClient()->m_VoiceChat.RenderHudTalkingIndicator(SCREEN_W, SCREEN_H, GameClient()->m_VoiceChat.GetHudTalkingIndicatorRect(SCREEN_W, SCREEN_H, false).h <= 0.0f);
	GameClient()->m_VoiceChat.RenderHudMuteStatusIndicator(SCREEN_W, SCREEN_H, GameClient()->m_VoiceChat.GetHudMuteStatusIndicatorRect(SCREEN_W, SCREEN_H, false).h <= 0.0f);
	GameClient()->m_Chat.RenderHud(true);
	GameClient()->m_Voting.RenderHud(!GameClient()->m_Voting.IsVoting());

	SModuleVisual aVisuals[5];
	int Count = 0;
	CollectModuleVisuals(aVisuals, Count);
	for(int i = 0; i < Count; ++i)
	{
		const bool Hovered = aVisuals[i].m_Module == m_HoveredModule;
		const bool Selected = aVisuals[i].m_Module == m_SelectedModule || aVisuals[i].m_Module == m_PressedModule;
		RenderModuleOutline(aVisuals[i], Hovered, Selected);
		if(Hovered)
			RenderModuleLabel(aVisuals[i]);
	}

	CUIRect ResetRect = {8.0f, 8.0f, 56.0f, 14.0f};
	if(GameClient()->m_Menus.DoButton_Menu(&m_ResetAllButton, Localize("Reset All"), 0, &ResetRect))
		HudLayout::ResetEditableModules();

	Ui()->RenderPopupMenus();
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
	const vec2 UiToHudScale(SCREEN_W / Ui()->Screen()->w, SCREEN_H / Ui()->Screen()->h);
	const vec2 MousePos = UiMousePos * UiToHudScale;
	const bool MouseDown = Input()->KeyIsPressed(KEY_MOUSE_1);

	m_HoveredModule = HitTestModule(MousePos);

	if(MouseDown && !m_MouseDownLast)
	{
		m_PressMousePos = MousePos;
		m_PressedModule = m_HoveredModule;
		m_PressedOnReset = false;
		if(m_PressedModule != HudLayout::MODULE_COUNT)
		{
			const SModuleVisual Visual = GetModuleVisual(m_PressedModule);
			m_DragMouseOffset = MousePos - vec2(Visual.m_Rect.x, Visual.m_Rect.y);
		}
	}
	else if(MouseDown && m_MouseDownLast && m_PressedModule != HudLayout::MODULE_COUNT)
	{
		if(!m_Dragging && distance(m_PressMousePos, MousePos) > 2.0f)
			m_Dragging = true;
		UpdateDragging(MousePos);
	}
	else if(!MouseDown && m_MouseDownLast)
	{
		if(m_PressedModule != HudLayout::MODULE_COUNT && !m_Dragging)
			OpenModuleSettings(GetModuleVisual(m_PressedModule));
		m_Dragging = false;
		m_PressedModule = HudLayout::MODULE_COUNT;
	}
	m_MouseDownLast = MouseDown;

	RenderOverlay(MousePos);
	Ui()->FinishCheck();
	Ui()->ClearHotkeys();
}
