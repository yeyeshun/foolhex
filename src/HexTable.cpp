#include "HexTable.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

// 设置并返回支持中文的等宽字体
Fl_Font HexTable::getFixedFont() {
    // 在FLTK中，通常使用预定义的等宽字体，并确保系统有支持中文的字体
    // FL_COURIER是标准等宽字体
    return FL_COURIER;
}

// 格式化字节为十六进制字符串
void HexTable::formatByte(char* buf, uint8_t byte) {
    sprintf(buf, "%02X", byte);
}

// 获取可打印字符或替代字符
char HexTable::getPrintableChar(uint8_t byte) {
    return (byte >= 32 && byte <= 126) ? byte : '.';
}

HexTable::HexTable(int x, int y, int w, int h)
    : Fl_Table(x, y, w, h), m_buffer(nullptr), m_bufferSize(0), 
      m_fileSize(0), m_bytesPerRow(16), m_startOffset(0), m_statusBuffer(nullptr),
      m_input(nullptr), m_rowEdit(-1), m_colEdit(-1) {
    m_fileName[0] = '\0';
    
    // 设置支持中文的等宽字体
    fl_font(getFixedFont(), 12);
    
    // 配置表格
    cols(1 + m_bytesPerRow + 1); // 偏移列 + 十六进制数据列 + ASCII列
    col_header(1);
    col_width(0, 80); // 偏移列宽度
    
    // 设置十六进制数据列宽度
    for (int i = 1; i <= m_bytesPerRow; i++) {
        col_width(i, 40);
    }
    
    // 设置ASCII列宽度
    col_width(m_bytesPerRow + 1, 180);
    
    rows(m_maxRows);
    row_header(0);
    row_height_all(m_rowHeight);
    
    // 设置回调函数
    callback(&event_callback, (void*)this);
    when(FL_WHEN_NOT_CHANGED | when());
    
    // 创建输入部件用于单元格编辑
    m_input = new Fl_Input(w/2, h/2, 0, 0);
    m_input->hide(); // 初始隐藏
    m_input->callback(input_cb, (void*)this);
    m_input->when(FL_WHEN_ENTER_KEY_ALWAYS); // 按下Enter键时触发回调
    m_input->maximum_size(2); // 十六进制值最多两位
    m_input->color(FL_YELLOW); // 编辑时显示黄色背景以突出显示
    
    end();
}

HexTable::~HexTable() {
    if (m_buffer) {
        free(m_buffer);
    }
    if (m_input) {
        delete m_input;
    }
    CloseFile();
}

// 打开文件
bool HexTable::OpenFile(const char* fileName) {
    if (m_largeFile.OpenFile(fileName) != 1) {
        return false;
    }
    
    // 存储文件名
    strncpy(m_fileName, fileName, sizeof(m_fileName) - 1);
    m_fileName[sizeof(m_fileName) - 1] = '\0';
    
    // 获取文件大小
    LargeInteger fileSize;
    m_largeFile.GetFileSizeEx(&fileSize);
    m_fileSize = fileSize.QuadPart;
    
    // 分配缓冲区
    m_bufferSize = m_maxRows * m_bytesPerRow;
    m_buffer = (uint8_t*)malloc(m_bufferSize);
    if (!m_buffer) {
        CloseFile();
        return false;
    }
    
    // 读取初始数据
    RefreshView();
    
    // 更新状态信息
    UpdateStatus();
    
    return true;
}

// 关闭文件
void HexTable::CloseFile() {
    m_largeFile.CloseFile();
    if (m_buffer) {
        free(m_buffer);
        m_buffer = nullptr;
    }
    m_bufferSize = 0;
    m_fileSize = 0;
    m_startOffset = 0;
    m_fileName[0] = '\0';
    UpdateStatus();
}

// 刷新视图
void HexTable::RefreshView() {
    if (!m_buffer || m_fileSize == 0) {
        return;
    }
    
    // 计算要读取的字节数
    size_t readSize = m_bufferSize;
    if (m_startOffset + readSize > m_fileSize) {
        readSize = m_fileSize - m_startOffset;
    }
    
    // 读取数据
    for (size_t i = 0; i < readSize; i++) {
        uint32_t avalibleSize = 0;
        void* dataPtr = m_largeFile.VisitFilePosition((uint32_t)(m_startOffset + i), 0, &avalibleSize);
        if (dataPtr && avalibleSize >= 1) {
            m_buffer[i] = *(uint8_t*)dataPtr;
        } else {
            m_buffer[i] = 0;
        }
    }
    
    // 清除未使用的缓冲区部分
    if (readSize < m_bufferSize) {
        memset(m_buffer + readSize, 0, m_bufferSize - readSize);
    }
    
    // 更新表格行数
    size_t rowCount = (readSize + m_bytesPerRow - 1) / m_bytesPerRow;
    rows(rowCount > m_maxRows ? m_maxRows : rowCount);
    redraw();
}

// 向上滚动
void HexTable::ScrollUp() {
    if (m_startOffset >= m_bytesPerRow * 10) {
        m_startOffset -= m_bytesPerRow * 10;
        RefreshView();
    }
}

// 向下滚动
void HexTable::ScrollDown() {
    if (m_startOffset + m_bufferSize < m_fileSize) {
        m_startOffset += m_bytesPerRow * 10;
        if (m_startOffset + m_bufferSize > m_fileSize) {
            m_startOffset = m_fileSize - m_bufferSize;
        }
        RefreshView();
    }
}

// 设置状态缓冲区
void HexTable::SetStatusBuffer(Fl_Text_Buffer* buffer) {
    m_statusBuffer = buffer;
}

// 更新状态信息
void HexTable::UpdateStatus() {
    if (!m_statusBuffer) return;
    
    char status[512];
    if (m_fileName[0] != '\0') {
        sprintf(status, "文件: %s | 大小: %zu 字节 | 当前偏移: 0x%zx", 
                m_fileName, m_fileSize, m_startOffset);
    } else {
        strcpy(status, "未打开文件");
    }
    m_statusBuffer->text(status);
}

// 表格绘制回调
void HexTable::draw_cell(TableContext context, int ROW, int COL, int X, int Y, int W, int H) {
    if (!m_buffer || m_fileSize == 0) {
        return;
    }
    
    switch (context) {
        case CONTEXT_COL_HEADER: {
            fl_push_clip(X, Y, W, H);
            fl_draw_box(FL_THIN_UP_BOX, X, Y, W, H, col_header_color());
            fl_color(FL_BLACK);
            
            // 确保使用支持中文的等宽字体
            fl_font(getFixedFont(), 12);
            
            if (COL == 0) {
                fl_draw("偏移量", X+2, Y, W, H, FL_ALIGN_LEFT, nullptr, 0);
            } else if (COL == m_bytesPerRow + 1) {
                fl_draw("ASCII", X+2, Y, W, H, FL_ALIGN_LEFT, nullptr, 0);
            } else {
                char label[4];
                sprintf(label, "%02X", COL-1);
                fl_draw(label, X, Y, W, H, FL_ALIGN_CENTER, nullptr, 0);
            }
            fl_pop_clip();
            break;
        }
        
        case CONTEXT_CELL: {
            // 如果是正在编辑的单元格，且输入框可见，则不绘制单元格内容
            if (ROW == m_rowEdit && COL == m_colEdit && m_input->visible()) {
                return;
            }
            
            fl_push_clip(X, Y, W, H);
            fl_color(FL_WHITE);
            fl_rectf(X, Y, W, H);
            
            // 确保使用支持中文的等宽字体
            fl_font(getFixedFont(), 12);
            
            // 计算实际偏移量
            size_t offset = m_startOffset + ROW * m_bytesPerRow;
            
            if (COL == 0) {
                // 偏移列
                fl_color(FL_BLUE);
                char offsetStr[20];
                sprintf(offsetStr, "%08x", offset);
                fl_draw(offsetStr, X+2, Y, W, H, FL_ALIGN_LEFT, nullptr, 0);
            } else if (COL == m_bytesPerRow + 1) {
                // ASCII列
                fl_color(FL_BLACK);
                char asciiStr[m_bytesPerRow + 1];
                for (int i = 0; i < m_bytesPerRow; i++) {
                    if (offset + i < m_fileSize) {
                        asciiStr[i] = getPrintableChar(m_buffer[ROW * m_bytesPerRow + i]);
                    } else {
                        asciiStr[i] = ' ';
                    }
                }
                asciiStr[m_bytesPerRow] = '\0';
                fl_draw(asciiStr, X+2, Y, W, H, FL_ALIGN_LEFT, nullptr, 0);
            } else {
                // 十六进制数据列
                fl_color(FL_BLACK);
                int byteIndex = COL - 1;
                if (offset + byteIndex < m_fileSize) {
                    char hexStr[4];
                    formatByte(hexStr, m_buffer[ROW * m_bytesPerRow + byteIndex]);
                    fl_draw(hexStr, X, Y, W, H, FL_ALIGN_CENTER, nullptr, 0);
                }
            }
            
            // 绘制单元格边框
            fl_color(color());
            fl_rect(X, Y, W, H);
            fl_pop_clip();
            break;
        }
        
        case CONTEXT_RC_RESIZE: {
            // 当表格大小调整时，如果有正在编辑的单元格，调整输入框的大小
            if (m_input->visible()) {
                int X, Y, W, H;
                find_cell(CONTEXT_TABLE, m_rowEdit, m_colEdit, X, Y, W, H);
                m_input->resize(X, Y, W, H);
                init_sizes();
            }
            return;
        }
        
        default:
            break;
    }
}

// 表格的事件回调（静态方法）
void HexTable::event_callback(Fl_Widget*, void *v) {
    ((HexTable*)v)->event_callback2();
}

// 表格的事件回调（实例方法）
void HexTable::event_callback2() {
    int R = callback_row();
    int C = callback_col();
    TableContext context = callback_context();

    switch (context) {
        case CONTEXT_CELL: {
            // 单元格上的事件
            switch (Fl::event()) {
                case FL_PUSH: {
                    // 鼠标点击事件
                    done_editing(); // 完成之前的编辑
                    // 只有十六进制数据列可以编辑
                    if (C >= 1 && C <= m_bytesPerRow) {
                        start_editing(R, C);
                    }
                    return;
                }
                
                case FL_KEYBOARD: {
                    // 键盘事件
                    if (Fl::event_key() == FL_Escape) {
                        done_editing(); // ESC键取消编辑
                        return;
                    }
                    
                    done_editing(); // 完成之前的编辑
                    
                    // 只有十六进制数据列可以编辑
                    if (C >= 1 && C <= m_bytesPerRow) {
                        // 检查是否输入了有效的十六进制字符
                        char key = Fl::e_text[0];
                        if ((key >= '0' && key <= '9') || 
                            (key >= 'a' && key <= 'f') || 
                            (key >= 'A' && key <= 'F') || 
                            key == ' ' || key == '.') {
                            start_editing(R, C);
                            m_input->handle(Fl::event()); // 将输入的字符传递给输入框
                        } else if (key == '\r' || key == '\n') {
                            // Enter键开始编辑
                            start_editing(R, C);
                        }
                    }
                    return;
                }
            }
            return;
        }
        
        case CONTEXT_TABLE: 
        case CONTEXT_ROW_HEADER: 
        case CONTEXT_COL_HEADER: {
            // 表格其他区域的事件
            done_editing(); // 完成编辑并隐藏输入框
            return;
        }
        
        default:
            return;
    }
}

// 输入部件的回调方法
void HexTable::input_cb(Fl_Widget*, void* v) {
    ((HexTable*)v)->set_value_hide();
}

// 开始编辑单元格
void HexTable::start_editing(int R, int C) {
    if (!m_buffer || m_fileSize == 0) return;
    
    m_rowEdit = R;
    m_colEdit = C;
    set_selection(R, C, R, C); // 清除之前的多选
    
    int X, Y, W, H;
    find_cell(CONTEXT_CELL, R, C, X, Y, W, H); // 找到单元格的位置和大小
    
    m_input->resize(X, Y, W, H); // 移动输入框到单元格位置
    
    // 加载单元格当前值到输入框
    size_t offset = m_startOffset + R * m_bytesPerRow;
    int byteIndex = C - 1;
    if (offset + byteIndex < m_fileSize) {
        char hexStr[4];
        formatByte(hexStr, m_buffer[R * m_bytesPerRow + byteIndex]);
        m_input->value(hexStr);
    } else {
        m_input->value("00");
    }
    
    m_input->insert_position(0, 2); // 选中整个输入字段
    m_input->show(); // 显示输入框
    m_input->take_focus(); // 将焦点设置到输入框
}

// 完成编辑
void HexTable::done_editing() {
    if (m_input->visible()) {
        set_value_hide(); // 应用值并隐藏输入框
        m_rowEdit = -1;
        m_colEdit = -1;
    }
}

// 应用编辑值并隐藏输入框
void HexTable::set_value_hide() {
    if (!m_buffer || m_fileSize == 0) return;
    
    // 检查输入是否有效
    const char* inputValue = m_input->value();
    if (strlen(inputValue) >= 2) {
        // 解析十六进制值
        char hex[3] = {inputValue[0], inputValue[1], '\0'};
        uint8_t newValue = strtol(hex, nullptr, 16);
        
        // 计算缓冲区中的位置
        size_t bufferPos = m_rowEdit * m_bytesPerRow + (m_colEdit - 1);
        size_t filePos = m_startOffset + bufferPos;
        
        // 更新缓冲区
        if (bufferPos < m_bufferSize) {
            m_buffer[bufferPos] = newValue;
        }
        
        // 更新文件 - 直接打开文件进行修改
        if (filePos < m_fileSize) {
            // 以读写方式打开文件
            int fileHandle = open(m_fileName, O_WRONLY);
            if (fileHandle != -1) {
                // 定位到要修改的位置
                lseek(fileHandle, filePos, SEEK_SET);
                // 写入新值
                write(fileHandle, &newValue, 1);
                // 关闭文件
                close(fileHandle);
            }
        }
        
        // 刷新视图以显示更新后的值
        RefreshView();
        // 更新状态信息
        UpdateStatus();
    }
    
    m_input->hide();
    window()->cursor(FL_CURSOR_DEFAULT); // 确保光标不会消失
}

// 启用表格单元格导航
void HexTable::enable_cell_nav(bool enable) {
    tab_cell_nav(enable);
}