#pragma once
#include "pch.h"


class User
{
public:
	IN_ADDR ip = { 0 };
	WORD port = 0;//WORD等同于unsigned short
	std::string username = "";
	int state = 0;
	CString strdirpath = ""; // 文件路径
};
class Fileinfo 
{
public:
	// 通用
	CFileException errFile;
	CHAR sequence = 0;
	// 上传文件相关
	CFile uploadFile;
	ULONGLONG leftToSend = 0;
	// 下载文件相关
	CFile downloadFile;
	ULONGLONG leftToRecv = 0;
};

class UserDoc {

public:
	UserDoc();
	~UserDoc();

public:
	std::unordered_map<std::string, std::string> UserDocMap;
	void writeP();
	void initDoc();

};

class LinkInfo
{
public:
	~LinkInfo();
	std::unordered_map<SOCKET, User*> SUMap;
	std::unordered_map<SOCKET, Fileinfo*> SFMap;
};


