#include "pch.h"
#include "Wlpm.h"
#include "afxdialogex.h"
#include "LoginDialog.h"
#include "PWManagementDlg.h"
#include "resource.h"
#include <WinUser.h>
#include <TlHelp32.h>

#pragma comment(lib, "user32.lib")

IMPLEMENT_DYNAMIC(LoginDialog, CDialogEx)

LoginDialog::LoginDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGINDIALOG, pParent)
{
#ifndef _WIN32_WCE
	EnableActiveAccessibility();
#endif

	EnableAutomation();
}

LoginDialog::~LoginDialog()
{
	UnregisterRawInput();

	if (!m_securePassword.empty())
	{
		std::fill(m_securePassword.begin(), m_securePassword.end(), L'\0');
		m_securePassword.clear();
	}
}

void LoginDialog::OnFinalRelease()
{
	CDialogEx::OnFinalRelease();
}

void LoginDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(LoginDialog, CDialogEx)
	ON_STN_CLICKED(IDC_STATIC_FORGOT, &LoginDialog::OnStnClickedStaticForgot)
	ON_BN_CLICKED(IDC_CHECK_SHOWPASS, &LoginDialog::OnBnClickedCheckShowpass)
	ON_BN_CLICKED(IDC_BUTTON_ACCESS, &LoginDialog::OnBnClickedButtonAccess)
	ON_BN_CLICKED(IDC_BUTTON2, &LoginDialog::OnBnClickedButton2)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_INPUT, &LoginDialog::OnInput)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(LoginDialog, CDialogEx)
END_DISPATCH_MAP()

static const IID IID_ILoginDialog =
{0x9975e788,0x4c97,0x4081,{0x96,0xdf,0xff,0xf0,0x2b,0xde,0xf8,0x86}};

BEGIN_INTERFACE_MAP(LoginDialog, CDialogEx)
	INTERFACE_PART(LoginDialog, IID_ILoginDialog, Dispatch)
END_INTERFACE_MAP()

BOOL LoginDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Anti screenshot
	HWND hWnd = GetSafeHwnd();
	HMODULE hUser32 = nullptr;
	typedef HRESULT(WINAPI* pSetWindowDisplayAffinity)(HWND, DWORD);
	pSetWindowDisplayAffinity fn = nullptr;
	hUser32 = LoadLibrary(L"user32.dll");
	if (hUser32) {
		pSetWindowDisplayAffinity fn = (pSetWindowDisplayAffinity)GetProcAddress(hUser32, "SetWindowDisplayAffinity");
		if (fn) {
			fn(hWnd, 0x00000011);
		}
		FreeLibrary(hUser32);
	}
	// Set timer
	m_nTimerID = SetTimer(1001, 2000, nullptr);  
	m_bMinimizedByDetection = false;
	if (CWnd* pEdit = GetDlgItem(IDC_EDIT_PASSWORD))
	{
		pEdit->SendMessage(EM_SETPASSWORDCHAR, (WPARAM)L'*');
		pEdit->Invalidate();
		pEdit->UpdateWindow();
	}

	RegisterRawInput();
	return TRUE;
}

void LoginDialog::RegisterRawInput()
{
	if (m_bRawInputRegistered)
		return;

	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x06;
	rid.dwFlags = RIDEV_INPUTSINK;
	rid.hwndTarget = m_hWnd;

	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)))
	{
		m_bRawInputRegistered = TRUE;
	}
	else
	{
		m_bRawInputRegistered = FALSE;
	}
}

void LoginDialog::UnregisterRawInput()
{
	if (!m_bRawInputRegistered)
		return;

	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x06;
	rid.dwFlags = RIDEV_REMOVE;
	rid.hwndTarget = nullptr;

	RegisterRawInputDevices(&rid, 1, sizeof(rid));
	m_bRawInputRegistered = FALSE;
}

wchar_t LoginDialog::ScanCodeToChar(UINT scanCode, BOOL isExtended, BOOL isShift, BOOL isCapsLock)
{
	BYTE keyboardState[256]{};
	if (!GetKeyboardState(keyboardState))
		return 0;

	if (isShift)
		keyboardState[VK_SHIFT] |= 0x80;
	if (isCapsLock)
		keyboardState[VK_CAPITAL] |= 0x01;

	HKL layout = GetKeyboardLayout(0);
	UINT vk = MapVirtualKeyEx(scanCode, MAPVK_VSC_TO_VK_EX, layout);
	if (isExtended)
		vk |= KF_EXTENDED;

	wchar_t unicodeChar[4] = { 0 };
	int result = ToUnicodeEx(vk, scanCode, keyboardState, unicodeChar, 4, 0, layout);
	if (result == 1)
		return unicodeChar[0];

	return 0;
}

void LoginDialog::ProcessRawKeyboardInput(const RAWKEYBOARD& keyboard)
{
	CWnd* pEdit = GetDlgItem(IDC_EDIT_PASSWORD);
	if (!pEdit || GetFocus() != pEdit)
		return;

	if (!(keyboard.Flags & RI_KEY_BREAK))
	{
		UINT scanCode = keyboard.MakeCode;
		BOOL isExtended = (keyboard.Flags & RI_KEY_E0) != 0;

		if (keyboard.VKey == VK_BACK)
		{
			if (!m_securePassword.empty())
				m_securePassword.pop_back();
		}
		else if (keyboard.VKey == VK_RETURN)
		{
			OnBnClickedButtonAccess();
			return;
		}
		else if (keyboard.VKey == VK_TAB || keyboard.VKey == VK_ESCAPE)
		{
			return;
		}
		else
		{
			BOOL isShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
			BOOL isCaps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

			wchar_t ch = ScanCodeToChar(scanCode, isExtended, isShift, isCaps);
			if (ch >= L' ' && ch != L'\r' && ch != L'\n')
			{
				m_securePassword.push_back(ch);
			}
		}

		std::wstring masked(m_securePassword.size(), L'*');
		pEdit->SetWindowTextW(masked.c_str());
		pEdit->SendMessage(EM_SETSEL, (WPARAM)masked.size(), (LPARAM)masked.size());
	}
}

LRESULT LoginDialog::OnInput(WPARAM wParam, LPARAM lParam)
{
	UINT dwSize = 0;
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
	if (dwSize == 0)
		return 0;

	std::vector<BYTE> buffer(dwSize);
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		return 0;

	RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		ProcessRawKeyboardInput(raw->data.keyboard);
	}

	return 0;
}

BOOL LoginDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP || pMsg->message == WM_CHAR)
	{
		CWnd* pEdit = GetDlgItem(IDC_EDIT_PASSWORD);
		if (pEdit && GetFocus() == pEdit)
		{
			if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
			{
				OnBnClickedButtonAccess();
				return TRUE;
			}

			if (pMsg->wParam == VK_TAB || pMsg->wParam == VK_ESCAPE)
			{
				return CDialogEx::PreTranslateMessage(pMsg);
			}

			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}
void LoginDialog::OnStnClickedStaticForgot()
{
	AfxMessageBox(L"Forgot master password means all encrypted passwords cannot be restored!\n\nThere is no recovery mechanism.", MB_ICONWARNING);
}

void LoginDialog::OnBnClickedCheckShowpass()
{
	BOOL bShow = IsDlgButtonChecked(IDC_CHECK_SHOWPASS);
	CWnd* pEdit = GetDlgItem(IDC_EDIT_PASSWORD);
	if (!pEdit)
		return;

	if (bShow)
	{
		pEdit->SendMessage(EM_SETPASSWORDCHAR, 0);
		pEdit->SetWindowTextW(m_securePassword.c_str());
	}
	else
	{
		pEdit->SendMessage(EM_SETPASSWORDCHAR, (WPARAM)L'*');
		std::wstring masked(m_securePassword.size(), L'*');
		pEdit->SetWindowTextW(masked.c_str());
	}

	pEdit->Invalidate();
	pEdit->UpdateWindow();
}

void LoginDialog::OnBnClickedButtonAccess()
{
	if (m_securePassword.empty())
	{
		AfxMessageBox(L"Please enter master password!", MB_ICONWARNING);
		return;
	}
	wchar_t modulePath[MAX_PATH] = { 0 };
	GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
	std::wstring path(modulePath);
	auto pos = path.find_last_of(L"\\/");
	if (pos != std::wstring::npos) path = path.substr(0, pos + 1);
	path += L"passwords.db";

	std::string utf8Path;
	int len = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), nullptr, 0, nullptr, nullptr);
	utf8Path.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), utf8Path.data(), len, nullptr, nullptr);

	sqlite3* db = nullptr;
	if (sqlite3_open(utf8Path.c_str(), &db) != SQLITE_OK)
	{
		AfxMessageBox(L"Failed to open database!", MB_ICONERROR);
		return;
	}

	std::vector<uint8_t> salt(32);
	sqlite3_stmt* stmt = nullptr;
	const char* getSaltSql = "SELECT salt FROM master_secret WHERE id = 1;";
	if (sqlite3_prepare_v2(db, getSaltSql, -1, &stmt, nullptr) == SQLITE_OK)
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			const void* saltBlob = sqlite3_column_blob(stmt, 0);
			int saltLen = sqlite3_column_bytes(stmt, 0);
			if (saltBlob && saltLen == 32)
			{
				salt.assign((const uint8_t*)saltBlob, (const uint8_t*)saltBlob + saltLen);
			}
		}
		sqlite3_finalize(stmt);
	}

	if (salt.size() != 32)
	{
		BCryptGenRandom(nullptr, salt.data(), 32, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		const char* insertSaltSql = "INSERT OR REPLACE INTO master_secret (id, salt) VALUES (1, ?);";
		if (sqlite3_prepare_v2(db, insertSaltSql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_blob(stmt, 1, salt.data(), 32, SQLITE_TRANSIENT);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		}
	}

	std::array<uint8_t, 32> masterKey{};
	if (!DeriveKeyArgon2id(m_securePassword, salt, masterKey))
	{
		sqlite3_close(db);
		AfxMessageBox(L"Failed to derive key!", MB_ICONERROR);
		return;
	}

	bool isValid = true;
	const char* countSql = "SELECT COUNT(*) FROM passwords;";
	if (sqlite3_prepare_v2(db, countSql, -1, &stmt, nullptr) == SQLITE_OK)
	{
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			int count = sqlite3_column_int(stmt, 0);
			if (count > 0)
			{
				const char* testSql = "SELECT ciphertext, nonce, tag FROM passwords LIMIT 1;";
				sqlite3_finalize(stmt);
				if (sqlite3_prepare_v2(db, testSql, -1, &stmt, nullptr) == SQLITE_OK)
				{
					if (sqlite3_step(stmt) == SQLITE_ROW)
					{
						const void* cipherBlob = sqlite3_column_blob(stmt, 0);
						int cipherLen = sqlite3_column_bytes(stmt, 0);
						const void* nonceBlob = sqlite3_column_blob(stmt, 1);
						int nonceLen = sqlite3_column_bytes(stmt, 1);
						const void* tagBlob = sqlite3_column_blob(stmt, 2);
						int tagLen = sqlite3_column_bytes(stmt, 2);

						if (cipherBlob && nonceBlob && tagBlob)
						{
							std::vector<uint8_t> ciphertext((const uint8_t*)cipherBlob, (const uint8_t*)cipherBlob + cipherLen);
							std::vector<uint8_t> nonce((const uint8_t*)nonceBlob, (const uint8_t*)nonceBlob + nonceLen);
							std::vector<uint8_t> tag((const uint8_t*)tagBlob, (const uint8_t*)tagBlob + tagLen);
							std::vector<uint8_t> plaintext;
							isValid = AesGcmDecrypt(masterKey, nonce, tag, ciphertext, plaintext);
						}
					}
					sqlite3_finalize(stmt);
				}
			}
		}
		else
		{
			sqlite3_finalize(stmt);
		}
	}

	sqlite3_close(db);

	if (isValid)
	{
		g_MasterKey = masterKey;
		g_HasMasterKey = true;
		AfxMessageBox(L"Unlock successful!", MB_ICONINFORMATION);

		PWManagementDlg PWManagementDlg;
		EndDialog(IDOK);
		PWManagementDlg.DoModal();

		GetDlgItem(IDC_EDIT_PASSWORD)->SetWindowText(L"");
		CheckDlgButton(IDC_CHECK_SHOWPASS, BST_UNCHECKED);
	}
	else
	{
		AfxMessageBox(L"Wrong master password!", MB_ICONERROR);

		CRect rect;
		GetWindowRect(&rect);
		for (int i = 0; i < 5; i++) {
			SetWindowPos(nullptr, rect.left + 10, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			Sleep(50);
			SetWindowPos(nullptr, rect.left - 10, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			Sleep(50);
		}
		SetWindowPos(nullptr, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}

void LoginDialog::OnBnClickedButton2()
{
}

void LoginDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == m_nTimerID) {
		bool detected = DetectDangerousTool();

		if (detected) {
			if (!IsIconic()) {  
				ShowWindow(SW_MINIMIZE);
			}
			m_bMinimizedByDetection = true;  
		}
		else {
			if (m_bMinimizedByDetection && IsIconic()) {
				ShowWindow(SW_RESTORE);
			}
			m_bMinimizedByDetection = false;
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

bool LoginDialog::DetectDangerousTool()
	{
	const wchar_t* dangerousProcesses[] = {
	L"ShareX.exe",
	L"Greenshot.exe",
	L"Lightshot.exe",   
	L"PicPick.exe",
	L"snagit64.exe",
	L"snagit32.exe",
	L"obs64.exe",
	L"obs32.exe",
	L"bandicam.exe",
	nullptr  
	};

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) return false;

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			for (int i = 0; dangerousProcesses[i] != nullptr; i++) {
				if (_wcsicmp(pe32.szExeFile, dangerousProcesses[i]) == 0) {
					CloseHandle(hSnapshot);
					return true;  
				}
			}
		} while (Process32NextW(hSnapshot, &pe32));
	}

	CloseHandle(hSnapshot);
	return false;
}
void LoginDialog::OnDestroy()
{
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = 0;
	}

	CDialogEx::OnDestroy();
}