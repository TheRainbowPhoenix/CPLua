#include <appdef.h>

#include <sdk/os/debug.h>
#include <sdk/os/lcd.h>
#include <sdk/os/file.h>

#include <sys/stat.h>

#include "heap_hack.hpp"

#include "os/gui/peg.hpp"
#include "os/gui/pegtypes.hpp"
#include "cplua.hpp"

#include <vector>
#include <string>
#include <cstring>

APP_NAME("CPLua")
APP_AUTHOR("jules")
APP_DESCRIPTION("Lua for Hollyhock")
APP_VERSION("1.0.0")

const WORD TEXTBOX_STYLE = FF_NONE | EF_WRAP | TJ_LEFT;

class LuaFilePickerWindow : public PegDecoratedWindow {
  enum Ids {
    Id_InfoText = 1,
    Id_List,
    Id_Btn
  };

public:
  LuaFilePickerWindow(PegRect &rect) : PegDecoratedWindow(rect) {
    auto title = new PegTitle("CPLua", TF_CLOSEBUTTON);

    PegRect textRect;
    textRect.SetAndCenterIfLandscape(20, 30, 300, 60, TRUE);
    infoBox = new PegTextBox(textRect, Id_InfoText, TEXTBOX_STYLE, "Select a Lua script to run:");

    Add(title->obj());
    Add(infoBox->obj());

    PegRect listRect;
    listRect.SetAndCenterIfLandscape(20, 70, 300, 180, TRUE);

    vertList = new PegVertList(listRect, Id_List, FF_THIN);

    // Search for lua files in root and cplua folder
    ScanLuaFiles("\\*.lua");
    ScanLuaFiles("\\cplua\\*.lua");

    for (size_t i = 0; i < luaFiles.size(); ++i) {
        // Allocate dynamically; the list manages the objects
        char* nameCopy = strdup(luaFiles[i].c_str());
        PegRadioButton* item = new PegRadioButton(0, 0, nameCopy, Id_List + 10 + i);
        options.push_back(item);
        vertList->Add(item->obj());
    }

    if (!options.empty()) {
        options[0]->SetSelected();
        vertList->SetSelected(options[0]->obj());
    }

    Add(vertList->obj());

    PegRect buttonRect;
    buttonRect.SetAndCenterIfLandscape(110, 200, 210, 230, TRUE);
    auto button = new PegTextButton(buttonRect, "Run Script", Id_Btn, AF_ENABLED | TJ_CENTER);
    Add(button->obj());
  }



  void ScanLuaFiles(const char* pattern) {
      int searchHandle;
      struct stat stat_buf;

      int result = findfirst(pattern, &searchHandle, &stat_buf);
      while (result == 0) {
          if ((stat_buf.fileMode & FILE_DIR) == 0) {
              std::string path;
              std::string pat(pattern);
              if (pat.find("\\cplua\\") != std::string::npos) {
                  path = "\\cplua\\";
              } else {
                  path = "\\";
              }
              path += stat_buf.fileName;

              if (path.length() >= 4 && path.substr(path.length() - 4) == ".lua") {
                  luaFiles.push_back(path);
              }
          }
          result = findnext(searchHandle, &stat_buf);
      }
      findclose(searchHandle);
  }
SIGNED Message(const PegMessage &mesg) override {
    switch (mesg.wType) {
    case PEG_SIGNAL(Id_Btn, PSF_CLICKED):
      for (size_t i = 0; i < options.size(); i++) {
        if (options[i]->IsSelected()) {
          infoBox->DataSet("Running...");
          Screen()->Invalidate(infoBox->obj()->mClip);
          infoBox->Draw();

          std::string scriptToRun = luaFiles[i];

          // Wait for drawing
          LCD_Refresh();

          RunLuaScript(scriptToRun);

          infoBox->DataSet("Select a Lua script to run:");
          Screen()->Invalidate(infoBox->obj()->mClip);
          infoBox->Draw();
          LCD_Refresh();
          break;
        }
      }
      return 0;
    }

    return PegDecoratedWindow::Message(mesg);
  }

private:
  PegTextBox *infoBox;
  PegVertList *vertList;
  std::vector<PegRadioButton*> options;
  std::vector<std::string> luaFiles;
};

extern "C" void calcInit() {
  auto *ptr = initFixedRegion(2 * 1024 * 1024);
  if (ptr != reinterpret_cast<void *>(0x8cc80000))
    heapReset();
}

extern "C" void calcExit() {
  heapReset();
}

int main(int argc, char **argv, char **envp) {
  calcInit();

  PegRect rectWin(10, 10, 310, 280);
  auto win = new LuaFilePickerWindow(rectWin);

  win->Execute();

  delete win;
  calcExit();
  return 0;
}
