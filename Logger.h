#ifndef __LOGGER_H__
#define  __LOGGER_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <queue>
#include <atlstr.h>
#include "../common/tstring.h"

// CLogger 명령 대상입니다.
enum class ELogItem: unsigned short
{
	Filename = 0x1,
	LineNumber = 0x2,
	Function = 0x4,
	Date = 0x8,
	Time = 0x10,
	ThreadId = 0x20,
	LoggerName = 0x40,
	Level = 0x80,
	NewLine = 0x100,
	Header = 0x200
};

inline int operator |(ELogItem a, ELogItem b)
{
	return static_cast<int>(a) | static_cast<int>(b);
}
inline int operator |(int a, ELogItem b)
{
	return a | static_cast<int>(b);
}
inline int operator |(ELogItem a, int b)
{
	return static_cast<int>(a) | b;
}

inline int operator &(ELogItem a, ELogItem b)
{
	return static_cast<int>(a) & static_cast<int>(b);
}
inline int operator &(int a, ELogItem b)
{
	return a & static_cast<int>(b);
}
inline int operator &(ELogItem a, int b)
{
	return static_cast<int>(a) & b;
}

enum class ELogFileType
{
	eHour = 0,		// 20130405(21)-test.log
	eDay = 1,		// 20130405-test.log
	eCapacity = 2,	// 20130405(21)-test-01.log
	e12Hour = 3,	// 20130405(AM)-test.log
	eSpecified = 4  // Specified Name
};

enum class ELogPathType
{                    // eg.
	DAY = 0,	     // D:\Log\20130312\test.log
	MONTH = 1,       // D:\Log\9월\test.log
	NAME = 2,        // D:\Log\Comm\test.log
	ROOT = 3,        // D:\Log\test.log
	DAY_NAME = 4,    // D:\Log\20130312\Comm\test.log
	NAME_MONTH = 5,  // D:\Log\Comm\9월\test.log
	NAME_DAY = 6,    // D:\Log\Comm\20130312\test.log
	Specified = 7,   // D:\Log\specified sub path\test.log
	TW1,             // special format1
	TW2              // special format2
};

class CLogger
{
public:
	CLogger();
	CLogger(LPCTSTR name);
	virtual ~CLogger();

	void SetDefaultOptions();
	void PutLog(LPCTSTR msg);
	void SetRootPath(LPCTSTR root);
	void SetSubDir(LPCTSTR sub);
	void SetExtension(LPCTSTR ext);
	void SetFileName(LPCTSTR fname);
	void SetLoggerName(LPCTSTR name);
	void SetHeader(LPCTSTR header);
	void SetLogItem(int item);
	void SetStoreType(ELogFileType type);
	void SetPathType(ELogPathType type);
	void SetSotoragePeriod(int day);
	ELogFileType GetStoreType();
	ELogPathType GetPathType();
	CString GetFileName();
	CString GetFullPath(); // except filename and extension
	int CheckDirectory(LPCTSTR dir, BOOL bCreate);
	//int Create(LPCTSTR name);
	void Enable(BOOL enable);
	BOOL IsEnabled();
	unsigned WriteProcedure();
	int WriteLog(std::tstring& msg);
	int GetLogItem();
	CString GetLoggerName();
	CString GetRootPath();
	CString GetSubDir();
	CString GetExtension();
	BOOL NeedCreateNew(LPCTSTR filename);
	BOOL Run();
	static unsigned WINAPI ThreadProc(LPVOID pParam);
	void DeleteLog();
	void DeleteExpiredDayDir(LPCTSTR path);
	void DeleteExpiredMonthDir(LPCTSTR path);
	bool DeleteDirectory(LPCTSTR lpszDir);
	int DeleteExpiredFile(CString strFolder);
	void CheckExpiredByFileName(LPCTSTR filename);
	void CheckExpiredByCreatedTime(LPCTSTR filename);

private:
	CString m_strRootPath;
	CString m_strSubDir;
	//CString m_strFullPath; // except filename and extension
	CString m_strLogExt;
	CString m_strFileName;
	CString m_strName;
	CString m_strHeader;

	std::queue<std::tstring> m_qMessage;

	BOOL m_bEnabled;
	volatile BOOL m_bRun;

	CRITICAL_SECTION m_csLog;

	int m_nLogItem;
	DWORD m_nCapacity;	// unit: Byte
	int m_nStoragePeriod;	// unit: day
	ELogFileType m_eStoreType;
	ELogPathType m_ePathType;

	HANDLE m_hThread;
	HANDLE m_hObject;
};


#endif  // __LOGGER_H__