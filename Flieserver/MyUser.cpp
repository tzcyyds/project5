#include "pch.h"
#include "MyUser.h"



UserDoc::UserDoc()
{
	//UserDocMap.insert(std::pair<std::string, std::string>("test1", "12345"));
	//UserDocMap.insert(std::pair<std::string, std::string>("test2", "12345"));
}

UserDoc::~UserDoc()
{
}

void UserDoc::writeP()
{
}


LinkInfo::~LinkInfo()
{//为了保证内存安全
	myclear();
}
void LinkInfo::myclear() {
	if (!SUMap.empty())
	{
		for (auto& it : SUMap)
		{
			closesocket(it.first);
			delete it.second;
		}
		SUMap.clear();
	}
	if (!SFMap.empty()) 
	{
		for (auto& it : SFMap)
		{
			closesocket(it.first);//释放套接字资源
			delete it.second;
		}
		SFMap.clear();
	}
}
