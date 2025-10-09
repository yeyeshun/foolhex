#include <FL/Fl.H>
#include "HexEditorWindow.h"

int main(int argc, char **argv) {
    // 初始化FLTK
    Fl::scheme("gtk+");
    
    // 创建主窗口
    HexEditorWindow* window = new HexEditorWindow(800, 600, "简易十六进制编辑器");
    window->end();
    window->show(argc, argv);
    
    // 运行主循环
    return Fl::run();
}
