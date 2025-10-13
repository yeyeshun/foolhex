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
      m_fileSize(0), m_bytesPerRow(16), m_startOffset(0), m_statusBuffer(nullptr),
      m_isSelecting(false), m_rowStartSelect(-1), m_colStartSelect(-1),
      m_rowEndSelect(-1), m_colEndSelect(-1) {
    m_fileName[0] = '\0';
    
    // 计算可见行数（基于屏幕高度和行高）
    m_visibleRows = Fl::h() / m_rowHeight;
    if (m_visibleRows < 10) m_visibleRows = 10; // 最小行数保护
    
    // 初始化缓冲区 - 在构造函数中就创建，与是否打开文件无关
    // 设置更大的额外缓存行数用于平滑滚动
    m_extraRows = m_visibleRows * 2; // 设置为可见行数的2倍作为缓存
    m_bufferSize = (m_visibleRows + m_extraRows) * m_bytesPerRow;
    m_buffer = (uint8_t*)malloc(m_bufferSize);
    if (m_buffer) {
        memset(m_buffer, 0, m_bufferSize);
    }
    
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
    
    // 如果缓冲区不存在，则创建
    if (!m_buffer) {
        m_bufferSize = (m_visibleRows + m_extraRows) * m_bytesPerRow;
        m_buffer = (uint8_t*)malloc(m_bufferSize);
        if (!m_buffer) {
            CloseFile();
            return false;
        }
    }
    
    // 重置起始偏移量
    m_startOffset = 0;
    
    // 读取初始数据
    RefreshView();
    
    // 更新状态信息
    UpdateStatus();
    
    return true;
}

// 关闭文件
void HexTable::CloseFile() {
    m_largeFile.CloseFile();
    // 清空缓冲区但不释放（因为缓冲区在构造时创建，析构时释放）
    if (m_buffer && m_bufferSize > 0) {
        memset(m_buffer, 0, m_bufferSize);
    }
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
    printf("offset: 0x%zx, readSize: %zu\n", m_startOffset, readSize);
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
    
    // 更新表格行数 - 使用实际文件行数
    size_t rowCount = (m_fileSize + m_bytesPerRow - 1) / m_bytesPerRow;
    rows(rowCount);
    redraw();
}

// 向上滚动
void HexTable::ScrollUp() {
    if (m_startOffset >= m_bytesPerRow) {
        // 一次滚动半屏行数，而不是固定的10行
        size_t scrollAmount = (m_visibleRows / 2) * m_bytesPerRow;
        if (scrollAmount > m_startOffset) {
            scrollAmount = m_startOffset;
        }
        m_startOffset -= scrollAmount;
        RefreshView();
    }
}

// 向下滚动
void HexTable::ScrollDown() {
    if (m_startOffset + m_bufferSize < m_fileSize) {
        // 一次滚动半屏行数
        size_t scrollAmount = (m_visibleRows / 2) * m_bytesPerRow;
        if (m_startOffset + m_bufferSize + scrollAmount > m_fileSize) {
            m_startOffset = m_fileSize - m_bufferSize;
        } else {
            m_startOffset += scrollAmount;
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
            fl_push_clip(X, Y, W, H);
            
            // 检查单元格是否在选择区域内
            bool isSelected = false;
            if (m_rowStartSelect != -1 && m_rowEndSelect != -1 && 
                m_colStartSelect != -1 && m_colEndSelect != -1 && 
                !(COL == 0) && !(COL == m_bytesPerRow + 1)) {
                // 检查是否按下了Alt键
                bool isAltPressed = Fl::event_state(FL_ALT);

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
                        if (ROW == m_rowStartSelect && COL >= m_colStartSelect && COL <= m_colEndSelect) {
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
            return;
        }
        
        default:
            break;
    }
}

// 重写的事件处理函数
int HexTable::handle(int event) {
    int handled = 0;
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
                m_rowStartSelect = m_rowEndSelect = R;
                m_colStartSelect = m_colEndSelect = C;
                int X,Y,W,H;
                find_cell(CONTEXT_CELL, R,C, X,Y,W,H);
                // 检查是否点击在高4位
                m_isLow4BitEditing = ((Fl::event_x() - X) >= W / 2);

                // 触发重绘以显示选中状态
                redraw();
                handled = 1;
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
                    handled = 1;
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
                handled = 1;
            }
            break;
        }
        
        case FL_KEYBOARD: {
            // 键盘事件
            if (Fl::event_key() == FL_Escape) {
                // ESC键处理
            } else {
                // 获取当前选中的单元格
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
                                if (m_colStartSelect > m_bytesPerRow) {
                                    m_colStartSelect = 1;
                                    m_rowStartSelect++;
                                }
                                if (m_rowStartSelect * m_bytesPerRow + m_colStartSelect >= m_fileSize) {
                                    m_colStartSelect = (m_fileSize - 1) % m_bytesPerRow + 1;
                                    m_rowStartSelect = (m_fileSize - 1) / m_bytesPerRow;
                                }
                                m_rowEndSelect = m_rowStartSelect;
                                m_colEndSelect = m_colStartSelect;
                            } else {
                                // 更新高4位
                                m_buffer[bufferIndex] = (m_buffer[bufferIndex] & 0x0F) | (value << 4);
                                m_isLow4BitEditing = 1;
                            }
                            
                            // 触发重绘以更新显示
                            redraw();
                            
                            // 更新状态信息
                            UpdateStatus();
                        }
                        handled = 1;
                    } else if (key == '\r' || key == '\n') {
                        // Enter键处理
                    }
                }
            }
            break;
        }
    }
    if (!handled) {
        return Fl_Table::handle(event);
    }
    
    return 1;
}



// 启用表格单元格导航
void HexTable::enable_cell_nav(bool enable) {
    tab_cell_nav(enable);
}