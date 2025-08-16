#pragma once

#include <nds.h>
#include <string>
#include <vector>
#include "button.h"
#include "menu.h"
#include "text.h"

class FontMenu : public Menu {
public:
    FontMenu(App* app);
    ~FontMenu();
    void draw();
    void Draw() override { draw(); } // Override to use the draw method
    void HandleInput(u16 keys) override {
        handleInput();
    }
    inline const std::vector<std::string>& getFiles() const { return files; }
    void handleInput();
    inline bool isDirty() const { return dirty; }
    inline void setDirty(bool d = true) { dirty = d; }
private:
    void findFiles();
    void handleButtonPress();
    void handleTouchInput();
    void nextPage();
    void previousPage();
    void selectNext();
    void selectPrevious();
    std::string dir;
    std::vector<std::string> files;
};
