#ifndef BASICTYPEMANAGERDIALOG_H
#define BASICTYPEMANAGERDIALOG_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include "BindingType.h"

// 基础类型管理对话框类
class BasicTypeManagerDialog : public Fl_Double_Window {
private:
    Fl_Browser* m_typeList;
    Fl_Input* m_typeNameInput;
    Fl_Int_Input* m_typeSizeInput;
    Fl_Button* m_closeButton;
    
    // 当前选中的类型索引
    int m_selectedIndex;
    
    // 静态回调函数
    static void typeListCallback(Fl_Widget* widget, void* data);
    static void closeButtonCallback(Fl_Widget* widget, void* data);
    
    // 更新类型详情显示
    void updateTypeDetails();
    
public:
    BasicTypeManagerDialog(int w, int h, const char* title);
    ~BasicTypeManagerDialog() {}
    
    // 刷新类型列表
    void refreshTypeList();
};

#endif // BASICTYPEMANAGERDIALOG_H
