#include "HexEditorWindow.h"
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <cstdlib>  // 添加exit函数
#include "BindingType.h"
#include "FakeType.h"

// 菜单项定义
Fl_Menu_Item HexEditorWindow::menuItems[] = {
    {"&文件", 0, 0, 0, FL_SUBMENU},
        {"&打开文件", FL_COMMAND + 'o', (Fl_Callback*)FileOpenCallback, 0},
        {"&保存文件", FL_COMMAND + 's', (Fl_Callback*)FileSaveCallback, 0},
        {"保存为...", FL_COMMAND + FL_SHIFT + 's', (Fl_Callback*)FileSaveCallback, 0, FL_MENU_DIVIDER},
        {"退&出", FL_COMMAND + 'q', (Fl_Callback*)FileExitCallback, 0},
        {0},
    {"&编辑", 0, 0, 0, FL_SUBMENU},
        {"&复制", FL_COMMAND + 'c', (Fl_Callback*)EditCopyCallback, 0},
        {"&粘贴", FL_COMMAND + 'v', (Fl_Callback*)EditPasteCallback, 0, FL_MENU_DIVIDER},
        {"&查找", FL_COMMAND + 'f', (Fl_Callback*)EditFindCallback, 0},
        {0},
    {"&数据管理", 0, 0, 0, FL_SUBMENU},
        {"基础类型管理", FL_COMMAND + '+', (Fl_Callback*)ManageBasicTypeCallback, 0},
        {"结构体类型管理", FL_COMMAND + '-', (Fl_Callback*)ManageStructTypeCallback, 0},
        {"变量管理", FL_COMMAND + '0', (Fl_Callback*)ManageVarCallback, 0},
        {0},
    {"&帮助", 0, 0, 0, FL_SUBMENU},
        {"关于", 0, (Fl_Callback*)HelpAboutCallback, 0},
        {0},
    {0}
};

void regBaseType()
{
	ADD_TYPE(char);
	ADD_TYPE(signed char);
	ADD_TYPE(unsigned char);

	ADD_TYPE(wchar_t);

	ADD_TYPE(short);
	ADD_TYPE(signed short);
	ADD_TYPE(unsigned short);

	ADD_TYPE(int);
	ADD_TYPE(signed int);
	ADD_TYPE(unsigned int);

	ADD_TYPE(long);
	ADD_TYPE(signed long);
	ADD_TYPE(unsigned long);

	ADD_TYPE(long long);
	ADD_TYPE(signed long long);
	ADD_TYPE(unsigned long long);
#ifdef _WIN32
	ADD_TYPE(__int64);
	ADD_TYPE(signed __int64);
	ADD_TYPE(unsigned __int64);
#endif

	ADD_TYPE(float);
	ADD_TYPE(double);
	ADD_TYPE(long double);
}

HexEditorWindow::HexEditorWindow(int w, int h, const char* title)
    : Fl_Double_Window(w, h, title) {
    // 创建菜单栏
    m_menuBar = new Fl_Menu_Bar(0, 0, w, 30);
    m_menuBar->menu(menuItems);
    
    // 为每个菜单项设置user_data为this指针
    for (int i = 0; menuItems[i].text != nullptr; i++) {
        if (menuItems[i].callback_ != nullptr) {
            // 为有回调函数的菜单项设置user_data
            menuItems[i].user_data_ = this;
        }
    }
    
    // 创建十六进制表格（调整位置，为菜单栏留出空间）
    m_hexTable = new HexTable(10, 40, w - 20, h - 80);
    
    // 启用表格单元格导航功能
    m_hexTable->enable_cell_nav(true);
        
    // 创建状态显示（紧贴窗口底部，宽度与窗口相同）
    m_statusBuffer = new Fl_Text_Buffer();
    m_statusDisplay = new Fl_Text_Display(0, h - 30, w, 30);
    m_statusDisplay->buffer(m_statusBuffer);
    
    // 设置状态显示使用支持中文的等宽字体
    m_statusDisplay->textfont(FL_COURIER);
    m_statusDisplay->textsize(12);
    
    // 将状态缓冲区关联到表格
    m_hexTable->SetStatusBuffer(m_statusBuffer);
    
    end();

    regBaseType();
    parseSimpleConfig("aliastype.conf", DefineFakeType);

        // 遍历并打印所有注册的基础类型
    size_t nCount = BindingType::m_vecTotalType.size();
    printf("已注册的基础类型 (%zu 个):\n", nCount);
    for (size_t i = 0; i < nCount; i++) {
        BindingType* pType = BindingType::m_vecTotalType[i];
        // 将宽字符串转换为窄字符串进行打印
        std::wstring wideStr = pType->m_strType;
        std::string narrowStr(wideStr.begin(), wideStr.end());
        printf("类型 %zu: %s, 大小: %d 字节\n", i + 1, narrowStr.c_str(), pType->m_nTypeSize);
    }
}

HexEditorWindow::~HexEditorWindow() {
    delete m_statusBuffer;
}

// 菜单回调函数
void HexEditorWindow::MenuCallback(Fl_Widget* widget, void* data) {
    // 通用菜单回调，可以在这里处理菜单项选择
    Fl_Menu_Bar* menuBar = static_cast<Fl_Menu_Bar*>(widget);
    const Fl_Menu_Item* item = menuBar->mvalue();
    if (item && item->label()) {
        // 可以在这里添加通用的菜单处理逻辑
    }
}

// 文件菜单回调函数
void HexEditorWindow::FileOpenCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    
    Fl_Native_File_Chooser chooser;
    chooser.title("选择文件");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    chooser.filter("所有文件\t*.*");
    
    if (chooser.show() == 0) {
        const char* fileName = chooser.filename();
        if (fileName) {
            if (!window->m_hexTable->OpenFile(fileName)) {
                fl_alert("无法打开文件: %s", fileName);
            } else {
                // 更新状态显示
                window->m_statusBuffer->text(fileName);
            }
        }
    }
}

void HexEditorWindow::FileSaveCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    fl_alert("保存文件功能尚未实现");
}

void HexEditorWindow::FileExitCallback(Fl_Widget* widget, void* data) {
    exit(0);
}

// 编辑菜单回调函数
void HexEditorWindow::EditCopyCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    fl_alert("复制功能尚未实现");
}

void HexEditorWindow::EditPasteCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    fl_alert("粘贴功能尚未实现");
}

void HexEditorWindow::EditFindCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    fl_alert("查找功能尚未实现");
}

// 视图菜单回调函数
void HexEditorWindow::ManageBasicTypeCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    fl_alert("功能尚未实现");
}

void HexEditorWindow::ManageStructTypeCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    fl_alert("功能尚未实现");
}

void HexEditorWindow::ManageVarCallback(Fl_Widget* widget, void* data) {
    HexEditorWindow* window = static_cast<HexEditorWindow*>(data);
    fl_alert("功能尚未实现");
}

// 帮助菜单回调函数
void HexEditorWindow::HelpAboutCallback(Fl_Widget* widget, void* data) {
    fl_alert("简易十六进制编辑器\n版本 1.0\n基于FLTK开发");
}
