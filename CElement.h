#pragma once
#include "pch.h"

#include <string>
#include <minwindef.h>
#include <atlstr.h>
#include <windef.h>
#include <map>
#include "Logger.h"

using namespace std;

class CElement
{
protected:
	
	tstring objectType;
	tstring ObjectID;
	tstring Description;
	bool InService;
	CLogger m_Logger;
	HANDLE m_hThread;

public : 

	CElement();
	
	~CElement();
	
	void doSomething();

};
