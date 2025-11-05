#include "LargeFile.h"
#include <cstring>

// 平台特定的头文件和实现
#ifdef _WIN32
#include <windows.h>

// Windows平台的GetLastError实现
ErrorCode GetLastError()
{
    return ::GetLastError();
}

// Windows平台的GetSystemInfo替代函数
void GetSystemPageSize(uint32_t& pageSize)
{
    SYSTEM_INFO si;
    ::GetSystemInfo(&si);
    pageSize = si.dwAllocationGranularity;
}

// Windows平台的文件大小获取函数
bool GetFileSize64(int fileHandle, LargeInteger& fileSize)
{
    return ::GetFileSizeEx((HANDLE)fileHandle, (LARGE_INTEGER*)&fileSize) != FALSE;
}

#else
// Linux/Unix平台的头文件
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

// Linux平台的GetLastError实现
ErrorCode GetLastError()
{
    return errno;
}

// Linux平台的GetSystemInfo替代函数
void GetSystemPageSize(uint32_t& pageSize)
{
    pageSize = sysconf(_SC_PAGE_SIZE);
}

// Linux平台的文件大小获取函数
bool GetFileSize64(int fileHandle, LargeInteger& fileSize)
{
    struct stat st;
    if (fstat(fileHandle, &st) < 0)
    {
        return false;
    }
    fileSize.QuadPart = st.st_size;
    return true;
}

#endif


CLargeFile::CLargeFile()
{
	uint32_t pageSize;
	GetSystemPageSize(pageSize);
	m_dwPageSize = pageSize;
	m_dwPageCount = 3;
	init();
}


CLargeFile::~CLargeFile()
{
    CloseFile();
}

int CLargeFile::OpenFile(const char* pFilePathName, uint32_t nPageCount /*= 3*/)
{
	CloseFile();

#ifdef _WIN32
	m_hFile = (int)CreateFileA(pFilePathName,
		GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	if (!GetFileSize64(m_hFile, m_nFileSize))
	{
		CloseHandle((HANDLE)m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	m_hMap = (int)CreateFileMappingA((HANDLE)m_hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);
	if (!m_hMap)
	{
		CloseHandle((HANDLE)m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		return FALSE;
	}
#else
	// Linux平台的文件打开和映射
	m_hFile = open(pFilePathName, O_RDWR);
	if (m_hFile < 0)
	{
		return FALSE;
	}
	if (!GetFileSize64(m_hFile, m_nFileSize))
	{
		close(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		errno = EINVAL;
		return FALSE;
	}
	// 在Linux中，我们不需要单独的文件映射对象，直接使用文件描述符
	m_hMap = m_hFile; // 简单地复用文件描述符
#endif
	
	if (!m_nFileSize.QuadPart)
	{
#ifdef _WIN32
		CloseHandle((HANDLE)m_hFile);
		CloseHandle((HANDLE)m_hMap);
#else
		close(m_hFile);
#endif
		m_hFile = INVALID_HANDLE_VALUE;
		m_hMap = 0;
		return FALSE;
	}
	strcpy(m_szFilePathName, pFilePathName);
	m_pView = 0;
	m_nViewStart.QuadPart = 0;
	m_dwPageCount = nPageCount;
	return TRUE;
}

int CLargeFile::IsOpenFile()
{
	return (m_hFile != INVALID_HANDLE_VALUE) && (m_hMap != 0);
}

const char* CLargeFile::GetFilePathName()
{
	return m_szFilePathName;
}

void CLargeFile::CloseFile()
{
	if (m_pView)
	{
        OnUnmapViewOfFile();
	}
	if (m_hMap)
	{
#ifdef _WIN32
		CloseHandle((HANDLE)m_hMap);
		// Linux平台不需要额外关闭m_hMap，因为它与m_hFile相同
#endif
	}
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
#ifdef _WIN32
		CloseHandle((HANDLE)m_hFile);
#else
		close(m_hFile);
#endif
	}
	init();
}

uint32_t CLargeFile::GetFileSizeLow()
{
	return m_nFileSize.LowPart;
}

uint32_t CLargeFile::GetFileSizeHigh()
{
	return m_nFileSize.HighPart;
}

void CLargeFile::GetFileSizeEx(LargeInteger* puFileSize)
{
	if (puFileSize)
		puFileSize->QuadPart = m_nFileSize.QuadPart;
}

void* CLargeFile::VisitFilePosition(LargeInteger nVisit, uint32_t* pdwAvalibleSize /*= 0*/)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}
	if (nVisit.QuadPart >= m_nFileSize.QuadPart)
	{
		return NULL;
	}
	uint32_t dwAvalibleSize = 0;
	uint32_t dwVisitOffset = 0;

	if (m_pView)
	{
		if (nVisit.QuadPart >= m_nViewStart.QuadPart + m_dwPageSize &&
			nVisit.QuadPart <= m_nViewStart.QuadPart + m_dwPageSize * 2)
		{
            dwVisitOffset = (uint32_t)(nVisit.QuadPart - m_nViewStart.QuadPart);
			dwAvalibleSize = m_dwPageSize * 3 - dwVisitOffset;
		}

		if (nVisit.QuadPart >= m_nViewStart.QuadPart && // m_nViewStart.QuadPart means 0
			nVisit.QuadPart < m_dwPageSize)
		{
            dwVisitOffset = (uint32_t)nVisit.QuadPart;
			dwAvalibleSize = m_dwPageSize * 3 - dwVisitOffset;
		}

		if (dwAvalibleSize)
		{
			if (dwAvalibleSize > m_nFileSize.QuadPart - nVisit.QuadPart)
			{
                dwAvalibleSize = (uint32_t)(m_nFileSize.QuadPart - nVisit.QuadPart);
			}
			if (pdwAvalibleSize)
			{
				*pdwAvalibleSize = dwAvalibleSize;
			}
			return m_pView + dwVisitOffset;
		}
	}

	if (m_pView)
	{
		OnUnmapViewOfFile();
	}

	if (nVisit.QuadPart < m_dwPageSize)
	{
		m_nViewStart.QuadPart = 0;
	}
	else
	{
		m_nViewStart.QuadPart = ALIGN_DOWN_BY(nVisit.QuadPart, m_dwPageSize) - m_dwPageSize;
	}

	m_dwMapSize = m_dwPageSize * m_dwPageCount;
	if (m_nViewStart.QuadPart + m_dwMapSize > m_nFileSize.QuadPart)
	{
        m_dwMapSize = (uint32_t)(m_nFileSize.QuadPart - m_nViewStart.QuadPart);
	}

	m_pView = OnMapViewOfFile(m_nViewStart, m_dwMapSize);
	if (!m_pView)
	{
		return NULL;
	}
    dwVisitOffset = (uint32_t)(nVisit.QuadPart - m_nViewStart.QuadPart);
	dwAvalibleSize = m_dwMapSize - dwVisitOffset;
	if (pdwAvalibleSize)
	{
		*pdwAvalibleSize = dwAvalibleSize;
	}

	return m_pView + dwVisitOffset;
}

void* CLargeFile::VisitFilePosition(uint32_t nVisitLow, uint32_t nVisitHigh /*= 0*/, uint32_t* pdwAvalibleSize /*= 0*/)
{
	LargeInteger nVisit;
	nVisit.LowPart = nVisitLow;
	nVisit.HighPart = nVisitHigh;
	return VisitFilePosition(nVisit, pdwAvalibleSize);
}

void* CLargeFile::GetMappingInfo(uint32_t& nFileOffsetLow, uint32_t& nFileOffsetHigh, uint32_t& dwAvalibleSize)
{
	LargeInteger nFileOffset;
	void* ptr = GetMappingInfo(nFileOffset, dwAvalibleSize);
	nFileOffsetLow = nFileOffset.LowPart;
	nFileOffsetHigh = nFileOffset.HighPart;
	return ptr;
}

void* CLargeFile::GetMappingInfo(LargeInteger& nFileOffset, uint32_t& dwAvalibleSize)
{
	nFileOffset = m_nViewStart;
	dwAvalibleSize = m_dwMapSize;
	return m_pView;
}

void CLargeFile::OnUnmapViewOfFile()
{
#ifdef _WIN32
	UnmapViewOfFile(m_pView);
#else
	munmap(m_pView, m_dwMapSize);
#endif
	m_pView = NULL;
}

uint8_t* CLargeFile::OnMapViewOfFile(LargeInteger nViewStart, uint32_t dwMapSize)
{
#ifdef _WIN32
	return (uint8_t*)MapViewOfFile((HANDLE)m_hMap, FILE_MAP_COPY, nViewStart.HighPart,
		nViewStart.LowPart, dwMapSize);
#else
	// Linux平台的内存映射
	uint64_t offset = nViewStart.QuadPart;
	// 确保offset是页对齐的
	size_t page_size = m_dwPageSize;
	uint64_t aligned_offset = offset & ~(page_size - 1);
	size_t offset_in_page = offset - aligned_offset;

	// 使用PROT_READ|PROT_WRITE和MAP_PRIVATE实现写时复制功能，与Windows的FILE_MAP_COPY对应
	void* addr = mmap(NULL, dwMapSize + offset_in_page, PROT_READ | PROT_WRITE, MAP_PRIVATE, m_hMap, aligned_offset);
	if (addr == MAP_FAILED)
	{
		return NULL;
	}
	// 返回调整后的指针，考虑页内偏移
	return (uint8_t*)addr + offset_in_page;
#endif
}

void CLargeFile::init()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_hMap = 0;
	m_pView = NULL;

	m_nFileSize.QuadPart = 0;
	m_nViewStart.QuadPart = 0;
	m_szFilePathName[0] = 0;
}
