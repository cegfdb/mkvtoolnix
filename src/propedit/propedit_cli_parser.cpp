/** \brief command line parsing

   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/os.h"

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/range/numeric.hpp>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#include "common/common_pch.h"
#include "common/ebml.h"
#include "common/strings/formatting.h"
#include "common/translation.h"
#include "propedit/propedit_cli_parser.h"

propedit_cli_parser_c::propedit_cli_parser_c(const std::vector<std::string> &args)
  : cli_parser_c(args)
  , m_options(options_cptr(new options_c))
  , m_target(m_options->add_target(target_c::tt_segment_info))
{
}

void
propedit_cli_parser_c::set_parse_mode() {
  try {
    m_options->set_parse_mode(m_next_arg);
  } catch (...) {
    mxerror(boost::format(Y("Unknown parse mode in '%1% %2%'.\n")) % m_current_arg % m_next_arg);
  }
}

void
propedit_cli_parser_c::add_target() {
  try {
    m_target = m_options->add_target(m_next_arg);
  } catch (...) {
    mxerror(boost::format(Y("Invalid selector in '%1% %2%'.\n")) % m_current_arg % m_next_arg);
  }
}

void
propedit_cli_parser_c::add_change() {
  try {
    change_c::change_type_e type = (m_current_arg == "-a") || (m_current_arg == "--add") ? change_c::ct_add
                                 : (m_current_arg == "-s") || (m_current_arg == "--set") ? change_c::ct_set
                                 :                                                         change_c::ct_delete;
    m_target->add_change(type, m_next_arg);
  } catch (std::runtime_error &error) {
    mxerror(boost::format(Y("Invalid change spec (%3%) in '%1% %2%'.\n")) % m_current_arg % m_next_arg % error.what());
  }
}

void
propedit_cli_parser_c::add_tags() {
  try {
    m_options->add_tags(m_next_arg);
  } catch (...) {
    mxerror(boost::format(Y("Invalid selector in '%1% %2%'.\n")) % m_current_arg % m_next_arg);
  }
}

void
propedit_cli_parser_c::add_chapters() {
  try {
    m_options->add_chapters(m_next_arg);
  } catch (...) {
    mxerror(boost::format(Y("Invalid selector in '%1% %2%'.\n")) % m_current_arg % m_next_arg);
  }
}

std::map<ebml_type_e, const char *> &
propedit_cli_parser_c::get_ebml_type_abbrev_map() {
  static std::map<ebml_type_e, const char *> s_ebml_type_abbrevs;
  if (s_ebml_type_abbrevs.empty()) {
    s_ebml_type_abbrevs[EBMLT_INT]     = "SI";
    s_ebml_type_abbrevs[EBMLT_UINT]    = "UI";
    s_ebml_type_abbrevs[EBMLT_BOOL]    = "B";
    s_ebml_type_abbrevs[EBMLT_STRING]  = "S";
    s_ebml_type_abbrevs[EBMLT_USTRING] = "US";
    s_ebml_type_abbrevs[EBMLT_BINARY]  = "X";
    s_ebml_type_abbrevs[EBMLT_FLOAT]   = "FP";
  }

  return s_ebml_type_abbrevs;
}

void
propedit_cli_parser_c::list_property_names() {
  mxinfo(Y("All known property names and their meaning\n"));

  list_property_names_for_table(property_element_c::get_table_for(KaxInfo::ClassInfos,   NULL, true), Y("Segment information"), "info");
  list_property_names_for_table(property_element_c::get_table_for(KaxTracks::ClassInfos, NULL, true), Y("Track headers"),       "track:...");

  mxinfo("\n");
  mxinfo(Y("Element types:\n"));
  mxinfo(Y("  SI: signed integer\n"));
  mxinfo(Y("  UI: unsigned integer\n"));
  mxinfo(Y("  B:  boolean (0 or 1)\n"));
  mxinfo(Y("  S:  string\n"));
  mxinfo(Y("  US: Unicode string\n"));
  mxinfo(Y("  X:  binary in hex\n"));
  mxinfo(Y("  FP: floating point number\n"));

  mxexit(0);
}

void
propedit_cli_parser_c::list_property_names_for_table(const std::vector<property_element_c> &table,
                                                     const std::string &title,
                                                     const std::string &edit_spec) {
  std::map<ebml_type_e, const char *> &ebml_type_map = get_ebml_type_abbrev_map();

  auto max_name_len = boost::accumulate(table, 0u, [](size_t a, const property_element_c &e) { return std::max(a, e.m_name.length()); });

  static boost::regex s_newline_re("\\s*\\n\\s*", boost::regex::perl);
  boost::format format((boost::format("%%|1$-%1%s| | %%|2$-2s| |") % max_name_len).str());
  std::string indent_string = std::string(max_name_len, ' ') + " |    | ";

  mxinfo("\n");
  mxinfo(boost::format(Y("Elements in the category '%1%' ('--edit %2%'):\n")) % title % edit_spec);

  for (auto &property : table) {
    std::string name        = (format % property.m_name % ebml_type_map[property.m_type]).str();
    std::string description = property.m_title.get_translated()
                            + ": "
                            + boost::regex_replace(property.m_description.get_translated(), s_newline_re, " ",  boost::match_default | boost::match_single_line);
    mxinfo(format_paragraph(description, max_name_len + 8, name, indent_string));
  }
}

void
propedit_cli_parser_c::set_file_name() {
  m_options->set_file_name(m_current_arg);
}

#define OPT(spec, func, description) add_option(spec, boost::bind(&propedit_cli_parser_c::func, this), description)

void
propedit_cli_parser_c::init_parser() {
  add_information(YT("mkvpropedit [options] <file> <actions>"));

  add_section_header(YT("Options"));
  OPT("l|list-property-names",      list_property_names, YT("List all valid property names and exit"));
  OPT("p|parse-mode=<mode>",        set_parse_mode,      YT("Sets the Matroska parser mode to 'fast' (default) or 'full'"));

  add_section_header(YT("Actions"));
  OPT("e|edit=<selector>",          add_target,          YT("Sets the Matroska file section that all following add/set/delete "
                                                            "actions operate on (see below and man page for syntax)"));
  OPT("a|add=<name=value>",         add_change,          YT("Adds a property with the value even if such a property already "
                                                            "exists"));
  OPT("s|set=<name=value>",         add_change,          YT("Sets a property to the value if it exists and add it otherwise"));
  OPT("d|delete=<name>",            add_change,          YT("Delete all occurences of a property"));
  OPT("t|tags=<selector:filename>", add_tags,            YT("Add or replace tags in the file with the ones from 'filename' "
                                                            "or remove them if 'filename' is empty "
                                                            "(see below and man page for syntax)"));
  OPT("c|chapters=<filename>",      add_chapters,        YT("Add or replace chapters in the file with the ones from 'filename' "
                                                            "or remove them if 'filename' is empty"));

  add_section_header(YT("Other options"));
  add_common_options();

  add_separator();
  add_information(YT("The order of the various options is not important."));

  add_section_header(YT("Edit selectors"), 0);
  add_section_header(YT("Segment information"), 1);
  add_information(YT("The strings 'info', 'segment_info' or 'segmentinfo' select the segment information element. This is also the default until the first '--edit' option is found."), 2);

  add_section_header(YT("Track headers"), 1);
  add_information(YT("The string 'track:n' with 'n' being a number selects the nth track."), 2);
  add_information(YT("The string 'track:' followed by one of the chars 'a', 'b', 's' or 'v' followed by a number 'n' selects the nth audio, button, subtitle or video track "
                     "(e.g. '--edit track:a2')."), 2);
  add_information(YT("The string 'track:=uid' with 'uid' being a number selects the track whose 'track UID' element equals 'uid'."), 2);
  add_information(YT("The string 'track:@number' with 'number' being a number selects the track whose 'track number' element equals 'number'."), 2);

  add_section_header(YT("Tag selectors"), 0);
  add_information(YT("The string 'all' works on all tags."), 1);
  add_information(YT("The string 'global' works on the global tags."), 1);
  add_information(YT("All other strings work just like the track header selectors (see above)."), 1);

  add_hook(cli_parser_c::ht_unknown_option, boost::bind(&propedit_cli_parser_c::set_file_name, this));
}

#undef OPT

options_cptr
propedit_cli_parser_c::run() {
  init_parser();

  parse_args();

  m_options->options_parsed();
  m_options->validate();

  return m_options;
}

