#pragma once

#include "pch.h"

bool SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c);
bool UpdateDir(CListBox& f_name, CString recv);
bool Enterdir(SOCKET hSocket, CListBox& f_name, CString& path);
bool Goback(SOCKET hSocket, CString& path);
bool Upload(CDisplayView* pView, bool is_share);
bool Download(CDisplayView* pView, bool is_share);
bool m_Delete(CDisplayView* pView, bool is_share);
bool m_Connect(CDisplayView* pView);