// CDispalyView.cpp: 实现文件
//

#include "pch.h"
#include "Client.h"
#include "CDisplayView.h"
#include "ClientDoc.h"


// CDispalyView
#define WM_SOCK WM_USER + 100// 自定义消息，在WM_USER的基础上进行

IMPLEMENT_DYNCREATE(CDisplayView, CFormView)

CDisplayView::CDisplayView()
	: CFormView(IDD_DISPLAYVIEW)
	, m_user(_T("test"))
	, m_password(_T("12345"))
	,client_state(0)
	, m_ip(0x7f000001)
	, m_SPort(9190)
	, m_LPort(9191)
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
		char sendbuf[MAX_BUF_SIZE] = { 0 };
		CClientDoc* pDoc = (CClientDoc*)GetDocument();
		
		// 判断异常情况
		if (m_ip == NULL)
		{
			AfxMessageBox((CString)"IP地址为空！");
			return;
		}
		else if (m_SPort == NULL)
		{
			AfxMessageBox((CString)"云端端口为空！");
			return;
		}
		else if (m_LPort == NULL)
		{
			AfxMessageBox((CString)"本地端口为空！");
			return;
		}

		servAdr.sin_family = AF_INET;
		servAdr.sin_addr.s_addr = htonl(m_ip);
		servAdr.sin_port = htons(m_SPort);

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			MessageBox("WSAStartup() failed", "Client", MB_OK);
			exit(1);
		}
		hCommSock = socket(AF_INET, SOCK_STREAM, 0);
		if (hCommSock == INVALID_SOCKET)
		{
			MessageBox("socket() failed", "Client", MB_OK);
			exit(1);
		}


		if (connect(hCommSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)//隐式绑定，连接服务器
		{
			MessageBox("connect() failed", "Client", MB_OK);
			exit(1);
		}
		//理论上这里的connect是阻塞的，但是winsock的实现是3秒延时，超时就返回错误。
		if (WSAAsyncSelect(hCommSock, m_hWnd, WM_SOCK, FD_READ | FD_CLOSE) == SOCKET_ERROR)
		{
			MessageBox("WSAAsyncSelect() failed", "Client", MB_OK);
			exit(1);
		}

		//发送用户名报文
		sendbuf[0] = 1;//填写事件号
		
		int strLen = m_user.GetLength();
		sendbuf[3] = strLen % 256;//填写字符串长度（用户名字符串长度需要小于256）
		memcpy(sendbuf + 4, m_user, strLen);//填写用户名字符串
		char* temp = &sendbuf[1];
		*(u_short*)temp = htons((4 + strLen));
		send(hCommSock, sendbuf, strLen + 4, 0);
		TRACE("send account");
		client_state = 1;//连接成功,已发送用户名，等待质询
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
	}
	else;
	return;
}


void CDisplayView::OnBnClickedEnterdir()
{
	if (client_state == 3) {
		CString selFile;

		FileName.GetText(FileName.GetCurSel(), selFile); //获取用户选择的目录名

		if (selFile.Find('.') == -1) // 判断是否为文件夹，原理：文件名有'.'
		{
			char sendbuf[MAX_BUF_SIZE] = { 0 };
			char* temp = nullptr;
			strdirpath = selFile + "\\*";// 本地保存当前的文件夹路径，在返回上一级文件夹时会使用到
			int strLen = strdirpath.GetLength();			
			sendbuf[0] = 5;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(strLen + 3);
			//temp = m_send.GetBuffer();
			//使用strcpy,长度全都需要+1！
			strcpy_s(&sendbuf[3], strLen + 1, strdirpath);
			//m_send.ReleaseBuffer();
			//此处可能不需要那么“安全”的发送方式
			send(hCommSock, sendbuf, strLen + 3, 0);
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedGoback()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		if (strdirpath.GetLength() != 0) // 判断不是初始化时的目录
		{
			char sendbuf[MAX_BUF_SIZE] = { 0 };
			char* temp = nullptr;
			int pos;
			//用字符串截取的方法获得上一级目录
			pos = strdirpath.ReverseFind('\\');
			strdirpath = strdirpath.Left(pos);
			pos = strdirpath.ReverseFind('\\');
			strdirpath = strdirpath.Left(pos);
			strdirpath = strdirpath + "\\*";
			int strLen = strdirpath.GetLength();

			sendbuf[0] = 5;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(strLen + 3);
			//temp = strdirpath.GetBuffer();
			strcpy_s(&sendbuf[3], strLen + 1, strdirpath);
			//strdirpath.ReleaseBuffer();
			//此处可能不需要那么“安全”的发送方式
			send(hCommSock, sendbuf, strLen + 3, 0);
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedUpload()
{
	if (client_state == 3) {
		char szFilters[] = "所有文件 (*.*)|*.*||";
		CFileDialog fileDlg(TRUE, NULL, NULL,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);

		char desktop[MAX_PATH] = { 0 };
		SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
		fileDlg.m_ofn.lpstrInitialDir = desktop;//把默认路径设置为桌面

		if (fileDlg.DoModal() == IDOK)//弹出“打开”对话框
		{
			CString fileAbsPath = fileDlg.GetPathName();
			CString uploadName = fileDlg.GetFileName();
			if (!(uploadFile.Open(fileAbsPath.GetString(),
				CFile::modeRead | CFile::typeBinary, &errFile)))
			{
				char errOpenFile[256];
				errFile.GetErrorMessage(errOpenFile, 255);
				TRACE("\nError occurred while opening file:\n"
					"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
					errFile.m_strFileName, errOpenFile, errFile.m_cause, errFile.m_lOsError);
				ASSERT(FALSE);
			}

			char sendbuf[MAX_BUF_SIZE] = { 0 };
			char* temp = sendbuf;
			u_short namelen = uploadName.GetLength();//此处有可能丢失信息
			ULONGLONG fileLength = uploadFile.GetLength();//64位

			sendbuf[0] = 15;
			temp = &sendbuf[3];
			*(u_short*)temp = htons(namelen);
			strcpy_s(sendbuf + 5, namelen + 1, uploadName);
			temp = &sendbuf[namelen + 5];
			*(u_long*)temp = htonl((u_long)fileLength);//32位，可能会丢失数据
			temp = &sendbuf[1];
			*(u_short*)temp = htons((u_short)(namelen + 9));

			leftToSend = fileLength;
			send(hCommSock, sendbuf, namelen + 9, 0);

			client_state = 4;//变为等待上传确认状态
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedDownload()
{
	if (client_state == 3) {
		CString downloadName;
		FileName.GetText(FileName.GetCurSel(), downloadName); //获得想要下载资源名
		if (!downloadName.IsEmpty())
		{
			//弹出另存为对话框
			CString fileExt = downloadName.Right(downloadName.GetLength() - downloadName.ReverseFind('.'));
			char szFilters[32] = { 0 };
			sprintf_s(szFilters, "(*%s)|*%s||", fileExt.GetString(), fileExt.GetString());
			CFileDialog fileDlg(FALSE, NULL, downloadName,
				OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);

			char desktop[MAX_PATH] = { 0 };
			SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
			fileDlg.m_ofn.lpstrInitialDir = desktop;//把默认路径设置为桌面

			if (fileDlg.DoModal() == IDOK)
			{
				CString fileAbsPath = fileDlg.GetPathName();
				if (fileDlg.GetFileExt() == "")
				{
					fileAbsPath += fileExt;
				}
				if (!(downloadFile.Open(fileAbsPath.GetString(),
					CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &errFile)))
				{
					char errOpenFile[256];
					errFile.GetErrorMessage(errOpenFile, 255);
					TRACE("\nError occurred while opening file:\n"
						"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
						errFile.m_strFileName, errOpenFile, errFile.m_cause, errFile.m_lOsError);
					ASSERT(FALSE);
				}

				u_short nameLength = downloadName.GetLength();//此处有可能丢失信息
				char sendbuf[MAX_BUF_SIZE] = { 0 };
				char* temp = sendbuf;
				*(char*)temp = 11;
				temp = temp + 1;
				*(u_short*)temp = htons(nameLength + 5);
				temp = temp + 2;
				*(u_short*)temp = htons(nameLength);
				strcpy_s(sendbuf + 5, nameLength + 1, downloadName);//downloadName不应该包含路径
				send(hCommSock, sendbuf, nameLength + 5, 0);

				client_state = 6;//变为等待下载确认状态
			}
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedDelete()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		CString deleteName;
		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = sendbuf;
		FileName.GetText(FileName.GetCurSel(), deleteName); // 获取用户要删除的文件名
		if (!deleteName.IsEmpty())
		{
			if (AfxMessageBox((CString)"确定要删除这个文件？", 4 + 48) == IDYES)
			{
				//deleteName = strdirpath.Left(strdirpath.GetLength() - 1) + deleteName;//拼成正确的文件名
				//得了，别带路径啦
				int nameLength = deleteName.GetLength();
				sendbuf[0] = 19;
				temp = &sendbuf[1];
				*(u_short*)temp = htons((nameLength + 3));
				temp = &sendbuf[3];
				strcpy_s(temp, nameLength + 1, deleteName);
				send(hCommSock, sendbuf, nameLength + 3, 0);
			}
		}
	}
	else;
	return;
}

void CDisplayView::UpdateDir(CString recv)  // 更新列表显示的文件目录
{
	FileName.ResetContent();

	std::vector<std::string> v;
	std::string strStr = recv.GetBuffer(0);
	SplitString(strStr, v, "|"); //可按多个字符来分隔;

	for (auto i = 0; i != v.size(); ++i)
		FileName.AddString(v[i].c_str());
}


void CDisplayView::SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c) // 字符串分割函数
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

//以下为独享目录相关函数，因为需要区分按钮及其对应的文件名列表框，这里决定复制一遍共享目录相关函数
void CDisplayView::OnBnClickedEnterdir2()
{
	if (client_state == 3) {
		CString selFile;

		FileName2.GetText(FileName2.GetCurSel(), selFile); //获取用户选择的目录名

		if (selFile.Find('.') == -1) // 判断是否为文件夹，原理：文件名有'.'
		{
			char sendbuf[MAX_BUF_SIZE] = { 0 };
			char* temp = nullptr;
			strdirpath2 = selFile + "\\*";// 本地保存当前的文件夹路径，在返回上一级文件夹时会使用到
			int strLen = strdirpath2.GetLength();
			sendbuf[0] = 5;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(strLen + 3);
			//temp = m_send.GetBuffer();
			//使用strcpy,长度全都需要+1！
			strcpy_s(&sendbuf[3], strLen + 1, strdirpath2);
			//m_send.ReleaseBuffer();
			//此处可能不需要那么“安全”的发送方式
			send(hCommSock, sendbuf, strLen + 3, 0);
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedGoback2()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		if (strdirpath2.GetLength() != 0) // 判断不是初始化时的目录
		{
			char sendbuf[MAX_BUF_SIZE] = { 0 };
			char* temp = nullptr;
			int pos;
			//用字符串截取的方法获得上一级目录
			pos = strdirpath2.ReverseFind('\\');
			strdirpath2 = strdirpath2.Left(pos);
			pos = strdirpath2.ReverseFind('\\');
			strdirpath2 = strdirpath2.Left(pos);
			strdirpath2 = strdirpath2 + "\\*";
			int strLen = strdirpath2.GetLength();

			sendbuf[0] = 5;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(strLen + 3);
			//temp = strdirpath.GetBuffer();
			strcpy_s(&sendbuf[3], strLen + 1, strdirpath2);
			//strdirpath.ReleaseBuffer();
			//此处可能不需要那么“安全”的发送方式
			send(hCommSock, sendbuf, strLen + 3, 0);
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedUpload2()
{
	if (client_state == 3) {
		char szFilters[] = "所有文件 (*.*)|*.*||";
		CFileDialog fileDlg(TRUE, NULL, NULL,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);

		char desktop[MAX_PATH] = { 0 };
		SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
		fileDlg.m_ofn.lpstrInitialDir = desktop;//把默认路径设置为桌面

		if (fileDlg.DoModal() == IDOK)//弹出“打开”对话框
		{
			CString fileAbsPath = fileDlg.GetPathName();
			CString uploadName = fileDlg.GetFileName();
			if (!(uploadFile.Open(fileAbsPath.GetString(),
				CFile::modeRead | CFile::typeBinary, &errFile)))
			{
				char errOpenFile[256];
				errFile.GetErrorMessage(errOpenFile, 255);
				TRACE("\nError occurred while opening file:\n"
					"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
					errFile.m_strFileName, errOpenFile, errFile.m_cause, errFile.m_lOsError);
				ASSERT(FALSE);
			}

			char sendbuf[MAX_BUF_SIZE] = { 0 };
			char* temp = sendbuf;
			u_short namelen = uploadName.GetLength();//此处有可能丢失信息
			ULONGLONG fileLength = uploadFile.GetLength();//64位

			sendbuf[0] = 15;
			temp = &sendbuf[3];
			*(u_short*)temp = htons(namelen);
			strcpy_s(sendbuf + 5, namelen + 1, uploadName);
			temp = &sendbuf[namelen + 5];
			*(u_long*)temp = htonl((u_long)fileLength);//32位，可能会丢失数据
			temp = &sendbuf[1];
			*(u_short*)temp = htons((u_short)(namelen + 9));

			leftToSend = fileLength;
			send(hCommSock, sendbuf, namelen + 9, 0);

			client_state = 4;//变为等待上传确认状态
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedDownload2()
{
	if (client_state == 3) {
		CString downloadName;
		FileName2.GetText(FileName2.GetCurSel(), downloadName); //获得想要下载资源名
		if (!downloadName.IsEmpty())
		{
			//弹出另存为对话框
			CString fileExt = downloadName.Right(downloadName.GetLength() - downloadName.ReverseFind('.'));
			char szFilters[32] = { 0 };
			sprintf_s(szFilters, "(*%s)|*%s||", fileExt.GetString(), fileExt.GetString());
			CFileDialog fileDlg(FALSE, NULL, downloadName,
				OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);

			char desktop[MAX_PATH] = { 0 };
			SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
			fileDlg.m_ofn.lpstrInitialDir = desktop;//把默认路径设置为桌面

			if (fileDlg.DoModal() == IDOK)
			{
				CString fileAbsPath = fileDlg.GetPathName();
				if (fileDlg.GetFileExt() == "")
				{
					fileAbsPath += fileExt;
				}
				if (!(downloadFile.Open(fileAbsPath.GetString(),
					CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &errFile)))
				{
					char errOpenFile[256];
					errFile.GetErrorMessage(errOpenFile, 255);
					TRACE("\nError occurred while opening file:\n"
						"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
						errFile.m_strFileName, errOpenFile, errFile.m_cause, errFile.m_lOsError);
					ASSERT(FALSE);
				}

				u_short nameLength = downloadName.GetLength();//此处有可能丢失信息
				char sendbuf[MAX_BUF_SIZE] = { 0 };
				char* temp = sendbuf;
				*(char*)temp = 11;
				temp = temp + 1;
				*(u_short*)temp = htons(nameLength + 5);
				temp = temp + 2;
				*(u_short*)temp = htons(nameLength);
				strcpy_s(sendbuf + 5, nameLength + 1, downloadName);//downloadName不应该包含路径
				send(hCommSock, sendbuf, nameLength + 5, 0);

				client_state = 6;//变为等待下载确认状态
			}
		}
	}
	else;
	return;
}


void CDisplayView::OnBnClickedDelete2()
{
	// TODO: 在此添加控件通知处理程序代码
	if (client_state == 3) {
		CString deleteName;
		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = sendbuf;
		FileName2.GetText(FileName2.GetCurSel(), deleteName); // 获取用户要删除的文件名
		if (!deleteName.IsEmpty())
		{
			if (AfxMessageBox((CString)"确定要删除这个文件？", 4 + 48) == IDYES)
			{
				//deleteName = strdirpath.Left(strdirpath.GetLength() - 1) + deleteName;//拼成正确的文件名
				//得了，别带路径啦
				int nameLength = deleteName.GetLength();
				sendbuf[0] = 19;
				temp = &sendbuf[1];
				*(u_short*)temp = htons((nameLength + 3));
				temp = &sendbuf[3];
				strcpy_s(temp, nameLength + 1, deleteName);
				send(hCommSock, sendbuf, nameLength + 3, 0);
			}
		}
	}
	else;
	return;
}

void CDisplayView::UpdateDir2(CString recv)  // 更新列表显示的文件目录
{
	FileName2.ResetContent();

	std::vector<std::string> v;
	std::string strStr = recv.GetBuffer(0);
	SplitString(strStr, v, "|"); //可按多个字符来分隔;

	for (auto i = 0; i != v.size(); ++i)
		FileName2.AddString(v[i].c_str());
}
