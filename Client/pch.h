// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include <afxinet.h>
#include "afxwin.h"
#include <string>
#include <vector>
#include <stdexcept>
//view和doc共用的表达式/宏
constexpr auto MAX_WSAE_TIMES = 10;// 单次发送或接收过程中所允许出现WSAEWOULDBLOCK的最大次数
constexpr auto CHUNK_SIZE = 4096;
constexpr auto MAX_BUF_SIZE = 256;
#endif //PCH_H
