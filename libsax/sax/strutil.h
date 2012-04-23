#ifndef __STRUTIL_H_2009__
#define __STRUTIL_H_2009__

/**
 * @file strutil.h
 * @brief some useful utilitis for std::string
 * @note http://www.codeproject.com/KB/stl/STL_string_util.aspx
 *
 * @author Qingshan
 * @date 2009-7-28, 2011.4.2
 */

#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)

#include <vector>
#include <string>
#include <sstream>

struct f64_t;

namespace sax {

typedef std::vector<std::string> strings;

/// removes any whitespace characters, be it standard space orTABs
std::string &trim(std::string &str, bool left=true, bool right=true);

/// removes all space characters
std::string &trimSpaces(std::string &str);

/// removes ending comments started by "#" or "//" etc
std::string &trimTails(std::string &str, const char *mark);

/// removes one specified character
std::string &trimAll(std::string &str, const char ch);

/// removes all characters specified by filter
std::string &trimAll(std::string &str, const char *filter);

/// replace one sub-string with another
std::string &replace(std::string &str, const char *sub, const char *neu);

/// split strings by delimiter characters
strings split(const std::string &str, const char *delims=" \t\r\n");

/// join a set of strings together into one
std::string join(const strings &ss, const std::string &sep);

/// wrap a string by number of colums
bool wrap(std::string &str, size_t cols, const char *sep);

/// upper-cases all the characters in the std::string.
std::string &toupper(std::string &str);

/// lower-cases all the characters in the std::string.
std::string &tolower(std::string &str);

int stricmp(const std::string &s1, const std::string &s2);
bool startsWith(const std::string &str, const std::string &sub);
bool endsWith(const std::string &str, const std::string &sub);

/// simple pattern-matching (wildcard '*')
bool match(const std::string &str, 
	const char *pattern, bool CaseSensitive=true);

/// convert something into std::string or back
std::string format(const char *fmt, ...);
int scan(const std::string &str, const char *fmt, ...);

/// standardising paths, append ending-slash when missing
void stdpath(std::string &str, char slash='/');

std::string dirName(const char *fpath);
const char *baseName(const char *fpath);
const char *extName(const char *fpath);
const char *mimeType(const uint8_t sig[4]);

/// calculate md5, if hex=false, return raw 16 bytes
std::string md5(const std::string &src, bool hex=true);
std::string md5(const char *fn, bool hex=true);
std::string md5(struct f64_t *fp, bool hex=true);

/// shorten an url by md5, max(cnt)=22
std::string shorten(const std::string &url, size_t cnt);

/// calculate sha1, if hex=false, return raw 20 bytes
std::string sha1(const std::string &src, bool hex=true);
std::string sha1(const char *fn, bool hex=true);
std::string sha1(struct f64_t *fp, bool hex=true);

/// calulate crc32, adler32 etc.
uint32_t crc32(const std::string &src, uint32_t start=0);
bool crc32(const char *fn, uint32_t &code);
uint32_t crc32(struct f64_t *fp);

uint32_t adler32(const std::string &src, uint32_t start=0);

/// roll back: from end to start.
uint32_t crc32_rev(uint32_t end, const std::string &src);

/// calculate hash32/hash64 by murmur2-hash
uint32_t hash32(const std::string &src);
uint64_t hash64(const std::string &src);

/// URL-decode/encode, see RFC 1866
std::string url_decode(const char *url, bool form_url_encoded);
std::string url_encode(const char *url);

/// byte-decode/encode to 64 bits string (not base64)
std::string b64_decode(const char *str, size_t len=0);
std::string b64_encode(const void *src, size_t len);

/// randomize a string with letters and digitals
std::string rand_str(size_t n, bool lower=true, bool upper=true);

/// for hex conversion:
std::string tohex(const std::string &str);

/// string IO: save into(load from) a file
bool save(const std::string &str, const char *fn);
bool load(std::string &str, const char *fn);


template<class T> 
inline std::string toString(const T& val) 
{
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

template<>
inline std::string toString<bool>(const bool& val)
{
	std::ostringstream oss;
	oss << std::boolalpha << val;
	return oss.str();
}

template<class T>
inline T parseString(const std::string& str) 
{
    T val;
    std::istringstream iss(str);
    iss >> val;
    return val;
}

template<>
inline bool parseString<bool>(const std::string& str)
{
	bool val;
	std::istringstream iss(str);
	iss >> std::boolalpha >> val;
	return val;
}

} // namespace

#endif // __cplusplus

#endif//__STRUTIL_H_2009__
