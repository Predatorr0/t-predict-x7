/* Copyright © 2026 BestProject Team */
#include "cef_runtime.h"

#include <base/log.h>
#include <base/math.h>
#include <base/str.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/storage.h>

#if defined(CONF_FAMILY_WINDOWS) && defined(CONF_BESTCLIENT_CEF)

#include <include/capi/cef_app_capi.h>
#include <include/capi/cef_browser_capi.h>
#include <include/capi/cef_client_capi.h>
#include <include/capi/cef_frame_capi.h>
#include <include/capi/cef_life_span_handler_capi.h>
#include <include/internal/cef_string.h>
#include <include/internal/cef_types.h>
#include <include/internal/cef_types_win.h>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <filesystem>

namespace
{
std::filesystem::path GetModulePath()
{
	wchar_t aPath[MAX_PATH] = {};
	const DWORD Length = GetModuleFileNameW(nullptr, aPath, sizeof(aPath) / sizeof(aPath[0]));
	if(Length == 0 || Length >= sizeof(aPath) / sizeof(aPath[0]))
		return {};
	return std::filesystem::path(aPath);
}

std::filesystem::path GetModuleDirectory()
{
	const std::filesystem::path ModulePath = GetModulePath();
	return ModulePath.empty() ? std::filesystem::path() : ModulePath.parent_path();
}

void SetCefWideString(cef_string_t &Target, const wchar_t *pText)
{
	cef_string_clear(&Target);
	if(pText != nullptr)
		cef_string_from_wide(pText, std::wcslen(pText), &Target);
}

void SetCefUtf8String(cef_string_t &Target, const char *pText)
{
	cef_string_clear(&Target);
	if(pText != nullptr)
		cef_string_from_utf8(pText, str_length(pText), &Target);
}

template<typename T>
void CefAddRef(T *pObject)
{
	if(pObject != nullptr && pObject->base.add_ref != nullptr)
		pObject->base.add_ref(&pObject->base);
}

template<typename T>
void CefReleaseRef(T *&pObject)
{
	if(pObject != nullptr && pObject->base.release != nullptr)
		pObject->base.release(&pObject->base);
	pObject = nullptr;
}
}

class CBestClientBrowserImpl;

struct SBestClientCefClientHandler
{
	cef_client_t m_Client;
	cef_life_span_handler_t m_LifeSpan;
	std::atomic<int> m_RefCount{1};
	CBestClientBrowserImpl *m_pOwner = nullptr;
};


static SBestClientCefClientHandler *ClientHandlerFromClientBase(cef_base_ref_counted_t *pBase)
{
	auto *pClient = reinterpret_cast<cef_client_t *>(reinterpret_cast<char *>(pBase) - offsetof(cef_client_t, base));
	return reinterpret_cast<SBestClientCefClientHandler *>(reinterpret_cast<char *>(pClient) - offsetof(SBestClientCefClientHandler, m_Client));
}

static SBestClientCefClientHandler *ClientHandlerFromLifeSpanBase(cef_base_ref_counted_t *pBase)
{
	auto *pLifeSpan = reinterpret_cast<cef_life_span_handler_t *>(reinterpret_cast<char *>(pBase) - offsetof(cef_life_span_handler_t, base));
	return reinterpret_cast<SBestClientCefClientHandler *>(reinterpret_cast<char *>(pLifeSpan) - offsetof(SBestClientCefClientHandler, m_LifeSpan));
}

static void CEF_CALLBACK ClientAddRef(cef_base_ref_counted_t *pBase)
{
	ClientHandlerFromClientBase(pBase)->m_RefCount.fetch_add(1, std::memory_order_relaxed);
}

static int CEF_CALLBACK ClientRelease(cef_base_ref_counted_t *pBase)
{
	SBestClientCefClientHandler *pHandler = ClientHandlerFromClientBase(pBase);
	if(pHandler->m_RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
	{
		delete pHandler;
		return 1;
	}
	return 0;
}

static int CEF_CALLBACK ClientHasOneRef(cef_base_ref_counted_t *pBase)
{
	return ClientHandlerFromClientBase(pBase)->m_RefCount.load(std::memory_order_acquire) == 1;
}

static int CEF_CALLBACK ClientHasAtLeastOneRef(cef_base_ref_counted_t *pBase)
{
	return ClientHandlerFromClientBase(pBase)->m_RefCount.load(std::memory_order_acquire) >= 1;
}

static void CEF_CALLBACK LifeSpanAddRef(cef_base_ref_counted_t *pBase)
{
	ClientHandlerFromLifeSpanBase(pBase)->m_RefCount.fetch_add(1, std::memory_order_relaxed);
}

static int CEF_CALLBACK LifeSpanRelease(cef_base_ref_counted_t *pBase)
{
	SBestClientCefClientHandler *pHandler = ClientHandlerFromLifeSpanBase(pBase);
	if(pHandler->m_RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
	{
		delete pHandler;
		return 1;
	}
	return 0;
}

static int CEF_CALLBACK LifeSpanHasOneRef(cef_base_ref_counted_t *pBase)
{
	return ClientHandlerFromLifeSpanBase(pBase)->m_RefCount.load(std::memory_order_acquire) == 1;
}

static int CEF_CALLBACK LifeSpanHasAtLeastOneRef(cef_base_ref_counted_t *pBase)
{
	return ClientHandlerFromLifeSpanBase(pBase)->m_RefCount.load(std::memory_order_acquire) >= 1;
}

class CBestClientBrowserImpl
{
public:
	CBestClientBrowserImpl(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage) :
		m_pClient(pClient),
		m_pGraphics(pGraphics),
		m_pStorage(pStorage)
	{
		str_copy(m_aStatus, "CEF not initialized", sizeof(m_aStatus));
		str_copy(m_aCurrentUrl, "https://www.google.com/", sizeof(m_aCurrentUrl));
	}

	void OnRender()
	{
		if(m_Initialized)
			cef_do_message_loop_work();

		if(m_BoundsDirty)
			ApplyBounds();
	}

	void OnWindowResize()
	{
		if(m_Visible && m_pBrowser != nullptr)
			m_BoundsDirty = true;
	}

	void Shutdown()
	{
		Hide();
		if(m_pBrowser != nullptr)
		{
			cef_browser_host_t *pHost = m_pBrowser->get_host(m_pBrowser);
			if(pHost != nullptr)
			{
				pHost->close_browser(pHost, 1);
				CefReleaseRef(pHost);
			}
			CefReleaseRef(m_pBrowser);
		}

		if(m_pClientHandler != nullptr)
			ClientRelease(&m_pClientHandler->m_Client.base);
		m_pClientHandler = nullptr;

		m_hBrowser = nullptr;
		m_hParent = nullptr;
		m_BoundsDirty = false;
		m_HasAppliedBounds = false;

		if(m_Initialized)
		{
			cef_do_message_loop_work();
			cef_shutdown();
			m_Initialized = false;
		}
	}

	void Show(int X, int Y, int Width, int Height, const char *pUrl)
	{
		if(!EnsureInitialized())
			return;

		const int NewX = maximum(X, 0);
		const int NewY = maximum(Y, 0);
		const int NewWidth = maximum(Width, 1);
		const int NewHeight = maximum(Height, 1);
		const bool BoundsChanged = NewX != m_X || NewY != m_Y || NewWidth != m_Width || NewHeight != m_Height;
		const bool VisibilityChanged = !m_Visible;

		m_X = NewX;
		m_Y = NewY;
		m_Width = NewWidth;
		m_Height = NewHeight;
		m_Visible = true;

		if(pUrl != nullptr && pUrl[0] != '\0' && str_comp(m_aCurrentUrl, pUrl) != 0)
		{
			str_copy(m_aCurrentUrl, pUrl, sizeof(m_aCurrentUrl));
			if(m_pBrowser != nullptr)
			{
				cef_frame_t *pFrame = m_pBrowser->get_main_frame(m_pBrowser);
				if(pFrame != nullptr)
				{
					cef_string_t Url = {};
					SetCefUtf8String(Url, m_aCurrentUrl);
					pFrame->load_url(pFrame, &Url);
					cef_string_clear(&Url);
					CefReleaseRef(pFrame);
				}
			}
		}

		EnsureBrowser();
		if(BoundsChanged || VisibilityChanged || !m_HasAppliedBounds)
			m_BoundsDirty = true;
	}

	void Hide()
	{
		if(!m_Visible)
			return;

		m_Visible = false;
		m_BoundsDirty = true;
	}

	bool IsAvailable() const
	{
		return m_Initialized && m_pBrowser != nullptr && m_hBrowser != nullptr;
	}

	const char *Status() const
	{
		return m_aStatus;
	}

	void OnAfterCreated(cef_browser_t *pBrowser)
	{
		if(pBrowser == nullptr)
			return;

		CefAddRef(pBrowser);
		CefReleaseRef(m_pBrowser);
		m_pBrowser = pBrowser;

		cef_browser_host_t *pHost = m_pBrowser->get_host(m_pBrowser);
		if(pHost != nullptr)
		{
			m_hBrowser = pHost->get_window_handle(pHost);
			CefReleaseRef(pHost);
		}

		m_BoundsDirty = true;
		str_copy(m_aStatus, "CEF browser is ready", sizeof(m_aStatus));
		log_info("bestclient-cef", "browser created");
	}

	void OnBeforeClose(cef_browser_t *pBrowser)
	{
		if(m_pBrowser != nullptr && pBrowser != nullptr && m_pBrowser->is_same != nullptr && m_pBrowser->is_same(m_pBrowser, pBrowser))
		{
			CefReleaseRef(m_pBrowser);
			m_hBrowser = nullptr;
			m_BoundsDirty = false;
			m_HasAppliedBounds = false;
			str_copy(m_aStatus, "CEF browser closed", sizeof(m_aStatus));
		}
	}

private:
	bool EnsureInitialized()
	{
		if(m_Initialized)
			return true;
		if(m_InitializationAttempted)
			return false;

		m_InitializationAttempted = true;
		m_hParent = static_cast<cef_window_handle_t>(m_pGraphics->NativeWindowHandle());
		if(m_hParent == nullptr)
		{
			str_copy(m_aStatus, "CEF init failed: missing native window handle", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return false;
		}

		m_pStorage->CreateFolder("BestClient", IStorage::TYPE_SAVE);
		m_pStorage->CreateFolder("BestClient/cef_cache", IStorage::TYPE_SAVE);

		char aCachePath[IO_MAX_PATH_LENGTH];
		char aLogPath[IO_MAX_PATH_LENGTH];
		m_pStorage->GetCompletePath(IStorage::TYPE_SAVE, "BestClient/cef_cache", aCachePath, sizeof(aCachePath));
		m_pStorage->GetCompletePath(IStorage::TYPE_SAVE, "BestClient/cef.log", aLogPath, sizeof(aLogPath));

		const std::filesystem::path ModulePath = GetModulePath();
		const std::filesystem::path ModuleDir = GetModuleDirectory();
		if(ModulePath.empty() || ModuleDir.empty())
		{
			str_copy(m_aStatus, "CEF init failed: unable to resolve module path", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return false;
		}

		cef_main_args_t MainArgs = {};
		MainArgs.instance = GetModuleHandleW(nullptr);

		cef_settings_t Settings = {};
		Settings.size = sizeof(Settings);
		Settings.no_sandbox = 1;
		Settings.multi_threaded_message_loop = 0;
		Settings.external_message_pump = 0;
		Settings.command_line_args_disabled = 0;
		Settings.log_severity = LOGSEVERITY_VERBOSE;

		SetCefWideString(Settings.browser_subprocess_path, ModulePath.c_str());
		SetCefWideString(Settings.resources_dir_path, ModuleDir.c_str());
		SetCefWideString(Settings.locales_dir_path, (ModuleDir / L"locales").c_str());
		SetCefUtf8String(Settings.cache_path, aCachePath);
		SetCefUtf8String(Settings.log_file, aLogPath);

		const int Initialized = cef_initialize(&MainArgs, &Settings, nullptr, nullptr);
		cef_string_clear(&Settings.browser_subprocess_path);
		cef_string_clear(&Settings.resources_dir_path);
		cef_string_clear(&Settings.locales_dir_path);
		cef_string_clear(&Settings.cache_path);
		cef_string_clear(&Settings.log_file);

		if(!Initialized)
		{
			str_copy(m_aStatus, "CEF init failed: cef_initialize returned false", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return false;
		}

		m_Initialized = true;
		str_copy(m_aStatus, "CEF initialized", sizeof(m_aStatus));
		log_info("bestclient-cef", "initialized");
		return true;
	}

	SBestClientCefClientHandler *CreateClientHandler()
	{
		auto *pHandler = new SBestClientCefClientHandler();
		std::memset(&pHandler->m_Client, 0, sizeof(pHandler->m_Client));
		std::memset(&pHandler->m_LifeSpan, 0, sizeof(pHandler->m_LifeSpan));
		pHandler->m_pOwner = this;

		pHandler->m_Client.base.size = sizeof(pHandler->m_Client);
		pHandler->m_Client.base.add_ref = ClientAddRef;
		pHandler->m_Client.base.release = ClientRelease;
		pHandler->m_Client.base.has_one_ref = ClientHasOneRef;
		pHandler->m_Client.base.has_at_least_one_ref = ClientHasAtLeastOneRef;
		pHandler->m_Client.get_life_span_handler = [](cef_client_t *pSelf) -> cef_life_span_handler_t * {
			SBestClientCefClientHandler *pClientHandler = reinterpret_cast<SBestClientCefClientHandler *>(reinterpret_cast<char *>(pSelf) - offsetof(SBestClientCefClientHandler, m_Client));
			LifeSpanAddRef(&pClientHandler->m_LifeSpan.base);
			return &pClientHandler->m_LifeSpan;
		};

		pHandler->m_LifeSpan.base.size = sizeof(pHandler->m_LifeSpan);
		pHandler->m_LifeSpan.base.add_ref = LifeSpanAddRef;
		pHandler->m_LifeSpan.base.release = LifeSpanRelease;
		pHandler->m_LifeSpan.base.has_one_ref = LifeSpanHasOneRef;
		pHandler->m_LifeSpan.base.has_at_least_one_ref = LifeSpanHasAtLeastOneRef;
		pHandler->m_LifeSpan.on_after_created = [](cef_life_span_handler_t *pSelf, cef_browser_t *pBrowser) {
			SBestClientCefClientHandler *pClientHandler = reinterpret_cast<SBestClientCefClientHandler *>(reinterpret_cast<char *>(pSelf) - offsetof(SBestClientCefClientHandler, m_LifeSpan));
			pClientHandler->m_pOwner->OnAfterCreated(pBrowser);
		};
		pHandler->m_LifeSpan.do_close = [](cef_life_span_handler_t *pSelf, cef_browser_t *pBrowser) -> int {
			(void)pSelf;
			(void)pBrowser;
			return 0;
		};
		pHandler->m_LifeSpan.on_before_close = [](cef_life_span_handler_t *pSelf, cef_browser_t *pBrowser) {
			SBestClientCefClientHandler *pClientHandler = reinterpret_cast<SBestClientCefClientHandler *>(reinterpret_cast<char *>(pSelf) - offsetof(SBestClientCefClientHandler, m_LifeSpan));
			pClientHandler->m_pOwner->OnBeforeClose(pBrowser);
		};

		return pHandler;
	}

	void EnsureBrowser()
	{
		if(m_pBrowser != nullptr || m_hParent == nullptr)
			return;

		if(m_pClientHandler == nullptr)
			m_pClientHandler = CreateClientHandler();

		cef_window_info_t WindowInfo = {};
		WindowInfo.size = sizeof(WindowInfo);
		WindowInfo.style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP;
		WindowInfo.parent_window = m_hParent;
		WindowInfo.bounds.x = m_X;
		WindowInfo.bounds.y = m_Y;
		WindowInfo.bounds.width = m_Width;
		WindowInfo.bounds.height = m_Height;
		WindowInfo.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

		cef_browser_settings_t BrowserSettings = {};
		BrowserSettings.size = sizeof(BrowserSettings);

		cef_string_t Url = {};
		SetCefUtf8String(Url, m_aCurrentUrl);
		cef_browser_t *pCreatedBrowser = cef_browser_host_create_browser_sync(&WindowInfo, &m_pClientHandler->m_Client, &Url, &BrowserSettings, nullptr, nullptr);
		cef_string_clear(&Url);

		if(pCreatedBrowser == nullptr)
		{
			str_copy(m_aStatus, "CEF browser creation failed", sizeof(m_aStatus));
			log_error("bestclient-cef", "%s", m_aStatus);
			return;
		}

		if(m_pBrowser == nullptr)
			OnAfterCreated(pCreatedBrowser);
		CefReleaseRef(pCreatedBrowser);
	}

	void ApplyBounds()
	{
		if(m_hBrowser == nullptr)
			return;

		const bool RectChanged = !m_HasAppliedBounds || m_LastAppliedX != m_X || m_LastAppliedY != m_Y || m_LastAppliedWidth != m_Width || m_LastAppliedHeight != m_Height;
		const bool VisibilityChanged = !m_HasAppliedBounds || m_LastAppliedVisible != m_Visible;
		if(!RectChanged && !VisibilityChanged)
		{
			m_BoundsDirty = false;
			return;
		}

		if(m_Visible)
		{
			SetWindowPos(m_hBrowser, HWND_TOP, m_X, m_Y, m_Width, m_Height, SWP_NOACTIVATE);
			ShowWindow(m_hBrowser, SW_SHOWNOACTIVATE);
		}
		else
		{
			ShowWindow(m_hBrowser, SW_HIDE);
		}

		if(m_pBrowser != nullptr)
		{
			cef_browser_host_t *pHost = m_pBrowser->get_host(m_pBrowser);
			if(pHost != nullptr)
			{
				if(RectChanged)
				{
					pHost->notify_move_or_resize_started(pHost);
					pHost->was_resized(pHost);
				}
				CefReleaseRef(pHost);
			}
		}

		m_LastAppliedX = m_X;
		m_LastAppliedY = m_Y;
		m_LastAppliedWidth = m_Width;
		m_LastAppliedHeight = m_Height;
		m_LastAppliedVisible = m_Visible;
		m_HasAppliedBounds = true;
		m_BoundsDirty = false;
	}

	IClient *m_pClient;
	IGraphics *m_pGraphics;
	IStorage *m_pStorage;
	bool m_InitializationAttempted = false;
	bool m_Initialized = false;
	bool m_Visible = false;
	bool m_BoundsDirty = false;
	bool m_HasAppliedBounds = false;
	int m_X = 0;
	int m_Y = 0;
	int m_Width = 1;
	int m_Height = 1;
	int m_LastAppliedX = 0;
	int m_LastAppliedY = 0;
	int m_LastAppliedWidth = 0;
	int m_LastAppliedHeight = 0;
	bool m_LastAppliedVisible = false;
	cef_window_handle_t m_hParent = nullptr;
	cef_window_handle_t m_hBrowser = nullptr;
	cef_browser_t *m_pBrowser = nullptr;
	SBestClientCefClientHandler *m_pClientHandler = nullptr;
	char m_aCurrentUrl[256];
	char m_aStatus[256];
};

CBestClientBrowser::CBestClientBrowser(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage) :
	m_pImpl(new CBestClientBrowserImpl(pClient, pGraphics, pStorage))
{
}

CBestClientBrowser::~CBestClientBrowser()
{
	delete m_pImpl;
}

void CBestClientBrowser::OnRender()
{
	m_pImpl->OnRender();
}

void CBestClientBrowser::OnWindowResize()
{
	m_pImpl->OnWindowResize();
}

void CBestClientBrowser::Shutdown()
{
	m_pImpl->Shutdown();
}

void CBestClientBrowser::Show(int X, int Y, int Width, int Height, const char *pUrl)
{
	m_pImpl->Show(X, Y, Width, Height, pUrl);
}

void CBestClientBrowser::Hide()
{
	m_pImpl->Hide();
}

bool CBestClientBrowser::IsAvailable() const
{
	return m_pImpl->IsAvailable();
}

const char *CBestClientBrowser::Status() const
{
	return m_pImpl->Status();
}

int BestClientCefExecuteSubprocess(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	cef_main_args_t MainArgs = {};
	MainArgs.instance = GetModuleHandleW(nullptr);
	return cef_execute_process(&MainArgs, nullptr, nullptr);
}

#else

class CBestClientBrowserImpl
{
public:
	CBestClientBrowserImpl(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage)
	{
		(void)pClient;
		(void)pGraphics;
		(void)pStorage;
	}

	void OnRender() {}
	void OnWindowResize() {}
	void Shutdown() {}
	void Show(int X, int Y, int Width, int Height, const char *pUrl)
	{
		(void)X;
		(void)Y;
		(void)Width;
		(void)Height;
		(void)pUrl;
	}
	void Hide() {}
	bool IsAvailable() const { return false; }
	const char *Status() const { return "CEF is only available in Windows builds with CONF_BESTCLIENT_CEF enabled"; }
};

CBestClientBrowser::CBestClientBrowser(IClient *pClient, IGraphics *pGraphics, IStorage *pStorage) :
	m_pImpl(new CBestClientBrowserImpl(pClient, pGraphics, pStorage))
{
}

CBestClientBrowser::~CBestClientBrowser()
{
	delete m_pImpl;
}

void CBestClientBrowser::OnRender()
{
	m_pImpl->OnRender();
}

void CBestClientBrowser::OnWindowResize()
{
	m_pImpl->OnWindowResize();
}

void CBestClientBrowser::Shutdown()
{
	m_pImpl->Shutdown();
}

void CBestClientBrowser::Show(int X, int Y, int Width, int Height, const char *pUrl)
{
	m_pImpl->Show(X, Y, Width, Height, pUrl);
}

void CBestClientBrowser::Hide()
{
	m_pImpl->Hide();
}

bool CBestClientBrowser::IsAvailable() const
{
	return m_pImpl->IsAvailable();
}

const char *CBestClientBrowser::Status() const
{
	return m_pImpl->Status();
}

int BestClientCefExecuteSubprocess(int argc, const char **argv)
{
	(void)argc;
	(void)argv;
	return -1;
}

#endif
