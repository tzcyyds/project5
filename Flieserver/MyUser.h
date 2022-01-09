#pragma once
#include "pch.h"


struct User
{
	IN_ADDR ip = { 0 };
	WORD port = 0;//WORD��ͬ��unsigned short
	std::string username = "";
	int state = 0;
	CString exclusive_path = ""; // ����Ŀ¼
	CString current_path = ""; // ��ǰ�û����ڿ���Ŀ¼
	u_short comparison = 0; //clientӦ�÷��ص���ѯ���
};
struct Fileinfo
{
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

};

class LinkInfo
{
public:
	~LinkInfo();
	void myclear();
	std::unordered_map<SOCKET, User*> SUMap;
	std::unordered_map<SOCKET, Fileinfo*> SFMap;
};


