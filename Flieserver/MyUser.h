#pragma once
#include "pch.h"


struct User
{
	IN_ADDR ip = { 0 };
	WORD port = 0;//WORD等同于unsigned short
	std::string username = "";
	int state = 0;
	CString exclusive_path = ""; // 独享目录
	CString current_path = ""; // 当前用户正在看的目录
	u_short comparison = 0; //client应该返回的质询结果
	CString current_path2 = ""; // 当前用户正在看的独享目录
};
struct Fileinfo
{
	// 通用
	CFileException errFile;
	CHAR sequence = 0;
	// 上传文件相关
	CFile uploadFile;
	ULONGLONG leftToSend = 0;
	// 下载文件相关
	CFile downloadFile;
	ULONGLONG leftToRecv = 0;
	// 中转文件相关
	ULONGLONG leftToTrans = 0;

};

class UserDoc {

public:
	UserDoc();
	~UserDoc();

public:
	std::unordered_map<std::string, std::string> UserDocMap;
	void writeP();

};

class LinkInfo
{
public:
	~LinkInfo();
	void myclear();
	std::unordered_map<SOCKET, User*> SUMap;
	std::unordered_map<SOCKET, Fileinfo*> SFMap;
	std::unordered_map<std::string, SOCKET> USMap;
	std::unordered_map<SOCKET, SOCKET> SSMap;
};


