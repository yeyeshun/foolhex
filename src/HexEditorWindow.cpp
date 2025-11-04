#include "HexEditorWindow.h"
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>

HexEditorWindow::HexEditorWindow(int w, int h, const char* title)
    : Fl_Double_Window(w, h, title) {
    // 创建十六进制表格
    m_hexTable = new HexTable(10, 10, w - 20, h - 80);
    
    // 启用表格单元格导航功能
    m_hexTable->enable_cell_nav(true);
    
    // 创建打开按钮
    m_openButton = new Fl_Button(w - 200, h - 60, 90, 30, "打开文件");
    m_openButton->callback(OpenButtonCallback, this);
        
    // 创建状态显示
    m_statusBuffer = new Fl_Text_Buffer();
    m_statusDisplay = new Fl_Text_Display(10, h - 60, w - 210, 30);
    m_statusDisplay->buffer(m_statusBuffer);
    
    // 设置状态显示使用支持中文的等宽字体
    m_statusDisplay->textfont(FL_COURIER);
    m_statusDisplay->textsize(12);
    
    // 将状态缓冲区关联到表格
    m_hexTable->SetStatusBuffer(m_statusBuffer);
    
    end();
}

HexEditorWindow::~HexEditorWindow() {
    delete m_statusBuffer;
}

// 打开文件按钮回调
void HexEditorWindow::OpenButtonCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    
    const char* fileName = fl_file_chooser("选择文件", "*", nullptr);
    if (fileName) {
        if (!window->m_hexTable->OpenFile(fileName)) {
            fl_alert("无法打开文件: %s", fileName);
        }
    }
}
