// Logger.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <share.h>
#include <atltime.h>
#include <shellapi.h>
#include "Logger.h"


#ifdef _DEBUG
#define new MYDEBUG_NEW
#endif

using namespace std;

// CLogger

CLogger::CLogger():m_strName(_T("Info"))
{
	SetDefaultOptions();
	InitializeCriticalSection(&m_csLog);
}

CLogger::CLogger(LPCTSTR name):m_strName(name)
{
	SetDefaultOptions();
	InitializeCriticalSection(&m_csLog);
}

CLogger::~CLogger()
{
	if (IsEnabled())
		Enable(FALSE);

	if (m_hThread != NULL)
	{
		DWORD result = ::WaitForSingleObject(m_hThread, 1000);
		if (result == WAIT_TIMEOUT)
			::TerminateThread(m_hThread, 0);
		m_hThread = NULL;
	}
	if (m_hObject != NULL)
	{
		::CloseHandle(m_hObject);
		m_hObject = NULL;
	}
	DeleteCriticalSection(&m_csLog);

	// clear queue	
	while (!m_qMessage.empty())
		m_qMessage.pop();
}

void CLogger::SetDefaultOptions()
{
	m_hObject = NULL;
	m_hThread = NULL;
	m_strRootPath = _T("D:\\Log");
	m_strLogExt = _T("log");
	m_strSubDir = _T("");
	m_strHeader = _T("");
	m_nLogItem = ELogItem::Time|ELogItem::Filename|ELogItem::LineNumber|ELogItem::Level|ELogItem::NewLine;
	m_eStoreType = ELogFileType::eHour;
	m_ePathType = ELogPathType::NAME_DAY;
	m_nCapacity = 5 * 1024 * 1024;	// default 5MB
	m_nStoragePeriod = 30;	// default 30 days
	m_bEnabled = FALSE;
	m_bRun = FALSE;
}

// CLogger 멤버 함수
void CLogger::Enable(BOOL enable)
{
	m_bEnabled = enable;
	if (!enable)
	{
		m_bRun = FALSE;
		::SetEvent(m_hObject);
	}
}

BOOL CLogger::IsEnabled()
{
	return m_bEnabled;
}

void CLogger::SetExtension(LPCTSTR ext)
{
	m_strLogExt = ext;
}

void CLogger::SetFileName(LPCTSTR fname)
{
	m_strFileName = fname;
}

void CLogger::SetRootPath(LPCTSTR root)
{
	m_strRootPath = root;
}

void CLogger::SetSubDir(LPCTSTR sub)
{
	m_strSubDir = sub;
}

void CLogger::SetLoggerName(LPCTSTR name)
{
	m_strName = name;
}

void CLogger::SetHeader(LPCTSTR header)
{
	m_strHeader = header;
}

void CLogger::SetLogItem(int item)
{
	m_nLogItem = item;
}

int CLogger::GetLogItem()
{
	return m_nLogItem;
}

CString CLogger::GetExtension()
{
	return m_strLogExt;
}

CString CLogger::GetRootPath()
{
	return m_strRootPath;
}

CString CLogger::GetLoggerName()
{
	return m_strName;
}

CString CLogger::GetSubDir()
{
	return m_strSubDir;
}

ELogFileType CLogger::GetStoreType()
{
	return m_eStoreType;
}

ELogPathType CLogger::GetPathType()
{
	return m_ePathType;
}

void CLogger::SetStoreType(ELogFileType type)
{
	m_eStoreType = type;
}
void CLogger::SetPathType(ELogPathType type)
{
	m_ePathType = type;
}

void CLogger::SetSotoragePeriod(int day)
{
	m_nStoragePeriod = day;
}

BOOL CLogger::Run()
{
	if (!m_bEnabled)
		return FALSE;

	if (m_bRun)
		return FALSE;

	CString name = _T("EVT_LOG_") + m_strName;

	m_hObject = ::CreateEvent(NULL, FALSE, FALSE, name);

	if (m_hObject == NULL)
		return FALSE;

	m_bRun = TRUE;

	if (m_hThread == 0)
	{
		unsigned thrdAddr;
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, 0, &thrdAddr);

		if (m_hThread == NULL)
		{
			::MessageBox(NULL, _T("ThreadWriteLog creation failed"), _T("Error"), MB_OK | MB_ICONERROR);
			m_bRun = FALSE;
			return FALSE;
		}
	}

	return TRUE;
}

void CLogger::PutLog(LPCTSTR msg)
{
	EnterCriticalSection(&m_csLog);
	m_qMessage.push(msg);
	LeaveCriticalSection(&m_csLog);
	::SetEvent(m_hObject);
}

unsigned CLogger::WriteProcedure()
{
	do 
	{
		WaitForSingleObject(m_hObject, INFINITE);
		
		while (!m_qMessage.empty())
		{
			EnterCriticalSection(&m_csLog);
			tstring msg = m_qMessage.front();
			m_qMessage.pop();
			LeaveCriticalSection(&m_csLog);
			WriteLog(msg);
		}
	} while (m_bRun);

	return 0;
}

int CLogger::WriteLog(tstring& msg)
{
	int hFile;
	CString strFullName;

	strFullName = GetFullPath() + /*_T("\\") + */GetFileName() + _T(".") + m_strLogExt;

	int oflag = 0;

#ifdef _UNICODE
	oflag = _O_CREAT | _O_WRONLY | _O_APPEND | _O_U8TEXT;
#else
	oflag = _O_CREAT | _O_WRONLY | _O_APPEND;
#endif

	_tsopen_s(&hFile, (LPCTSTR)strFullName, oflag, _SH_DENYWR, _S_IREAD | _S_IWRITE );
	if( -1 == hFile){
		Sleep( 5 );
		_tsopen_s(&hFile, (LPCTSTR)strFullName, oflag, _SH_DENYWR, _S_IREAD | _S_IWRITE );
		if( -1 == hFile ) {
			return -1;
		}
	}
	if (m_nLogItem & static_cast<int>(ELogItem::Header) && _filelength(hFile) == 0 && !m_strHeader.IsEmpty())
		_write(hFile, (LPCTSTR)m_strHeader, m_strHeader.GetLength() * sizeof(TCHAR));
	_write(hFile, msg.c_str(), static_cast<unsigned int>(msg.size() * sizeof(TCHAR)));
	_close(hFile);

	return 0;
}

int CLogger::CheckDirectory(LPCTSTR dir, BOOL bCreate)
{
	TCHAR drive[_MAX_DRIVE];
	TCHAR path[_MAX_DIR];
	TCHAR fileName[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];

	_tsplitpath_s(dir, drive, _MAX_DRIVE, path, _MAX_DIR, fileName, _MAX_FNAME, ext, _MAX_EXT);

	TCHAR szFullPath[_MAX_PATH];
	_stprintf_s(szFullPath, _MAX_PATH, _T("%s%s\0"), drive, path);

	if (_taccess(szFullPath, 0) == 0) // if exist
	{
		return 0;
	}

	if (bCreate)
	{
		TCHAR* pPath = szFullPath;
		TCHAR* pCurStr = szFullPath;

		while (true)
		{
			if (*pCurStr == _T('\\') || *pCurStr == _T('/') || *pCurStr == _T('\0'))
			{
				TCHAR ch = *pCurStr;
				*pCurStr = _T('\0');

				if (_taccess(pPath, 0) != 0) // if not exist
				{
					if (CreateDirectory(pPath, NULL) == FALSE)
					{
						return -2; // fail to create directory
					}
				}
				*pCurStr = ch;

				if (*pCurStr == _T('\0'))
					break;
			}
			pCurStr++;
		}
	}
	else
		return -1; // directory does not exist

	return 0;
}

CString CLogger::GetFullPath()
{
	CString strFullPath;
	CString strPath = _T("");
	SYSTEMTIME st;
	GetLocalTime(&st);

	switch(m_ePathType)
	{
	case ELogPathType::Specified:
		strPath = m_strSubDir;
		break;
	case ELogPathType::DAY:
		strPath.Format(_T("%04d%02d%02d"), st.wYear, st.wMonth, st.wDay);
		break;
	case ELogPathType::MONTH:
		strPath.Format(_T("%04d-%02d"), st.wYear, st.wMonth);
		break;
	case ELogPathType::NAME:
		strPath.Format(_T("%s"), m_strName);
		break;
	case ELogPathType::ROOT:
		break;
	case ELogPathType::DAY_NAME:
		strPath.Format(_T("%04d%02d%02d\\%s"), st.wYear, st.wMonth, st.wDay, m_strName);
		break;
	case ELogPathType::NAME_MONTH:
		strPath.Format(_T("%s\\%04d-%02d"), m_strName, st.wYear, st.wMonth);
		break;
	case ELogPathType::NAME_DAY:
		strPath.Format(_T("%s\\%04d%02d%02d"), m_strName, st.wYear, st.wMonth, st.wDay);
		break;
	default:
		break;
	}
	strFullPath = (strPath.IsEmpty()) ? (m_strRootPath + _T("\\")) : ((m_strRootPath + _T("\\") + strPath + _T("\\")));
	CheckDirectory(strFullPath, TRUE);
	
	return strFullPath;
}

CString CLogger::GetFileName()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	int nFileNumber = 1;
	CString strFullName;
	CString strFileName;

	switch (m_eStoreType)
	{
	case ELogFileType::eSpecified:
		strFileName = m_strFileName;
		break;
	case ELogFileType::e12Hour:
		if (st.wHour > 11)
			strFileName.Format(_T("%04d%02d%02d(PM)-%s"), st.wYear, st.wMonth, st.wDay, m_strName);
		else
			strFileName.Format(_T("%04d%02d%02d(AM)-%s"), st.wYear, st.wMonth, st.wDay, m_strName);
		break;
	case ELogFileType::eDay:
		strFileName.Format(_T("%04d%02d%02d-%s"), st.wYear, st.wMonth, st.wDay, m_strName);
		break;
	case ELogFileType::eCapacity:		
		while (TRUE)
		{
			strFileName.Format(_T("%04d%02d%02d(%02d)-%s-%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, m_strName, nFileNumber);
			strFullName = GetFullPath() + _T("\\") + strFileName + _T(".") + m_strLogExt;
			if (NeedCreateNew(strFullName))
			{
				nFileNumber++;
				continue;
			}
			break;
		}
		break;
	case ELogFileType::eHour:
		strFileName.Format(_T("%04d%02d%02d(%02d)-%s"), st.wYear, st.wMonth, st.wDay, st.wHour, m_strName);
		break;
	default:
		strFileName.Format(_T("%04d%02d%02d(%02d)-%s"), st.wYear, st.wMonth, st.wDay, st.wHour, m_strName);
		break;
	}

	return strFileName;
}

BOOL CLogger::NeedCreateNew(LPCTSTR filename)
{
	WIN32_FIND_DATA wfd;
	HANDLE hSrch;
	DWORD nSize = -1;
	BOOL bRet = FALSE;

	hSrch = FindFirstFile(filename, &wfd);
	if (hSrch == INVALID_HANDLE_VALUE)
	{
		FindClose(hSrch);
		return bRet;
	}

	nSize = /*wfd.nFileSizeHigh*(MAXDWORD+1) +*/ wfd.nFileSizeLow;	// log 파일이 4GB 넘는 경우는 없다고 보므로..., 4GB이상도 고려하면 /* */ 해제후 type __int64로 변경.

	FindClose(hSrch);

	if (nSize >= m_nCapacity)
		bRet = TRUE;

	return bRet;
}

int CLogger::DeleteExpiredFile(CString strFolder)
{
	strFolder.TrimRight(_T("\\"));

	WIN32_FIND_DATA fd = { 0 };
	HANDLE hFile = FindFirstFile(strFolder + _T("\\*"), &fd);

	if (hFile == INVALID_HANDLE_VALUE)
		return -1;

	do
	{
		if (TEXT('.') == fd.cFileName[0])
			continue;
		if (fd.dwFileAttributes == (fd.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY))
			continue;
		else
		{
			CString strPath = strFolder + _T("\\") + CString(fd.cFileName);
			CheckExpiredByCreatedTime(strPath);
		}
	} while (FindNextFile(hFile, &fd) != 0);

	return 0;
}

void CLogger::CheckExpiredByFileName(LPCTSTR filename)
{
	CString strFilePath = filename;
	CString strFileName = strFilePath.Right(strFilePath.GetLength() - strFilePath.ReverseFind(_T('\\')) - 1);
	int nYear = _ttoi(strFileName.Left(4));
	int nMonth = _ttoi(strFileName.Mid(4, 2));
	int nDay = _ttoi(strFileName.Mid(6, 2));

	if (nYear < 1900)   // it's not log,  cf. CTime year >= 1900
		return;

	CTime createdDate = CTime(nYear, nMonth, nDay, 0, 0, 0);
	CTime currentTime = CTime::GetCurrentTime();
	CTimeSpan timeSpan = currentTime - createdDate;

	if (timeSpan.GetDays() > m_nStoragePeriod)
		::DeleteFile(strFilePath);
}

void CLogger::CheckExpiredByCreatedTime(LPCTSTR filename)
{
	CString strFileName = filename;
	WIN32_FIND_DATA fd = {0};
	HANDLE hFile = FindFirstFile(strFileName, &fd);
	FindClose(hFile);

	SYSTEMTIME st, lt;
	FileTimeToSystemTime(&fd.ftLastWriteTime, &st);
	SystemTimeToTzSpecificLocalTime(NULL, &st, &lt);
	CTime createdTime(lt);
	CTime currentTime = CTime::GetCurrentTime();
	CTimeSpan timeSpan = currentTime - createdTime;

	if (timeSpan.GetDays() > m_nStoragePeriod)
		::DeleteFile(strFileName);
}

bool CLogger::DeleteDirectory(LPCTSTR lpszDir)
{
	size_t len = _tcslen(lpszDir);
	TCHAR *pszFrom = new TCHAR[len+2];
	_tcscpy_s(pszFrom, len+2, lpszDir);
	pszFrom[len] = 0;
	pszFrom[len+1] = 0;

	SHFILEOPSTRUCT fileop;
	fileop.hwnd   = NULL;    // no status display
	fileop.wFunc  = FO_DELETE;  // delete operation
	fileop.pFrom  = pszFrom;  // source file name as double null terminated string
	fileop.pTo    = NULL;    // no destination needed
	fileop.fFlags = FOF_NOCONFIRMATION|FOF_SILENT;  // do not prompt the user

	fileop.fAnyOperationsAborted = FALSE;
	fileop.lpszProgressTitle     = NULL;
	fileop.hNameMappings         = NULL;

	int ret = SHFileOperation(&fileop);
	delete [] pszFrom;  
	return (ret == 0);
}

void CLogger::DeleteExpiredDayDir(LPCTSTR path)
{
	CString strDirPath = path;
	strDirPath.TrimRight(_T("\\"));
	WIN32_FIND_DATA fd = { 0 };
	HANDLE hFile = FindFirstFile(strDirPath + _T("\\*"), &fd);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (TEXT('.') == fd.cFileName[0])
			continue;
		if (fd.dwFileAttributes == (fd.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY))
		{
			CString strPath = strDirPath + _T("\\") + CString(fd.cFileName);
			CString strDirName(fd.cFileName);
			int nYear = _ttoi(strDirName.Left(4));
			int nMonth = _ttoi(strDirName.Mid(4, 2));
			int nDay = _ttoi(strDirName.Mid(6, 2));

            if (nYear <1900)
				continue;
			CTime createdDate = CTime(nYear, nMonth, nDay, 0, 0, 0);
			CTime currentTime = CTime::GetCurrentTime();
			CTimeSpan timeSpan = currentTime - createdDate;

			if (timeSpan.GetDays() > m_nStoragePeriod + 1)
				DeleteDirectory(strPath);
		}
	} while (FindNextFile(hFile, &fd) != 0);
}

void CLogger::DeleteExpiredMonthDir(LPCTSTR path)
{
	CString strDirPath = path;
	strDirPath.TrimRight(_T("\\"));
	WIN32_FIND_DATA fd = { 0 };
	HANDLE hFile = FindFirstFile(strDirPath + _T("\\*"), &fd);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (TEXT('.') == fd.cFileName[0])
			continue;
		if (fd.dwFileAttributes == (fd.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY))
		{
			CString strPath = strDirPath + _T("\\") + CString(fd.cFileName);
			CString strDirName(fd.cFileName);
			int nYear = _ttoi(strDirName.Left(4));
			int nMonth = _ttoi(strDirName.Mid(5, 2));

			if (nYear <1900)
				continue;
			CTime createdDate = CTime(nYear, nMonth, 28, 0, 0, 0);
			CTime currentTime = CTime::GetCurrentTime();
			CTimeSpan timeSpan = currentTime - createdDate;

			if (timeSpan.GetDays() > m_nStoragePeriod + 2)
				DeleteDirectory(strPath);
		}
	} while (FindNextFile(hFile, &fd) != 0);
}

void CLogger::DeleteLog()
{
	CString strPath;

	switch (m_ePathType)
	{
	case ELogPathType::NAME:
	case ELogPathType::NAME_DAY:
	case ELogPathType::NAME_MONTH:
		strPath = m_strRootPath + _T("\\") + m_strName;
		break;
	case ELogPathType::Specified:
		strPath = m_strRootPath + _T("\\") + m_strSubDir;
		break;
	default:
		strPath = m_strRootPath;
		break;
	}

	switch (m_ePathType)
	{
	case ELogPathType::NAME:
	case ELogPathType::ROOT:
	case ELogPathType::Specified:
		DeleteExpiredFile(strPath);
		break;
	case ELogPathType::NAME_DAY:
	case ELogPathType::DAY_NAME:
	case ELogPathType::DAY:
		DeleteExpiredDayDir(strPath);
		break;
	case ELogPathType::NAME_MONTH:
	case ELogPathType::MONTH:
		DeleteExpiredMonthDir(strPath);
		break;	
	default:
		break;
	}
}

unsigned WINAPI CLogger::ThreadProc(LPVOID pParam)
{
	CLogger *pThis = reinterpret_cast<CLogger*>(pParam);

	return pThis->WriteProcedure();
}