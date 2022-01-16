### 总体思路

有事件触发的时候，窗口处理函数回返回套接字，我们通过自己设计的结构体找到这个套接字对应的状态，然后再提取第一个字节即报文的事件，传递到套接字该状态的处理函数里。

### 控制报文格式设计

第一个字节用来存储事件号，后面的格式根据前面的事件号来确定

| 事件号       | 事件             | 格式                                                         |
| ------------ | ---------------- | ------------------------------------------------------------ |
| 质询相关     |                  |                                                              |
| 01           | 发送用户名报文   | 事件号（1）+总报文长度（2）+用户名字符串长度（1）+用户名字符串 |
| 02           | 质询报文         | 事件号（1）+总报文长度（2）+整数个数N（1）+质询数据          |
| 03           | 返回质询结果报文 | 事件号（1）+总报文长度（2）+质询结果（2）                    |
| 04           | 返回认证结果报文 | 事件号（1）+总报文长度（2）+认证结果（1）                    |
|              |                  |                                                              |
| 目录相关     |                  |                                                              |
| 05           | 请求目录         | 事件号（1）+总报文长度（2）+目录                             |
| 06           | 返回目录         | 事件号（1）+总报文长度（2）+文件名                           |
| 30           | 返回独享目录     | 事件号（1）+总报文长度（2）+文件名                           |
|              |                  |                                                              |
| 文件管理相关 |                  |                                                              |
| 11           | 请求下载         | 事件号（1）+总报文长度（2）+文件名长度（2）+文件名           |
| 12           | 回应下载请求     | 事件号（1）+总报文长度（2）+是否确认（1）+下载文件长度（4）( 拒绝时没有长度) |
|              |                   |                                                              |
|              |                   |                                                |
|                  |                  |                                                |
| 15               | 请求上传         | 事件号（1）+总报文长度（2）+文件名长度（2）+文件名+文件长度（4） |
| 16               | 回应上传请求     | 事件号（1）+总报文长度（2）+是否允许（1）                 |
|              |                   |                                                              |
|              |                   |                                                |
|                  |                  |                                                |
| 19               | 请求删除         | 事件号+文件名长度+文件名（不需要文件名长度也不需要路径） |
| 20           | 回应删除请求      | 事件号（1）+总报文长度（2）+是否删除成功（1）+失败原因（1）  |
|              |                   |                                                              |
| 24           | 中转文件请求      | 事件号（1）+总报文长度（2）+接收用户名长度（1）+发送用户名长度（1）+接收用户名+发送用户名+文件名 +文件长度（4） |
| 25           | 回应中转请求      | 事件号（1）+总报文长度（2）+接收用户名长度（1）+发送用户名长度（1）+接收用户名+发送用户名+是否同意请求（1）（同意请求为1，不同意请求为2） |
| 26           | **中转数据报文**  | 事件号（1）+总报文长度（2）+序号（1）+数据包文件长度（2）+数据 |
| 27           | **中转数据 确认** | 事件号（1）+总报文长度（2）+序号（1） |
| 28           | 结束中转状态      | 事件号（1）+总报文长度（2）+填充（1） |
| 独享目录相关 |                  |                                                              |
| 31           | 请求独享目录下载 | 事件号（1）+总报文长度（2）+文件名长度（2）+文件名           |
| 32           | 请求独享目录上传 | 事件号（1）+总报文长度（2）+文件名长度（2）+文件名+文件长度（4） |
| 33           | 请求独享目录删除 | 事件号（1）+报文长度（2）+文件名 不需要文件名长度总也不需要路径 |
|              |                  |                                                              |
| 在线用户列表 |                  |                                                              |
| 21           | 用户初始化       | 事件号（1）+总报文长度（2）+用户列表，用 ’\|‘ 分隔           |
| 22           | 中转消息报文     | 事件号（1）+总报文长度（2）+接收用户名长度（2）+发送用户名长度（2）+接收用户名+发送用户名+聊天数据 |

这里不再设计某种增量或者减量策略，因为会增加复杂度，节省的流量也不一定值得。

报文中一般不包含本地路径，因为路径一般只在本地有意义。除非做了某种同步机制。

### 数据报文格式

数据应该可以不设定格式，如果需要的话，可以按下面的来。

| 事件号（16进制） | 事件 | 格式 |
| ---------------- | ---- | ---- |
| 上传下载数据     |      |      |
| F0               |      |      |
| 聊天数据         |      |      |
| F1               |      |      |

数据报文格式

07				数据报文				事件号（1）+总报文长度（2）+序号（1）+数据包文件长度（2）+数据
08				正确确认				事件号（1）+总报文长度（2）+序号（1）
09				重传确认				事件号（1）+总报文长度（2）+序号（1）

这里有个问题，序号如果是0，1，可能会导致发送和接收长度出错，应该改为递增序。发送完即清空。溢出也无所谓！类似TCP那种序号，这里统一初始化为0，应该没人会攻击的吧。

其实也是写的assert有点太严格了，要求必须收到正确的、非0的“总报文长度”。为了防止在整个TCP流中多收或少收数据（一个字节也不能差），对接收0x00不友好？确实是。

### 套接字状态设计

client ：就直接使用int变量吧，想高级点还可以使用枚举类型。

server：

​	LinkInfo结构体：

```c++
unordered_map<SOCKET, myUser> myMap;

struct myUser
{
	DWORD ip = 0;
	WORD port = 0;  //WORD等同于unsigned short
	string username = "";
    int state = 0;
};
```



### 服务器套接字

#### 项目3

质询相关：
1	连接建立状态，等待用户名到来/质询失败，等待用户名再次到来
2	已质询，等待质询结果
3	质询成功，发送ack，进入等待状态		主状态！
4	接收上传文件数据状态
5	等待下载数据确认状态

质询失败，发送失败确认，关闭套接字

目录相关：

3主状态可以返回目录

文件管理相关：

从主状态3回应下载请求，发送回应，允许则进入下载传输状态4。传输完毕后发送下载完毕报文，进入状态6等待确认，收到确认，返回主状态3

从主状态3回应上传请求，发送回应，允许则进入上传传输状态6，收到上传结束报文后，发送确认后返回3

从主状态3回应删除请求，回应，允许则进入删除确认状态9，等待确认，收到确认则删除，并回应完毕报文，并返回状态3

聊天相关：

聊天数据采用特殊的报文格式，只要收到就会记录，当然，发送也会记录。

#### 项目5

在线中转相关：

接入用户列表相关：

广播相关：

用户注册相关：

新建删除相关：



### 客户机套接字

质询相关：连接建立之后，立即发送用户名
1	已建立连接，发送用户名，等待质询状态
2	已返回质询结果，等待确认
3	收到确认，等待操作状态，主状态！
4	等待上传确认状态
5	等待上传数据确认状态
6	等待下载确认状态
7	等待下载数据状态

收到失败确认，弹窗提示，关闭套接字，要求重新连接

目录相关：

3主状态可以请求目录

文件管理相关：

3主状态单击下载按钮，进入等待状态4，收到允许下载，进入下载数据传输状态5，收到下载结束后结束下载，发送确认，返回主状态3



3主状态单击上传按钮，进入等待状态6，收到允许上传，进入数据传输状态7，发送上传完毕报文，进入完成状态8，等待回应，收到回应后返回状态3



3主状态单击删除按钮，发送删除命令，进入等待状态9，收到允许删除，弹出模态对话框，发送确认，返回状态3

聊天相关：

聊天数据采用特殊的报文格式，只要收到就会记录，当然，发送也会记录。

