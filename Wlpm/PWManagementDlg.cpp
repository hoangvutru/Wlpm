#include "pch.h"
#include "Wlpm.h"
#include "afxdialogex.h"
#include "PWManagementDlg.h"
#include "resource.h"
#include <WinUser.h>

#pragma comment(lib,"user32.lib")
IMPLEMENT_DYNAMIC(PWManagementDlg, CDialogEx)

PWManagementDlg::PWManagementDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PWMANAGEMENT, pParent)
{

}

PWManagementDlg::~PWManagementDlg()
{
}

void PWManagementDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, ID_PWLIST, pwlist_ctrl);
	DDX_Control(pDX, ID_CHECK_SHOWPW, showpw_ctrl);
	DDX_Control(pDX, ID_CHECK_UPDATEPW, updatepw_ctrl);
	DDX_Control(pDX, IDC_EDIT1, username_ctrl);
	DDX_Control(pDX, IDC_EDIT2, password_ctrl);
	DDX_Control(pDX, ID_DELETE_BTN, delete_btn_ctrl);
	DDX_Control(pDX, ID_COPY_USERNAME, copy_username_ctrl);
	DDX_Control(pDX, ID_COPY_PASSWORD, copy_pw_ctrl);
	DDX_Control(pDX, ID_WEBSITE, website_ctrl);
	DDX_Control(pDX, ID_UPDATE_BTN, update_btn_ctrl);
	DDX_Control(pDX, ID_COPY_NOTIFY, copy_notify_ctrl);
	DDX_Control(pDX, ID_ADD_BTN, add_btn_ctrl);
}


BEGIN_MESSAGE_MAP(PWManagementDlg, CDialogEx)
	ON_BN_CLICKED(ID_CHECK_SHOWPW, &PWManagementDlg::OnBnClickedCheckShowpw)
	ON_NOTIFY(LVN_ITEMCHANGED, ID_PWLIST, &PWManagementDlg::OnLvnItemchangedPwlist)
	ON_BN_CLICKED(ID_DELETE_BTN, &PWManagementDlg::OnBnClickedDeleteBtn)
	ON_BN_CLICKED(ID_CHECK_UPDATEPW, &PWManagementDlg::OnBnClickedCheckUpdatepw)
	ON_BN_CLICKED(ID_COPY_USERNAME, &PWManagementDlg::OnBnClickedCopyUsername)
	ON_BN_CLICKED(ID_COPY_PASSWORD, &PWManagementDlg::OnBnClickedCopyPassword)
	ON_BN_CLICKED(ID_UPDATE_BTN, &PWManagementDlg::OnBnClickedUpdateBtn)
	ON_EN_CHANGE(ID_WEBSITE, &PWManagementDlg::OnEnChangeWebsite)
	ON_EN_CHANGE(IDC_EDIT1, &PWManagementDlg::OnEnChangeEdit1)
	ON_EN_CHANGE(ID_PW, &PWManagementDlg::OnEnChangePw)
	ON_BN_CLICKED(ID_ADD_BTN, &PWManagementDlg::OnBnClickedAddBtn)
END_MESSAGE_MAP()

struct DbEntry
{
	int id;
	CString website;
	CString username;
	CString password;
	std::vector<uint8_t> ciphertext;
	std::vector<uint8_t> nonce;
	std::vector<uint8_t> tag;
};
	
static sqlite3* g_db = nullptr;

static std::string WStrToUtf8(const std::wstring& ws)
{
	if (ws.empty()) return {};
	int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
	std::string out(len, 0);
	WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), out.data(), len, nullptr, nullptr);
	return out;
}

static bool EnsureDb()
{
	if (g_db) return true;

	wchar_t modulePath[MAX_PATH] = { 0 };
	GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
	std::wstring path(modulePath);
	auto pos = path.find_last_of(L"\\/");
	if (pos != std::wstring::npos) path = path.substr(0, pos + 1);
	path += L"passwords.db";

	std::string utf8Path = WStrToUtf8(path);
	if (sqlite3_open(utf8Path.c_str(), &g_db) != SQLITE_OK)
	{
		return false;
	}

	const char* createMasterSql =
		"CREATE TABLE IF NOT EXISTS master_secret ("
		"id INTEGER PRIMARY KEY,"
		"salt BLOB NOT NULL);";
	char* errMsg = nullptr;
	if (sqlite3_exec(g_db, createMasterSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
	{
		sqlite3_free(errMsg);
		sqlite3_close(g_db);
		g_db = nullptr;
		return false;
	}

	const char* createSql =
		"CREATE TABLE IF NOT EXISTS passwords ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"website TEXT NOT NULL,"
		"username TEXT NOT NULL,"
		"ciphertext BLOB NOT NULL,"
		"nonce BLOB NOT NULL,"
		"tag BLOB NOT NULL);";
	if (sqlite3_exec(g_db, createSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
	{
		sqlite3_free(errMsg);
		sqlite3_close(g_db);
		g_db = nullptr;
		return false;
	}
	return true;
}

static std::vector<DbEntry> LoadAllEntries()
{
	std::vector<DbEntry> rows;
	if (!EnsureDb() || !g_HasMasterKey) return rows;

	sqlite3_stmt* stmt = nullptr;
	const char* sql = "SELECT id, website, username, ciphertext, nonce, tag FROM passwords ORDER BY id;";
	if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
		return rows;

	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		DbEntry e;
		e.id = sqlite3_column_int(stmt, 0);
		e.website = CString((LPCSTR)sqlite3_column_text(stmt, 1));
		e.username = CString((LPCSTR)sqlite3_column_text(stmt, 2));
		
		const void* cipherBlob = sqlite3_column_blob(stmt, 3);
		int cipherLen = sqlite3_column_bytes(stmt, 3);
		const void* nonceBlob = sqlite3_column_blob(stmt, 4);
		int nonceLen = sqlite3_column_bytes(stmt, 4);
		const void* tagBlob = sqlite3_column_blob(stmt, 5);
		int tagLen = sqlite3_column_bytes(stmt, 5);

		if (cipherBlob && nonceBlob && tagBlob)
		{
			e.ciphertext.assign((const uint8_t*)cipherBlob, (const uint8_t*)cipherBlob + cipherLen);
			e.nonce.assign((const uint8_t*)nonceBlob, (const uint8_t*)nonceBlob + nonceLen);
			e.tag.assign((const uint8_t*)tagBlob, (const uint8_t*)tagBlob + tagLen);

			std::vector<uint8_t> plainBytes;
			if (AesGcmDecrypt(g_MasterKey, e.nonce, e.tag, e.ciphertext, plainBytes))
			{
				std::string utf8Pw((const char*)plainBytes.data(), plainBytes.size());
				int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Pw.c_str(), (int)utf8Pw.size(), nullptr, 0);
				std::vector<wchar_t> wbuf(wlen + 1);
				MultiByteToWideChar(CP_UTF8, 0, utf8Pw.c_str(), (int)utf8Pw.size(), wbuf.data(), wlen);
				e.password = CString(wbuf.data());
			}
			else
			{
				e.password = L"[Decrypt failed]";
			}
		}
		rows.push_back(std::move(e));
	}
	sqlite3_finalize(stmt);
	return rows;
}

static void RefreshList(PWManagementDlg* dlg)
{
	dlg->pwlist_ctrl.DeleteAllItems();
	auto entries = LoadAllEntries();
	for (const auto& entry : entries)
	{
		CString idStr;
		idStr.Format(L"%d", entry.id);
		int index = dlg->pwlist_ctrl.GetItemCount();
		dlg->pwlist_ctrl.InsertItem(index, idStr);
		dlg->pwlist_ctrl.SetItemText(index, 1, entry.website);
		dlg->pwlist_ctrl.SetItemText(index, 2, entry.username);
		dlg->pwlist_ctrl.SetItemText(index, 3, L"******");
	}
}

static bool GetEntryById(int id, DbEntry& out)
{
	if (!EnsureDb() || !g_HasMasterKey) return false;
	sqlite3_stmt* stmt = nullptr;
	const char* sql = "SELECT id, website, username, ciphertext, nonce, tag FROM passwords WHERE id = ?;";
	if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
	sqlite3_bind_int(stmt, 1, id);
	bool ok = false;
	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		out.id = sqlite3_column_int(stmt, 0);
		out.website = CString((LPCSTR)sqlite3_column_text(stmt, 1));
		out.username = CString((LPCSTR)sqlite3_column_text(stmt, 2));
		
		const void* cipherBlob = sqlite3_column_blob(stmt, 3);
		int cipherLen = sqlite3_column_bytes(stmt, 3);
		const void* nonceBlob = sqlite3_column_blob(stmt, 4);
		int nonceLen = sqlite3_column_bytes(stmt, 4);
		const void* tagBlob = sqlite3_column_blob(stmt, 5);
		int tagLen = sqlite3_column_bytes(stmt, 5);

		if (cipherBlob && nonceBlob && tagBlob)
		{
			out.ciphertext.assign((const uint8_t*)cipherBlob, (const uint8_t*)cipherBlob + cipherLen);
			out.nonce.assign((const uint8_t*)nonceBlob, (const uint8_t*)nonceBlob + nonceLen);
			out.tag.assign((const uint8_t*)tagBlob, (const uint8_t*)tagBlob + tagLen);

			std::vector<uint8_t> plainBytes;
			if (AesGcmDecrypt(g_MasterKey, out.nonce, out.tag, out.ciphertext, plainBytes))
			{
				std::string utf8Pw((const char*)plainBytes.data(), plainBytes.size());
				int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Pw.c_str(), (int)utf8Pw.size(), nullptr, 0);
				std::vector<wchar_t> wbuf(wlen + 1);
				MultiByteToWideChar(CP_UTF8, 0, utf8Pw.c_str(), (int)utf8Pw.size(), wbuf.data(), wlen);
				out.password = CString(wbuf.data());
				ok = true;
			}
		}
	}
	sqlite3_finalize(stmt);
	return ok;
}

static bool InsertEntry(const CString& website, const CString& username, const CString& password)
{
	if (!EnsureDb() || !g_HasMasterKey) return false;
	
	std::vector<uint8_t> plainBytes = CStringToBytes(password);
	std::vector<uint8_t> ciphertext, nonce, tag;
	if (!AesGcmEncrypt(g_MasterKey, plainBytes, ciphertext, nonce, tag))
		return false;

	sqlite3_stmt* stmt = nullptr;
	const char* sql = "INSERT INTO passwords (website, username, ciphertext, nonce, tag) VALUES (?, ?, ?, ?, ?);";
	if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

	auto w = WStrToUtf8((LPCTSTR)website);
	auto u = WStrToUtf8((LPCTSTR)username);
	sqlite3_bind_text(stmt, 1, w.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, u.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 3, ciphertext.data(), (int)ciphertext.size(), SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 4, nonce.data(), (int)nonce.size(), SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 5, tag.data(), (int)tag.size(), SQLITE_TRANSIENT);

	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

static bool UpdateEntry(int id, const CString& website, const CString& username, const CString& password)
{
	if (!EnsureDb() || !g_HasMasterKey) return false;
	
	std::vector<uint8_t> plainBytes = CStringToBytes(password);
	std::vector<uint8_t> ciphertext, nonce, tag;
	if (!AesGcmEncrypt(g_MasterKey, plainBytes, ciphertext, nonce, tag))
		return false;

	sqlite3_stmt* stmt = nullptr;
	const char* sql = "UPDATE passwords SET website=?, username=?, ciphertext=?, nonce=?, tag=? WHERE id=?;";
	if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

	auto w = WStrToUtf8((LPCTSTR)website);
	auto u = WStrToUtf8((LPCTSTR)username);
	sqlite3_bind_text(stmt, 1, w.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, u.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 3, ciphertext.data(), (int)ciphertext.size(), SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 4, nonce.data(), (int)nonce.size(), SQLITE_TRANSIENT);
	sqlite3_bind_blob(stmt, 5, tag.data(), (int)tag.size(), SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 6, id);

	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

static bool DeleteEntry(int id)
{
	if (!EnsureDb()) return false;
	sqlite3_stmt* stmt = nullptr;
	const char* sql = "DELETE FROM passwords WHERE id=?;";
	if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
	sqlite3_bind_int(stmt, 1, id);
	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

// PWManagementDlg message handlers
BOOL PWManagementDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	//Anti screenshot
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

	SetWindowTheme(pwlist_ctrl, L"Explorer", NULL);
	pwlist_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	pwlist_ctrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 50);
	pwlist_ctrl.InsertColumn(1, _T("Website"), LVCFMT_LEFT, 250);
	pwlist_ctrl.InsertColumn(2, _T("Username"), LVCFMT_LEFT, 200);
	pwlist_ctrl.InsertColumn(3, _T("Password"), LVCFMT_LEFT, 200);

	RefreshList(this);

	return TRUE;
}

void PWManagementDlg::OnBnClickedCheckShowpw()
{
	if (showpw_ctrl.GetCheck() == BST_CHECKED) {
		password_ctrl.SetPasswordChar(0);
	}
	else {
		password_ctrl.SetPasswordChar(L'•');
	}
	password_ctrl.Invalidate();
	password_ctrl.UpdateWindow();
}

void PWManagementDlg::OnLvnItemchangedPwlist(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (!(pNMLV->uChanged & LVIF_STATE))
		return;

	if (!(pNMLV->uNewState & LVIS_SELECTED))
		return;
	delete_btn_ctrl.EnableWindow(TRUE);
	copy_username_ctrl.EnableWindow(TRUE);
	copy_pw_ctrl.EnableWindow(TRUE);

	id = pwlist_ctrl.GetItemText(pNMLV->iItem, 0);
	int selectedId = _wtoi(id);
	DbEntry e;
	if (GetEntryById(selectedId, e))
		{
		website_ctrl.SetWindowTextW(e.website);
		username_ctrl.SetWindowTextW(e.username);
		password_ctrl.SetWindowTextW(e.password);
	}

	*pResult = 0;
}

void PWManagementDlg::OnBnClickedDeleteBtn()
{
	int selectedId = _wtoi(id);
	if (DeleteEntry(selectedId))
	{
		RefreshList(this);
			username_ctrl.SetWindowTextW(L"");
			password_ctrl.SetWindowTextW(L"");
		website_ctrl.SetWindowTextW(L"");
			username_ctrl.EnableWindow(FALSE);
			password_ctrl.EnableWindow(FALSE);
			updatepw_ctrl.EnableWindow(FALSE);
			delete_btn_ctrl.EnableWindow(FALSE);
			copy_username_ctrl.EnableWindow(FALSE);
			copy_pw_ctrl.EnableWindow(FALSE);
	}
}

void PWManagementDlg::OnBnClickedCheckUpdatepw()
{
	if (updatepw_ctrl.GetCheck() == BST_CHECKED) {
		username_ctrl.EnableWindow(TRUE);
		password_ctrl.EnableWindow(TRUE);
		website_ctrl.EnableWindow(TRUE);
		update_btn_ctrl.EnableWindow(TRUE);
		add_btn_ctrl.EnableWindow(TRUE);
	}
	else {
		username_ctrl.EnableWindow(FALSE);
		password_ctrl.EnableWindow(FALSE);
		website_ctrl.EnableWindow(FALSE);
		update_btn_ctrl.EnableWindow(FALSE);
		add_btn_ctrl.EnableWindow(FALSE);
	}
}

UINT showCopyNotify(LPVOID pParam)
{
	PWManagementDlg* dlg = (PWManagementDlg*)pParam;
	dlg->copy_notify_ctrl.ShowWindow(SW_SHOW);
	Sleep(1000);
	dlg->copy_notify_ctrl.ShowWindow(SW_HIDE);
	Sleep(10000);
	CStringW copiedUsername = L"";
	OpenClipboard(dlg->GetSafeHwnd());
	EmptyClipboard();
	HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (copiedUsername.GetLength() + 1) * sizeof(WCHAR));
	if (hGlob) {
		memcpy(GlobalLock(hGlob), (LPCWSTR)copiedUsername, (copiedUsername.GetLength() + 1) * sizeof(WCHAR));
		GlobalUnlock(hGlob);
		SetClipboardData(CF_UNICODETEXT, hGlob);
		GlobalFree(hGlob);
	}
	CloseClipboard();
	return 0;
}

UINT disableCopyBtn(LPVOID pParam)
{
	PWManagementDlg* dlg = (PWManagementDlg*)pParam;
	dlg->copy_username_ctrl.EnableWindow(FALSE);
	dlg->copy_pw_ctrl.EnableWindow(FALSE);
	Sleep(10000);
	dlg->copy_username_ctrl.EnableWindow(TRUE);
	dlg->copy_pw_ctrl.EnableWindow(TRUE);
	return 0;
}

void PWManagementDlg::OnBnClickedCopyUsername()
{
	CStringW copiedUsername;
	username_ctrl.GetWindowText(copiedUsername);
	OpenClipboard();
	EmptyClipboard();
	HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (copiedUsername.GetLength() + 1) * sizeof(WCHAR));
	if (hGlob) {
		memcpy(GlobalLock(hGlob), (LPCWSTR)copiedUsername, (copiedUsername.GetLength() + 1) * sizeof(WCHAR));
		GlobalUnlock(hGlob);
		SetClipboardData(CF_UNICODETEXT, hGlob);
		GlobalFree(hGlob);
	}
	CloseClipboard();
	AfxBeginThread(showCopyNotify, this);
	AfxBeginThread(disableCopyBtn, this);
}

void PWManagementDlg::OnBnClickedCopyPassword()
{
	CStringW copiedPassword;
	password_ctrl.GetWindowText(copiedPassword);
	OpenClipboard();
	EmptyClipboard();
	HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (copiedPassword.GetLength() + 1) * sizeof(WCHAR));
	if (hGlob) {
		memcpy(GlobalLock(hGlob), (LPCWSTR)copiedPassword, (copiedPassword.GetLength() + 1) * sizeof(WCHAR));
		GlobalUnlock(hGlob);
		SetClipboardData(CF_UNICODETEXT, hGlob);
		GlobalFree(hGlob);
	}
	CloseClipboard();
	AfxBeginThread(showCopyNotify, this);
	AfxBeginThread(disableCopyBtn, this);
}

void PWManagementDlg::OnBnClickedUpdateBtn()
{
	if(id == L"")
	{
		AfxMessageBox(L"No password entry selected.");
		return;
	}
	CStringW newUsername, newPassword, newWebsite;
	username_ctrl.GetWindowText(newUsername);
	password_ctrl.GetWindowText(newPassword);
	website_ctrl.GetWindowText(newWebsite);
	
	if (newWebsite.IsEmpty())
	{
		AfxMessageBox(L"Please enter website!", MB_ICONWARNING);
		website_ctrl.SetFocus();
		return;
	}
	if (newUsername.IsEmpty())
	{
		AfxMessageBox(L"Please enter username!", MB_ICONWARNING);
		username_ctrl.SetFocus();
		return;
	}
	if (newPassword.IsEmpty())
	{
		AfxMessageBox(L"Please enter password!", MB_ICONWARNING);
		password_ctrl.SetFocus();
		return;
	}
	
	int entryId = _wtoi(id);
	if (UpdateEntry(entryId, newWebsite, newUsername, newPassword))
	{
		RefreshList(this);
		AfxMessageBox(L"Password entry updated successfully.");
	}
	else
	{
		AfxMessageBox(L"Update failed.", MB_ICONERROR);
	}
}

void PWManagementDlg::OnEnChangeWebsite()
{
	CString text;
	GetDlgItemText(ID_WEBSITE, text);

	CString filtered;
	for (int i = 0; i < text.GetLength(); ++i)
	{
		TCHAR ch = text[i];
		if (isalnum(ch) || ch == '-' || ch == '.' || ch == '/' || ch == ':')
			filtered += ch;
	}

	if (filtered != text)
	{
		SetDlgItemText(ID_WEBSITE, filtered);
		CEdit* pEdit = (CEdit*)GetDlgItem(ID_WEBSITE);
		pEdit->SetSel(filtered.GetLength(), filtered.GetLength());
	}
}

void PWManagementDlg::OnEnChangeEdit1()
{
	CString text;
	GetDlgItemText(ID_USERNAME, text);

	CString filtered;
	for (int i = 0; i < text.GetLength(); ++i)
	{
		TCHAR ch = text[i];
		if (iswalnum(ch) || ch == '_' || ch == '@' || ch == '#' || ch == '&')
			filtered += ch;
	}

	if (filtered != text)
	{
		SetDlgItemText(ID_USERNAME, filtered);
		CEdit* pEdit = (CEdit*)GetDlgItem(ID_USERNAME);
		pEdit->SetSel(filtered.GetLength(), filtered.GetLength());
	}
}

void PWManagementDlg::OnEnChangePw()
{
	CString text;
	GetDlgItemText(ID_PW, text);

	CString filtered;
	for (int i = 0; i < text.GetLength(); ++i)
	{
		TCHAR ch = text[i];
		if (iswalnum(ch) || ch == '@' || ch == '#')
			filtered += ch;
	}

	if (filtered != text)
	{
		SetDlgItemText(ID_PW, filtered);
		CEdit* pEdit = (CEdit*)GetDlgItem(ID_PW);
		pEdit->SetSel(filtered.GetLength(), filtered.GetLength());
	}
}

void PWManagementDlg::OnBnClickedAddBtn()
{
	CStringW newWebsite, newUsername, newPassword;
	website_ctrl.GetWindowTextW(newWebsite);
	username_ctrl.GetWindowTextW(newUsername);
	password_ctrl.GetWindowTextW(newPassword);
	
	if (newWebsite.IsEmpty())
	{
		AfxMessageBox(L"Please enter website!", MB_ICONWARNING);
		website_ctrl.SetFocus();
		return;
	}
	if (newUsername.IsEmpty())
	{
		AfxMessageBox(L"Please enter username!", MB_ICONWARNING);
		username_ctrl.SetFocus();
		return;
	}
	if (newPassword.IsEmpty())
	{
		AfxMessageBox(L"Please enter password!", MB_ICONWARNING);
		password_ctrl.SetFocus();
		return;
	}
	
	if (InsertEntry(newWebsite, newUsername, newPassword))
	{
		RefreshList(this);
		AfxMessageBox(L"Password entry added successfully.");
		
		website_ctrl.SetWindowTextW(L"");
		username_ctrl.SetWindowTextW(L"");
		password_ctrl.SetWindowTextW(L"");
		updatepw_ctrl.SetCheck(BST_UNCHECKED);
		username_ctrl.EnableWindow(FALSE);
		password_ctrl.EnableWindow(FALSE);
		website_ctrl.EnableWindow(FALSE);
		update_btn_ctrl.EnableWindow(FALSE);
		add_btn_ctrl.EnableWindow(FALSE);
	}
	else
	{
		AfxMessageBox(L"Add failed.", MB_ICONERROR);
	}
}
