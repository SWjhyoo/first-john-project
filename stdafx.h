// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#include "targetver.h"

#define _BIND_TO_CURRENT_VCLIBS_VERSION 1

#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include <Windows.h>
// #include <afx.h>
// #include <afxwin.h>         // MFC 핵심 및 표준 구성 요소입니다.
// //#include <afxext.h>
// #include <afxtempl.h>


// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
// https://support.microsoft.com/ko-kr/help/140858/the-crtdbg-map-alloc-macro-does-not-work-as-expected
#ifdef _DEBUG
#define MYDEBUG_NEW   new( _NORMAL_BLOCK, __FILE__, __LINE__)
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
//allocations to be of _CLIENT_BLOCK type
#else
#define MYDEBUG_NEW
#endif // _DEBUG