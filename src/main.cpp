#include <appdef.h>

#include <sdk/os/debug.h>
#include <sdk/os/lcd.h>
#include <sdk/os/file.h>

#include <sys/stat.h>

#include "heap_hack.hpp"

#include "os/gui/peg.hpp"
#include "os/gui/pegtypes.hpp"
#include "cplua.hpp"

#ifdef DEBUG
#define TRACE_MAIN(msg) Debug_Printf(0, 0, false, 0, msg); LCD_Refresh()
#else
#define TRACE_MAIN(msg)
#endif

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
    ScanLuaFiles("\\fls0\\*.lua");


    for (size_t i = 0; i < luaFiles.size(); ++i) {
        // Allocate dynamically; the list manages the objects
        char* nameCopy = new char[luaFiles[i].length() + 1];
        strcpy(nameCopy, luaFiles[i].c_str());
        allocatedNames.push_back(nameCopy);
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










  ~LuaFilePickerWindow() {
    for (char* name : allocatedNames) {
      delete[] name;
    }
  }

  void ScanLuaFiles(const char* /*pattern*/) {
      int findHandle = 0;
      File_FindInfo findInfo{};

      const char16_t search_pattern[] = u"\\fls0\\*.lua";

      // Filename buffer without the wildcard
      char16_t filename_buf[100] = u"\\fls0\\";

      // filename_buf + 6 is the address where the SDK will write the found file name.
      int ret = File_FindFirst(reinterpret_cast<const char_const16_t*>(search_pattern), &findHandle, reinterpret_cast<char_const16_t*>(filename_buf + 6), &findInfo);

      while (ret >= 0) { // FILE_OK is typically 0
          if (findInfo.type == 1) { // 1 is usually File_FindInfo::EntryTypeFile in standard CP SDK
              std::string path;
              for(int i=0; filename_buf[i] != 0; ++i) {
                  path += (char)filename_buf[i];
              }
              luaFiles.push_back(path);
          }
          ret = File_FindNext(findHandle, reinterpret_cast<char_const16_t*>(filename_buf + 6), &findInfo);
      }
      File_FindClose(findHandle);
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

          TRACE_MAIN("Running...");
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
  std::vector<char*> allocatedNames;
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
  (void)argc;
  (void)argv;
  (void)envp;
  calcInit();

  PegRect rectWin(10, 10, 310, 280);
  auto win = new LuaFilePickerWindow(rectWin);

  win->Execute();

  delete win;
  calcExit();
  return 0;
}
