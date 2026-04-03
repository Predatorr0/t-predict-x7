/* Copyright © 2026 BestProject Team */
#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_HUD_EDITOR_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_HUD_EDITOR_H

#include <game/client/component.h>
#include <game/client/components/hud_layout.h>
#include <game/client/ui.h>

class CHudEditor : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	void OnRender() override;
	bool OnInput(const IInput::CEvent &Event) override;
	bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
	void OnStateChange(int NewState, int OldState) override;

	void Activate();
	void Deactivate();
	bool IsActive() const { return m_Active; }

private:
	static constexpr int MAX_MODULE_VISUALS = 24;
	enum EResizeHandle
	{
		RESIZE_NONE = 0,
		RESIZE_TOP_LEFT,
		RESIZE_TOP_RIGHT,
		RESIZE_BOTTOM_LEFT,
		RESIZE_BOTTOM_RIGHT,
	};

	struct SModuleVisual
	{
		HudLayout::EModule m_Module = HudLayout::MODULE_MUSIC_PLAYER;
		CUIRect m_Rect{};
		float m_Rounding = 6.0f;
		bool m_UsesBottomAnchor = false;
		bool m_Editable = false;
		bool m_IsFallbackPreview = false;
	};

	static CUi::EPopupMenuFunctionResult PopupModuleSettings(void *pContext, CUIRect View, bool Active);

	bool m_Active = false;
	bool m_MouseDownLast = false;
	bool m_RightMouseDownLast = false;
	bool m_Dragging = false;
	bool m_Resizing = false;
	bool m_PressedOnReset = false;
	HudLayout::EModule m_PressedModule = HudLayout::MODULE_COUNT;
	HudLayout::EModule m_HoveredModule = HudLayout::MODULE_COUNT;
	HudLayout::EModule m_SelectedModule = HudLayout::MODULE_COUNT;
	EResizeHandle m_HoveredResizeHandle = RESIZE_NONE;
	EResizeHandle m_PressedResizeHandle = RESIZE_NONE;
	vec2 m_DragMouseOffset = vec2(0.0f, 0.0f);
	vec2 m_PressMousePos = vec2(0.0f, 0.0f);
	CUIRect m_ResizeStartRect{};
	vec2 m_ResizeFixedAnchor = vec2(0.0f, 0.0f);
	int m_ResizeStartScale = 100;
	SPopupMenuId m_SettingsPopupId;
	CButtonContainer m_ResetAllButton;

	SModuleVisual GetModuleVisual(HudLayout::EModule Module) const;
	CUIRect GetFallbackModuleRect(HudLayout::EModule Module) const;
	float HudWidth() const;
	float HudHeight() const;
	bool IsEditableModule(HudLayout::EModule Module) const;
	void RenderModulePreview(const SModuleVisual &Visual) const;
	void RenderChatExtraPreview(const SModuleVisual &Visual) const;
	void CollectModuleVisuals(SModuleVisual *pOut, int &Count) const;
	HudLayout::EModule HitTestModule(vec2 MousePos) const;
	EResizeHandle HitTestResizeHandle(vec2 MousePos, HudLayout::EModule &OutModule) const;
	CUIRect GetResizeHandleRect(const CUIRect &Rect, EResizeHandle Handle) const;
	void BeginResize(HudLayout::EModule Module, EResizeHandle Handle);
	void UpdateResizing(vec2 MousePos);
	void UpdateDragging(vec2 MousePos);
	void RenderOverlay(vec2 MousePos);
	void RenderModuleOutline(const SModuleVisual &Visual, bool Hovered, bool Selected) const;
	void RenderModuleLabel(const SModuleVisual &Visual) const;
	void OpenModuleSettings(const SModuleVisual &Visual);
	void ApplyDraggedPosition(HudLayout::EModule Module, const CUIRect &Rect);
	CUIRect SnapRect(const CUIRect &Rect, HudLayout::EModule DraggedModule) const;
};

#endif
