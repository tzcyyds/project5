
// ClientDoc.cpp: CClientDoc 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "Client.h"
#endif

#include "ClientDoc.h"
#include "client_func.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// CClientDoc

IMPLEMENT_DYNCREATE(CClientDoc, CDocument)

BEGIN_MESSAGE_MAP(CClientDoc, CDocument)
END_MESSAGE_MAP()


// CClientDoc 构造/析构

CClientDoc::CClientDoc() noexcept
{
	// TODO: 在此添加一次性构造代码
	pView = nullptr;
}

CClientDoc::~CClientDoc()
{
}

BOOL CClientDoc::UploadOnce(const char* buf, u_int length)
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
		bytesSend = send(pView->hCommSock, sendBuf, leftToSend, 0);
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

BOOL CClientDoc::RecvOnce(char* buf, u_int length)
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
		bytesRecv = recv(pView->hCommSock, recvBuf, leftToRecv, 0);
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

void CClientDoc::socket_state1_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 2://收到质询报文
		{
			unsigned char num_N = recvbuf[3];
			temp = &recvbuf[4];

			std::string password(pView->m_password);
			//只有点击连接时，才会刷新用户名和密码，此时一定可以获取到上次正确的密码
			u_short correct_result = 0;
			u_short correct_password = 0;
			u_short correct_sum = 0;
			assert(num_N <= 20);
			for (size_t i = 0; i < num_N; i++)
			{

				correct_sum += ntohs(*(u_short*)temp);
				temp = temp + 2;
			}
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

			correct_password = (u_short)t_p;
			correct_result = correct_sum ^ correct_password;

					
			sendbuf[0] = 3;//填写事件号
			temp = &sendbuf[1];
			*(u_short*)temp = htons(5);//packet_len=5
			temp = &sendbuf[3];
			*(u_short*)temp = htons(correct_result);//写入赋值，挺复杂的写法
			send(s, sendbuf, 5, 0);
			TRACE("respond challenge");
			pView->client_state = 2;//状态转换，已返回质询结果，等待确认
		}
		break;
	default:
		break;
	}
	return;
}

void CClientDoc::socket_state2_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp); // 这里应该等于packet_len = 4
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 4://认证结果报文
		if (recvbuf[3] == 1) {
			pView->client_state = 3;//认证成功，进入等待操作状态
			TRACE("认证成功");
		}
		else;//认证失败
		break;
	default:
		break;
	}
	return;
}

void CClientDoc::socket_state3_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp); 
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;
	switch (event)
	{
	case 6://接收返回目录
		{
			//std::string file_list(&recvbuf[3], packet_len - 3);			
			CString file_list(&recvbuf[3], packet_len - 3);
			UpdateDir(pView->FileName,file_list);
		}
		break;
	case 30://接收返回独享目录
		{
			//std::string file_list(&recvbuf[3], packet_len - 3);			
			CString file_list(&recvbuf[3], packet_len - 3);
			UpdateDir(pView->FileName2, file_list);
		}
		break;
	case 20://删除结果
		{
			if (recvbuf[3] == 1) {
				TRACE("\n 删除成功");
			}
			else{
			}
		}
	case 21:
		{
			//用户列表初始化
			CString user_list(&recvbuf[3], packet_len - 3);
			//strcpy((char*)(user_list.GetBufferSetLength(packet_len - 3)), &recvbuf[3]);//安全
			UpdateDir(pView->UserList, user_list);//以同样方式更新用户列表
		}
		break;
	case 22://接收经由服务器中转的聊天消息
		{
			temp = recvbuf + 3;
			u_short recvnamelen = ntohs(*(u_short*)temp);//接收消息用户名长度
			CString to(&recvbuf[7], recvnamelen);
			temp = recvbuf + 5;
			u_short sendnamelen = ntohs(*(u_short*)temp);//发送消息用户名长度
			CString from(&recvbuf[7 + recvnamelen], sendnamelen);
			if (to != pView->m_user) TRACE("\n聊天消息转发的用户不匹配");
			CString recvtext(&recvbuf[7 + recvnamelen + sendnamelen], packet_len - (7 + recvnamelen + sendnamelen));
			UpdateMsg(pView->Msg_list, from, to, recvtext, RGB(0, 0, 255), 15);
		}
	case 53:
	{
		//收到服务器中转类型请求报文
		temp = recvbuf + 3;
		char User1NameLen = 0;
		char User2NameLen = 0;
		User1NameLen = *(char*)temp;
		temp = temp + 1;
		CString User1Name(temp, User1NameLen);
		temp = temp + User1NameLen;
		User2NameLen = *(char*)temp;
		temp = temp + 1;
		CString User2Name(temp, User2NameLen);
		temp = temp + User2NameLen;
		u_short namelen = ntohs(*(u_short*)temp);
		temp = temp + 2;
		CString downloadName(temp, namelen);//文件名（不包含路径）
		temp = temp + namelen;
		u_long fileLength = ntohl(*(u_long*)temp);
		pView->leftToRecv = fileLength;//会自动转换类型
		char desktop[MAX_PATH] = { 0 };
		SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
		CString fileAbsPath = desktop;
		CString fileExt = downloadName;
		fileAbsPath += fileExt;
		//本地打开，接收上传文件
		//这里判断逻辑稍复杂，区分了共享和独享两种情况
		if (!((event == 53 && (pView->downloadFile.Open(fileAbsPath.GetString(),
			CFile::modeCreate | CFile::modeWrite | CFile::typeBinary,
			&pView->errFile)))))
		{
			char errOpenFile[256];
			pView->errFile.GetErrorMessage(errOpenFile, 255);
			TRACE("\nError occurred while opening file:\n"
				"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
				pView->errFile.m_strFileName, errOpenFile,
				pView->errFile.m_cause,
				pView->errFile.m_lOsError);
			//ASSERT(FALSE);
			//回复拒绝上传
			sendbuf[0] = 54;
			temp = &sendbuf[1];
			*(u_short*)temp = 6 + User1NameLen + User2NameLen;
			temp = temp + 2;
			*(char*)temp = 2;
			temp = temp + 1;
			*(char*)temp = User1NameLen;
			temp = temp + 1;
			strcpy_s(temp, User1NameLen + 1, User1Name);
			temp = temp + User1NameLen;
			*(char*)temp = User2NameLen;
			temp = temp + 1;
			strcpy_s(temp, User2NameLen + 1, User2Name);
			temp = temp + User2NameLen;
			*(u_long*)temp = htonl(fileLength);
			send(s, sendbuf, 10 + User1NameLen + User2NameLen, 0);

		}
		else
		{
			//回复允许上传
			sendbuf[0] = 54;
			temp = &sendbuf[1];
			*(u_short*)temp = 6 + User1NameLen + User2NameLen;
			temp = temp + 2;
			*(char*)temp = 1;
			temp = temp + 1;
			*(char*)temp = User1NameLen;
			temp = temp + 1;
			strcpy_s(temp, User1NameLen + 1, User1Name);
			temp = temp + User1NameLen;
			*(char*)temp = User2NameLen;
			temp = temp + 1;
			strcpy_s(temp, User2NameLen + 1, User2Name);
			temp = temp + User2NameLen;
			*(u_long*)temp = htonl(fileLength);
			send(s, sendbuf, 10 + User1NameLen + User2NameLen, 0);
			pView->client_state = 10;
		}
	}
	break;
	default:
		break;
	}
	return;
}

void CClientDoc::socket_state4_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char chunk_send_buf[CHUNK_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 16://收到回应上传请求
		{
			if (recvbuf[3] == 1) {
				if (pView->leftToSend > 0)
				{	//第一次发数据报文
					u_short readChunkSize = pView->uploadFile.Read(chunk_send_buf + 6, CHUNK_SIZE - 6);
					temp = chunk_send_buf;
					pView->sequence = 0;

					chunk_send_buf[0] = 7;
					temp = temp + 1;
					*(u_short*)temp = htons(readChunkSize + 6);//不可能溢出，因为最大4096+6
					temp = temp + 2;
					*temp = pView->sequence;
					temp = temp + 1;
					*(u_short*)temp = htons(readChunkSize);

					if (UploadOnce(chunk_send_buf, readChunkSize + 6) == FALSE)
					{
						DWORD errSend = WSAGetLastError();
						TRACE("\nError occurred while sending file chunks\n"
							"\tGetLastError = %d\n", errSend);
						ASSERT(errSend != WSAEWOULDBLOCK);
					}
					pView->leftToSend -= readChunkSize;
					(pView->sequence)++;//序号递增,表示准备发送下一个数据报文
				}
				pView->client_state = 5;
			}
		}
		break;
	case 21:
	{
		//用户列表初始化
		CString user_list(&recvbuf[3], packet_len - 3);
		//strcpy((char*)(user_list.GetBufferSetLength(packet_len - 3)), &recvbuf[3]);//安全
		UpdateDir(pView->UserList, user_list);//以同样方式更新用户列表
	}
	break;
	default:
		break;
	}
}

void CClientDoc::socket_state5_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);
	char chunk_send_buf[CHUNK_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 8://收到上传确认
		{
			if (recvbuf[3] == pView->sequence) {

				if (pView->leftToSend > 0)
				{	//准备下一次的数据报文
					u_short readChunkSize = pView->uploadFile.Read(chunk_send_buf + 6, CHUNK_SIZE - 6);//不会溢出
					chunk_send_buf[0] = 7;
					temp = chunk_send_buf + 1;
					*(u_short*)temp = htons(readChunkSize + 6);
					temp = temp + 2;
					*temp = pView->sequence;
					temp = temp + 1;
					*(u_short*)temp = htons(readChunkSize);

					if (UploadOnce(chunk_send_buf, readChunkSize + 6) == FALSE)
					{
						DWORD errSend = WSAGetLastError();
						TRACE("\nError occurred while sending file chunks\n"
							"\tGetLastError = %d\n", errSend);
						ASSERT(errSend != WSAEWOULDBLOCK);
					}
					pView->leftToSend -= readChunkSize;
					(pView->sequence)++;//表示准备发送下一个数据报文
					pView->client_state = 5;
				}
				else if (pView->leftToSend == 0) {
					//close
					pView->uploadFile.Close();
					pView->client_state = 3;
					//肯定是发方先回到主状态，此时还有一个悬而未决的ack报文，必须处理，否则会阻碍后续的传输？？不会，那个ack被扔了
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
	case 21:
	{
		//用户列表初始化
		CString user_list(&recvbuf[3], packet_len - 3);
		//strcpy((char*)(user_list.GetBufferSetLength(packet_len - 3)), &recvbuf[3]);//安全
		UpdateDir(pView->UserList, user_list);//以同样方式更新用户列表
	}
	break;
	default:
		break;
	}
}

void CClientDoc::socket_state6_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char chunk_send_buf[CHUNK_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 12://收到回应下载请求
		{
			if (recvbuf[3] == 1) {
				temp = &recvbuf[4];
				pView->leftToRecv = ntohl(*(u_long*)temp);
				pView->sequence = 0;
				socket_state7_fsm(s);
				//pView->client_state = 7;
			}
			else {
				//服务器拒绝下载请求
				pView->client_state = 3;
			}
		}
		break;
	case 21:
	{
		//用户列表初始化
		CString user_list(&recvbuf[3], packet_len - 3);
		//strcpy((char*)(user_list.GetBufferSetLength(packet_len - 3)), &recvbuf[3]);//安全
		UpdateDir(pView->UserList, user_list);//以同样方式更新用户列表
	}
	break;
	default:
		break;
	}
}

void CClientDoc::socket_state7_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char chunk_recv_buf[CHUNK_SIZE] = { 0 };
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		//assert(packet_len > 3);
		//此处将要接收数据报文，应该换更大的buf
	}
	else return;

	switch (event)
	{
	case 7://收到下载数据
		{
			u_int writeChunkSize = (pView->leftToRecv < CHUNK_SIZE - 6) ? pView->leftToRecv : CHUNK_SIZE - 6;//#define CHUNK_SIZE 4096
			if (RecvOnce(chunk_recv_buf, writeChunkSize + 3) == FALSE)//太奇怪了，这里为啥要加3才能收完所有数据？
			{	//奥！因为前面多收了sequence和data_len
				//淦，还要考虑两个边界，最大只能收4093个
				DWORD errSend = WSAGetLastError();
				TRACE("\nError occurred while receiving file chunks\n"
					"\tGetLastError = %d\n", errSend);
				ASSERT(errSend != WSAEWOULDBLOCK);
			}
			temp = chunk_recv_buf + 1;
			u_short data_len = ntohs(*(u_short*)temp);
			temp = temp + 2;
			pView->leftToRecv -= data_len;
			pView->downloadFile.Write(temp, (UINT)data_len);
			//发送ack
			(pView->sequence)++;
			sendbuf[0] = 8;//char不用转换字节序
			temp = sendbuf + 1;
			*(u_short*)temp = htons(4);
			temp = temp + 2;
			*temp = pView->sequence;
			send(s, sendbuf, 4, 0);
			//收数据的逻辑必须这么写，不然很可能导致死锁，即停在7状态出不去
			if (pView->leftToRecv > 0) {
				pView->client_state = 7;
			}
			else if(pView->leftToRecv == 0) {
				//记得要close文件句柄
				pView->downloadFile.Close();
				//好像应该重新初始化这个文件，不然会出问题的
				pView->sequence = 0;
				pView->client_state = 3;
			}
			else {
				TRACE("leftToSend error!!!/n");
			}
		}
		break;
	case 21:
	{
		//用户列表初始化
		CString user_list(&recvbuf[3], packet_len - 3);
		//strcpy((char*)(user_list.GetBufferSetLength(packet_len - 3)), &recvbuf[3]);//安全
		UpdateDir(pView->UserList, user_list);//以同样方式更新用户列表
	}
	break;
	default:
		break;
	}
}

void CClientDoc::socket_state8_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);

	char chunk_send_buf[CHUNK_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 54://收到回应上传请求
	{
		if (recvbuf[3] == 1) {
			if (pView->leftToSend > 0)
			{	//第一次发数据报文
				u_short readChunkSize = pView->uploadFile.Read(chunk_send_buf + 6, CHUNK_SIZE - 6);
				temp = chunk_send_buf;
				pView->sequence = 0;

				chunk_send_buf[0] = 7;
				temp = temp + 1;
				*(u_short*)temp = htons(readChunkSize + 6);//不可能溢出，因为最大4096+6
				temp = temp + 2;
				*temp = pView->sequence;
				temp = temp + 1;
				*(u_short*)temp = htons(readChunkSize);

				if (UploadOnce(chunk_send_buf, readChunkSize + 6) == FALSE)
				{
					DWORD errSend = WSAGetLastError();
					TRACE("\nError occurred while sending file chunks\n"
						"\tGetLastError = %d\n", errSend);
					ASSERT(errSend != WSAEWOULDBLOCK);
				}
				pView->leftToSend -= readChunkSize;
				(pView->sequence)++;//序号递增,表示准备发送下一个数据报文
			}
			pView->client_state = 9;
		}
		else if(recvbuf[3] == 2){
			pView->client_state = 3;
		}
	}
	break;
	default:
		break;
	}
}
void CClientDoc::socket_state9_fsm(SOCKET s)
{
	POSITION pos = GetFirstViewPosition();
	pView = (CDisplayView*)GetNextView(pos);
	char chunk_send_buf[CHUNK_SIZE] = { 0 };
	char recvbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = nullptr;
	char event;
	u_short packet_len;
	int strLen = recv(s, recvbuf, 3, 0);
	if (strLen == 3) {
		event = recvbuf[0];
		temp = &recvbuf[1];
		packet_len = ntohs(*(u_short*)temp);
		assert(packet_len > 3);
		strLen = recv(s, recvbuf + 3, packet_len - 3, 0);
		assert(strLen == packet_len - 3);
	}
	else return;

	switch (event)
	{
	case 8://收到上传确认
	{
		if (recvbuf[3] == pView->sequence) {

			if (pView->leftToSend > 0)
			{	//准备下一次的数据报文
				u_short readChunkSize = pView->uploadFile.Read(chunk_send_buf + 6, CHUNK_SIZE - 6);//不会溢出
				chunk_send_buf[0] = 7;
				temp = chunk_send_buf + 1;
				*(u_short*)temp = htons(readChunkSize + 6);
				temp = temp + 2;
				*temp = pView->sequence;
				temp = temp + 1;
				*(u_short*)temp = htons(readChunkSize);

				if (UploadOnce(chunk_send_buf, readChunkSize + 6) == FALSE)
				{
					DWORD errSend = WSAGetLastError();
					TRACE("\nError occurred while sending file chunks\n"
						"\tGetLastError = %d\n", errSend);
					ASSERT(errSend != WSAEWOULDBLOCK);
				}
				pView->leftToSend -= readChunkSize;
				(pView->sequence)++;//表示准备发送下一个数据报文
				pView->client_state = 9;
			}
			else if (pView->leftToSend == 0) {
				//close
				pView->uploadFile.Close();
				pView->client_state = 3;
				//肯定是发方先回到主状态，此时还有一个悬而未决的ack报文，必须处理，否则会阻碍后续的传输？？不会，那个ack被扔了
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
	case 21:
	{

	}
	case 55://收到传输完毕报文
	{
		if ((pView->leftToSend == 0)&&(recvbuf[5] == 1)) {
			pView->sequence = 0;
			pView->uploadFile.Close();
			pView->client_state = 3;
		}
		else {
			TRACE("Trans error!!!/n");
		}
	}
	break;
	default:
		break;
	}
}
void CClientDoc::socket_state10_fsm(SOCKET hSocket)
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
		u_int writeChunkSize = (pView->leftToRecv < CHUNK_SIZE - 6) ? pView->leftToRecv : CHUNK_SIZE - 6;//#define CHUNK_SIZE 4096
		if (RecvOnce(chunk_recv_buf, writeChunkSize + 3) == FALSE)//太奇怪了，这里为啥要加3才能收完所有数据？
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
		pView->leftToRecv -= data_len;
		pView->downloadFile.Write(temp, (UINT)data_len);

		//收数据的逻辑必须这么写，不然很可能导致死锁，即停在4状态出不去
		if (pView->leftToRecv > 0) {
			// 发送ack
			pView->sequence++;//序号递增，编译器默认为unsigned char，溢出也无所谓
			sendbuf[0] = 8;//char不用转换字节序
			temp = sendbuf + 1;
			*(u_short*)temp = htons(4);
			temp = temp + 2;
			*temp = pView->sequence;//表示期待下一个报文
			send(hSocket, sendbuf, 4, 0);
			pView->client_state = 10;
		}
		else if (pView->leftToRecv == 0)
		{
			// 发送传输完毕
			pView->sequence++;//序号递增，编译器默认为unsigned char，溢出也无所谓
			sendbuf[0] = 55;//char不用转换字节序
			temp = sendbuf + 1;
			*(u_short*)temp = htons(5);
			temp = temp + 2;
			*temp = pView->sequence;//表示期待下一个报文
			temp = temp + 1;
			*temp = 1;
			send(hSocket, sendbuf, 5, 0);
			//记得要close文件句柄
			CString downloadName = pView->downloadFile.GetFileName();
			pView->downloadFile.Close();
			//好像应该重新初始化这个文件，不然会出问题的
			pView->sequence = 0;
			pView->client_state = 3;

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


BOOL CClientDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)

	return TRUE;
}




// CClientDoc 序列化

void CClientDoc::Serialize(CArchive& ar)
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
void CClientDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// 修改此代码以绘制文档数据
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// 搜索处理程序的支持
void CClientDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// 从文档数据设置搜索内容。
	// 内容部分应由“;”分隔

	// 例如:     strSearchContent = _T("point;rectangle;circle;ole object;")；
	SetSearchContent(strSearchContent);
}

void CClientDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CClientDoc 诊断

#ifdef _DEBUG
void CClientDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CClientDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CClientDoc 命令
