#include "pch.h"
#include "MyUser.h"



UserDoc::UserDoc()
{
}

UserDoc::~UserDoc()
{
}

void UserDoc::writeP()
{
}

void UserDoc::initDoc()
{
	UserDocMap.insert(std::pair<std::string, std::string>("test", "12345"));
}

LinkInfo::~LinkInfo()
{//Ϊ�˱�֤�ڴ氲ȫ
	for (auto &it: SUMap)
	{
		delete it.second;
	}
	SUMap.clear();
	for (auto& it : SFMap)
	{
		delete it.second;
	}
	SFMap.clear();
}
