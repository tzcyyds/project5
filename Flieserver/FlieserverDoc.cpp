// FlieserverDoc.cpp: CFlieserverDoc 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "Flieserver.h"
#endif

#include "FlieserverDoc.h"
#include "sever_func.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CFlieserverDoc

IMPLEMENT_DYNCREATE(CFlieserverDoc, CDocument)

BEGIN_MESSAGE_MAP(CFlieserverDoc, CDocument)
END_MESSAGE_MAP()


// CFlieserverDoc 构造/析构

CFlieserverDoc::CFlieserverDoc() noexcept
{
	// TODO: 在此添加一次性构造代码

	SetTitle(TEXT("fileserver"));
	pView = nullptr;
	shared_path = "..\\sever_shared_path\\";
}

CFlieserverDoc::~CFlieserverDoc()
{
}

BOOL CFlieserverDoc::send_dir(SOCKET hSocket, bool is_share) {
	//发送目录函数
	char buf[MAX_BUF_SIZE] = { 0 };
	CString m_list;
	if (is_share) {
		//共享目录
		buf[0] = 6;
		m_list = PathtoList(m_linkInfo.SUMap[hSocket]->current_path + '*');
	}
	else {
		//独享目录
		buf[0] = 30;
		m_list = PathtoList(m_linkInfo.SUMap[hSocket]->current_path2 + '*');
	}
	char* temp = &buf[1];

	int strLen = m_list.GetLength();
	assert(strLen > 0);//若为空目录，则要特殊处理
	*(u_short*)temp = htons(strLen + 3);//packet_len=strLen + 3
	strcpy_s(buf + 3, strLen + 1, m_list);
	send(hSocket, buf, strLen + 3, 0);
	return 1;
}
CString CFlieserverDoc::PathtoList(CString path) // 获取指定目录下的文件列表，文件之间用|隔开
{
	CString file_list; // 文件列表
	CFileFind file_find; // 创建一个CFileFind实例

	BOOL bfind = file_find.FindFile(path);
	while (bfind)
	{
		bfind = file_find.FindNextFile(); // FindNextFile()必须放在循环中的最前面
		CString strpath;
		if (file_find.IsDots())
			continue;
		if (!file_find.IsDirectory())          //判断是目录还是文件
		{
			strpath = file_find.GetFileName(); //文件则读取文件名
			file_list += strpath;
			file_list += '|';
		}
		else
		{
			strpath = file_find.GetFilePath(); //获取到的是绝对路径

			file_list += strpath;
			file_list += '|';
		}

	}
	return file_list;
}

BOOL CFlieserverDoc::UploadOnce(SOCKET hSocket, const char* buf, u_int length)
{
	//此时pView应该是对的，不用再刷新了
	//POSITION pos = GetFirstViewPosition();
	//pView = (CDisplayView*)GetNextView(pos);

	int leftToSend = length;
	int bytesSend = 0;
	int WSAECount = 0;

	do// 单次发送
	{
		const char* sendBuf = buf + bytesSend;
		bytesSend = send(hSocket, sendBuf, leftToSend, 0);
		if (bytesSend == SOCKET_ERROR)
		{
			ASSERT(WSAGetLastError() == WSAEWOULDBLOCK);
			bytesSend = 0;
			WSAECount++;
			if (WSAECount > MAX_WSAE_TIMES) return FALSE;
		}
		leftToSend -= bytesSend;
	} while (leftToSend > 0);

	return TRUE;
}

BOOL CFlieserverDoc::RecvOnce(SOCKET hSocket, char* buf, u_int length)
{
	//此时pView应该是对的，不用再刷新了
	//POSITION pos = GetFirstViewPosition();
	//pView = (CDisplayView*)GetNextView(pos);
	int leftToRecv = length;
	int bytesRecv = 0;
	int WSAECount = 0;

	do// 单次接收
	{
		char* recvBuf = buf + bytesRecv;
		bytesRecv = recv(hSocket, recvBuf, leftToRecv, 0);
		if (bytesRecv == SOCKET_ERROR)
		{
			ASSERT(WSAGetLastError() == WSAEWOULDBLOCK);
			bytesRecv = 0;
			WSAECount++;
			if (WSAECount > MAX_WSAE_TIMES) return FALSE;
		}
		leftToRecv -= bytesRecv;
	} while (leftToRecv > 0);

	return TRUE;
}

void CFlieserverDoc::state1_fsm(SOCKET hSocket)
{
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(hSocket, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(hSocket, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;
	//函数：拿到用户名，查用户名对应的密码，随机出随机数，和密钥异或，存起来，然后发送随机数。最后，更改状态
	switch (event)
	{
	case 0://非法数据
		break;
	case 1://用户名到来
	{
		using namespace std;
		int namelen = recvbuf[3];
		assert(namelen >= 0 && namelen <= MAX_BUF_SIZE);
		string username(&recvbuf[4], namelen);
		if (m_UserInfo.UserDocMap.count(username))
		{
			m_linkInfo.SUMap[hSocket]->username = username;
			string password = m_UserInfo.UserDocMap[username];
			//处理密码，转换成可以计算的
			int t_p = 0;
			try
			{
				t_p = std::stoi(password);
			}
			catch (const std::invalid_argument&)
			{
				for (const auto c : password)
				{
					t_p += c;
				}
			}

			//准备要发送的质询数据，N，N个随机数
			u_int seed;//保证随机数足够随机
			seed = (u_int)time(0);
			srand(seed);
			constexpr auto MIN_VALUE = 1;
			constexpr auto MAX_VALUE = 20;
			unsigned char num_N = (unsigned char)rand() % (MAX_VALUE - MIN_VALUE + 1) + MIN_VALUE;

			sendbuf[0] = 2;//填事件号
			temp = &sendbuf[1];
			*(u_short*)temp = htons(4 + num_N * 2);//send_packet_len=4+num_N*2
			sendbuf[3] = num_N;//整数个数
			temp = sendbuf + 4;//指针就位

			u_short correct_sum = 0;//本地计算值
			u_short correct_result = 0;
			u_short correct_password = (u_short)t_p;
			u_short x = 0;//两字节u_short;
			for (size_t i = 0; i < num_N; i++)
			{
				x = rand();//最大65535
				correct_sum += x;
				x = htons(x);
				memcpy(temp, &x, 2);
				temp += 2;
			}
			send(hSocket, sendbuf, 4 + num_N * 2, 0);//发送质询报文

			correct_result = correct_sum ^ correct_password;//异或
			m_linkInfo.SUMap[hSocket]->comparison = correct_result;
			m_linkInfo.SUMap[hSocket]->state = 2;//状态转移
			TRACE("Challenge finish");
		}
		else;//非法用户
	}
	break;
	case 2://质询结果到来,不应到来
		break;
	default:
		break;
	}
	return;
}

void CFlieserverDoc::state2_fsm(SOCKET hSocket)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(hSocket, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);//若是正确报文，此处total_length=5
		assert(packet_len > 3);
		strLen = recv(hSocket, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;
	//函数：提取有用信息，把信息和存的值比较，如果正确，返回认证结果，更改状态，用户在线。如果错误，就...
	switch (event)
	{
	case 3://质询结果到来
	{
		temp = recvbuf + 3;
		u_short answer = ntohs(*(u_short*)temp);

		if (answer == m_linkInfo.SUMap[hSocket]->comparison)
		{
			sendbuf[0] = 4;//填事件号
			temp = &sendbuf[1];
			*(u_short*)temp = htons(4);//packet_len=4
			sendbuf[3] = 1;//认证成功
			send(hSocket, sendbuf, 4, 0);//发送

			m_linkInfo.SUMap[hSocket]->state = 3;//进入主状态
			Fileinfo* m_file = new Fileinfo;//用户在线后立即为用户建立文件相关信息档案
			m_linkInfo.SFMap.insert(std::pair<SOCKET, Fileinfo*>(hSocket, m_file));
			//UserOL_list.push_front(m_linkInfo.SUMap[hSocket]->username);
			m_linkInfo.USMap.insert(
				std::pair<std::string, SOCKET>(m_linkInfo.SUMap[hSocket]->username, hSocket));
			pView->box_UserOL.AddString((m_linkInfo.SUMap[hSocket]->username).c_str()); // 添加在线用户的用户名
			send_userlist(this);//给包括它在内的所有用户发送所有用户列表信息

			{
				//设置此用户的初始current目录，防止直接上传后更新目录出错
				m_linkInfo.SUMap[hSocket]->current_path = shared_path;
				//设置此用户的独享目录
				m_linkInfo.SUMap[hSocket]->exclusive_path =
					("..\\client_exclusive_path\\"
						+ m_linkInfo.SUMap[hSocket]->username + "\\").c_str();
				m_linkInfo.SUMap[hSocket]->current_path2 = m_linkInfo.SUMap[hSocket]->exclusive_path;
				send_dir(hSocket, true);
				send_dir(hSocket, false);
			}
		}
		else
		{
			TRACE("质询结果错");
			//错了，就删除用户，关闭连接。要记得释放内存！！
			delete m_linkInfo.SUMap[hSocket];
			m_linkInfo.SUMap.erase(hSocket);
			closesocket(hSocket);
		}
	}
	break;
	default:
		break;
	}
	return;
}

void CFlieserverDoc::state3_fsm(SOCKET hSocket)
{
	//POSITION pos = GetFirstViewPosition();
	//pView = (CDisplayView*)GetNextView(pos);

	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	//char recvbuf[CHUNK_SIZE] = { 0 };//为了接收中转数据，换更大的buffer，改设计了，不需要了。
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(hSocket, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(hSocket, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;
	switch (event)
	{
	case 5://请求目录
	{
		CString m_recvdir(&recvbuf[3], packet_len - 3);
		if (m_recvdir.Find(shared_path.Mid(2)) != -1
			|| m_recvdir.Find(m_linkInfo.SUMap[hSocket]->exclusive_path.Mid(2)) != -1)
			//如果请求的目录合法，是共享或独享目录（需要删去前两个点！！）
		{
			if (m_recvdir.Find(shared_path.Mid(2)) != -1)
			{
				// 共享目录
				m_linkInfo.SUMap[hSocket]->current_path
					= m_recvdir.Left(m_recvdir.GetLength() - 1);
				// 让服务器用current_path记住用户正在看的目录，去掉'*'
				send_dir(hSocket, true);
			}
			else
			{
				// 独享目录
				m_linkInfo.SUMap[hSocket]->current_path2
					= m_recvdir.Left(m_recvdir.GetLength() - 1);
				send_dir(hSocket, false);
			}


		}
		else;//请求目录不合法
	}
	break;
	case 11://请求下载
	case 31://独享目录请求下载
	{
		temp = recvbuf + 3;
		u_short namelen = ntohs(*(u_short*)temp);
		CString downloadName(&recvbuf[5], namelen);
		//本地打开文件
		//这里判断逻辑稍复杂，区分了共享和独享两种情况
		if (!((event == 11 && (m_linkInfo.SFMap[hSocket]->downloadFile.Open(
			m_linkInfo.SUMap[hSocket]->current_path + downloadName,
			CFile::modeRead | CFile::typeBinary, &m_linkInfo.SFMap[hSocket]->errFile)))
			||
			(event == 31 && (m_linkInfo.SFMap[hSocket]->downloadFile.Open(
				m_linkInfo.SUMap[hSocket]->current_path2 + downloadName,
				CFile::modeRead | CFile::typeBinary, &m_linkInfo.SFMap[hSocket]->errFile)))))
		{
			char errOpenFile[256];
			m_linkInfo.SFMap[hSocket]->errFile.GetErrorMessage(errOpenFile, 255);
			TRACE("\nError occurred while opening file:\n"
				"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
				m_linkInfo.SFMap[hSocket]->errFile.m_strFileName,
				errOpenFile, m_linkInfo.SFMap[hSocket]->errFile.m_cause,
				m_linkInfo.SFMap[hSocket]->errFile.m_lOsError);
			//ASSERT(FALSE);
			//回复拒绝下载
			sendbuf[0] = 12;
			sendbuf[3] = 0;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(4);
			//m_linkInfo.SUMap[hSocket]->state = 3;
			//保持主状态不变
		}
		else //成功打开
		{
			//回应允许下载
			sendbuf[0] = 12;
			sendbuf[3] = 1;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(8);
			ULONGLONG fileLength = m_linkInfo.SFMap[hSocket]->downloadFile.GetLength();//约定文件长度用ULONGLONG存储，长度是8个字节
			m_linkInfo.SFMap[hSocket]->leftToSend = fileLength;
			temp = &sendbuf[4];
			*(u_long*)temp = htonl((u_long)fileLength);//32位，可能会丢失数据
			send(hSocket, sendbuf, 8, 0);
			//第一次发送数据报文
			char chunk_send_buf[CHUNK_SIZE] = { 0 };
			u_short readChunkSize = m_linkInfo.SFMap[hSocket]->downloadFile.Read(chunk_send_buf + 6, CHUNK_SIZE - 6);
			m_linkInfo.SFMap[hSocket]->sequence = 0;
			chunk_send_buf[0] = 7;
			temp = chunk_send_buf + 1;
			*(u_short*)temp = htons(readChunkSize + 6);//不可能溢出，因为最大4096+6
			temp = temp + 2;
			*temp = m_linkInfo.SFMap[hSocket]->sequence;
			temp = temp + 1;
			*(u_short*)temp = htons(readChunkSize);

			if (UploadOnce(hSocket, chunk_send_buf, readChunkSize + 6) == FALSE)
			{
				DWORD errSend = WSAGetLastError();
				TRACE("\nError occurred while sending file chunks\n"
					"\tGetLastError = %d\n", errSend);
				ASSERT(errSend != WSAEWOULDBLOCK);
			}

			m_linkInfo.SFMap[hSocket]->leftToSend -= readChunkSize;
			m_linkInfo.SFMap[hSocket]->sequence++;
			m_linkInfo.SUMap[hSocket]->state = 5;
		}
	}
	break;
	case 15://请求上传
	case 32://独享目录请求上传
	{
		temp = recvbuf + 3;
		u_short namelen = ntohs(*(u_short*)temp);
		CString uploadName(&recvbuf[5], namelen);//文件名（不包含路径）
		temp = recvbuf + packet_len - 4;
		u_long fileLength = ntohl(*(u_long*)temp);
		m_linkInfo.SFMap[hSocket]->leftToRecv = fileLength;//会自动转换类型
		//本地打开，接收上传文件
		//这里判断逻辑稍复杂，区分了共享和独享两种情况
		if (!((event == 15 && (m_linkInfo.SFMap[hSocket]->uploadFile.Open(
			m_linkInfo.SUMap[hSocket]->current_path + uploadName,
			CFile::modeCreate | CFile::modeWrite | CFile::typeBinary,
			&m_linkInfo.SFMap[hSocket]->errFile)))
			||
			(event == 32 && (m_linkInfo.SFMap[hSocket]->uploadFile.Open(
				m_linkInfo.SUMap[hSocket]->current_path2 + uploadName,
				CFile::modeCreate | CFile::modeWrite | CFile::typeBinary,
				&m_linkInfo.SFMap[hSocket]->errFile)))))
		{
			char errOpenFile[256];
			m_linkInfo.SFMap[hSocket]->errFile.GetErrorMessage(errOpenFile, 255);
			TRACE("\nError occurred while opening file:\n"
				"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
				m_linkInfo.SFMap[hSocket]->errFile.m_strFileName, errOpenFile,
				m_linkInfo.SFMap[hSocket]->errFile.m_cause,
				m_linkInfo.SFMap[hSocket]->errFile.m_lOsError);
			//ASSERT(FALSE);
			//回复拒绝上传
			sendbuf[0] = 16;
			sendbuf[3] = 0;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(4);
			send(hSocket, sendbuf, 4, 0);
			//m_linkInfo.SUMap[hSocket]->state = 3;
			//保持主状态不变
		}
		else
		{
			//回复允许上传
			sendbuf[0] = 16;
			sendbuf[3] = 1;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(4);
			send(hSocket, sendbuf, 4, 0);
			//进入接收上传文件数据状态
			m_linkInfo.SFMap[hSocket]->sequence = 0;
			m_linkInfo.SUMap[hSocket]->state = 4;
		}
	}
	break;
	case 19://请求删除
	case 33://独享目录请求删除
	{
		CString m_filename(&recvbuf[3], packet_len - 3);//不带路径的文件名
		if (event == 19)
			m_filename = m_linkInfo.SUMap[hSocket]->current_path + m_filename;//带路径的文件名
		else
			m_filename = m_linkInfo.SUMap[hSocket]->current_path2 + m_filename;

		if (DeleteFile(m_filename))//WIN32 API
		{//成功
			sendbuf[0] = 20;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(5);
			sendbuf[3] = 1;
			sendbuf[4] = 0;
			send(hSocket, sendbuf, 5, 0);
			//删除成功后令client立即更新目录
			if (event == 19) {
				for (auto& iter : m_linkInfo.SFMap) send_dir(iter.first, true);
			}
			else {
				send_dir(hSocket, false);
			}
		}
		else
		{
			TRACE("\nError occurred while deleting file:\n");
			sendbuf[0] = 20;
			temp = &sendbuf[1];
			*(u_short*)temp = htons(5);
			sendbuf[3] = 0;
			sendbuf[4] = 0;
			send(hSocket, sendbuf, 5, 0);
		}
	}
	break;
	case 22://请求中转消息
	{
		temp = recvbuf + 3;
		u_short tar_userlen = ntohs(*(u_short*)temp);//目标用户名长度
		std::string tar_user(&recvbuf[7], tar_userlen);
		send(m_linkInfo.USMap.at(tar_user), recvbuf, packet_len, 0);//把报文转发给目标用户
	}
	break;
	case 24://中转文件请求
	{
		char tar_userlen = recvbuf[3];//目标用户名长度
		std::string tar_user(&recvbuf[5], tar_userlen);
		send(m_linkInfo.USMap.at(tar_user), recvbuf, packet_len, 0);//把报文转发给目标用户
		//sever 这时还是不操作，等待回应到来
	}
	break;
	case 25://回应中转文件请求
	{
		char tar_userlen = recvbuf[3];//目标用户名长度
		char source_userlen = recvbuf[4];
		std::string tar_user(&recvbuf[5], tar_userlen);
		std::string source_user(&recvbuf[5 + tar_userlen], source_userlen);
		send(m_linkInfo.USMap.at(tar_user), recvbuf, packet_len, 0);//把报文转发给目标用户
		if (recvbuf[packet_len - 1] == 1) {
			//如果同意，则为两用户套接字建立对应关系,暂不设置sever端中转态。
			//因为麻烦，还得想个机制退出来。但是这样也就导致，一旦中转过文件，这两个套接字的就被绑定了，下次还要解绑。
			m_linkInfo.SSMap.insert(
				std::pair<SOCKET, SOCKET>(
					m_linkInfo.USMap.at(tar_user)//这里的目的user，即最初的发起方
					, m_linkInfo.USMap.at(source_user)));
			SOCKET s;
			s = m_linkInfo.USMap.at(tar_user); m_linkInfo.SUMap[s]->state = 6;
			s = m_linkInfo.USMap.at(source_user); m_linkInfo.SUMap[s]->state = 6;
		}
	}
		break;
	case 26://中转数据，来自发起方
		break;
	case 27://中转ack，来自接收方
		break;
	default:
		break;
	}
}

void CFlieserverDoc::state4_fsm(SOCKET hSocket)
{
	char chunk_recv_buf[CHUNK_SIZE] = { 0 };
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(hSocket, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);//此处用不到
		//assert(packet_len > 3);
		//此处将要接收数据报文，应该换更大的buf
	}
	else return;

	switch (event)
	{
	case 7://收到上传数据
	{
		u_int writeChunkSize = (m_linkInfo.SFMap[hSocket]->leftToRecv < CHUNK_SIZE - 6) ? m_linkInfo.SFMap[hSocket]->leftToRecv : CHUNK_SIZE - 6;//#define CHUNK_SIZE 4096
		if (RecvOnce(hSocket, chunk_recv_buf, writeChunkSize + 3) == FALSE)//太奇怪了，这里为啥要加3才能收完所有数据？
		{//奥！因为前面多收了sequence和data_len
		//淦，还要考虑两个边界，最大只能收4093个
			DWORD errSend = WSAGetLastError();
			TRACE("\nError occurred while receiving file chunks\n"
				"\tGetLastError = %d\n", errSend);
			ASSERT(errSend != WSAEWOULDBLOCK);
		}
		//数据报文中的序号被我忽略了，按理说，可以做一个判断
		temp = chunk_recv_buf + 1;
		u_short data_len = ntohs(*(u_short*)temp);
		temp = temp + 2;
		m_linkInfo.SFMap[hSocket]->leftToRecv -= data_len;
		m_linkInfo.SFMap[hSocket]->uploadFile.Write(temp, (UINT)data_len);
		// 发送ack
		(m_linkInfo.SFMap[hSocket]->sequence)++;//序号递增，编译器默认为unsigned char，溢出也无所谓
		sendbuf[0] = 8;//char不用转换字节序
		temp = sendbuf + 1;
		*(u_short*)temp = htons(4);
		temp = temp + 2;
		*temp = m_linkInfo.SFMap[hSocket]->sequence;//表示期待下一个报文
		send(hSocket, sendbuf, 4, 0);
		//收数据的逻辑必须这么写，不然很可能导致死锁，即停在4状态出不去
		if (m_linkInfo.SFMap[hSocket]->leftToRecv > 0) {
			m_linkInfo.SUMap[hSocket]->state = 4;
		}
		else if (m_linkInfo.SFMap[hSocket]->leftToRecv == 0)
		{
			//记得要close文件句柄
			CString uploadName = m_linkInfo.SFMap[hSocket]->uploadFile.GetFileName();
			m_linkInfo.SFMap[hSocket]->uploadFile.Close();
			//好像应该重新初始化这个文件，不然会出问题的
			m_linkInfo.SFMap[hSocket]->sequence = 0;
			m_linkInfo.SUMap[hSocket]->state = 3;
			//下面两个if都是权宜之计，最好的办法还是区分独享和共享数据报文！

			//上传完成，应该立即向所有在线用户发送一次共享目录
			if (PathtoList(m_linkInfo.SUMap[hSocket]->current_path + '*').Find(uploadName) != -1)
			{
				//上传到了共享目录中
				for (auto& iter : m_linkInfo.SFMap) send_dir(iter.first, true);
			}
			//这里不能用else if逻辑，因为可能两个目录里都有这个文件
			if (PathtoList(m_linkInfo.SUMap[hSocket]->current_path2 + '*').Find(uploadName) != -1)
			{
				//上传到了独享目录中
				send_dir(hSocket, false);// 上传完成，向该client发送一次独享目录
			}
		}
		else
		{
			TRACE("leftToSend error!!!/n");
		}
	}
	break;
	default:
		break;
	}
}


void CFlieserverDoc::state5_fsm(SOCKET hSocket)
{
	char chunk_send_buf[CHUNK_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(hSocket, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(hSocket, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 8://收到下载确认
	{
		if (recvbuf[3] == m_linkInfo.SFMap[hSocket]->sequence) {

			if (m_linkInfo.SFMap[hSocket]->leftToSend > 0)
			{
				u_short readChunkSize = m_linkInfo.SFMap[hSocket]->downloadFile.Read(chunk_send_buf + 6, CHUNK_SIZE - 6);//不会溢出
				chunk_send_buf[0] = 7;
				temp = chunk_send_buf + 1;
				*(u_short*)temp = htons(readChunkSize + 6);
				temp = temp + 2;
				*temp = m_linkInfo.SFMap[hSocket]->sequence;//暂时忽略数据报文的序号
				temp = temp + 1;
				*(u_short*)temp = htons(readChunkSize);

				if (UploadOnce(hSocket, chunk_send_buf, readChunkSize + 6) == FALSE)
				{
					DWORD errSend = WSAGetLastError();
					TRACE("\nError occurred while sending file chunks\n"
						"\tGetLastError = %d\n", errSend);
					ASSERT(errSend != WSAEWOULDBLOCK);
				}
				m_linkInfo.SFMap[hSocket]->leftToSend -= readChunkSize;
				(m_linkInfo.SFMap[hSocket]->sequence)++;//只要发送数据报文就递增
				m_linkInfo.SUMap[hSocket]->state = 5;
			}
			else if (m_linkInfo.SFMap[hSocket]->leftToSend == 0) {
				//全部发送完成，并收到了所有确认
				//记得要close文件句柄
				m_linkInfo.SFMap[hSocket]->downloadFile.Close();
				//好像应该重新初始化这个文件，不然会出问题的
				m_linkInfo.SFMap[hSocket]->sequence = 0;
				m_linkInfo.SUMap[hSocket]->state = 3;
			}
			else {
				TRACE("leftToSend error!!!/n");
			}
		}
		else {
			//报文序号错误，不予发送下一个
		}
	}
	break;
	case 9://收到重传确认
		break;
	default:
		break;
	}

}

void CFlieserverDoc::state6_fsm(SOCKET hSocket)
{
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	//char recvbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[CHUNK_SIZE] = { 0 };//为了接收中转数据，换更大的buffer
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(hSocket, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(hSocket, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;
	switch (event)
	{
	case 22://请求中转消息
	{
		temp = recvbuf + 3;
		u_short tar_userlen = ntohs(*(u_short*)temp);//目标用户名长度
		std::string tar_user(&recvbuf[7], tar_userlen);
		send(m_linkInfo.USMap.at(tar_user), recvbuf, packet_len, 0);//把报文转发给目标用户
	}
	break;
	case 26://中转数据，来自发起方
	{
		SOCKET s = m_linkInfo.SSMap.at(hSocket);
		send(s, recvbuf, packet_len, 0);
	}
	break;
	case 27://中转ack，来自接收方
	{
		for (const auto& it : m_linkInfo.SSMap)
		{
			if (it.second == hSocket) {
				SOCKET s = it.first;
				send(s, recvbuf, packet_len, 0);
				break;
			}
		}
	}
	break;
	case 28://结束中转报文，来自发起方
	{
		m_linkInfo.SUMap[hSocket]->state = 3; //sever发起方回主状态。
		m_linkInfo.SUMap.at(
			m_linkInfo.SSMap.at(hSocket)
		)->state = 3;//sever接收方回主状态。
		m_linkInfo.SSMap.erase(hSocket);//解除绑定
	}
	break;
	default:
		break;
	}
}

BOOL CFlieserverDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)

	return TRUE;
}




// CFlieserverDoc 序列化

void CFlieserverDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 在此添加存储代码
	}
	else
	{
		// TODO: 在此添加加载代码
	}
}

#ifdef SHARED_HANDLERS

// 缩略图的支持
void CFlieserverDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// 修改此代码以绘制文档数据
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 搜索处理程序的支持
void CFlieserverDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// 从文档数据设置搜索内容。
	// 内容部分应由“;”分隔

	// 例如:     strSearchContent = _T("point;rectangle;circle;ole object;")；
	SetSearchContent(strSearchContent);
}

void CFlieserverDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl* pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CFlieserverDoc 诊断

#ifdef _DEBUG
void CFlieserverDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFlieserverDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG
