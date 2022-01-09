﻿#pragma once



// CDispalyView 窗体视图

class CDisplayView : public CFormView
{
	DECLARE_DYNCREATE(CDisplayView)

protected:
	CDisplayView();           // 动态创建所使用的受保护的构造函数
	virtual ~CDisplayView();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DISPLAYVIEW };
#endif
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnInitialUpdate();

protected:
	SOCKET hCommSock;
	SOCKADDR_IN clntAdr;
	int clntAdrLen;
public:
	CListBox box_UserOL;
	CListBox m_accounts;
	UINT m_port;
	afx_msg void OnBnClickedListen();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedDecrypt();
	CString m_key;
};


