
#include "pch.h"
#include "FlieserverDoc.h"

void split(const std::string& s, std::vector<std::string>& tokens, char delim = ' ') {
	tokens.clear();
	auto string_find_first_not = [s, delim](size_t pos = 0) -> size_t {
		for (size_t i = pos; i < s.size(); i++) {
			if (s[i] != delim) return i;
		}
		return std::string::npos;
	};
	size_t lastPos = string_find_first_not(0);
	size_t pos = s.find(delim, lastPos);
	while (lastPos != std::string::npos) {
		tokens.emplace_back(s.substr(lastPos, pos - lastPos));
		lastPos = string_find_first_not(pos);
		pos = s.find(delim, lastPos);
	}
}
void send_userlist(CFlieserverDoc* pDoc) {
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	sendbuf[0] = 21;
	std::string tempstr;
	for (auto& it : pDoc->m_linkInfo.USMap) tempstr = tempstr + it.first + '|';
	auto strLen = tempstr.length();
	char* temp = &sendbuf[1];
	*(u_short*)temp = htons((u_short)(strLen + 3));//应该不会溢出
	strcpy_s(&sendbuf[3], strLen + 1, tempstr.c_str());
	for (auto& it : pDoc->m_linkInfo.USMap)
		send(it.second, sendbuf, strLen + 3, 0);//发给除它之外的所有在线用户（不在主状态也可以）

}