#include "BasicTypeManagerDialog.h"
#include "FakeType.h"
#include <FL/Fl.H>
#include <string>

// BasicTypeManagerDialog 类的实现

BasicTypeManagerDialog::BasicTypeManagerDialog(int w, int h, const char* title)
    : Fl_Double_Window(w, h, title), m_selectedIndex(-1) {
    
    // 设置窗口为模态
    set_modal();
    
    // 创建类型列表（左边）
    m_typeList = new Fl_Browser(10, 10, 200, h - 80);
    m_typeList->type(FL_HOLD_BROWSER);
    m_typeList->callback(typeListCallback, this);
    
    // 创建类型详情区域（右边）
    Fl_Box* typeLabel = new Fl_Box(220, 10, 100, 25, "类型名称:");
    typeLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    m_typeNameInput = new Fl_Input(220, 35, 200, 25);
    m_typeNameInput->readonly(1);  // 设置为只读
    
    Fl_Box* sizeLabel = new Fl_Box(220, 70, 100, 25, "类型大小:");
    sizeLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    m_typeSizeInput = new Fl_Int_Input(220, 95, 100, 25);
    m_typeSizeInput->readonly(1);  // 设置为只读
    
    // 创建关闭按钮
    m_closeButton = new Fl_Button(w - 110, h - 40, 100, 30, "关闭");
    m_closeButton->callback(closeButtonCallback, this);
    
    end();
    
    // 刷新类型列表
    refreshTypeList();
}

void BasicTypeManagerDialog::refreshTypeList() {
    m_typeList->clear();
    
    size_t nCount = BindingType::m_vecTotalType.size();
    for (size_t i = 0; i < nCount; i++) {
        BindingType* pType = BindingType::m_vecTotalType[i];
        m_typeList->add(ws2s(pType->m_strType).c_str());
    }
    
    // 如果有类型，默认选中第一个
    if (nCount > 0) {
        m_typeList->select(1);  // FLTK 列表索引从1开始
        m_selectedIndex = 0;
        updateTypeDetails();
    }
}

void BasicTypeManagerDialog::updateTypeDetails() {
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)BindingType::m_vecTotalType.size()) {
        BindingType* pType = BindingType::m_vecTotalType[m_selectedIndex];
        
        // 更新类型名称
        std::wstring wideStr = pType->m_strType;
        std::string narrowStr(wideStr.begin(), wideStr.end());
        m_typeNameInput->value(narrowStr.c_str());
        
        // 更新类型大小
        char sizeStr[20];
        snprintf(sizeStr, sizeof(sizeStr), "%d", pType->m_nTypeSize);
        m_typeSizeInput->value(sizeStr);
    } else {
        m_typeNameInput->value("");
        m_typeSizeInput->value("");
    }
}

void BasicTypeManagerDialog::typeListCallback(Fl_Widget* widget, void* data) {
    BasicTypeManagerDialog* dialog = static_cast<BasicTypeManagerDialog*>(data);
    Fl_Browser* browser = static_cast<Fl_Browser*>(widget);
    
    int selectedLine = browser->value();
    if (selectedLine > 0) {
        dialog->m_selectedIndex = selectedLine - 1;  // FLTK 列表索引从1开始
        dialog->updateTypeDetails();
    }
}

void BasicTypeManagerDialog::closeButtonCallback(Fl_Widget* widget, void* data) {
    BasicTypeManagerDialog* dialog = static_cast<BasicTypeManagerDialog*>(data);
    dialog->hide();
}
