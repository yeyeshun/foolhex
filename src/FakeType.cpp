#include "FakeType.h"
#include "BindingType.h"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

std::string ws2s(const std::wstring& wstr)
{
    std::string str(wstr.size() * 4, ' ');
    str.resize(std::wcstombs(&str[0], wstr.c_str(), str.size()));
    return str;
}

std::wstring s2ws(const std::string& str)
{
    std::wstring wstr(str.size(), L' ');
    wstr.resize(std::mbstowcs(&wstr[0], str.c_str(), str.size()));
    return wstr;
}

void RegAliasType(const std::string& strFakeNameA, const std::string& strRealTypeNameA)
{
    std::wstring strFakeName = s2ws(strFakeNameA);
    std::wstring strRealTypeName = s2ws(strRealTypeNameA);

    if (BindingType::FindTypeByName(strFakeName.c_str()))
    {
        return;
    }

    BindingType* pType = BindingType::FindTypeByName(strRealTypeName.c_str());
    if (!pType)
    {
        return;
    }

    BindingType* pFakeType = new BindingType();
    *pFakeType = *pType;
    pFakeType->m_strType = strFakeName;
    BindingType::m_vecAllTypes.push_back(pFakeType);
    return;
}

// 辅助函数：去除字符串两端的空白字符
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// 辅助函数：判断是否为注释行
bool isCommentLine(const std::string& line) {
    std::string trimmed = trim(line);
    return trimmed.empty() || 
           trimmed[0] == '#' || 
           (trimmed.length() > 1 && trimmed[0] == '/' && trimmed[1] == '/');
}

// 解析配置文件的主要函数
void parseSimpleConfig(const std::string& filename, 
                      std::function<void(const std::string&, const std::string&)> parseKeyValue) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误: 无法打开配置文件 '" << filename << "'" << std::endl;
        return;
    }
    
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // 去除行首尾的空白字符
        std::string trimmedLine = trim(line);
        
        // 跳过空行和注释行
        if (isCommentLine(trimmedLine)) {
            continue;
        }
        
        // 查找等号分隔符
        size_t delimiterPos = trimmedLine.find('=');
        if (delimiterPos == std::string::npos) {
            std::cerr << "警告: 第 " << lineNumber << " 行格式错误 (缺少等号): " << line << std::endl;
            continue;
        }
        
        // 提取键和值
        std::string key = trim(trimmedLine.substr(0, delimiterPos));
        std::string value = trim(trimmedLine.substr(delimiterPos + 1));
        
        // 验证键不为空
        if (key.empty()) {
            std::cerr << "警告: 第 " << lineNumber << " 行键名为空" << std::endl;
            continue;
        }
        
        // 调用回调函数处理键值对
        parseKeyValue(key, value);
    }
    
    file.close();
}

