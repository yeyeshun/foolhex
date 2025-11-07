#include <string>
#include <functional>


std::string ws2s(const std::wstring& ws);
std::wstring s2ws(const std::string& s);
void DefineFakeType(const std::string& strFakeNameA, const std::string& strRealTypeNameA);
void parseSimpleConfig(const std::string& filename, std::function<void(const std::string&, const std::string&)> parseKeyValue);