#pragma once
#include "afxdialogex.h"
#include "resource.h"	
#include "afxwin.h"
#include <string>

class LoginDialog : public CDialogEx
{
	DECLARE_DYNAMIC(LoginDialog)

public:
	LoginDialog(CWnd* pParent = nullptr);
	virtual ~LoginDialog();

	virtual void OnFinalRelease();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LOGINDIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

private:
	std::wstring m_securePassword;
	BOOL m_bRawInputRegistered = FALSE;

	void RegisterRawInput();
	void UnregisterRawInput();

	wchar_t ScanCodeToChar(UINT scanCode, BOOL isExtended, BOOL isShift, BOOL isCapsLock);

	void ProcessRawKeyboardInput(const RAWKEYBOARD& keyboard);

public:
	afx_msg void OnStnClickedStaticForgot();
	afx_msg void OnBnClickedCheckShowpass();
	afx_msg void OnBnClickedButtonAccess();
	afx_msg void OnBnClickedButton2();

	afx_msg LRESULT OnInput(WPARAM wParam, LPARAM lParam);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
