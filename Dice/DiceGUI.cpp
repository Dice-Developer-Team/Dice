#include "DiceGUI.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <cassert>
#include <map>
#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <shellapi.h>
#include <type_traits>
#include <utility>

#include "DiceEvent.h"
#include "DiceConsole.h"
#include "GlobalVar.h"
#include "Jsonio.h"
#include "resource.h"
#include "DDAPI.h"
#include "DiceMod.h"

#pragma comment(lib, "comctl32.lib")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


template <typename T>
class BaseWindow
{
public:
	virtual ~BaseWindow() = default;

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		T* pThis;

		if (uMsg == WM_NCCREATE)
		{
			auto pCreate = reinterpret_cast<LPCREATESTRUCT>(lParam);
			pThis = static_cast<T*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = reinterpret_cast<T*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}

		if (pThis)
		{
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	BaseWindow() : m_hwnd(nullptr)
	{
	}

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = nullptr,
		HMENU hMenu = nullptr
	)
	{
		WNDCLASS wc{};

		wc.lpfnWndProc = T::WindowProc;
		wc.hInstance = hDllModule;
		wc.lpszClassName = ClassName();
		wc.hIcon = static_cast<HICON>(LoadImage(hDllModule, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 256, 256,
		                                         LR_LOADTRANSPARENT | LR_SHARED));

		RegisterClass(&wc);

		m_hwnd = CreateWindowEx(
			dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, hDllModule, this
		);

		return (m_hwnd ? TRUE : FALSE);
	}

	[[nodiscard]] HWND Window() const { return m_hwnd; }

protected:

	[[nodiscard]] virtual PCWSTR ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	HWND m_hwnd;
};

// 基础ListView类
// 使用之前必须先调用InitCommonControl/InitCommonControlEx
class BasicListView
{
public:
	BasicListView() : hwnd(nullptr)
	{
	}

	[[nodiscard]] HWND Window() const { return hwnd; }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = nullptr,
		HMENU hMenu = nullptr
	)
	{
		hwnd = CreateWindowEx(
			dwExStyle, WC_LISTVIEWW, lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, hDllModule, this
		);

		return (hwnd ? TRUE : FALSE);
	}

	int AddTextColumn(const char* pszText, int width = 150, int fmt = LVCFMT_LEFT, int isubItem = -1)
	{
		LVCOLUMN lvC;
		lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		std::u16string wstrText = convert_a2w(pszText);
		lvC.pszText = reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrText.c_str()));
		lvC.cx = width;
		lvC.fmt = fmt;
		if (isubItem == -1)
		{
			HWND header = ListView_GetHeader(hwnd);
			isubItem = Header_GetItemCount(header);
		}
		return ListView_InsertColumn(hwnd, isubItem, &lvC);
	}

	// 带宽度
	void AddAllTextColumn(const std::vector<std::pair<std::string, int>>& texts)
	{
		for (const auto& item : texts)
		{
			AddTextColumn(item.first.c_str(), item.second);
		}
	}

	void AddAllTextColumn(const std::initializer_list<std::string>& texts)
	{
		for (const auto& item : texts)
		{
			AddTextColumn(item.c_str());
		}
	}

	void AddTextRow(const std::initializer_list<std::string>& texts, int index = -1)
	{
		if (texts.size() == 0) return;
		LVITEM lvI;
		lvI.mask = LVIF_TEXT;
		std::u16string wstrText = convert_a2w(texts.begin()->c_str());
		lvI.pszText = reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrText.c_str()));
		if (index == -1)
		{
			index = ListView_GetItemCount(hwnd);
		}
		lvI.iItem = index;
		lvI.iSubItem = 0;
		ListView_InsertItem(hwnd, &lvI);
		int curr = 1;
		for (auto s = texts.begin() + 1; s != texts.end(); s++)
		{
			std::u16string wstrIText = convert_a2w(s->c_str());
			ListView_SetItemText(hwnd, index, curr,  reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrIText.c_str())));
			curr++;
		}
	}

	DWORD SetExtendedListViewStyle(DWORD style)
	{
		return ListView_SetExtendedListViewStyle(hwnd, style);
	}

	[[nodiscard]] int GetItemIndexByText(const std::string& text, int iStart = -1)
	{
		LVFINDINFO info;
		info.flags = LVFI_STRING;
		std::u16string wstrText = convert_a2w(text.c_str());
		info.psz = reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrText.c_str()));
		return ListView_FindItem(hwnd, iStart, &info);
	}

	void SetItemText(const string& text, int index, int subindex = 0)
	{
		if (index < 0)return;
		std::u16string wstrText = convert_a2w(text.c_str());
		ListView_SetItemText(hwnd, index, subindex, reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrText.c_str())));
	}

	// 长度最长为1000
	[[nodiscard]] std::string GetItemText(int index, int subindex = 0)
	{
		wchar_t buffer[1000];
		ListView_GetItemText(hwnd, index, subindex, buffer, 1000);
		return convert_w2a(reinterpret_cast<char16_t*>(buffer));
	}

	BOOL DeleteItemByIndex(int index)
	{
		return ListView_DeleteItem(hwnd, index);
	}

	BOOL DeleteAllItems()
	{
		return ListView_DeleteAllItems(hwnd);
	}

	BOOL DeleteColumn(int iCol = 0)
	{
		return ListView_DeleteColumn(hwnd, iCol);
	}

	BOOL Show(bool show = true)
	{
		return ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
	}

protected:
	HWND hwnd;
};

// 基础Edit类
class BasicEdit
{
public:
	BasicEdit() : hwnd(nullptr)
	{
	}

	[[nodiscard]] HWND Window() const { return hwnd; }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = nullptr,
		HMENU hMenu = nullptr
	)
	{
		hwnd = CreateWindowEx(
			dwExStyle, TEXT("EDIT"), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, hDllModule, this
		);

		return (hwnd ? TRUE : FALSE);
	}

	LRESULT SetFont(HFONT hFont)
	{
		return SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 1);
	}

	void SetText(const std::string& text)
	{
		std::u16string wstrText = convert_a2w(text.c_str());
		Edit_SetText(hwnd, reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrText.c_str())));
	}

	[[nodiscard]] std::string GetText()
	{
		const int length = Edit_GetTextLength(hwnd) + 1;
		const std::unique_ptr<wchar_t[]> uptr = std::make_unique<wchar_t[]>(length);
		if (uptr)
		{
			Edit_GetText(hwnd, uptr.get(), length);
			return convert_w2a(reinterpret_cast<char16_t*>(uptr.get()));
		}
		return "";
	}

	BOOL Show(bool show = true)
	{
		return ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
	}

protected:
	HWND hwnd;
};

// 基础Button类
class BasicButton
{
public:
	BasicButton() : hwnd(nullptr)
	{
	}

	[[nodiscard]] HWND Window() const { return hwnd; }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = nullptr,
		HMENU hMenu = nullptr
	)
	{
		hwnd = CreateWindowEx(
			dwExStyle, TEXT("BUTTON"), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, hDllModule, this
		);

		return (hwnd ? TRUE : FALSE);
	}

	LRESULT SetFont(HFONT hFont)
	{
		return SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 1);
	}

	BOOL Show(bool show = true)
	{
		return ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
	}

	void SetText(const std::string& text)
	{
		std::u16string wstrText = convert_a2w(text.c_str());
		Button_SetText(hwnd, reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrText.c_str())));
	}

protected:
	HWND hwnd;
};

// 基础Static类
class BasicStatic
{
public:
	BasicStatic() : hwnd(nullptr)
	{
	}

	[[nodiscard]] HWND Window() const { return hwnd; }

	BOOL Create(
		PCWSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = nullptr,
		HMENU hMenu = nullptr
	)
	{
		hwnd = CreateWindowEx(
			dwExStyle, TEXT("STATIC"), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, hDllModule, this
		);

		return (hwnd ? TRUE : FALSE);
	}

	LRESULT SetFont(HFONT hFont)
	{
		return SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 1);
	}

	BOOL Show(bool show = true)
	{
		return ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
	}

	void SetText(const std::string& text)
	{
		std::u16string wstrText = convert_a2w(text.c_str());
		Static_SetText(hwnd, reinterpret_cast<wchar_t*>(const_cast<char16_t*>(wstrText.c_str())));
	}

	[[nodiscard]] std::string GetText()
	{
		const int length = Static_GetTextLength(hwnd) + 1;
		const std::unique_ptr<wchar_t[]> uptr = std::make_unique<wchar_t[]>(length);
		if (uptr)
		{
			Static_GetText(hwnd, uptr.get(), length);
			return convert_w2a(reinterpret_cast<char16_t*>(uptr.get()));
		}
		return "";
	}

	void SetBitmap(HBITMAP hBitmap)
	{
		SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
	}

protected:
	HWND hwnd;
};

class DiceGUI final : public BaseWindow<DiceGUI>
{
public:
	int CurrentTab = 0;
	map<int, std::function<LRESULT(bool)>> TabChangeFunction;
	HWND Tab{};

	// CustomMsg
	BasicButton ButtonSaveCustomMsg;
	BasicListView ListViewCustomMsg;
	int ListViewCustomMsgCurrentActivated = -1;
	BasicEdit EditCustomMsg;
	BasicStatic StaticMainLabel;

	// Master
	BasicListView ListViewUserTrust;
	BasicButton ButtonMaster;
	BasicStatic StaticMasterLabel;
	BasicEdit EditMaster;
	BasicStatic StaticUserTrustLabel;
	BasicEdit EditUserTrustID;
	BasicButton ButtonUserRemove;
	BasicEdit EditUserTrustLevel;
	BasicButton ButtonUserTrustLevelSet;
	BasicListView ListViewConfig;
	BasicStatic StaticCurrentSettingLabel;
	BasicStatic StaticCurrentConfigLabel;
	BasicStatic StaticValueLabel;
	BasicEdit EditConfigValue;
	BasicButton ButtonConfigSet;

	// About
	BasicStatic StaticImageDiceLogo;
	BasicStatic StaticVersionInfo;
	BasicStatic StaticAuthorInfo;
	BasicStatic StaticSupportLabel;
	BasicButton ButtonSupport;
	BasicButton ButtonDocument;
	BasicButton ButtonGithub;
	BasicButton ButtonQQGroup;

	// All
	std::map<std::string, HFONT> Fonts;
	std::vector<HBITMAP> Bitmaps;
	std::unordered_map<long long, string> nicknameMp;

	DiceGUI(const std::unordered_map<long long, string>& nicknameMp);
	DiceGUI(std::unordered_map<long long, string>&& nicknameMp);
	[[nodiscard]] PCWSTR ClassName() const override { return TEXT("DiceGUI"); }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	LRESULT CreateCustomMsgPage();
	LRESULT CreateMasterPage();
	LRESULT CreateAboutPage();
	LRESULT ShowCustomMsgPage(bool Show);
	LRESULT ShowMasterPage(bool Show);
	LRESULT ShowAboutPage(bool Show);
};

struct ListViewSorting
{
	BasicListView* pLV;
	int Column;
	bool isAsc;
};

int CALLBACK ListViewUserTrustComp(LPARAM lp1, LPARAM lp2, LPARAM sortParam)
{
	ListViewSorting sort = *reinterpret_cast<ListViewSorting*>(sortParam);
	const bool isAsc = sort.isAsc;
	const int column = sort.Column;
	const std::string str1 = sort.pLV->GetItemText(lp1, column);
	const std::string str2 = sort.pLV->GetItemText(lp2, column);
	try
	{
		switch (column)
		{
		case 0:
			{
				if (isAsc)
					return static_cast<int>(std::clamp(std::stoll(str1) - std::stoll(str2),
					                                   static_cast<long long>(INT_MIN),
					                                   static_cast<long long>(INT_MAX)));
				return static_cast<int>(std::clamp(std::stoll(str2) - std::stoll(str1), static_cast<long long>(INT_MIN),
				                                   static_cast<long long>(INT_MAX)));
			}
		case 1:
			return 0;
		case 2:
			{
				if (isAsc)
					return static_cast<int>(std::clamp(std::stoll(str1) - std::stoll(str2),
					                                   static_cast<long long>(INT_MIN),
					                                   static_cast<long long>(INT_MAX)));
				return static_cast<int>(std::clamp(std::stoll(str2) - std::stoll(str1), static_cast<long long>(INT_MIN),
				                                   static_cast<long long>(INT_MAX)));
			}
		default:
			return 0;
		}
	}
	catch (...)
	{
		return 0;
	}
}

DiceGUI::DiceGUI(std::unordered_map<long long, string>&& nicknameMp) : nicknameMp(std::move(nicknameMp))
{
	TabChangeFunction[0] = std::bind(&DiceGUI::ShowCustomMsgPage, std::ref(*this), std::placeholders::_1);
	TabChangeFunction[1] = std::bind(&DiceGUI::ShowMasterPage, std::ref(*this), std::placeholders::_1);
	TabChangeFunction[2] = std::bind(&DiceGUI::ShowAboutPage, std::ref(*this), std::placeholders::_1);
}

DiceGUI::DiceGUI(const std::unordered_map<long long, string>& nicknameMp) : nicknameMp(nicknameMp)
{
	TabChangeFunction[0] = std::bind(&DiceGUI::ShowCustomMsgPage, std::ref(*this), std::placeholders::_1);
	TabChangeFunction[1] = std::bind(&DiceGUI::ShowMasterPage, std::ref(*this), std::placeholders::_1);
	TabChangeFunction[2] = std::bind(&DiceGUI::ShowAboutPage, std::ref(*this), std::placeholders::_1);
}

LRESULT DiceGUI::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		{
			RECT rcClient; // The parent window's client area.
			GetClientRect(m_hwnd, &rcClient);

			Tab = CreateWindow(WC_TABCONTROL, TEXT(""),
			                    WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | TCS_FIXEDWIDTH,
			                    0, 0, rcClient.right, rcClient.bottom,
			                    m_hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

			TCITEM tie;
			// Tab 0
			tie.mask = TCIF_TEXT;
			tie.pszText = const_cast<wchar_t*>(TEXT("自定义回复"));
			if (TabCtrl_InsertItem(Tab, 0, &tie) == -1)
			{
				DestroyWindow(Tab);
				return -1;
			}
			// Tab 1
			tie.pszText = const_cast<wchar_t*>(TEXT("Master设置"));
			if (TabCtrl_InsertItem(Tab, 1, &tie) == -1)
			{
				DestroyWindow(Tab);
				return -1;
			}
			// Tab 2
			tie.pszText = const_cast<wchar_t*>(TEXT("关于"));
			if (TabCtrl_InsertItem(Tab, 2, &tie) == -1)
			{
				DestroyWindow(Tab);
				return -1;
			}

			// 添加字体/图片等
			Fonts["Yahei14"] = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE,
			                               FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS,
			                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, TEXT("微软雅黑"));
			Fonts["Yahei18"] = CreateFont(18, 0, 0, 0, FW_DONTCARE, FALSE,
			                               FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS,
			                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, TEXT("微软雅黑"));
			Fonts["Yahei22"] = CreateFont(22, 0, 0, 0, FW_DONTCARE, FALSE,
			                               FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS,
			                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE | DEFAULT_PITCH, TEXT("微软雅黑"));

			SendMessage(m_hwnd, WM_SETFONT, (WPARAM)Fonts["Yahei14"], 1);
			SendMessage(Tab, WM_SETFONT, (WPARAM)Fonts["Yahei18"], 1);
			CreateCustomMsgPage();
			ShowCustomMsgPage(true);
			CreateMasterPage();
			ShowMasterPage(false);
			CreateAboutPage();
			ShowAboutPage(false);

			return 0;
		}
	case WM_CLOSE:
		if (MessageBox(m_hwnd, TEXT("未点击保存/设置的项目不会被保存，确认退出?"), TEXT("Dice! GUI"), MB_OKCANCEL) == IDOK)
		{
			DestroyWindow(m_hwnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		for (const auto& font : Fonts)
		{
			DeleteObject(font.second);
		}
		for (const auto& bitmap : Bitmaps)
		{
			DeleteObject(bitmap);
		}
		Fonts.clear();
		Bitmaps.clear();
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(m_hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(m_hwnd, &ps);
		}
		return 0;
	case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			//SetTextColor(hdcStatic, RGB(255, 255, 255));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (INT_PTR)static_cast<HBRUSH>(GetStockObject(COLOR_WINDOW + 1));
		}
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDB_BUTTON_SAVE:
				{
					if (ListViewCustomMsgCurrentActivated == -1) return 0;
					std::string curr = ListViewCustomMsg.GetItemText(ListViewCustomMsgCurrentActivated);
					std::string str = EditCustomMsg.GetText();
					fmt->msg_edit(curr, str);
					ListViewCustomMsg.SetItemText(str, ListViewCustomMsgCurrentActivated, 1);
				}
				return 0;
			case ID_MASTER_BUTTONMASTER:
				{
					std::string str = EditMaster.GetText();
					while (str.length() > 1 && str[0] == '0')str.erase(str.begin());
					if (str == "0") {
						if (console) {
							console.killMaster();
							StaticMasterLabel.SetText("Master模式已关闭");
							MessageBox(nullptr, TEXT("Master模式已关闭√\nmaster已清除"), TEXT("Master模式切换"), MB_OK | MB_ICONINFORMATION);
						}
						return 0;
					}

					if (str.length() < 5 || str.length() > 17)
					{
						MessageBox(m_hwnd, TEXT("QQ号无效!"), TEXT("Dice! GUI"), MB_OK | MB_ICONWARNING);
						return 0;
					}
					long long uid = std::stoll(str);
					StaticMasterLabel.SetText("当前的Master为" + to_string(uid) + "(设置QQ为0以关闭Master模式)");
					if (console)
					{
						if (console != uid)
						{
							console.killMaster();
							console.newMaster(uid);
						}
					}
					else {
						MessageBox(nullptr, reinterpret_cast<wchar_t*>(const_cast<char16_t*>(convert_a2w(console["Private"] ? getMsg("strNewMasterPrivate").c_str() : getMsg("strNewMasterPublic").c_str()).c_str())), TEXT("Master模式初始化"), MB_OK | MB_ICONINFORMATION);
						console.newMaster(uid);
					}
				}
				return 0;
			case ID_MASTER_BUTTONUSERTRUSTREMOVE:
				{
					std::string str = EditUserTrustID.GetText();
					while (str.length() > 1 && str[0] == '0')str.erase(str.begin());
					if (str.length() < 5 || str.length() > 17)
					{
						MessageBox(m_hwnd, TEXT("QQ号无效!"), TEXT("Dice! GUI"), MB_OK | MB_ICONWARNING);
						return 0;
					}
					long long uid = std::stoll(str);
					int ret = ListViewUserTrust.GetItemIndexByText(str);
					if (ret == -1 && !UserList.count(uid))
					{
						MessageBox(m_hwnd, TEXT("找不到此用户"), TEXT("Dice GUI!"), MB_OK | MB_ICONWARNING);
					}
					else
					{
						if (ret != -1)
							ListView_DeleteItem(ListViewUserTrust.Window(), ret);
						if (UserList.count(uid)) UserList.erase(uid);
					}
				}
				return 0;
			case ID_MASTER_BUTTONUSERTRUSTSET:
				{
					std::string str = EditUserTrustID.GetText();
					while (str.length() > 1 && str[0] == '0')str.erase(str.begin());
					if (str.length() < 5 || str.length() > 17)
					{
						MessageBox(m_hwnd, TEXT("QQ号无效!"), TEXT("Dice! GUI"), MB_OK | MB_ICONWARNING);
						return 0;
					}
					long long uid = std::stoll(str);
					int ret = ListViewUserTrust.GetItemIndexByText(str);

					string trust = EditUserTrustLevel.GetText();
					if (trust.length() != 1)
					{
						MessageBox(m_hwnd, TEXT("信任等级无效!"), TEXT("Dice! GUI"), MB_OK | MB_ICONWARNING);
						return 0;
					}
					int trustlevel = std::stoi(trust);
					if (trustlevel < 0 || trustlevel > 5)
					{
						MessageBox(m_hwnd, TEXT("信任等级无效!"), TEXT("Dice! GUI"), MB_OK | MB_ICONWARNING);
						return 0;
					}
					UserList[uid]->trust(trustlevel);

					if (ret == -1)
					{
						string nickname = DD::getQQNick(uid);
						ListViewUserTrust.AddTextRow({str, nickname, trust});
					}
					else
					{
						ListViewUserTrust.SetItemText(trust, ret, 2);
					}
				}
				return 0;
			case ID_MASTER_BUTTONCONFIGSET:
				{
					std::string text = StaticCurrentConfigLabel.GetText();
					int index = ListViewConfig.GetItemIndexByText(text);
					if (index == -1)return 0;
					std::string valstr = EditConfigValue.GetText();
					while (valstr.length() > 1 && valstr[0] == '0')valstr.erase(valstr.begin());
					if (valstr.length() == 0 || valstr.length() > 9)
					{
						MessageBox(m_hwnd, TEXT("属性值无效!"), TEXT("Dice! GUI"), MB_OK | MB_ICONWARNING);
						return 0;
					}
					int val = std::stoi(valstr);
					console.set(text, val);
					ListViewConfig.SetItemText(valstr, index, 1);
				}
				return 0;
			case ID_ABOUT_BUTTONSUPPORT:
				{
					ShellExecute(m_hwnd, TEXT("open"), TEXT("https://afdian.net/@suhuiw4123"), nullptr, nullptr, SW_SHOWDEFAULT);
				}
				return 0;
			case ID_ABOUT_BUTTONGITHUB:
				{
					ShellExecute(m_hwnd, TEXT("open"), TEXT("https://github.com/w4123/Dice"), nullptr, nullptr, SW_SHOWDEFAULT);
				}
				return 0;
			case ID_ABOUT_BUTTONQQGROUP:
				{
					ShellExecute(m_hwnd, TEXT("open"), TEXT("https://jq.qq.com/?_wv=1027&k=Wy9CsHcq"), nullptr, nullptr,
					             SW_SHOWDEFAULT);
				}
				return 0;
			case ID_ABOUT_BUTTONDOCUMENT:
				{
					ShellExecute(m_hwnd, TEXT("open"), TEXT("https://v2docs.kokona.tech"), nullptr, nullptr, SW_SHOWDEFAULT);
				}
				return 0;
			default:
				return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
			}
		}
	case WM_NOTIFY:
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case LVN_ITEMACTIVATE:
				{
					if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDM_LIST_MSG)
					{
						auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
						if (lpnmitem->iItem != -1)
						{
							std::string text = ListViewCustomMsg.GetItemText(lpnmitem->iItem);
							ListViewCustomMsgCurrentActivated = lpnmitem->iItem;
							EditCustomMsg.SetText(getMsg(text));
						}
						return 0;
					}
					if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == ID_MASTER_LISTVIEWUSERTRUST)
					{
						auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
						if (lpnmitem->iItem != -1)
						{
							string text = ListViewUserTrust.GetItemText(lpnmitem->iItem);
							EditUserTrustID.SetText(text);
							string trust = ListViewUserTrust.GetItemText(lpnmitem->iItem, 2);
							EditUserTrustLevel.SetText(trust);
						}
					}
					else if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == ID_MASTER_LISTVIEWCONFIG)
					{
						auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
						if (lpnmitem->iItem != -1)
						{
							string text = ListViewConfig.GetItemText(lpnmitem->iItem);
							StaticCurrentConfigLabel.SetText(text);
							string value = ListViewConfig.GetItemText(lpnmitem->iItem, 1);
							EditConfigValue.SetText(value);
						}
					}
				}
				return 0;
			case LVN_COLUMNCLICK:
				if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == ID_MASTER_LISTVIEWUSERTRUST)
				{
					auto pLVInfo = reinterpret_cast<LPNMLISTVIEW>(lParam);
					static int nSortColumn = 0;
					static BOOL bSortAscending = TRUE;

					// get new sort parameters
					if (pLVInfo->iSubItem == nSortColumn)
						bSortAscending = !bSortAscending;
					else
					{
						nSortColumn = pLVInfo->iSubItem;
						bSortAscending = TRUE;
					}
					ListViewSorting sort{&this->ListViewUserTrust, nSortColumn, static_cast<bool>(bSortAscending)};
					ListView_SortItemsEx(ListViewUserTrust.Window(), ListViewUserTrustComp,
					                     reinterpret_cast<LPARAM>(&sort));
				}
			case TCN_SELCHANGING:
				return 0;
			case TCN_SELCHANGE:
				{
					if (TabChangeFunction[CurrentTab](false)) return -1;
					CurrentTab = TabCtrl_GetCurSel(Tab);
					if (TabChangeFunction[CurrentTab](true)) return -1;
				}
				return 0;
			default:
				return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
			}
		}
	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}

LRESULT DiceGUI::CreateCustomMsgPage()
{
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);

	ButtonSaveCustomMsg.Create(TEXT("保存"), WS_CHILD | WS_VISIBLE, 0,
	                           80, rcClient.bottom - 70, 70, 30, m_hwnd, reinterpret_cast<HMENU>(IDB_BUTTON_SAVE));

	EditCustomMsg.Create(TEXT("请双击列表中的项目"),
	                     WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_BORDER, 0,
	                     300, rcClient.bottom - 80, rcClient.right - rcClient.left - 320, 60, m_hwnd, reinterpret_cast<HMENU>(ID_EDIT));


	StaticMainLabel.Create(TEXT("欢迎来到Dice!自定义回复修改面板\r\n请双击右侧标题，在下方更改文本\r\n更改文本后请点击保存\r\n每个文本修改后均需点击一次"),
	                       WS_CHILD | WS_VISIBLE, 0,
	                       25, 40, 230, 200, m_hwnd, reinterpret_cast<HMENU>(ID_MAINLABEL));

	ListViewCustomMsg.Create(TEXT(""),
	                         WS_CHILD | LVS_REPORT | WS_VISIBLE | WS_BORDER | LVS_SINGLESEL,
	                         0,
	                         300, 40,
	                         rcClient.right - rcClient.left - 320,
	                         rcClient.bottom - rcClient.top - 140,
	                         m_hwnd,
	                         reinterpret_cast<HMENU>(IDM_LIST_MSG));
	ListViewCustomMsg.SetExtendedListViewStyle(
		LVS_EX_DOUBLEBUFFER | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_TWOCLICKACTIVATE | LVS_EX_UNDERLINEHOT);

	ListViewCustomMsg.AddAllTextColumn(std::vector<std::pair<std::string, int>>{ {"标题", 150}, { "内容", 500 }, { "备注", 150 } });
	int index = 0;
	std::shared_lock lock(GlobalMsgMutex);
	for (const auto& item : GlobalMsg)
	{
		ListViewCustomMsg.AddTextRow({ item.first, item.second,getComment(item.first) }, index);
		index++;
	}

	HFONT Yahei18 = Fonts["Yahei18"];
	ButtonSaveCustomMsg.SetFont(Yahei18);
	EditCustomMsg.SetFont(Yahei18);
	StaticMainLabel.SetFont(Yahei18);
	return 0;
}

LRESULT DiceGUI::CreateMasterPage()
{
	RECT rcClient;
	GetClientRect(m_hwnd, &rcClient);
	ButtonMaster.Create(TEXT("设置Master"),
	                    WS_CHILD | WS_VISIBLE, 0, rcClient.right - 180, 40, 140, 30,
	                    m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_BUTTONMASTER));

	StaticMasterLabel.Create(
		reinterpret_cast<wchar_t*>(const_cast<char16_t*>(convert_a2w(!console ? "Master模式已关闭" : ("当前的Master为" + to_string(console) + "(设置QQ为0以关闭Master模式)").c_str()).c_str())),
		WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 0, 30, 40, 450, 30,
		m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_LABELMASTER));

	EditMaster.Create(TEXT("0"),
	                  WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_BORDER,
	                  0, 490, 40, 200, 30,
	                  m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_EDITMASTER));

	ListViewUserTrust.Create(TEXT(""),
	                         WS_CHILD | LVS_REPORT | WS_VISIBLE | WS_BORDER | LVS_SINGLESEL,
	                         0,
	                         30, 140,
	                         500,
	                         rcClient.bottom - rcClient.top - 170,
	                         m_hwnd,
	                         reinterpret_cast<HMENU>(ID_MASTER_LISTVIEWUSERTRUST));
	ListViewUserTrust.SetExtendedListViewStyle(
		LVS_EX_DOUBLEBUFFER | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_TWOCLICKACTIVATE | LVS_EX_UNDERLINEHOT);
	ListViewUserTrust.AddAllTextColumn({"QQ", "昵称", "信任等级"});
	int index = 0;
	for (const auto& item : UserList)
	{
		const string qq = to_string(item.first);
		const string trust = to_string(item.second->nTrust);
		const string nick = nicknameMp[item.first];
		ListViewUserTrust.AddTextRow({qq, nick, trust}, index);
		index++;
	}


	EditUserTrustID.Create(nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 0,
	                       30, 90, 200, 30, m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_EDITUSERTRUSTID));

	ButtonUserRemove.Create(TEXT("移除"), WS_CHILD | WS_VISIBLE, 0, 240, 90, 80, 30, m_hwnd,
	                        reinterpret_cast<HMENU>(ID_MASTER_BUTTONUSERTRUSTREMOVE));

	StaticUserTrustLabel.Create(TEXT("信任等级:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, 0,
	                            330, 90, 60, 30, m_hwnd);

	EditUserTrustLevel.Create(TEXT("0"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 0,
	                          400, 90, 40, 30, m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_EDITUSERTRUSTLEVEL));

	ButtonUserTrustLevelSet.Create(TEXT("设置"), WS_CHILD | WS_VISIBLE, 0, 450, 90, 80, 30, m_hwnd,
	                               reinterpret_cast<HMENU>(ID_MASTER_BUTTONUSERTRUSTSET));

	ListViewConfig.Create(nullptr, WS_CHILD | LVS_REPORT | WS_VISIBLE | WS_BORDER | LVS_SINGLESEL,
	                      0, 550, 140, rcClient.right - rcClient.left - 590, rcClient.bottom - rcClient.top - 170,
	                      m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_LISTVIEWCONFIG));

	ListViewConfig.SetExtendedListViewStyle(
		LVS_EX_DOUBLEBUFFER | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_TWOCLICKACTIVATE | LVS_EX_UNDERLINEHOT);

	ListViewConfig.AddAllTextColumn({"项目", "值"});
	int index1 = 0;
	for (const auto& item : console.intDefault)
	{
		const string value = to_string(console[item.first.c_str()]);
		ListViewConfig.AddTextRow({item.first, value}, index1);
		index1++;
	}

	StaticCurrentSettingLabel.Create(TEXT("当前项目:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, 0,
	                                 550, 90, 60, 30, m_hwnd);

	StaticCurrentConfigLabel.Create(TEXT("无"), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 0,
	                                620, 90, 150, 30, m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_STATICCURRENTCONFIGLABEL));

	StaticValueLabel.Create(TEXT("值:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, 0,
	                        780, 90, 40, 30, m_hwnd);

	EditConfigValue.Create(TEXT("0"), WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_BORDER, 0,
	                       830, 90, 100, 30, m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_EDITCONFIGVALUE));

	ButtonConfigSet.Create(TEXT("设置"), WS_CHILD | WS_VISIBLE, 0,
	                       940, 90, rcClient.right - rcClient.left - 980, 30, m_hwnd, reinterpret_cast<HMENU>(ID_MASTER_BUTTONCONFIGSET));

	HFONT Yahei18 = Fonts["Yahei18"];
	ButtonMaster.SetFont(Yahei18);
	StaticMasterLabel.SetFont(Yahei18);
	EditMaster.SetFont(Yahei18);
	EditUserTrustID.SetFont(Yahei18);
	ButtonUserRemove.SetFont(Yahei18);
	EditUserTrustLevel.SetFont(Yahei18);
	ButtonUserTrustLevelSet.SetFont(Yahei18);
	StaticUserTrustLabel.SetFont(Yahei18);
	StaticCurrentSettingLabel.SetFont(Yahei18);
	StaticCurrentConfigLabel.SetFont(Yahei18);
	StaticValueLabel.SetFont(Yahei18);
	EditConfigValue.SetFont(Yahei18);
	ButtonConfigSet.SetFont(Yahei18);
	return 0;
}

LRESULT DiceGUI::CreateAboutPage()
{
	StaticImageDiceLogo.Create(nullptr, WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_REALSIZECONTROL, 0,
	                           40, 40, 300, 300, m_hwnd);

	StaticVersionInfo.Create(reinterpret_cast<wchar_t*>(const_cast<char16_t*>(convert_a2w(Dice_Full_Ver_On.c_str()).c_str())), WS_CHILD | WS_VISIBLE, 0,
	                         40, 350, 300, 50, m_hwnd);

	StaticAuthorInfo.Create(TEXT("主要作者: 溯洄 Shiki\r\n本程序于AGPLv3协议下开源"), WS_CHILD | WS_VISIBLE, 0,
	                        40, 400, 300, 50, m_hwnd);

	StaticSupportLabel.Create(TEXT("如果您喜欢此应用, 您可以考虑:"), WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 0,
	                          40, 440, 200, 30, m_hwnd);

	ButtonSupport.Create(TEXT("赞助我们"), WS_CHILD | WS_VISIBLE, 0,
	                     260, 440, 80, 30, m_hwnd, reinterpret_cast<HMENU>(ID_ABOUT_BUTTONSUPPORT));

	ButtonDocument.Create(TEXT("访问文档"), WS_CHILD | WS_VISIBLE, 0,
	                      40, 480, 80, 30, m_hwnd, reinterpret_cast<HMENU>(ID_ABOUT_BUTTONDOCUMENT));

	ButtonGithub.Create(TEXT("访问源码"), WS_CHILD | WS_VISIBLE, 0,
	                    150, 480, 80, 30, m_hwnd, reinterpret_cast<HMENU>(ID_ABOUT_BUTTONGITHUB));

	ButtonQQGroup.Create(TEXT("加官方群"), WS_CHILD | WS_VISIBLE, 0,
	                     260, 480, 80, 30, m_hwnd, reinterpret_cast<HMENU>(ID_ABOUT_BUTTONQQGROUP));

	auto hBitmap = static_cast<HBITMAP>(LoadImage(hDllModule, MAKEINTRESOURCE(ID_BITMAP_DICELOGO), IMAGE_BITMAP, 0,
	                                               0, LR_SHARED));
	StaticImageDiceLogo.SetBitmap(hBitmap);


	HFONT Yahei18 = Fonts["Yahei18"];
	StaticVersionInfo.SetFont(Yahei18);
	StaticAuthorInfo.SetFont(Yahei18);
	StaticSupportLabel.SetFont(Yahei18);
	ButtonSupport.SetFont(Yahei18);
	ButtonDocument.SetFont(Yahei18);
	ButtonGithub.SetFont(Yahei18);
	ButtonQQGroup.SetFont(Yahei18);

	return 0;
}

LRESULT DiceGUI::ShowCustomMsgPage(bool Show)
{
	ButtonSaveCustomMsg.Show(Show);
	ListViewCustomMsg.Show(Show);
	EditCustomMsg.Show(Show);
	StaticMainLabel.Show(Show);
	return 0;
}

LRESULT DiceGUI::ShowMasterPage(bool Show)
{
	StaticMasterLabel.Show(Show);
	EditMaster.Show(Show);
	ButtonMaster.Show(Show);
	ListViewUserTrust.Show(Show);
	EditUserTrustID.Show(Show);
	ButtonUserRemove.Show(Show);
	EditUserTrustLevel.Show(Show);
	ButtonUserTrustLevelSet.Show(Show);
	StaticUserTrustLabel.Show(Show);
	ListViewConfig.Show(Show);
	StaticCurrentSettingLabel.Show(Show);
	StaticCurrentConfigLabel.Show(Show);
	StaticValueLabel.Show(Show);
	EditConfigValue.Show(Show);
	ButtonConfigSet.Show(Show);
	return 0;
}

LRESULT DiceGUI::ShowAboutPage(bool Show)
{
	StaticImageDiceLogo.Show(Show);
	StaticVersionInfo.Show(Show);
	StaticAuthorInfo.Show(Show);
	StaticSupportLabel.Show(Show);
	ButtonSupport.Show(Show);
	ButtonDocument.Show(Show);
	ButtonGithub.Show(Show);
	ButtonQQGroup.Show(Show);
	return 0;
}

int WINAPI GUIMain()
{
	// hDllModule不应为空
	assert(hDllModule);

	auto port{ ManagerServer->getListeningPorts()[0] };
	// Dice GUI已被弃用，提示迁移
	if(MessageBox(nullptr, TEXT("Dice! GUI已被弃用，请使用Dice! WebUI。按确认键继续使用GUI，按取消键打开WebUI。"), TEXT("Dice! GUI"), MB_OKCANCEL) == IDCANCEL)
	{
		ShellExecute(nullptr, TEXT("open"), (TEXT("http://127.0.0.1:") + std::to_wstring(port)).c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
		return 0;
	}

	// 初始化CommonControl
	INITCOMMONCONTROLSEX icex; // Structure for control initialization.
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_STANDARD_CLASSES | ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icex);

	// Nickname存储结构
	std::unordered_map<long long, string> nicknameMp;

	bool LoadStranger = true;
	//用户记录过多时，仅加载活跃用户
	bool LoadActiveUser = false;
	
	if (UserList.size() > 1000)
	{
		LoadActiveUser = true;
		LoadStranger = false;
		MessageBox(nullptr, TEXT("用户数量超过1000，仅加载周活跃用户, 跳过非好友用户昵称加载"), TEXT("Dice! GUI"), MB_OK);
	}
	else if (UserList.size() > 100)
	{
		LoadStranger = false;
		MessageBox(nullptr, TEXT("用户数量超过100, 跳过非好友用户昵称加载"), TEXT("Dice! GUI"), MB_OK);
	}

	// 进度条
	HWND progress = CreateWindow(PROGRESS_CLASS, nullptr,
		WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_BORDER | PBS_SMOOTHREVERSE | PBS_SMOOTH
		, CW_USEDEFAULT, CW_USEDEFAULT, 200, 50, nullptr, nullptr, hDllModule, nullptr);
	SendMessage(progress, PBM_SETRANGE, 0, MAKELPARAM(0, UserList.size()));

	SendMessage(progress, PBM_SETSTEP, static_cast<WPARAM>(1), 0);
	ShowWindow(progress, SW_SHOWDEFAULT);

	const std::set<long long> FriendMp{ DD::getFriendQQList() };

	// 获取Nickname
	time_t tLimit{ time(NULL) - (time_t)86400 * 7 };
	for (const auto& item : UserList)
	{
		if (LoadActiveUser && item.second->updated() < tLimit)continue;
		if (FriendMp.count(item.first) || LoadStranger) {
			nicknameMp[item.first] = DD::getQQNick(item.first);
		}
		SendMessage(progress, PBM_STEPIT, 0, 0);
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyWindow(progress);
	
	// 主GUI
	DiceGUI MainWindow(std::move(nicknameMp));

	if (!MainWindow.Create(TEXT("Dice! GUI"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_CLIPSIBLINGS, 0,
	                       CW_USEDEFAULT, CW_USEDEFAULT, 1146, 564))
	{
		return 0;
	}

	ShowWindow(MainWindow.Window(), SW_SHOWDEFAULT);

	// Run the message loop.

	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
#endif
