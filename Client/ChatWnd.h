#pragma once
#include "afxdialogex.h"


// ChatWnd dialog

class ChatWnd : public CDialogEx
{
	DECLARE_DYNAMIC(ChatWnd)

public:
	ChatWnd(CWnd* pParent = nullptr);   // standard constructor
	virtual ~ChatWnd();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATWND };
#endif

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL ChatWnd::OnInitDialog();

	DECLARE_MESSAGE_MAP()
};
