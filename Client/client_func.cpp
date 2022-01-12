#include "pch.h"

#include "CDisplayView.h"
#include "ClientDoc.h"

bool SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c) {
	// 字符串分割函数
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
	return 1;
}

bool UpdateDir(CListBox& f_name, CString recv) {
	// 更新列表显示的文件目录
	f_name.ResetContent();

	std::vector<std::string> v;
	std::string strStr = recv.GetBuffer(0);
	SplitString(strStr, v, "|"); //可按多个字符来分隔;

	for (auto i = 0; i != v.size(); ++i)
		f_name.AddString(v[i].c_str());
	return 1;
}

bool Enterdir(SOCKET hSocket,CListBox& f_name, CString& path) {
	CString selFile;

	f_name.GetText(f_name.GetCurSel(), selFile); //获取用户选择的目录名

	if (selFile.Find('.') == -1) // 判断是否为文件夹，原理：文件名有'.'
	{
		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = nullptr;
		path = selFile + "\\*";// 本地保存当前的文件夹路径，在返回上一级文件夹时会使用到
		int strLen = path.GetLength();
		sendbuf[0] = 5;
		temp = &sendbuf[1];
		*(u_short*)temp = htons(strLen + 3);
		//temp = m_send.GetBuffer();
		//使用strcpy,长度全都需要+1！
		strcpy_s(&sendbuf[3], strLen + 1, path);
		//m_send.ReleaseBuffer();
		//此处可能不需要那么“安全”的发送方式
		send(hSocket, sendbuf, strLen + 3, 0);
	}
	else return 0;

	return 1;
}
bool Goback(SOCKET hSocket, CString& path) {
	if (path.GetLength() != 0) // 判断不是初始化时的目录
	{
		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = nullptr;
		int pos;
		//用字符串截取的方法获得上一级目录
		pos = path.ReverseFind('\\');
		path = path.Left(pos);
		pos = path.ReverseFind('\\');
		path = path.Left(pos);
		path = path + "\\*";
		int strLen = path.GetLength();

		sendbuf[0] = 5;
		temp = &sendbuf[1];
		*(u_short*)temp = htons(strLen + 3);
		//temp = strdirpath.GetBuffer();
		strcpy_s(&sendbuf[3], strLen + 1, path);
		//strdirpath.ReleaseBuffer();
		//此处可能不需要那么“安全”的发送方式
		send(hSocket, sendbuf, strLen + 3, 0);
	}
	else return 0;

	return 1;
}
bool Upload(CDisplayView* pView,bool is_share) {
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
		if (!(pView->uploadFile.Open(fileAbsPath.GetString(),
			CFile::modeRead | CFile::typeBinary, &pView->errFile)))
		{
			char errOpenFile[MAX_BUF_SIZE];
			pView->errFile.GetErrorMessage(errOpenFile, 255);
			TRACE("\nError occurred while opening file:\n"
				"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
				pView->errFile.m_strFileName, errOpenFile, pView->errFile.m_cause, pView->errFile.m_lOsError);
			return 0;
		}

		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = sendbuf;
		u_short namelen = uploadName.GetLength();//此处有可能丢失信息
		ULONGLONG fileLength = pView->uploadFile.GetLength();//64位

		if(is_share) sendbuf[0] = 15;//共享目录上传请求
		else sendbuf[0] = 32;//独享目录上传请求

		temp = &sendbuf[3];
		*(u_short*)temp = htons(namelen);
		strcpy_s(sendbuf + 5, namelen + 1, uploadName);
		temp = &sendbuf[namelen + 5];
		*(u_long*)temp = htonl((u_long)fileLength);//32位，可能会丢失数据
		temp = &sendbuf[1];
		*(u_short*)temp = htons((u_short)(namelen + 9));

		pView->leftToSend = fileLength;
		send(pView->hCommSock, sendbuf, namelen + 9, 0);

		pView->client_state = 4;//变为等待上传确认状态
	}
	return 1;
}
bool Download(CDisplayView* pView, bool is_share) {
	CString downloadName;

	if (is_share) pView->FileName.GetText(pView->FileName.GetCurSel(), downloadName); //独享目录获得想要下载资源名
	else pView->FileName2.GetText(pView->FileName2.GetCurSel(), downloadName); //独享目录获得想要下载资源名
	
	if (!downloadName.IsEmpty())
	{
		//弹出另存为对话框
		CString fileExt = downloadName.Right(downloadName.GetLength() - downloadName.ReverseFind('.'));
		char szFilters[MAX_PATH] = { 0 };//居然有可能太小
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
			if (!(pView->downloadFile.Open(fileAbsPath.GetString(),
				CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &pView->errFile)))
			{
				char errOpenFile[MAX_BUF_SIZE];
				pView->errFile.GetErrorMessage(errOpenFile, 255);
				TRACE("\nError occurred while opening file:\n"
					"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
					pView->errFile.m_strFileName, errOpenFile, pView->errFile.m_cause, pView->errFile.m_lOsError);
				return 0;
			}

			u_short nameLength = downloadName.GetLength();//此处有可能丢失信息
			char sendbuf[MAX_BUF_SIZE] = { 0 };
		
			if (is_share) sendbuf[0] = 11;//共享目录下载请求
			else sendbuf[0] = 31;//独享目录下载请求

			char* temp = sendbuf + 1;
			*(u_short*)temp = htons(nameLength + 5);
			temp = temp + 2;
			*(u_short*)temp = htons(nameLength);
			strcpy_s(sendbuf + 5, nameLength + 1, downloadName);//downloadName不应该包含路径
			send(pView->hCommSock, sendbuf, nameLength + 5, 0);

			pView->client_state = 6;//变为等待下载确认状态
		}
	}
	else return 0;//下载文件名为空
	return 1;
}
bool m_Delete(CDisplayView* pView, bool is_share) {
	CString deleteName;
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = sendbuf;

	if (is_share)pView->FileName.GetText(pView->FileName.GetCurSel(), deleteName); // 获取用户要删除的共享目录文件名
	else pView->FileName2.GetText(pView->FileName2.GetCurSel(), deleteName); // 获取用户要删除的独享目录文件名

	if (!deleteName.IsEmpty())
	{
		if (AfxMessageBox((CString)"确定要删除这个文件？", 4 + 48) == IDYES)
		{
			//deleteName = strdirpath.Left(strdirpath.GetLength() - 1) + deleteName;//拼成正确的文件名
			//不带路径
			int nameLength = deleteName.GetLength();

			if (is_share) sendbuf[0] = 19;//共享目录删除请求
			else sendbuf[0] = 33;//独享目录删除请求

			temp = &sendbuf[1];
			*(u_short*)temp = htons((nameLength + 3));
			temp = &sendbuf[3];
			strcpy_s(temp, nameLength + 1, deleteName);
			send(pView->hCommSock, sendbuf, nameLength + 3, 0);
		}
	}
	else return 0;//删除文件名为空
	return 1;
}
bool m_Connect(CDisplayView* pView) {
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	CClientDoc* pDoc = (CClientDoc*)pView->GetDocument();

	// 判断异常情况
	if (pView->m_ip == NULL)
	{
		TRACE(_T("IP地址为空！"));
		return 0;
	}
	else if (pView->m_SPort == NULL)
	{
		TRACE(_T("云端端口为空！"));
		return 0;
	}
	else if (pView->m_LPort == NULL)
	{
		TRACE(_T("本地端口为空！"));
		return 0;
	}

	pView->servAdr.sin_family = AF_INET;
	pView->servAdr.sin_addr.s_addr = htonl(pView->m_ip);
	pView->servAdr.sin_port = htons(pView->m_SPort);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		TRACE("WSAStartup() failed");
		return 0;
	}
	pView->hCommSock = socket(AF_INET, SOCK_STREAM, 0);
	if (pView->hCommSock == INVALID_SOCKET)
	{
		TRACE("socket() failed");
		return 0;
	}


	if (connect(pView->hCommSock, (SOCKADDR*)&(pView->servAdr), sizeof(pView->servAdr)) == SOCKET_ERROR)//隐式绑定，连接服务器
	{
		TRACE("connect() failed");
		return 0;
	}
	//理论上这里的connect是阻塞的，但是winsock的实现是3秒延时，超时就返回错误。
	if (WSAAsyncSelect(pView->hCommSock, pView->m_hWnd, WM_SOCK, FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		TRACE("WSAAsyncSelect() failed");
		return 0;
	}

	//发送用户名报文
	sendbuf[0] = 1;//填写事件号

	int strLen = pView->m_user.GetLength();
	sendbuf[3] = strLen % 256;//填写字符串长度（用户名字符串长度需要小于256）
	memcpy(sendbuf + 4, pView->m_user, strLen);//填写用户名字符串
	char* temp = &sendbuf[1];
	*(u_short*)temp = htons((4 + strLen));
	send(pView->hCommSock, sendbuf, strLen + 4, 0);
	TRACE("send account");
	pView->client_state = 1;//连接成功,已发送用户名，等待质询
	return 1;
}