#ifndef HEXEDITORWINDOW_H
#define HEXEDITORWINDOW_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include "HexTable.h"

// 主应用窗口类
class HexEditorWindow : public Fl_Double_Window {
private:
    HexTable* m_hexTable;
    Fl_Button* m_openButton;
    Fl_Text_Display* m_statusDisplay;
    Fl_Text_Buffer* m_statusBuffer;

public:
    HexEditorWindow(int w, int h, const char* title);
    ~HexEditorWindow();

    // 打开文件按钮回调
    static void OpenButtonCallback(Fl_Widget* widget, void* data);

    // 向上滚动按钮回调
    static void UpButtonCallback(Fl_Widget* widget, void* data);

    // 向下滚动按钮回调
    static void DownButtonCallback(Fl_Widget* widget, void* data);
};

#endif // HEXEDITORWINDOW_H