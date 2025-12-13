#pragma once
#include "afxdialogex.h"
#include "resource.h"	
#include "afxwin.h"

// LoginDialog dialog

class LoginDialog : public CDialogEx
{
	DECLARE_DYNAMIC(LoginDialog)

public:
	LoginDialog(CWnd* pParent = nullptr);   // standard constructor
	virtual ~LoginDialog();

	virtual void OnFinalRelease();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LOGINDIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
public:
	afx_msg void OnStnClickedStaticForgot();
	afx_msg void OnBnClickedCheckShowpass();
	afx_msg void OnBnClickedButtonAccess();
	afx_msg void OnBnClickedButton2();
};
