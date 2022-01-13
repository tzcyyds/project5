// CDispalyView.cpp: 实现文件
//

#include "pch.h"
#include "Client.h"
#include "CDisplayView.h"
#include "ClientDoc.h"
#include "client_func.h"

// CDispalyView

IMPLEMENT_DYNCREATE(CDisplayView, CFormView)

CDisplayView::CDisplayView()
	: CFormView(IDD_DISPLAYVIEW)
	, m_user(_T("test"))
	, m_password(_T("12345"))
	,client_state(0)
	, m_ip(0x7f000001)
	, m_SPort(9190)
	, m_LPort(9191)
	, Msg_edit(_T(""))
{
	hCommSock = 0;
	memset(&servAdr, 0, sizeof(servAdr));

}

CDisplayView::~CDisplayView()
{
}

void CDisplayView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, m_user);
	DDX_Text(pDX, IDC_EDIT3, m_password);
	DDX_Control(pDX, IDC_LIST1, FileName2);
	DDX_Control(pDX, IDC_LIST2, FileName);
	DDX_IPAddress(pDX, IDC_IPADDRESS1, m_ip);
	DDX_Text(pDX, IDC_EDIT4, m_SPort);
	DDX_Text(pDX, IDC_EDIT5, m_LPort);
	DDX_Control(pDX, IDC_USERS, UserList);
	DDX_Control(pDX, IDC_MSGLIST, Msg_list);
	//  DDX_Control(pDX, IDC_MSGEDIT, Msg_edit);
	DDX_Text(pDX, IDC_MSGEDIT, Msg_edit);
}

BEGIN_MESSAGE_MAP(CDisplayView, CFormView)
	ON_BN_CLICKED(IDC_CONNECT, &CDisplayView::OnBnClickedConnect)
	ON_BN_CLICKED(IDC_DISCONNECT, &CDisplayView::OnBnClickedDisconnect)
	ON_BN_CLICKED(IDC_ENTERDIR, &CDisplayView::OnBnClickedEnterdir)
	ON_BN_CLICKED(IDC_GOBACK, &CDisplayView::OnBnClickedGoback)
	ON_BN_CLICKED(IDC_UPLOAD, &CDisplayView::OnBnClickedUpload)
	ON_BN_CLICKED(IDC_DOWNLOAD, &CDisplayView::OnBnClickedDownload)
	ON_BN_CLICKED(IDC_DELETE, &CDisplayView::OnBnClickedDelete)
	ON_BN_CLICKED(IDC_ENTERDIR2, &CDisplayView::OnBnClickedEnterdir2)
	ON_BN_CLICKED(IDC_GOBACK2, &CDisplayView::OnBnClickedGoback2)
	ON_BN_CLICKED(IDC_UPLOAD2, &CDisplayView::OnBnClickedUpload2)
	ON_BN_CLICKED(IDC_DOWNLOAD2, &CDisplayView::OnBnClickedDownload2)
	ON_BN_CLICKED(IDC_DELETE2, &CDisplayView::OnBnClickedDelete2)
	ON_BN_CLICKED(IDC_SENDMSG, &CDisplayView::OnBnClickedSendmsg)
	ON_BN_CLICKED(IDC_SENDFILE, &CDisplayView::OnBnClickedSendfile)
END_MESSAGE_MAP()


// CDispalyView 诊断

#ifdef _DEBUG
void CDisplayView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CDisplayView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG

// CDispalyView 消息处理程序

LRESULT CDisplayView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: 在此添加专用代码和/或调用基类
	SOCKET hSocket;
	int newEvent;
	CClientDoc* pDoc = (CClientDoc*)GetDocument();
	switch (message)
	{
	case WM_SOCK:
		hSocket = (SOCKET)LOWORD(wParam);
		newEvent = LOWORD(lParam);
		switch (newEvent)
		{
		case FD_READ:
			switch (client_state)
			{
			case 1://等待质询
				pDoc->socket_state1_fsm(hSocket);
				break;
			case 2://等待认证结果
				pDoc->socket_state2_fsm(hSocket);
				break;
			case 3://主状态
				pDoc->socket_state3_fsm(hSocket);
				break;
			case 4://等待上传确认状态
				pDoc->socket_state4_fsm(hSocket);
				break;
			case 5://等待上传数据确认状态
				pDoc->socket_state5_fsm(hSocket);
				break;
			case 6://等待下载确认状态
				pDoc->socket_state6_fsm(hSocket);
				break;
			case 7://等待下载数据状态
				pDoc->socket_state7_fsm(hSocket);
				break;
			default:
				break;
			}
			break;
		case FD_CLOSE:
			FileName.ResetContent();
			FileName2.ResetContent();
			UserList.ResetContent();
			WSAAsyncSelect(hCommSock, m_hWnd, 0, 0);//取消注册
			closesocket(hSocket);
			client_state = 0;
			MessageBox("Connection closed");
			break;
		}
		break;
	}
	return CFormView::WindowProc(message, wParam, lParam);
}



void CDisplayView::OnBnClickedConnect()
{
	UpdateData(TRUE);//刷新IP,端口，用户名和密码
	if (client_state == 0) {
		if (m_Connect(this)) {

		}
		else {
			AfxMessageBox("fail to connect");
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedDisconnect()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		WSAAsyncSelect(hCommSock, m_hWnd, 0, 0);//取消注册
		closesocket(hCommSock);
		client_state = 0;
		FileName.ResetContent();
		FileName2.ResetContent();
		UserList.ResetContent();
	}
	else;
	return;
}


void CDisplayView::OnBnClickedEnterdir()
{
	if (client_state == 3) {
		if (Enterdir(hCommSock, FileName, strdirpath)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedEnterdir2()
{
	if (client_state == 3) {
		if (Enterdir(hCommSock, FileName2, strdirpath2)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedGoback()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		if (Goback(hCommSock, strdirpath)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedGoback2()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		if (Goback(hCommSock, strdirpath2)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedUpload()
{
	if (client_state == 3) {
		if (Upload(this, true)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedUpload2()
{
	if (client_state == 3) {
		if (Upload(this, false)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedDownload()
{
	if (client_state == 3) {
		if (Download(this, true)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedDownload2()
{
	if (client_state == 3) {
		if (Download(this, false)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedDelete()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		if (m_Delete(this, true)) {

		}
		else {
			AfxMessageBox("failed");
		}
	}
	else;
	return;
}

void CDisplayView::OnBnClickedDelete2()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		if (m_Delete(this, false)){

		}
		else {
			AfxMessageBox("failed to delete");
		}
	}
	else;
	return;
}




void CDisplayView::OnBnClickedSendmsg()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);//得到用户名和要发的消息
	CString tar_user;
	UserList.GetText(UserList.GetCurSel(), tar_user);
	char sendbuf[MAX_BUF_SIZE]{};
	int namelength = tar_user.GetLength() + m_user.GetLength();
	int Slength = 5 + namelength + Msg_edit.GetLength();
	sendbuf[0] = 22;
	char* temp = &sendbuf[1];
	// 多字节字符集下保证正确
	*(u_short*)temp = htons(Slength);
	temp = &sendbuf[3];
	*(u_short*)temp = htons(namelength);
	send(hCommSock, sendbuf, Slength, 0);
}


void CDisplayView::OnBnClickedSendfile()
{
	// TODO: 在此添加控件通知处理程序代码
}
