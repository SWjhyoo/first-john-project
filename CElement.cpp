#include "CElement.h"


CElement::CElement()
{
	this->m_Logger = new CLogger()
}
CElement::~CElement()
{
	delete [] this->m_hThread;
}