#include "BindingType.h"
#include <cstring>
using namespace std;

#ifndef _WIN32
int IsBadWritePtr(void*,  int)
{
	return 0;
}
int IsBadReadPtr(void*,  int)
{
	return 0;
}
#endif


std::vector<BindingType*> BindingType::m_vecAllTypes;

BindingType* BindingType::FindTypeByName(const wchar_t* pszTypeName)
{
	size_t nCount = m_vecAllTypes.size();
	for (size_t n = 0; n < nCount; n++)
	{
		BindingType* p = m_vecAllTypes.at(n);
		if (p->m_strType.compare(pszTypeName) == 0)
		{
			return p;
		}
	}
	return 0;
}

void BindingType::getValue(unsigned long long nValueAdr, unsigned long long& nValue)
{
	nValue = 0;
	getValue(nValueAdr, &nValue);
}

void BindingType::getValue(unsigned long long nValueAdr, long long& nValue)
{
	nValue = 0;
	getValue(nValueAdr, &nValue);
}

void BindingType::getValue(unsigned long long nValueAdr, float& fValue)
{
	if (m_nTypeSize == sizeof(float))
	{
		fValue = *(float*)nValueAdr;
	}
}
void BindingType::getValue(unsigned long long nValueAdr, double& fValue)
{
	if (m_nTypeSize == sizeof(double))
	{
		fValue = *(double*)nValueAdr;
	}
}
void BindingType::getValue(unsigned long long nValueAdr, long double& fValue)
{
	if (m_nTypeSize == sizeof(long double))
	{
		fValue = *(long double*)nValueAdr;
	}
}

void BindingType::getValue(unsigned long long nValueAdr, void* pnValue)
{
	if (m_nTypeSize == 1 || m_nTypeSize == 2 || m_nTypeSize == 4 || m_nTypeSize == 8)
	{
		if ((pnValue && IsBadWritePtr(pnValue, m_nTypeSize) == 0) &&
			(nValueAdr && IsBadReadPtr((void*)nValueAdr, m_nTypeSize) == 0))
		{
			memcpy(pnValue, (void*)nValueAdr, m_nTypeSize);
		}
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
BindingStructType::~BindingStructType()
{
	size_t nCount = m_vecChild.size();
	for (size_t n = 0; n < nCount; n++)
	{
		delete m_vecChild.at(n);
	}
	m_vecChild.clear();
}


/************************************************************************/
/*                                                                      */
/************************************************************************/
BindingVariant::BindingVariant(void)
{
	m_pType = 0;
	m_strArraySize = L"1";
}

std::vector<BindingVariant*> BindingVariant::m_vecTotalVar;

std::wstring BindingVariant::Output()
{
	return L"";
}

unsigned long long BindingVariant::GetTotalSize()
{
	return 0;
}