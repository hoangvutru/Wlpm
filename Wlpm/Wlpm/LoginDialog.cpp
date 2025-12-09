// LoginDialog.cpp : implementation file
//

#include "pch.h"
#include "Wlpm.h"
#include "afxdialogex.h"
#include "LoginDialog.h"
#include "PWManagementDlg.h"


// LoginDialog dialog

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
}

void LoginDialog::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

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
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(LoginDialog, CDialogEx)
END_DISPATCH_MAP()

// Note: we add support for IID_ILoginDialog to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {9975e788-4c97-4081-96df-fff02bdef886}
static const IID IID_ILoginDialog =
{0x9975e788,0x4c97,0x4081,{0x96,0xdf,0xff,0xf0,0x2b,0xde,0xf8,0x86}};

BEGIN_INTERFACE_MAP(LoginDialog, CDialogEx)
	INTERFACE_PART(LoginDialog, IID_ILoginDialog, Dispatch)
END_INTERFACE_MAP()


// LoginDialog message handlers
 BOOL LoginDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	return TRUE;
}
void LoginDialog::OnStnClickedStaticForgot()
{
	AfxMessageBox(L"Forgot master password means all encrypted passwords cannot be restored!\n\nThere is no recovery mechanism.", MB_ICONWARNING);
}

void LoginDialog::OnBnClickedCheckShowpass()
{
	BOOL bShow = IsDlgButtonChecked(IDC_CHECK_SHOWPASS);
	CWnd* pEdit = GetDlgItem(IDC_EDIT_PASSWORD);
	if (bShow) {
		pEdit->SendMessage(EM_SETPASSWORDCHAR, 0);
	}
	else {
		pEdit->SendMessage(EM_SETPASSWORDCHAR, L'•');
	}

	pEdit->Invalidate();
	pEdit->UpdateWindow();
}

void LoginDialog::OnBnClickedButtonAccess()
{
	CString input;
	GetDlgItemText(IDC_EDIT_PASSWORD, input);

	const CString CORRECT_MASTER = L"Password1234!"; //testing with a hardcoded password

	if (input == CORRECT_MASTER) {
		AfxMessageBox(L"Unlock successful!", MB_ICONINFORMATION);

		PWManagementDlg PWManagementDlg;
		EndDialog(IDOK);
		PWManagementDlg.DoModal();

		GetDlgItem(IDC_EDIT_PASSWORD)->SetWindowText(L"");
		CheckDlgButton(IDC_CHECK_SHOWPASS, BST_UNCHECKED);
	}
	else {
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
