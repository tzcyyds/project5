#pragma once
#include "pch.h"
//#include "FlieserverDoc.h"

void split(const std::string& s, std::vector<std::string>& tokens, char delim = ' ');
void send_userlist(CFlieserverDoc* pDoc);