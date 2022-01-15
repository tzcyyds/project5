#include "pch.h"

#include "CDisplayView.h"
#include "ClientDoc.h"

bool SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c) {
	// �ַ����ָ��
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
	return 1;
}

bool UpdateDir(CListBox& f_name, CString recv) {
	// �����б���ʾ���ļ�Ŀ¼
	f_name.ResetContent();

	std::vector<std::string> v;
	std::string strStr = recv.GetBuffer(0);
	SplitString(strStr, v, "|"); //�ɰ�����ַ����ָ�;

	for (auto i = 0; i != v.size(); ++i)
		f_name.AddString(v[i].c_str());
	return 1;
}

bool Enterdir(SOCKET hSocket,CListBox& f_name, CString& path) {
	CString selFile;

	f_name.GetText(f_name.GetCurSel(), selFile); //��ȡ�û�ѡ���Ŀ¼��

	if (selFile.Find('.') == -1) // �ж��Ƿ�Ϊ�ļ��У�ԭ�����ļ�����'.'
	{
		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = nullptr;
		path = selFile + "\\*";// ���ر��浱ǰ���ļ���·�����ڷ�����һ���ļ���ʱ��ʹ�õ�
		int strLen = path.GetLength();
		sendbuf[0] = 5;
		temp = &sendbuf[1];
		*(u_short*)temp = htons(strLen + 3);
		//temp = m_send.GetBuffer();
		//ʹ��strcpy,����ȫ����Ҫ+1��
		strcpy_s(&sendbuf[3], strLen + 1, path);
		//m_send.ReleaseBuffer();
		//�˴����ܲ���Ҫ��ô����ȫ���ķ��ͷ�ʽ
		send(hSocket, sendbuf, strLen + 3, 0);
	}
	else return 0;

	return 1;
}

bool Goback(SOCKET hSocket, CString& path) {
	if (path.GetLength() != 0) // �жϲ��ǳ�ʼ��ʱ��Ŀ¼
	{
		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = nullptr;
		int pos;
		//���ַ�����ȡ�ķ��������һ��Ŀ¼
		pos = path.ReverseFind('\\');
		path = path.Left(pos);
		pos = path.ReverseFind('\\');
		path = path.Left(pos);
		path = path + "\\*";
		int strLen = path.GetLength();

		sendbuf[0] = 5;
		temp = &sendbuf[1];
		*(u_short*)temp = htons(strLen + 3);
		//temp = strdirpath.GetBuffer();
		strcpy_s(&sendbuf[3], strLen + 1, path);
		//strdirpath.ReleaseBuffer();
		//�˴����ܲ���Ҫ��ô����ȫ���ķ��ͷ�ʽ
		send(hSocket, sendbuf, strLen + 3, 0);
	}
	else return 0;

	return 1;
}

bool Upload(CDisplayView* pView,bool is_share) {
	char szFilters[] = "�����ļ� (*.*)|*.*||";
	CFileDialog fileDlg(TRUE, NULL, NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);

	char desktop[MAX_PATH] = { 0 };
	SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
	fileDlg.m_ofn.lpstrInitialDir = desktop;//��Ĭ��·������Ϊ����

	if (fileDlg.DoModal() == IDOK)//�������򿪡��Ի���
	{
		CString fileAbsPath = fileDlg.GetPathName();
		CString uploadName = fileDlg.GetFileName();
		if (!(pView->uploadFile.Open(fileAbsPath.GetString(),
			CFile::modeRead | CFile::typeBinary, &pView->errFile)))
		{
			char errOpenFile[MAX_BUF_SIZE];
			pView->errFile.GetErrorMessage(errOpenFile, 255);
			TRACE("\nError occurred while opening file:\n"
				"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
				pView->errFile.m_strFileName, errOpenFile, pView->errFile.m_cause, pView->errFile.m_lOsError);
			return 0;
		}

		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = sendbuf;
		u_short namelen = uploadName.GetLength();//�˴��п��ܶ�ʧ��Ϣ
		ULONGLONG fileLength = pView->uploadFile.GetLength();//64λ

		if(is_share) sendbuf[0] = 15;//����Ŀ¼�ϴ�����
		else sendbuf[0] = 32;//����Ŀ¼�ϴ�����

		temp = &sendbuf[3];
		*(u_short*)temp = htons(namelen);
		strcpy_s(sendbuf + 5, namelen + 1, uploadName);
		temp = &sendbuf[namelen + 5];
		*(u_long*)temp = htonl((u_long)fileLength);//32λ�����ܻᶪʧ����
		temp = &sendbuf[1];
		*(u_short*)temp = htons((u_short)(namelen + 9));

		pView->leftToSend = fileLength;
		send(pView->hCommSock, sendbuf, namelen + 9, 0);

		pView->client_state = 4;//��Ϊ�ȴ��ϴ�ȷ��״̬
	}
	return 1;
}

bool Transfer(CDisplayView* pView) {
	char szFilters[] = "�����ļ� (*.*)|*.*||";
	CFileDialog fileDlg(TRUE, NULL, NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);

	char desktop[MAX_PATH] = { 0 };
	SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
	fileDlg.m_ofn.lpstrInitialDir = desktop;//��Ĭ��·������Ϊ����

	if (fileDlg.DoModal() == IDOK)//�������򿪡��Ի���
	{
		CString fileAbsPath = fileDlg.GetPathName();
		CString uploadName = fileDlg.GetFileName();
		if (!(pView->uploadFile.Open(fileAbsPath.GetString(),
			CFile::modeRead | CFile::typeBinary, &pView->errFile)))
		{
			char errOpenFile[MAX_BUF_SIZE];
			pView->errFile.GetErrorMessage(errOpenFile, 255);
			TRACE("\nError occurred while opening file:\n"
				"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
				pView->errFile.m_strFileName, errOpenFile, pView->errFile.m_cause, pView->errFile.m_lOsError);
			return 0;
		}

		char sendbuf[MAX_BUF_SIZE] = { 0 };
		char* temp = sendbuf;

		//��ȡ�ͻ�1���ͻ�2��Ϣ
		CString User2Name;
		pView->UserList.GetText(pView->UserList.GetCurSel(), User2Name);
		char User1NameLen = pView->m_user.GetLength();
		char User2NameLen = User2Name.GetLength();
		//��ȡ�ļ���Ϣ
		u_short namelen = uploadName.GetLength();//�˴��п��ܶ�ʧ��Ϣ
		ULONGLONG fileLength = pView->uploadFile.GetLength();//64λ
		//װ����ת������
		sendbuf[0] = 51;
		temp = &sendbuf[1];
		*(u_short*)temp = htons(11 + User1NameLen + User2NameLen + namelen);
		sendbuf[3] = User1NameLen;
		temp = &sendbuf[4];
		strcpy_s(sendbuf + 4, User1NameLen + 1, pView->m_user);
		temp = temp + User1NameLen;
		*(char*)temp = User2NameLen;
		temp = temp + 1;
		strcpy_s(temp, User2NameLen + 1, User2Name);
		temp = temp + User2NameLen;
		*(u_short*)temp = htons(namelen);
		temp = temp + 2;
		strcpy_s(temp, namelen + 1, uploadName);
		temp = temp + namelen;
		*(u_long*)temp = htonl((u_long)fileLength);

		pView->leftToSend = fileLength;
		send(pView->hCommSock, sendbuf, 11 + User1NameLen + User2NameLen + namelen, 0);
		pView->client_state = 8;
	}
	return 1;
}

bool Download(CDisplayView* pView, bool is_share) {
	CString downloadName;

	if (is_share) pView->FileName.GetText(pView->FileName.GetCurSel(), downloadName); //����Ŀ¼�����Ҫ������Դ��
	else pView->FileName2.GetText(pView->FileName2.GetCurSel(), downloadName); //����Ŀ¼�����Ҫ������Դ��
	
	if (!downloadName.IsEmpty())
	{
		//��������Ϊ�Ի���
		CString fileExt = downloadName.Right(downloadName.GetLength() - downloadName.ReverseFind('.'));
		char szFilters[MAX_PATH] = { 0 };//��Ȼ�п���̫С
		sprintf_s(szFilters, "(*%s)|*%s||", fileExt.GetString(), fileExt.GetString());
		CFileDialog fileDlg(FALSE, NULL, downloadName,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilters);

		char desktop[MAX_PATH] = { 0 };
		SHGetSpecialFolderPath(NULL, desktop, CSIDL_DESKTOP, FALSE);
		fileDlg.m_ofn.lpstrInitialDir = desktop;//��Ĭ��·������Ϊ����

		if (fileDlg.DoModal() == IDOK)
		{
			CString fileAbsPath = fileDlg.GetPathName();
			if (fileDlg.GetFileExt() == "")
			{
				fileAbsPath += fileExt;
			}
			if (!(pView->downloadFile.Open(fileAbsPath.GetString(),
				CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &pView->errFile)))
			{
				char errOpenFile[MAX_BUF_SIZE];
				pView->errFile.GetErrorMessage(errOpenFile, 255);
				TRACE("\nError occurred while opening file:\n"
					"\tFile name: %s\n\tCause: %s\n\tm_cause = %d\n\t m_IOsError = %d\n",
					pView->errFile.m_strFileName, errOpenFile, pView->errFile.m_cause, pView->errFile.m_lOsError);
				return 0;
			}

			u_short nameLength = downloadName.GetLength();//�˴��п��ܶ�ʧ��Ϣ
			char sendbuf[MAX_BUF_SIZE] = { 0 };
		
			if (is_share) sendbuf[0] = 11;//����Ŀ¼��������
			else sendbuf[0] = 31;//����Ŀ¼��������

			char* temp = sendbuf + 1;
			*(u_short*)temp = htons(nameLength + 5);
			temp = temp + 2;
			*(u_short*)temp = htons(nameLength);
			strcpy_s(sendbuf + 5, nameLength + 1, downloadName);//downloadName��Ӧ�ð���·��
			send(pView->hCommSock, sendbuf, nameLength + 5, 0);

			pView->client_state = 6;//��Ϊ�ȴ�����ȷ��״̬
		}
	}
	else return 0;//�����ļ���Ϊ��
	return 1;
}

bool m_Delete(CDisplayView* pView, bool is_share) {
	CString deleteName;
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	char* temp = sendbuf;

	if (is_share)pView->FileName.GetText(pView->FileName.GetCurSel(), deleteName); // ��ȡ�û�Ҫɾ���Ĺ���Ŀ¼�ļ���
	else pView->FileName2.GetText(pView->FileName2.GetCurSel(), deleteName); // ��ȡ�û�Ҫɾ���Ķ���Ŀ¼�ļ���

	if (!deleteName.IsEmpty())
	{
		if (AfxMessageBox((CString)"ȷ��Ҫɾ������ļ���", 4 + 48) == IDYES)
		{
			//deleteName = strdirpath.Left(strdirpath.GetLength() - 1) + deleteName;//ƴ����ȷ���ļ���
			//����·��
			int nameLength = deleteName.GetLength();

			if (is_share) sendbuf[0] = 19;//����Ŀ¼ɾ������
			else sendbuf[0] = 33;//����Ŀ¼ɾ������

			temp = &sendbuf[1];
			*(u_short*)temp = htons((nameLength + 3));
			temp = &sendbuf[3];
			strcpy_s(temp, nameLength + 1, deleteName);
			send(pView->hCommSock, sendbuf, nameLength + 3, 0);
		}
	}
	else return 0;//ɾ���ļ���Ϊ��
	return 1;
}

bool m_Connect(CDisplayView* pView) {
	char sendbuf[MAX_BUF_SIZE] = { 0 };
	CClientDoc* pDoc = (CClientDoc*)pView->GetDocument();

	// �ж��쳣���
	if (pView->m_ip == NULL)
	{
		TRACE(_T("IP��ַΪ�գ�"));
		return 0;
	}
	else if (pView->m_SPort == NULL)
	{
		TRACE(_T("�ƶ˶˿�Ϊ�գ�"));
		return 0;
	}
	else if (pView->m_LPort == NULL)
	{
		TRACE(_T("���ض˿�Ϊ�գ�"));
		return 0;
	}

	pView->servAdr.sin_family = AF_INET;
	pView->servAdr.sin_addr.s_addr = htonl(pView->m_ip);
	pView->servAdr.sin_port = htons(pView->m_SPort);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		TRACE("WSAStartup() failed");
		return 0;
	}
	pView->hCommSock = socket(AF_INET, SOCK_STREAM, 0);
	if (pView->hCommSock == INVALID_SOCKET)
	{
		TRACE("socket() failed");
		return 0;
	}


	if (connect(pView->hCommSock, (SOCKADDR*)&(pView->servAdr), sizeof(pView->servAdr)) == SOCKET_ERROR)//��ʽ�󶨣����ӷ�����
	{
		TRACE("connect() failed");
		return 0;
	}
	//�����������connect�������ģ�����winsock��ʵ����3����ʱ����ʱ�ͷ��ش���
	if (WSAAsyncSelect(pView->hCommSock, pView->m_hWnd, WM_SOCK, FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		TRACE("WSAAsyncSelect() failed");
		return 0;
	}

	//�����û�������
	sendbuf[0] = 1;//��д�¼���

	int strLen = pView->m_user.GetLength();
	sendbuf[3] = strLen % 256;//��д�ַ������ȣ��û����ַ���������ҪС��256��
	memcpy(sendbuf + 4, pView->m_user, strLen);//��д�û����ַ���
	char* temp = &sendbuf[1];
	*(u_short*)temp = htons((4 + strLen));
	send(pView->hCommSock, sendbuf, strLen + 4, 0);
	TRACE("send account");
	pView->client_state = 1;//���ӳɹ�,�ѷ����û������ȴ���ѯ
	return 1;
}

bool UpdateMsg(CRichEditCtrl& f_name, CString from, CString to, CString recvtext, COLORREF rgb, int size) {
	//�����յ�����Ϣ

	time_t rawtime;
	struct tm timeinfo;
	char timE[40] = { 0 };
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	//strftime(timE, 40, "Date:\n%Y-%m-%d\nTime:\n%I:%M:%S\n", &timeinfo);
	strftime(timE, 40, "%Y-%m-%d %H:%M:%S", &timeinfo);
	//printf("%s", timE);
	CString m_time(timE);

	CHARFORMAT cf;
	ZeroMemory(&cf, sizeof(CHARFORMAT));
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE |
		CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE;
	// cf.dwEffects = CFE_UNDERLINE;
	cf.yHeight = (size - 2) * (size - 2);//���ָ߶�
	cf.crTextColor = rgb; //������ɫ
	strcpy_s(cf.szFaceName, _T("����"));//��������
	f_name.SetSelectionCharFormat(cf);
	f_name.SetSel(-1, -1);
	f_name.ReplaceSel(m_time + "    from " + from + " to " + to + ":\n");

	cf.yHeight = size * size;//���ָ߶�
	cf.crTextColor = RGB(0, 0, 0); //������ɫ
	f_name.SetSelectionCharFormat(cf);
	f_name.SetSel(-1, -1);
	f_name.ReplaceSel(recvtext + "\n");

	return 1;
}