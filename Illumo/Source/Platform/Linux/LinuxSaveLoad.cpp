#include <Illumo/Platform/SaveLoad.h>

#include <gtkmm-3.0/gtkmm.h>

class FileChooser : public Gtk::FileChooserDialog
{
public:
  static std::string getOpenFileName()
  {
    FileChooser dialog("Select file", Gtk::FILE_CHOOSER_ACTION_OPEN);
    kit.run(dialog);
    return dialog.chosenFile;
  }
  static std::string getSaveFileName()
  {
    FileChooser dialog("Select file", Gtk::FILE_CHOOSER_ACTION_SAVE);
    kit.run(dialog);
    return dialog.chosenFile;
  }

protected:
  static Gtk::Main kit;
  std::string chosenFile;

  FileChooser(const Glib::ustring& title,
              Gtk::FileChooserAction action = Gtk::FILE_CHOOSER_ACTION_OPEN)
    : Gtk::FileChooserDialog(title, action)
  {
    chosenFile = std::string("");
    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
    signal_response().connect(
      sigc::mem_fun(*this, &FileChooser::on_my_response));
  }

  void on_my_response(int)
  {
    chosenFile = get_filename();
    hide();
  }
};
Gtk::Main FileChooser::kit(false);

std::string
SaveLoad::GetLoadLocation(const SaveLoadDialogSpec& specification)
{
  (void)specification;
  return FileChooser::getOpenFileName();
}

std::string
SaveLoad::GetSaveLocation(const SaveLoadDialogSpec& specification)
{
  (void)specification;
  return FileChooser::getSaveFileName();
}
