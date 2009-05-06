/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   locale handling functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/os.h"

#include <errno.h>
#include <iconv.h>
#if HAVE_NL_LANGINFO
# include <langinfo.h>
#elif HAVE_LOCALE_CHARSET
# include <libcharset.h>
#endif
#include <locale.h>
#include <string>
#include <vector>
#if SYS_WINDOWS
# include <windows.h>
#endif

#include "common/common.h"
#include "common/locale.h"
#include "common/memory.h"

struct kax_conv_t {
  iconv_t ict_from_utf8, ict_to_utf8;
  std::string charset;

  kax_conv_t(iconv_t n_ict_from_utf8, iconv_t n_ict_to_utf8,
             const std::string &n_charset):
    ict_from_utf8(n_ict_from_utf8), ict_to_utf8(n_ict_to_utf8),
    charset(n_charset) { }
};

static std::vector<kax_conv_t> s_kax_convs;
int cc_local_utf8 = -1;

static int
add_kax_conv(const std::string &charset,
             iconv_t ict_from,
             iconv_t ict_to) {
  int i;

  for (i = 0; s_kax_convs.size() > i; ++i)
    if (s_kax_convs[i].charset == charset)
      return i;

  s_kax_convs.push_back(kax_conv_t(ict_from, ict_to, charset));

  return s_kax_convs.size() - 1;
}

std::string
get_local_charset() {
  std::string lc_charset;

  setlocale(LC_CTYPE, "");
#if defined(COMP_MINGW) || defined(COMP_MSC)
  lc_charset = "CP" + to_std::string(GetACP());
#elif defined(SYS_SOLARIS)
  int i;

  lc_charset = nl_langinfo(CODESET);
  if (parse_int(lc_charset, i))
    lc_charset = std::string("ISO") + lc_charset + std::string("-US");
#elif HAVE_NL_LANGINFO
  lc_charset = nl_langinfo(CODESET);
#elif HAVE_LOCALE_CHARSET
  lc_charset = locale_charset();
#endif

  return lc_charset;
}

std::string
get_local_console_charset() {
#if defined(SYS_WINDOWS)
  return std::string("CP") + to_std::string(GetOEMCP());
#else
  return get_local_charset();
#endif
}

int
utf8_init(const std::string &charset) {
  std::string lc_charset;
  iconv_t ict_from_utf8, ict_to_utf8;
  int i;

  if (charset == "")
    lc_charset = get_local_charset();
  else
    lc_charset = charset;

  if ((lc_charset == "UTF8") || (lc_charset == "UTF-8"))
    return -1;

  for (i = 0; i < s_kax_convs.size(); i++)
    if (s_kax_convs[i].charset == lc_charset)
      return i;

  ict_to_utf8 = iconv_open("UTF-8", lc_charset.c_str());
  if (ict_to_utf8 == (iconv_t)(-1))
    mxwarn(boost::format(Y("Could not initialize the iconv library for the conversion from %1% to UFT-8. "
                           "Some strings will not be converted to UTF-8 and the resulting Matroska file "
                           "might not comply with the Matroska specs (error: %2%, %3%).\n"))
           % lc_charset % errno % strerror(errno));

  ict_from_utf8 = iconv_open(lc_charset.c_str(), "UTF-8");
  if (ict_from_utf8 == (iconv_t)(-1))
    mxwarn(boost::format(Y("Could not initialize the iconv library for the conversion from UFT-8 to %1%. "
                           "Some strings cannot be converted from UTF-8 and might be displayed incorrectly (error: %2%, %3%).\n"))
           % lc_charset % errno % strerror(errno));

  return add_kax_conv(lc_charset.c_str(), ict_from_utf8, ict_to_utf8);
}

void
utf8_done() {
  int i;

  for (i = 0; s_kax_convs.size() > i; ++i) {
    if (s_kax_convs[i].ict_from_utf8 != (iconv_t)(-1))
      iconv_close(s_kax_convs[i].ict_from_utf8);

    if (s_kax_convs[i].ict_to_utf8 != (iconv_t)(-1))
      iconv_close(s_kax_convs[i].ict_to_utf8);
  }

  s_kax_convs.clear();
}

static std::string
convert_charset(iconv_t ict,
                const std::string &src) {
  if (ict == (iconv_t)(-1))
    return src;

  int len   = src.length() * 4;
  char *dst = (char *)safemalloc(len + 1);
  memset(dst, 0, len + 1);

  iconv(ict, NULL, 0, NULL, 0);      // Reset the iconv state.

  size_t lsrc   = len / 4;
  size_t ldst   = len;
  char *srccopy = safestrdup(src);
  char *psrc    = srccopy;
  char *pdst    = dst;
  iconv(ict, (ICONV_CONST char **)&psrc, &lsrc, &pdst, &ldst);
  iconv(ict, NULL, NULL, &pdst, &ldst);

  safefree(srccopy);
  std::string result = dst;
  safefree(dst);

  return result;
}

std::string
to_utf8(int handle,
        const std::string &local) {
  std::string s;

  if (-1 == handle)
    return local;

  if (s_kax_convs.size() <= handle)
    mxerror(boost::format(Y("locale.cpp/to_utf8(): Invalid conversion handle %1% (num: %2%).\n")) % handle % s_kax_convs.size());

  return convert_charset(s_kax_convs[handle].ict_to_utf8, local);
}

std::string
from_utf8(int handle,
          const std::string &utf8) {
  std::string s;

  if (-1 == handle)
    return utf8;

  if (s_kax_convs.size() <= handle)
    mxerror(boost::format(Y("locale.cpp/from_utf8(): Invalid conversion handle %1% (num: %2%).\n")) % handle % s_kax_convs.size());

  return convert_charset(s_kax_convs[handle].ict_from_utf8, utf8);
}
