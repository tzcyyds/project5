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
{//Ϊ�˱�֤�ڴ氲ȫ
	if (!SUMap.empty())
	{
		for (auto& it : SUMap)
		{
			delete it.second;
		}
		SUMap.clear();
	}
	if (!SFMap.empty()) {
		for (auto& it : SFMap)
		{
			delete it.second;
		}
		SFMap.clear();
	}

}
