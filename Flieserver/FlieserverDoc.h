
// FlieserverDoc.h: CFlieserverDoc 类的接口
//


#pragma once
#include "MyUser.h"
#include "CDisplayView.h"

class CFlieserverDoc : public CDocument
{
protected: // 仅从序列化创建
	CFlieserverDoc() noexcept;
	DECLARE_DYNCREATE(CFlieserverDoc)

// 特性
public:
	CDisplayView* pView;
// 操作
public:
	BOOL send_dir(SOCKET hSocket, bool is_share);
	CString PathtoList(CString path);
	BOOL UploadOnce(SOCKET hSocket,const char* buf, u_int length);
	BOOL RecvOnce(SOCKET hSocket,char* buf, u_int length);

	void state1_fsm(SOCKET hSocket);
	void state2_fsm(SOCKET hSocket);
	void state3_fsm(SOCKET hSocket);
	void state4_fsm(SOCKET hSocket);
	void state5_fsm(SOCKET hSocket);
	void state6_fsm(SOCKET hSocket);
	void state7_fsm(SOCKET hSocket);
	void state8_fsm(SOCKET hSocket);
	void state9_fsm(SOCKET hSocket);
	void state10_fsm(SOCKET hSocket);
	void state11_fsm(SOCKET hSocket);
	void state_event_interface(SOCKET hSocket, char* buftemp, u_short buflen);
	void state3_fsm_internal(SOCKET hSocket, char* buftemp, u_short buflen);
	void state6_fsm_internal(SOCKET hSocket, char* buftemp, u_short buflen);
	void state8_fsm_internal(SOCKET hSocket, char* buftemp, u_short buflen);
	void state10_fsm_internal(SOCKET hSocket, char* buftemp, u_short buflen);


	UserDoc m_UserInfo;//本地用户信息 string-string,用户名-密码
	LinkInfo m_linkInfo;//SUMap:<SOCKET, User>,IP地址，端口号,质询结果,用户名等等，重要的数据结构！
	CString shared_path;
//共享区
	//std::forward_list<std::string> UserOL_list;
// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 实现
public:
	virtual ~CFlieserverDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};

