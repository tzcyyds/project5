﻿
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

#include <propkey.h>

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
			u_int num_N = recvbuf[3];
			temp = &recvbuf[4];

			u_int password_value = 0;
			u_int password_len = pView->m_password.GetLength();//只有点击连接时，才会刷新用户名和密码，此时一定可以获取到上次正确的密码
			u_short correct_result = 0;
			u_short correct_password = 0;
			u_short correct_sum = 0;

			if (num_N <= 20)//整数个数小于20,u_int一定大于0
			{
				for (size_t i = 0; i < num_N; i++)
				{
					//这两行待修改
					correct_sum += ntohs(*(u_short*)temp);
					temp = temp + 2;
				}

				if ((password_len > 0) && (password_len <= 5))//两字节，最大65535
				{
					password_value = (u_int)_ttoi(pView->m_password);
					correct_password = (u_short)(password_value % 65536);
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
				else
				{//密码长度不对
				}
			}
			else
			{//整数个数不对
			}
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
			pView->UpdateDir(file_list);
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
