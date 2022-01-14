// ChatWnd.cpp : implementation file
//

#include "pch.h"
#include "Client.h"
#include "afxdialogex.h"
#include "ChatWnd.h"


// ChatWnd dialog

IMPLEMENT_DYNAMIC(ChatWnd, CDialogEx)

ChatWnd::ChatWnd(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATWND, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

ChatWnd::~ChatWnd()
{
}

void ChatWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ChatWnd, CDialogEx)
END_MESSAGE_MAP()


// ChatWnd message handlers

BOOL ChatWnd::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	return TRUE;
}