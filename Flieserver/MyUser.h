#pragma once
#include "pch.h"


class User
{
public:
	IN_ADDR ip = { 0 };
	WORD port = 0;//WORD��ͬ��unsigned short
	std::string username = "";
	int state = 0;
	CString strdirpath = ""; // �ļ�·��
};
class Fileinfo 
{
public:
	// ͨ��
	CFileException errFile;
	CHAR sequence = 0;
	// �ϴ��ļ����
	CFile uploadFile;
	ULONGLONG leftToSend = 0;
	// �����ļ����
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


