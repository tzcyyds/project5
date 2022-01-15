// CDispalyView.cpp: 实现文件
//

#include "pch.h"
#include "Flieserver.h"
#include "CDisplayView.h"
#include "FlieserverDoc.h"
#include "sever_func.h"

// CDispalyView

IMPLEMENT_DYNCREATE(CDisplayView, CFormView)

CDisplayView::CDisplayView()
	: CFormView(IDD_DISPLAYVIEW)
	, m_port(9190)
	, m_key(_T(""))
{
	hCommSock = 0;
	memset(&clntAdr, 0, sizeof(clntAdr));
	clntAdrLen = sizeof(clntAdr);
}

CDisplayView::~CDisplayView()
{
}

void CDisplayView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_USEROL, box_UserOL);
	DDX_Text(pDX, IDC_PORT, m_port);
	DDX_Control(pDX, IDC_ACCOUNTS, m_accounts);
	DDX_Text(pDX, IDC_KEY, m_key);
}

BEGIN_MESSAGE_MAP(CDisplayView, CFormView)
	ON_BN_CLICKED(IDC_LISTEN, &CDisplayView::OnBnClickedListen)
	ON_BN_CLICKED(IDC_STOP, &CDisplayView::OnBnClickedStop)
	ON_BN_CLICKED(IDC_DECRYPT, &CDisplayView::OnBnClickedDecrypt)
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


void CDisplayView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();

	// TODO: 在此添加专用代码和/或调用基类
}

LRESULT CDisplayView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: 在此添加专用代码和/或调用基类
	SOCKET hSocket;
	int newEvent;
	CFlieserverDoc* pDoc = (CFlieserverDoc*)GetDocument();
	switch (message)
	{
	case WM_SOCK:
		hSocket = (SOCKET)LOWORD(wParam);
		newEvent = LOWORD(lParam);
		switch (newEvent)
		{
		case FD_ACCEPT:
		{
			hCommSock = accept(hSocket, (sockaddr*)&clntAdr, &clntAdrLen);
			if (hCommSock == SOCKET_ERROR)
			{
				closesocket(hSocket);
				break;
			}

			//hCommSock进入连接建立状态,等待用户名
			User* p_user=new User;
			p_user->ip = clntAdr.sin_addr;
			p_user->port = clntAdr.sin_port;
			p_user->username = "WAIT";
			p_user->state = 1;//
			//sprintf_s(p_user->strdirpath,"..\\%s_exclusive_path\\",);
			//p_user->strdirpath = "..\\_exclusive_path\\";//独享路径
			p_user->comparison = 0xff;
			pDoc->m_linkInfo.SUMap.insert(std::pair<SOCKET, User*>(hCommSock, p_user));
			TRACE("wait account");
		}
			break;
		case FD_READ:
		{
			//设定状态
			int m_state = pDoc->m_linkInfo.SUMap[hSocket]->state;
			//switch 根据状态
			switch (m_state)
			{
			case 0://不该有的状态。
				break;
			case 1://等待用户名，并发送质询
				pDoc->state1_fsm(hSocket);
				//连接建立状态
				break;
			case 2://等待质询结果
				pDoc->state2_fsm(hSocket);
				//等待质询结果状态
				break;
			case 3:
				//用户已在线，等待其它指令。
				pDoc->state3_fsm(hSocket);//调用主状态处理函数
				break;
			case 4:
				//在接收上传文件数据状态，接收数据
				pDoc->state4_fsm(hSocket);
				break;
			case 5:
				//等待下载数据确认状态
				pDoc->state5_fsm(hSocket);
				break;
			case 7:
				//等待发送端发送数据状态
				pDoc->state7_fsm(hSocket);
				break;
			case 9:
				//等待接收端客户回应请求
				pDoc->state9_fsm(hSocket);
				break;
			case 11:
				//等待接收端客户回应数据
				pDoc->state11_fsm(hSocket);
				break;
			default:
				break;
			}
		}
			break;
		case FD_CLOSE:
		{	
			int m_state = pDoc->m_linkInfo.SUMap[hSocket]->state;//只要连接上了，有socket，就有user结构体
			switch (m_state)
			{
			case 0://不该有的状态。
				MessageBox("state = 0 ？？？");
				break;
			case 1://等待用户名
				//break;//合并
			case 2://等待质询结果
			{
				delete pDoc->m_linkInfo.SUMap.at(hSocket);//一定成功
				pDoc->m_linkInfo.SUMap.erase(hSocket);
			}
				break;
			case 3:
			{
				//pDoc->UserOL_list.remove(pDoc->m_linkInfo.SUMap[hSocket]->username);
				pDoc->m_linkInfo.USMap.erase(
					pDoc->m_linkInfo.SUMap[hSocket]->username);//从用户列表中删除
				box_UserOL.ResetContent();//更新box
				for (const auto& it : pDoc->m_linkInfo.USMap) box_UserOL.AddString(it.first.c_str());
				delete pDoc->m_linkInfo.SUMap.at(hSocket);//一定成功
				pDoc->m_linkInfo.SUMap.erase(hSocket);
				delete pDoc->m_linkInfo.SFMap.at(hSocket);//一定成功
				pDoc->m_linkInfo.SFMap.erase(hSocket);
				send_userlist(pDoc);
			}
				break;
			case 4://接收上传文件数据状态
			{
				MessageBox("close while upload?");
			}
				break;
			case 5://等待下载数据确认状态
			{
				MessageBox("close while download?");
			}
				break;
			default:
				break;
			}
			WSAAsyncSelect(hSocket, m_hWnd, 0, 0);//取消注册
			closesocket(hSocket);//释放socket资源
		}
			break;//中断的是case FD_CLOSE
		}
		break;//中断的是case WM_SOCK:
	}
	return CFormView::WindowProc(message, wParam, lParam);
}

void CDisplayView::OnBnClickedListen()
{
	// TODO: 在此添加控件通知处理程序代码
	WSADATA wsaData;
	SOCKADDR_IN servAdr;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		MessageBox("WSAStartup() failed", "Server", MB_OK);
		exit(1);
	}
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(m_port);
	hListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (hListenSock == INVALID_SOCKET)
	{
		MessageBox("socket() failed", "Server", MB_OK);
		exit(1);
	}
	if (bind(hListenSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		MessageBox("bind() failed", "Server", MB_OK);
		exit(1);
	}
	if (listen(hListenSock, 5) == SOCKET_ERROR)
	{
		MessageBox("listen() failed", "Server", MB_OK);
		exit(1);
	}
	if (WSAAsyncSelect(hListenSock, m_hWnd, WM_SOCK, FD_ACCEPT | FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		MessageBox("WSAAsyncSelect() failed", "Server", MB_OK);
		exit(1);
	}
	TRACE("init finish");//微软的文档里说：如果对监听套接字用了WSAAsyncSelect，则其创建的子套接字也会被自动设置一遍。
}


void CDisplayView::OnBnClickedStop()
{
	CFlieserverDoc* pDoc = (CFlieserverDoc*)GetDocument();
	bool flag = false;
	//保证大家都在主状态，不能还有什么需要等待的事件。
	for (const auto& it: pDoc->m_linkInfo.SUMap)
	{
		if (it.second->state != 3) flag = true;
	}
	if(flag) MessageBox("cannot stop listen");
	else
	{
		//结束监听
		closesocket(hListenSock);
		pDoc->m_linkInfo.myclear();
		box_UserOL.ResetContent();
		MessageBox("already stop");
	}
}


void CDisplayView::OnBnClickedDecrypt()
{
	// TODO: 在此添加控件通知处理程序代码
	CFlieserverDoc* pDoc = (CFlieserverDoc*)GetDocument();
	using namespace std;
	ifstream in(".\\accounts.txt");
	string accstr;
	in >> accstr;
	in.close();
	vector<string> m_tokens;
	split(accstr, m_tokens, ';');

	m_accounts.ResetContent();
	for (const auto& it : m_tokens) {
		string::size_type subpos = 0;
		string username, password;
		if ((subpos = it.find('-'))
			!= string::npos)
		{
			username = it.substr(0, subpos);
			password = it.substr(subpos + 1);
			pDoc->m_UserInfo.UserDocMap.insert(std::pair<std::string, std::string>(username, password));
			m_accounts.AddString((username + ':' + password).c_str());
		}
		else TRACE("file error");
	}

}
