#ifndef HEXTABLE_H
#define HEXTABLE_H

#include <FL/Fl_Table.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
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
    
    // 编辑功能相关变量
    Fl_Input* m_input;           // 用于单元格编辑的输入部件
    int m_rowEdit, m_colEdit;    // 当前编辑的行和列
    
    // 选中单元格相关变量
    int m_rowSelect, m_colSelect; // 当前选中的行和列
    
    // 事件回调处理
    void event_callback2();      // 表格的事件回调（实例方法）
    static void event_callback(Fl_Widget*, void *v); // 表格的事件回调（静态方法）
    static void input_cb(Fl_Widget*, void* v); // 输入部件的回调方法
    
    // 编辑操作方法
    void start_editing(int R, int C); // 开始编辑单元格
    void done_editing();             // 完成编辑
    void set_value_hide();           // 应用编辑值并隐藏输入框
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