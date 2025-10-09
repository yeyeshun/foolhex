#pragma once
#include <stdint.h>
#include <limits.h>
#include <stddef.h>

// 跨平台类型定义
typedef uint32_t ErrorCode;

// 跨平台常量定义
#define MAX_PATH PATH_MAX
#define INVALID_HANDLE_VALUE (-1)
#define TRUE 1
#define FALSE 0

// 跨平台的对齐宏
#define ALIGN_DOWN_BY(length, alignment) \
    ((uintptr_t)(length) & ~(alignment - 1))

#define ALIGN_UP_BY(length, alignment) \
    (ALIGN_DOWN_BY(((uintptr_t)(length) + alignment - 1), alignment))

#define ALIGN_DOWN_POINTER_BY(address, alignment) \
    ((void*)((uintptr_t)(address) & ~((uintptr_t)alignment - 1)))

#define ALIGN_UP_POINTER_BY(address, alignment) \
    (ALIGN_DOWN_POINTER_BY(((uintptr_t)(address) + alignment - 1), alignment))

#define ALIGN_DOWN(length, type) \
    ALIGN_DOWN_BY(length, sizeof(type))

#define ALIGN_UP(length, type) \
    ALIGN_UP_BY(length, sizeof(type))

#define ALIGN_DOWN_POINTER(address, type) \
    ALIGN_DOWN_POINTER_BY(address, sizeof(type))

#define ALIGN_UP_POINTER(address, type) \
    ALIGN_UP_POINTER_BY(address, sizeof(type))

// 跨平台的64位整数结构
typedef struct {
    union {
        struct {
            uint32_t LowPart;
            uint32_t HighPart;
        };
        uint64_t QuadPart;
    };
} LargeInteger;

class CLargeFile
{
public:
	CLargeFile();
	~CLargeFile();

	/************************************************************************/
	/* open file, return 1 if success; if 0, use GetLastError to get error code
	/* error may occur at file opening or memory mapping
	/************************************************************************/
	int OpenFile(const char* pFilePathName, uint32_t nPageCount = 3);

	/************************************************************************/
	/* check if i have opened a file.
	/************************************************************************/
	int IsOpenFile();

	/************************************************************************/
	/* get file path name.
	/************************************************************************/
	const char* GetFilePathName();

	/************************************************************************/
	/* just close file, NOTHING saved.
	/* if you want to save, call SaveFile() before CloseFile().
	/************************************************************************/
	void CloseFile();

	/************************************************************************/
	/* get file size.
	/************************************************************************/
	uint32_t GetFileSizeLow();
	uint32_t GetFileSizeHigh();
	void GetFileSizeEx(LargeInteger* puFileSize);

	/************************************************************************/
	/* visit file position. return a pointer point to the data at the position.
	/* dwAvalibleSize received the avalible size of the data that you can use, 
	/* if out of the size, you should call VisitFilePosition() 
	/* with the new position to get a new pointer and new avalible size.
	/************************************************************************/
	void* VisitFilePosition(uint32_t nVisitLow, uint32_t nVisitHigh = 0, uint32_t* pdwAvalibleSize = 0);
	void* VisitFilePosition(LargeInteger nVisit, uint32_t* pdwAvalibleSize = 0);

protected:
	virtual void OnUnmapViewOfFile();
	virtual uint8_t* OnMapViewOfFile(LargeInteger nViewStart, uint32_t dwMapSize);
	LargeInteger m_nViewStart;
	uint32_t m_dwMapSize;
	uint32_t m_dwPageSize;
	uint32_t m_dwPageCount;
	uint8_t* m_pView;
private:
	void init();
	char m_szFilePathName[MAX_PATH];
	int m_hFile;
	int m_hMap;

	LargeInteger m_nFileSize;
};
