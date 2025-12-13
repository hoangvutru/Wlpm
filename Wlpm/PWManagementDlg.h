#pragma once
#include "afxdialogex.h"


// PWManagementDlg dialog

class PWManagementDlg : public CDialogEx
{
	DECLARE_DYNAMIC(PWManagementDlg)

public:
	PWManagementDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~PWManagementDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PWMANAGEMENT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CStringW id = L"";
	CListCtrl pwlist_ctrl;
	CButton showpw_ctrl;
	CButton updatepw_ctrl;
	CEdit username_ctrl;
	CEdit password_ctrl;
	CButton delete_btn_ctrl;
	afx_msg void OnBnClickedCheckShowpw();
	afx_msg void OnLvnItemchangedPwlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedDeleteBtn();
	afx_msg void OnBnClickedCheckUpdatepw();
	afx_msg void OnBnClickedCopyUsername();
	afx_msg void OnBnClickedCopyPassword();
	CButton copy_username_ctrl;
	CButton copy_pw_ctrl;
	CEdit website_ctrl;
	afx_msg void OnBnClickedUpdateBtn();
	CButton update_btn_ctrl;
	CStatic copy_notify_ctrl;
	afx_msg void OnEnChangeWebsite();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangePw();
	afx_msg void OnBnClickedAddBtn();
	CButton add_btn_ctrl;
};
