// PWManagementDlg.cpp : implementation file
//

#include "pch.h"
#include "Wlpm.h"
#include "afxdialogex.h"
#include "PWManagementDlg.h"
#include "resource.h"
#include <vector>


using namespace std;


// PWManagementDlg dialog

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

struct PasswordEntry {
	WCHAR id[4];
	WCHAR website[256];
	WCHAR username[256];
	WCHAR password[256];
};
	
PasswordEntry sampleEntry1 = { L"1", L"example1.com", L"username123", L"password123" };
PasswordEntry sampleEntry2 = { L"2", L"example2.com", L"username1234", L"password1234" };
vector<PasswordEntry> mockPasswordList = { sampleEntry1, sampleEntry2 };

void loadPasswordList(PWManagementDlg* dlg)
{
	//This function will load the password list from the database
	//For demonstration, we are using mock data defined above
	dlg->pwlist_ctrl.DeleteAllItems();
	for (const PasswordEntry& entry : mockPasswordList)
	{
		int index = dlg->pwlist_ctrl.GetItemCount();
		dlg->pwlist_ctrl.InsertItem(index, entry.id);
		dlg->pwlist_ctrl.SetItemText(index, 1, entry.website);
		dlg->pwlist_ctrl.SetItemText(index, 2, entry.username);
		dlg->pwlist_ctrl.SetItemText(index, 3, entry.password);
	}
}

// PWManagementDlg message handlers
BOOL PWManagementDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
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

	SetWindowTheme(pwlist_ctrl, L"Explorer", NULL);
	pwlist_ctrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	pwlist_ctrl.InsertColumn(0, _T("ID"), LVCFMT_LEFT, 50);
	pwlist_ctrl.InsertColumn(1, _T("Website"), LVCFMT_LEFT, 550);


	// TODO: Add extra initialization here
	loadPasswordList(this);
	
	//Call LoadPasswordList function here to populate the list control


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void PWManagementDlg::OnBnClickedCheckShowpw()
{
	// TODO: Add your control notification handler code here
	if (showpw_ctrl.GetCheck() == BST_CHECKED) {
		password_ctrl.SetPasswordChar(0); // Show password
	}
	else {
		password_ctrl.SetPasswordChar(L'•'); // Hide password
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
	// TODO: Add your control notification handler code here
	delete_btn_ctrl.EnableWindow(TRUE);
	copy_username_ctrl.EnableWindow(TRUE);
	copy_pw_ctrl.EnableWindow(TRUE);

	id = pwlist_ctrl.GetItemText(pNMLV->iItem, 0);



	//Call function to get username and password from database using the id
	//Using mock data for demonstration

	for (const PasswordEntry& entry : mockPasswordList)
	{
		if (wcscmp(entry.id, id.GetString()) == 0)
		{
			website_ctrl.SetWindowTextW(entry.website);
			username_ctrl.SetWindowTextW(entry.username);
			password_ctrl.SetWindowTextW(entry.password);
			break;
		}
	}

	*pResult = 0;
}

void PWManagementDlg::OnBnClickedDeleteBtn()
{
	// TODO: Add your control notification handler code here
	for(const PasswordEntry& entry : mockPasswordList)
	{
		if (wcscmp(entry.id, id.GetString()) == 0)
		{
			//Remove entry from mock data
			mockPasswordList.erase(remove_if(mockPasswordList.begin(), mockPasswordList.end(),
				[&](const PasswordEntry& e) { return wcscmp(e.id, id.GetString()) == 0; }),
				mockPasswordList.end());

			loadPasswordList(this);
			username_ctrl.SetWindowTextW(L"");
			password_ctrl.SetWindowTextW(L"");
			username_ctrl.EnableWindow(FALSE);
			password_ctrl.EnableWindow(FALSE);
			updatepw_ctrl.EnableWindow(FALSE);
			delete_btn_ctrl.EnableWindow(FALSE);
			copy_username_ctrl.EnableWindow(FALSE);
			copy_pw_ctrl.EnableWindow(FALSE);
			break;
		}
	}
}

void PWManagementDlg::OnBnClickedCheckUpdatepw()
{
	// TODO: Add your control notification handler code here
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
	return 0;
}

void PWManagementDlg::OnBnClickedCopyUsername()
{
	// TODO: Add your control notification handler code here
	CStringW copiedUsername;
	username_ctrl.GetWindowText(copiedUsername);
	OpenClipboard();
	EmptyClipboard();
	HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (copiedUsername.GetLength() + 1) * sizeof(WCHAR));
	if (hGlob) {
		memcpy(GlobalLock(hGlob), (LPCWSTR)copiedUsername, (copiedUsername.GetLength() + 1) * sizeof(WCHAR));
		GlobalUnlock(hGlob);
		SetClipboardData(CF_UNICODETEXT, hGlob);
	}
	CloseClipboard();
	AfxBeginThread(showCopyNotify, this);
}

void PWManagementDlg::OnBnClickedCopyPassword()
{
	// TODO: Add your control notification handler code here
	CStringW copiedPassword;
	password_ctrl.GetWindowText(copiedPassword);
	OpenClipboard();
	EmptyClipboard();
	HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (copiedPassword.GetLength() + 1) * sizeof(WCHAR));
	if (hGlob) {
		memcpy(GlobalLock(hGlob), (LPCWSTR)copiedPassword, (copiedPassword.GetLength() + 1) * sizeof(WCHAR));
		GlobalUnlock(hGlob);
		SetClipboardData(CF_UNICODETEXT, hGlob);
	}
	CloseClipboard();
	AfxBeginThread(showCopyNotify, this);
}

void PWManagementDlg::OnBnClickedUpdateBtn()
{
	// TODO: Add your control notification handler code here
	if(id == L"")
	{
		AfxMessageBox(L"No password entry selected.");
		return;
	}
	CStringW newUsername, newPassword, newWebsite;
	username_ctrl.GetWindowText(newUsername);
	password_ctrl.GetWindowText(newPassword);
	website_ctrl.GetWindowText(newWebsite);
	for (PasswordEntry& entry : mockPasswordList)
	{
		if (wcscmp(entry.id, id.GetString()) == 0)
		{
			wcscpy_s(entry.username, newUsername);
			wcscpy_s(entry.password, newPassword);
			wcscpy_s(entry.website, newWebsite);
			loadPasswordList(this);
			break;
		}
	}
	AfxMessageBox(L"Password entry updated successfully.");
}

void PWManagementDlg::OnEnChangeWebsite()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
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
		// Move caret to end
		CEdit* pEdit = (CEdit*)GetDlgItem(ID_WEBSITE);
		pEdit->SetSel(filtered.GetLength(), filtered.GetLength());
	}
}

void PWManagementDlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
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
		// Move caret to end
		CEdit* pEdit = (CEdit*)GetDlgItem(ID_USERNAME);
		pEdit->SetSel(filtered.GetLength(), filtered.GetLength());
	}
}

void PWManagementDlg::OnEnChangePw()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
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
		// Move caret to end
		CEdit* pEdit = (CEdit*)GetDlgItem(ID_PW);
		pEdit->SetSel(filtered.GetLength(), filtered.GetLength());
	}
}

void PWManagementDlg::OnBnClickedAddBtn()
{
	// TODO: Add your control notification handler code here
	CStringW newWebsite, newUsername, newPassword;
	website_ctrl.GetWindowTextW(newWebsite);
	username_ctrl.GetWindowTextW(newUsername);
	password_ctrl.GetWindowTextW(newPassword);
	PasswordEntry newEntry;
	wcscpy_s(newEntry.id, L"3"); // ID for testing
	wcscpy_s(newEntry.website, newWebsite.GetString());
	wcscpy_s(newEntry.username, newUsername.GetString());
	wcscpy_s(newEntry.password, newPassword.GetString());

	//Should call function to add new entry to database here
	mockPasswordList.push_back(newEntry);
	loadPasswordList(this);
	AfxMessageBox(L"Password entry added successfully.");
}
