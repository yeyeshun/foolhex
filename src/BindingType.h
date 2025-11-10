#include <string>
#include <vector>
#include <sstream>

class BindingType
{
public:
	BindingType() { m_nTypeSize = 0; m_bIsStruct = 0; }
	~BindingType() {;}

	static std::vector<BindingType*> m_vecAllTypes;
	static BindingType* FindTypeByName(const wchar_t* pszTypeName);

	int IsStruct() { return m_bIsStruct; }
	void getValue(unsigned long long nValueAdr, unsigned long long& nValue);
	void getValue(unsigned long long nValueAdr, long long& nValue);
	void getValue(unsigned long long nValueAdr, float& fValue);
	void getValue(unsigned long long nValueAdr, double& fValue);
	void getValue(unsigned long long nValueAdr, long double& fValue);
    void registerOutputFunc(void(**f)(std::wstring&, void*)){ m_pFunctionOutput = *f; }
    void Output(std::wstring& str, void* pAdr){ if (m_pFunctionOutput) m_pFunctionOutput(str, pAdr); }

	std::wstring m_strType;
	int m_nTypeSize;
protected:
	void getValue(unsigned long long nValueAdr, void* pnValue);
	void(*m_pFunctionOutput)(std::wstring&, void*);
	int m_bIsStruct;
};

class BindingStructMemberType
{
public:
	BindingStructMemberType(){ m_strArraySize = L"1"; }
	~BindingStructMemberType(){ ; }

	BindingType* m_pType;
	std::wstring m_strName;
	std::wstring m_strArraySize;
};

class BindingStructType : public BindingType
{
public:
	BindingStructType(){ m_nTypeSize = -1; m_bIsStruct = 1; }
	~BindingStructType();
	std::vector<BindingStructMemberType*>* GetChild() { return &m_vecChild; }
protected:
	std::vector<BindingStructMemberType*> m_vecChild;

};

class BindingVariant
{
public:
	BindingVariant(void);
	~BindingVariant(void){ ; }
	static std::vector<BindingVariant*> m_vecTotalVar;

    BindingType* m_pType;
	std::wstring m_strName;
	std::wstring m_strArraySize;
	std::wstring m_strViewOffsetAddr;

	std::wstring Output();
	unsigned long long GetTotalSize();
};

#define ADD_TYPE(typeName) \
do \
{\
	BindingType* p = new BindingType();\
	p->m_strType = std::wstring(L ## #typeName);\
	p->m_nTypeSize = sizeof(typeName);\
	p->registerOutputFunc(new (void(*)(std::wstring&, void*))([](std::wstring& str, void* pAdr){\
		str.clear();\
		std::wstringstream ss;\
		typeName* p = (typeName*)pAdr;\
		ss << *p;\
		str = ss.str();\
				}));\
	BindingType::m_vecAllTypes.push_back(p);\
} while (0);
