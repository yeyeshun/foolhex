#ifndef HEXTABLE_H
#define HEXTABLE_H

#include <FL/Fl_Table.H>
#include <FL/Fl_Text_Buffer.H>
#include <cstdint>
#include "LargeFile.h"

// 十六进制表格类
class HexTable : public Fl_Table {
private:
    CLargeFile m_largeFile;
    uint8_t* m_buffer;          // 数据缓冲区
    size_t m_bufferSize;        // 缓冲区大小
    size_t m_fileSize;          // 文件大小
    size_t m_bytesPerRow;       // 每行显示的字节数
    size_t m_startOffset;       // 当前显示的起始偏移量
    const size_t m_maxRows = 100; // 最大显示行数
    const size_t m_rowHeight = 20; // 行高
    char m_fileName[256];       // 当前打开的文件名
    Fl_Text_Buffer* m_statusBuffer; // 状态信息缓冲区
    
    // 选择相关变量
    bool m_isSelecting;           // 是否正在进行选择
    int m_rowStartSelect, m_colStartSelect; // 选择的起始位置
    int m_rowEndSelect, m_colEndSelect;     // 选择的结束位置
    bool m_isLow4BitEditing; // 是否正在选择高4位
    
    // 事件处理方法
    virtual int handle(int event) override; // 重写的事件处理函数
    // 设置并返回支持中文的等宽字体
    Fl_Font getFixedFont();

    // 格式化字节为十六进制字符串
    void formatByte(char* buf, uint8_t byte);

    // 获取可打印字符或替代字符
    char getPrintableChar(uint8_t byte);

public:
    HexTable(int x, int y, int w, int h);
    ~HexTable();

    // 打开文件
    bool OpenFile(const char* fileName);

    // 关闭文件
    void CloseFile();

    // 刷新视图
    void RefreshView();

    // 向上滚动
    void ScrollUp();

    // 向下滚动
    void ScrollDown();

    // 设置状态缓冲区
    void SetStatusBuffer(Fl_Text_Buffer* buffer);

    // 更新状态信息
    void UpdateStatus();

    // 表格绘制回调
    void draw_cell(TableContext context, int ROW, int COL, int X, int Y, int W, int H) override;
    
    // 启用表格单元格导航
    void enable_cell_nav(bool enable = true);
};

#endif // HEXTABLE_H