#include "HexTable.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

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
      m_fileSize(0), m_bytesPerRow(16), m_visitOffset(0), m_statusBuffer(nullptr),
      m_isSelecting(false), m_isVertSelecting(false), m_rowStartSelect(-1),
      m_colStartSelect(-1), m_rowEndSelect(-1), m_colEndSelect(-1) {
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
    
    row_header(0);
    row_height_all(m_rowHeight);
    
    end();
}

HexTable::~HexTable() {
    if (m_buffer) {
        free(m_buffer);
    }
    CloseFile();
}

// 打开文件
bool HexTable::OpenFile(const char* fileName) {
    // 先关闭可能已经打开的文件
    CloseFile();
    
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

    // 重置起始偏移量
    m_visitOffset = 0;
    // 更新表格行数 - 使用实际文件行数
    size_t rowCount = (m_fileSize + m_bytesPerRow - 1) / m_bytesPerRow;
    rows(rowCount);

    m_largeFile.VisitFilePosition(0);
    uint32_t nFileOffsetLow, nFileOffsetHigh, dwAvalibleSize;
    m_buffer = (uint8_t*)m_largeFile.GetMappingInfo(nFileOffsetLow, nFileOffsetHigh, dwAvalibleSize);
    m_bufferSize = dwAvalibleSize;
    if (!m_buffer || !m_bufferSize)
        CloseFile();
    // 更新状态信息
    UpdateStatus();
    redraw();
    return true;
}

// 关闭文件
void HexTable::CloseFile() {
    // 清空缓冲区但不释放（因为缓冲区在构造时创建，析构时释放）
    m_buffer = 0;
    m_fileSize = 0;
    m_visitOffset = 0;
    m_fileName[0] = '\0';
    m_largeFile.CloseFile();
    UpdateStatus();
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
                m_fileName, m_fileSize, m_visitOffset);
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
            fl_push_clip(X, Y, W, H);
            
            // 检查单元格是否在选择区域内
            bool isSelected = false;
            if (m_rowStartSelect != -1 && m_rowEndSelect != -1 && 
                m_colStartSelect != -1 && m_colEndSelect != -1 && 
                !(COL == 0) && !(COL == m_bytesPerRow + 1)) {
                // 检查是否按下了Alt键
                bool isAltPressed = m_isVertSelecting;

                if (isAltPressed) {
                    int minRow = std::min(m_rowStartSelect, m_rowEndSelect);
                    int maxRow = std::max(m_rowStartSelect, m_rowEndSelect);
                    int minCol = std::min(m_colStartSelect, m_colEndSelect);
                    int maxCol = std::max(m_colStartSelect, m_colEndSelect);
                    // Alt键按下，使用块选择模式
                    if (ROW >= minRow && ROW <= maxRow && COL >= minCol && COL <= maxCol) {
                        isSelected = true;
                    }
                } else {
                    // 默认模式，使用多行文本选择模式
                    if ( m_rowStartSelect == m_rowEndSelect) {
                        int minCol = std::min(m_colStartSelect, m_colEndSelect);
                        int maxCol = std::max(m_colStartSelect, m_colEndSelect);
                        if (ROW == m_rowStartSelect && COL >= minCol && COL <= maxCol) {
                            isSelected = true;
                        }
                    } else if (m_rowStartSelect < m_rowEndSelect) {
                        if ((ROW == m_rowStartSelect && COL >= m_colStartSelect) ||
                            (ROW == m_rowEndSelect && COL <= m_colEndSelect) ||
                            (ROW > m_rowStartSelect && ROW < m_rowEndSelect)) {
                            isSelected = true;
                        }
                    } else {
                        if ((ROW == m_rowStartSelect && COL <= m_colStartSelect) ||
                            (ROW == m_rowEndSelect && COL >= m_colEndSelect) ||
                            (ROW < m_rowStartSelect && ROW > m_rowEndSelect)) {
                            isSelected = true;
                        }
                    }
                }
            }
            fl_color(FL_WHITE); // 白色背景
            fl_rectf(X, Y, W, H);
            
            // 设置背景色
            if (isSelected) {
                fl_color(FL_LIGHT1); // 浅蓝色背景
                if (m_isLow4BitEditing) {
                    fl_rectf(X + W / 2, Y, W / 2, H);
                } else {
                    fl_rectf(X, Y, W, H);
                }
            }
            
            // 确保使用支持中文的等宽字体
            fl_font(getFixedFont(), 12);
            
            // 计算实际偏移量
            size_t offset = ROW * m_bytesPerRow;
            
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
                    if (offset + i - m_visitOffset < m_fileSize &&
                        ROW * m_bytesPerRow + i - m_visitOffset < m_bufferSize) {
                        asciiStr[i] = getPrintableChar(m_buffer[ROW * m_bytesPerRow + i - m_visitOffset]);
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
                if (offset + byteIndex - m_visitOffset < m_fileSize &&
                        ROW * m_bytesPerRow + byteIndex - m_visitOffset < m_bufferSize) {
                    char hexStr[4];
                    formatByte(hexStr, m_buffer[ROW * m_bytesPerRow + byteIndex - m_visitOffset]);
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
            return;
        }
        
        default:
            break;
    }
}

// 重写的事件处理函数
int HexTable::handle(int event) {
    // 先处理我们关心的特定事件
    switch (event) {
        case FL_PUSH: {
            // 鼠标点击事件
            take_focus();
            int R, C;
            ResizeFlag resizeflag;
            // 获取鼠标点击位置对应的单元格行列
            TableContext context = cursor2rowcol(R, C, resizeflag);
            if (context == CONTEXT_CELL) {
                // 开始选择
                m_isSelecting = true;
                if (Fl::get_key(FL_Alt_L) || Fl::get_key(FL_Alt_R))
                    m_isVertSelecting = true;
                else
                    m_isVertSelecting = false;
                m_rowStartSelect = m_rowEndSelect = R;
                m_colStartSelect = m_colEndSelect = C;
                int X,Y,W,H;
                find_cell(CONTEXT_CELL, R,C, X,Y,W,H);
                // 检查是否点击在高4位
                m_isLow4BitEditing = ((Fl::event_x() - X) >= W / 2);

                // 触发重绘以显示选中状态
                redraw();
            }
            break;
        }
        
        case FL_DRAG: {
            // 鼠标拖动事件
            int R, C;
            ResizeFlag resizeflag;
            // 获取鼠标拖动位置对应的单元格行列
            TableContext context = cursor2rowcol(R, C, resizeflag);
            if (context == CONTEXT_CELL) {
                if (m_isSelecting) {
                    // 更新选择的结束位置
                    m_rowEndSelect = R;
                    m_colEndSelect = C;
                    // 触发重绘以更新选择区域
                    if (m_rowStartSelect != m_rowEndSelect || m_colStartSelect != m_colEndSelect) {
                        m_isLow4BitEditing = 0;
                    }
                    redraw();
                }
            }
            break;
        }
        
        case FL_RELEASE: {
            // 鼠标释放事件
            if (m_isSelecting) {
                // 结束选择
                m_isSelecting = false;
                // 触发重绘以清除选中状态
                redraw();
            }
            break;
        }
        
        case FL_KEYBOARD: {
            // 键盘事件
            switch (Fl::event_key())
            {
                case FL_Escape:
                    return 0;

                case FL_Page_Up:
                case FL_Page_Down:
                case FL_Home:
                case FL_End:
                    event = FL_SHORTCUT;
                    break;

                case FL_Up:
                case FL_Down:
                case FL_Left:
                case FL_Right:
                    {
                        Fl_Table::handle(event);
                        get_selection(m_rowStartSelect, m_colStartSelect, m_rowEndSelect, m_colEndSelect);
                        return 1; // return 0 cause future arrow key fail
                    }
                    break;

                default:
                {
                // 获取当前选中的单元格
                    if (m_rowStartSelect != m_rowEndSelect || m_colStartSelect != m_colEndSelect)
                    {
                        if (m_colStartSelect == 0) m_colStartSelect = 1;
                        if (m_colEndSelect == 0) m_colEndSelect = 1;
                    }
                    int R = m_rowStartSelect;
                    int C = m_colStartSelect;

                    // 只有十六进制数据列可以编辑
                    if (C >= 1 && C <= m_bytesPerRow) {
                        // 检查是否输入了有效的十六进制字符
                        char key = Fl::e_text[0];
                        if ((key >= '0' && key <= '9') || 
                            (key >= 'a' && key <= 'f') || 
                            (key >= 'A' && key <= 'F')) {
                            // 计算缓冲区中的索引
                            size_t bufferIndex = R * m_bytesPerRow + (C - 1);
                            
                            // 确保索引在缓冲区范围内
                            if (bufferIndex < m_bufferSize) {
                                // 将字符转换为数值
                                int value = 0;
                                if (key >= '0' && key <= '9') {
                                    value = key - '0';
                                } else if (key >= 'a' && key <= 'f') {
                                    value = key - 'a' + 10;
                                } else if (key >= 'A' && key <= 'F') {
                                    value = key - 'A' + 10;
                                }

                                // 根据m_isLow4BitEditing标志决定更新高4位还是低4位
                                if (m_isLow4BitEditing) {
                                    // 更新低4位
                                    m_buffer[bufferIndex] = (m_buffer[bufferIndex] & 0xF0) | value;
                                    m_isLow4BitEditing = 0;
                                    m_colStartSelect++;
                                    // 确保不超出当前行的边界
                                    int oldRow = m_rowStartSelect;
                                    if (m_colStartSelect > m_bytesPerRow) {
                                        m_colStartSelect = 1;
                                        m_rowStartSelect++;
                                    }
                                    if (m_rowStartSelect * m_bytesPerRow + m_colStartSelect >= m_fileSize) {
                                        m_colStartSelect = (m_fileSize - 1) % m_bytesPerRow + 1;
                                        m_rowStartSelect = (m_fileSize - 1) / m_bytesPerRow;
                                    } else if (oldRow != m_rowStartSelect) {
                                        int r1, r2, c1, c2;
                                        visible_cells(r1, r2, c1, c2);
                                        if (m_rowStartSelect > r2)
                                            row_position(r1 + 1);
                                    }
                                } else {
                                    // 更新高4位
                                    m_buffer[bufferIndex] = (m_buffer[bufferIndex] & 0x0F) | (value << 4);
                                    m_isLow4BitEditing = 1;
                                }
                                m_rowEndSelect = m_rowStartSelect;
                                m_colEndSelect = m_colStartSelect;
                                
                                // 触发重绘以更新显示
                                redraw();
                                
                                // 更新状态信息
                                UpdateStatus();
                            }
                        } else if (key == '\r' || key == '\n') {
                            // Enter键处理
                        }
                    }
                }
                break;
            }
        }
    }
    int result = Fl_Table::handle(event);
    int r1, r2, c1, c2;
    visible_cells(r1, r2, c1, c2);
    if (r1 >= 0 && r2 >= 0 &&
        (r1 * m_bytesPerRow < m_visitOffset || r2 * m_bytesPerRow >= m_visitOffset + m_bufferSize))
    {
        uint32_t visitOffset = (r1 + (r2 - r1) / 2) * m_bytesPerRow;
        m_largeFile.VisitFilePosition(visitOffset);
        uint32_t nFileOffsetLow, nFileOffsetHigh, dwAvalibleSize;
        m_buffer = (uint8_t*)m_largeFile.GetMappingInfo(nFileOffsetLow, nFileOffsetHigh, dwAvalibleSize);
        m_visitOffset = nFileOffsetLow;
        m_bufferSize = dwAvalibleSize;
        if (!m_buffer || !m_bufferSize)
            CloseFile();
        redraw();
    }
    
    return result;
}



// 启用表格单元格导航
void HexTable::enable_cell_nav(bool enable) {
    tab_cell_nav(enable);
}