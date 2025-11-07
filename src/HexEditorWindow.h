#ifndef HEXEDITORWINDOW_H
#define HEXEDITORWINDOW_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include "HexTable.h"

// 主应用窗口类
class HexEditorWindow : public Fl_Double_Window {
private:
    HexTable* m_hexTable;
    Fl_Text_Display* m_statusDisplay;
    Fl_Text_Buffer* m_statusBuffer;
    Fl_Menu_Bar* m_menuBar;

    // 菜单项数组
    static Fl_Menu_Item menuItems[];

public:
    HexEditorWindow(int w, int h, const char* title);
    ~HexEditorWindow();

    // 菜单回调函数
    static void MenuCallback(Fl_Widget* widget, void* data);

    // 文件菜单回调函数
    static void FileOpenCallback(Fl_Widget* widget, void* data);
    static void FileSaveCallback(Fl_Widget* widget, void* data);
    static void FileExitCallback(Fl_Widget* widget, void* data);

    // 编辑菜单回调函数
    static void EditCopyCallback(Fl_Widget* widget, void* data);
    static void EditPasteCallback(Fl_Widget* widget, void* data);
    static void EditFindCallback(Fl_Widget* widget, void* data);

    // 数据管理菜单回调函数
    static void ManageBasicTypeCallback(Fl_Widget* widget, void* data);
    static void ManageStructTypeCallback(Fl_Widget* widget, void* data);
    static void ManageVarCallback(Fl_Widget* widget, void* data);

    // 帮助菜单回调函数
    static void HelpAboutCallback(Fl_Widget* widget, void* data);
};

#endif // HEXEDITORWINDOW_H
