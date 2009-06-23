/*
   mkvmerge GUI -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   the mmg_options_t struct

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/os.h"

#include <wx/wx.h>

#include "common/extern_data.h"
#include "common/wx.h"
#include "mmg/mmg.h"

void
mmg_options_t::init_popular_languages(const wxString &list) {
  int i;

  popular_languages.clear();

  if (!list.IsEmpty()) {
    std::vector<wxString> codes = split(list, wxU(" "));
    for (i = 0; codes.size() > i; ++i)
      if (is_valid_iso639_2_code(wxMB(codes[i])))
        popular_languages.Add(codes[i]);
  }

  if (popular_languages.IsEmpty()) {
    std::map<std::string, bool> codes_found;
    for (i = 0; iso639_languages[i].english_name != NULL; i++)
      if (!codes_found[std::string(iso639_languages[i].iso639_2_code)] && is_popular_language_code(iso639_languages[i].iso639_2_code)) {
        popular_languages.Add(wxU(iso639_languages[i].iso639_2_code));
        codes_found[std::string(iso639_languages[i].iso639_2_code)] = true;
      }
  }

  popular_languages.Sort();
}

void
mmg_options_t::validate() {
  if (   (ODM_FROM_FIRST_INPUT_FILE != output_directory_mode)
      && (ODM_PREVIOUS              != output_directory_mode)
      && (ODM_FIXED                 != output_directory_mode))
    output_directory_mode = ODM_FROM_FIRST_INPUT_FILE;

  if (   (priority != wxU("highest")) && (priority != wxU("higher")) && (priority != wxU("normal"))
      && (priority != wxU("lower"))   && (priority != wxU("lowest")))
    priority = wxU("normal");
}
