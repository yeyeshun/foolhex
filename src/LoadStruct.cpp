#include "LoadStruct.h"
#include "BindingType.h"
#include <string>
#include <cwchar>
#include <sstream>

void preprocess(std::wstring& str)
{
	const wchar_t *szMarkBegin[] = { L"//", L"/*" };
	const wchar_t *szMarkEnd[] = { L"\n", L"*/" };
	std::wstring::size_type nStart = 0;
	while (1)
	{
		std::wstring::size_type nOffsetBegin[2];
		std::wstring::size_type nOffsetEnd = 0;

		nOffsetBegin[0] = str.find(szMarkBegin[0], nStart);
		nOffsetBegin[1] = str.find(szMarkBegin[1], nStart);
		if (nOffsetBegin[0] == std::wstring::npos && nOffsetBegin[1] == std::wstring::npos)
		{
			break;
		}
		int nSelectIndex = nOffsetBegin[0] < nOffsetBegin[1] ? 0 : 1;
		nOffsetEnd = str.find(szMarkEnd[nSelectIndex], nOffsetBegin[nSelectIndex]);
		if (nOffsetEnd == std::wstring::npos)
		{
			str.erase(nOffsetBegin[nSelectIndex]);
			break;
		}
		str.erase(nOffsetBegin[nSelectIndex], nOffsetEnd - nOffsetBegin[nSelectIndex] + wcslen(szMarkEnd[nSelectIndex]));
		nStart = nOffsetBegin[nSelectIndex];
	}

	const wchar_t *szMarkNoSpace[] = { L"[", L"]" };
	int szMarkNoSpaceCount = sizeof(szMarkNoSpace) / sizeof(szMarkNoSpace[0]);
	for (int n = 0; n < szMarkNoSpaceCount; n++)
	{
		nStart = 0;
		while (1)
		{
			std::wstring::size_type nOffset = str.find(szMarkNoSpace[n], nStart);
			if (nOffset == std::wstring::npos)
			{
				break;
			}
			while (1)
			{
				if (nOffset + 1 >= str.length())
				{
					break;
				}
				wchar_t ch = str.at(nOffset + 1);
				if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
				{
					str.erase(nOffset + 1, 1);
				}
				else
				{
					break;
				}
			}
			while (1)
			{
				if (nOffset == 0)
				{
					break;
				}
				wchar_t ch = str.at(nOffset - 1);
				if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
				{
					str.erase(nOffset - 1, 1);
					nOffset--;
				}
				else
				{
					break;
				}
			}
			nStart = nOffset + wcslen(szMarkNoSpace[n]);
		}
	}

	const wchar_t *szMarkHaveSpace[] = { L"=", L";", L"{", L"}" };
	int szMarkHaveSpaceCount = sizeof(szMarkHaveSpace) / sizeof(szMarkHaveSpace[0]);
	for (int n = 0; n < szMarkHaveSpaceCount; n++)
	{
		nStart = 0;
		while (1)
		{
			std::wstring::size_type nOffset = str.find(szMarkHaveSpace[n], nStart);
			if (nOffset == std::wstring::npos)
			{
				break;
			}
			if (nOffset + 1 < str.length() && str.at(nOffset + 1) != L' ')
			{
				str.insert(nOffset + 1, 1, L' ');
			}
			if (nOffset > 0 && str.at(nOffset - 1) != L' ')
			{
				str.insert(nOffset, 1, L' ');
				nOffset++;
			}
			nStart = nOffset + wcslen(szMarkHaveSpace[n]);
		}
	}
}

int IsStringStreamEmpty(std::wstringstream& ss)
{
	if (ss.eof() || ss.tellp() == std::wstreampos(0))
	{
		return 1;
	}
	return 0;
}

int getWord(std::wstringstream& ssTotal, std::wstringstream& ssLine, std::wstring& strWord)
{
	strWord.clear();
	if (IsStringStreamEmpty(ssTotal) && IsStringStreamEmpty(ssLine))
	{
		return 0;
	}
	if (IsStringStreamEmpty(ssLine))
	{
		ssLine.clear();
		ssLine.str(L"");
		wchar_t sz[1024] = { 0 };
		do 
		{
			ssTotal.clear();
			ssTotal.getline(sz, 1024);
			ssLine << sz;
		} while (ssTotal.rdstate() & std::ios::failbit);
	}
	ssLine >> strWord;
	while (strWord.empty())
	{
		if (!getWord(ssTotal, ssLine, strWord))
		{
			return 0;
		}
	}
	return 1;
}

int parseName(std::wstring& strName, std::wstring& strOutName, std::wstring& strOutArraySize)
{
	std::wstring::size_type nFind1 = strName.find(L'[');
	if (nFind1 == std::wstring::npos)
	{
		strOutName = strName;
		strOutArraySize = L"1";
		return 1;
	}
	std::wstring::size_type nFind2 = strName.rfind(L']');
	if (nFind2 == std::wstring::npos)
	{
		return 0;
	}
	strOutName = strName.substr(0, nFind1);
	nFind1++;
	std::wstring str = strName.substr(nFind1, nFind2 - nFind1);
	strOutArraySize = str;
	return 1;
}

int regNewStructType(std::wstringstream& ssTotal, std::wstringstream& ssLine, std::wstring& strWord)
{
	BindingStructType* pNewType = new BindingStructType();
	int bSuccess = 0;
	do
	{
		if (!getWord(ssTotal, ssLine, strWord))
			break;
		pNewType->m_strType = strWord;

		if (!getWord(ssTotal, ssLine, strWord) || strWord.length() != 1 || strWord.at(0) != L'{')
			break;

		int bAddChildSuccess = 0;
		while (1)
		{
			if (!getWord(ssTotal, ssLine, strWord))
				break;
			if (strWord.length() == 1 && strWord.at(0) == L'}')
			{
				bAddChildSuccess = 1;
				break;
			}
			BindingType* pSubType = BindingType::FindTypeByName(strWord.c_str());
			if (!pSubType)
				break;
			BindingStructMemberType* pSubVar = new BindingStructMemberType();
			pSubVar->m_pType = pSubType;
			pNewType->GetChild()->push_back(pSubVar);

			if (!getWord(ssTotal, ssLine, strWord))
				break;
			if (!parseName(strWord, pSubVar->m_strName, pSubVar->m_strArraySize ))
				break;

			if (!getWord(ssTotal, ssLine, strWord) || strWord.length() != 1 || strWord.at(0) != L';')
				break;
		}
		if (!bAddChildSuccess)
			break;

		bSuccess = 1;
		while (1)
		{
			if (!getWord(ssTotal, ssLine, strWord) || (strWord.length() == 1 && strWord.at(0) == L';'))
				break;
		}
	} while (0);

	if (bSuccess)
	{
		BindingType::m_vecAllTypes.push_back(pNewType);
	}
	else
	{
		delete pNewType;
	}
	return bSuccess;
}

BindingVariant* addNewVariant(std::wstringstream& ssTotal, std::wstringstream& ssLine, std::wstring& strWord)
{
	BindingVariant* pVar = new BindingVariant();
	int bSuccess = 0;
	do
	{
		BindingType* pType = BindingType::FindTypeByName(strWord.c_str());
		if (!pType)
			break;
		pVar->m_pType = pType;

		if (!getWord(ssTotal, ssLine, strWord))
			break;
		if (!parseName(strWord, pVar->m_strName, pVar->m_strArraySize))
			break;

		if (!getWord(ssTotal, ssLine, strWord) || strWord.length() != 1 || strWord.at(0) != L'=')
			break;

		std::wstring strValue;
		while (1)
		{
			if (!getWord(ssTotal, ssLine, strWord) || (strWord.length() == 1 && strWord.at(0) == L';'))
				break;

			strValue += strWord;
		}
		pVar->m_strViewOffsetAddr = strValue;


		bSuccess = 1;
	} while (0);

	if (bSuccess)
	{
		BindingVariant::m_vecTotalVar.push_back(pVar);
	}
	else
	{
		delete pVar;
		pVar = 0;
	}
	return pVar;
}

void LoadStruct(std::wstring& str)
{
	preprocess(str);
	std::wstringstream ssTotal;
	std::wstringstream ssLine;
	std::wstring strWord;
	ssTotal << str;
	while (1)
	{
		if (!getWord(ssTotal, ssLine, strWord))
			break;
		if (strWord.compare(L"typedef") == 0)
		{
			continue;
		}
		else if (strWord.compare(L"struct") == 0)
		{
			if (!regNewStructType(ssTotal, ssLine, strWord))
			{
				std::wstring strLine = ssLine.str();
				wprintf(L"error: [regNewStructType] %s\n", strLine.c_str());
				break;
			}
		}
		else
		{
			if (!addNewVariant(ssTotal, ssLine, strWord))
			{
				std::wstring strLine = ssLine.str();
				wprintf(L"error: [addNewVariant] %s\n", strLine.c_str());
				break;
			}
		}
	}
}
