/**
 * @file strutil.cpp
 * @brief some useful utilitis for std::string
 *
 * @author Qingshan
 * @date 2009-7-28
 */
#include "os_types.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <fstream>

#include "strutil.h"
#include "md5.h"
#include "sha1.h"
#include "os_api.h"
#include "crc.h"
#include "adler32.h"
#include "hash.h"

namespace sax {

std::string &trim(std::string &str, bool left, bool right)
{
	static const std::string delims = " \t\r\n";
	if(right) str.erase(str.find_last_not_of(delims)+1);
	if(left) str.erase(0, str.find_first_not_of(delims));
	return str;
}

std::string &trimSpaces(std::string &str)
{
	std::string::iterator it = std::remove_if(
		str.begin(), str.end(), ::isspace);
	str.resize(it - str.begin());
	return str;
}

std::string &trimTails(std::string &str, const char *mark)
{
	size_t i = str.find(mark);
	if (i != std::string::npos) str.erase(i);
	return str;
}

std::string &trimAll(std::string &str, const char ch)
{
	std::string::iterator it = std::remove_if(
		str.begin(), str.end(), std::bind1st(std::equal_to<char>(), ch));
	str.resize(it - str.begin());
	return str;
}

std::string &trimAll(std::string &str, const char *filter)
{
	size_t len = str.length();
	if (len) {
		char *p = (char *) malloc(len);
		size_t i = 0, t = 0;
		const char *s = str.c_str();
		while (i < len) {
			char ch = s[i++];
			if (!::strchr(filter, ch)) p[t++] = ch;
		}
		if (t != len) str.assign(p, t);
		free(p);
	}
	return str;
}

std::string &replace(std::string &str, const char *sub, const char *neu)
{
	// find the first string to replace
	size_t i = str.find(sub);
	
	while (i != std::string::npos)
	{
		str.replace(i, strlen(sub), neu);
		i = str.find(sub, i + strlen(neu));
	}
	return str;
}

strings split(
	const std::string &str, const char *delims)
{
	strings ret;
	size_t s = 0;
	for (;;) {
		s = str.find_first_not_of(delims, s);
		if (s == std::string::npos) break;
		
		size_t i = str.find_first_of(delims, s);
		if (i == std::string::npos)
		{
			ret.push_back(str.substr(s));
			break; // copy the rest and exit
		}
		else {
			ret.push_back(str.substr(s, i-s));
			s = i + 1; // copy up to delimiter
		}
	}
	return ret;
}

std::string join(
	const strings &ss, const std::string &sep)
{
	if (!ss.size()) return std::string();
	
	size_t len = sep.length();
	size_t tot = ss[0].length();
	for (size_t i=1; i<ss.size(); i++)
	{
		tot += len;
		tot += ss[i].length();
	}
	
	std::string ret;
	ret.reserve(tot+1);
	
	ret.append(ss[0]);
	for (size_t j=1; j<ss.size(); j++)
	{
		const std::string &sub = ss[j];
		if (len) ret.append(sep);
		if (sub.length()) ret.append(sub);
	}
	return ret;
}

bool wrap(std::string &str, size_t cols, const char *sep)
{
	if (cols == 0) return false;
	
	size_t s0 = str.length();
	size_t s1 = strlen(sep);
	size_t s2 = (s0/cols*s1) + s0;
	if (s2 == s0) return true;
	
	char *tmp = (char *) malloc(s2);
	if (!tmp) return false;
	
	const char *ptr = str.c_str();
	size_t i = 0, j = 0;
	
	for (; s0 >= cols; s0 -= cols)
	{
		memcpy(tmp+j, ptr+i, cols);
		i += cols; j += cols;
		memcpy(tmp+j, sep, s1);
		j += s1;
	}
	if (s0 > 0) memcpy(tmp+j, ptr+i, s0);
	
	str.assign(tmp, s2);
	free(tmp); return true;
}

std::string &toupper(std::string &str)
{
	std::transform(str.begin(),str.end(), str.begin(), ::toupper);
	return str;
}

std::string &tolower(std::string &str)
{
	std::transform(str.begin(),str.end(), str.begin(), ::tolower);
	return str;
}

int stricmp(const std::string &s1, const std::string &s2)
{
	return g_stricmp(s1.c_str(), s2.c_str());
}


bool startsWith(const std::string& str, const std::string& sub)
{
	return (str.find(sub) == 0);
}

bool endsWith(const std::string& str, const std::string& sub)
{
	size_t i = str.rfind(sub);
	return (i != std::string::npos) && (i == (str.length() - sub.length()));
}

bool match(const std::string &str, 
	const char *pattern, bool CaseSensitive)
{
	return (0 != g_strmatch(
		str.c_str(), pattern, CaseSensitive, '\0'));
}

void stdpath(std::string &path, char slash)
{
	if (!path.length()) return;
	char a, b;
	if (slash == '/') {a = '/'; b = '\\';}
	else {a = '\\'; b = '/';}
	
	std::replace(path.begin(), path.end(), b, a);
	if (path[path.length()-1] != a) path += a;
}

static const char *jump_over(const char *s, char ch)
{
	const char *p;
	while ((p=strchr(s, ch))) s = p+1;
	return s;
}

std::string dirName(const char *fpath)
{
	const char *s = jump_over(jump_over(fpath, '/'), '\\');
	return std::string(fpath, s-fpath);
}

const char *baseName(const char *fpath)
{
	return jump_over(jump_over(fpath, '/'), '\\');
}

const char *extName(const char *fpath)
{
	const char *s = jump_over(fpath, '.');
	return (s==fpath ? 0 : s);
}

const char *mimeType(const uint8_t sig[4])
{
	if(sig[0]==0xFF && sig[1]==0xD8) return "image/jpeg";
	
	if(sig[0]==0x89 && memcmp(sig+1, "PNG", 3) == 0) 
		return "image/png";
		
	if(memcmp(sig, "GIF8", 4) == 0) return "image/gif";
	
	if(sig[0]=='B' && sig[1]=='M') return "image/bmp";

	if(sig[0]=='P' && sig[1]>='1' && sig[1]<='6') 
		return "image/x-portable-anymap";
	
	if(sig[0]==0 && sig[1]==0 && sig[2]==1 && sig[3]==0) 
		return "image/x-icon";
	
	return "application/octet-stream"; /* Nothing found. */
}

#ifdef _MSC_VER

#define vsnprintf(a,b,c,d) _vsnprintf(a,b,c,d)
#define vsscanf(a,b,c) _vsscanf(a,b,c)

// VC++ provides the vsprintf() function but for strange 
// reasons does not provide the complementary vsscanf() 
// function which is available on most Unix systems. 
// http://www.codeguru.com/cpp/cpp/string/article.php/c5631
// guess an upper bound for the # of args
static int do_guess_argc(const char *fmt)
{
	const char *p = fmt;
	int cnt = 0;
	while (*p) {
		if (*p++ != '%') continue;
		switch (*p)
		{
		case '\0': return cnt;
		case '*': 
		case '%': p++; break;
		default: ++cnt;
		}
	}
	return cnt;
}

int _vsscanf(const char *buf, const char *fmt, va_list ap)
{
	void *argv[16];
	memset(argv, 0, sizeof(argv));
	
	int argc = do_guess_argc(fmt);
	const int mc = sizeof(argv)/sizeof(argv[0]);
	if (argc > mc) argc = mc;
	
	// Do exception handling in case we go too far
	try {
		for (int i = 0; i < argc; i++) {
			argv[i] = va_arg(ap, void *);
			if (!argv[i]) break;
		}
	}
	catch(...) {}

	// This is lame, but the extra arguments won't be used by sscanf
	return sscanf(buf, fmt, argv[0], argv[1], argv[2], argv[3],
		argv[4], argv[5], argv[6], argv[7], argv[8], argv[9],
		argv[10], argv[11], argv[12], argv[13], argv[14], argv[15]);
}
#endif

std::string format(const char *fmt, ...)
{
	const int sz = 8192;
	char text[sz] = {0, };
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(text, sz, fmt, ap);
	va_end(ap);
	return std::string(text);
}

int scan(const std::string &str, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int got = vsscanf(str.c_str(), fmt, ap);
	va_end(ap);
	return got;
}

static std::string hex_digest(const md5_byte_t digest[], size_t size)
{
	static const char hex[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','a','b','c','d','e','f'};
	size_t i=0, j=0;
	char *str = (char *) malloc(size*2);
	
	for (; i<size; i++)
	{
		int x = (int) digest[i];
		str[j++] = hex[x>>4];
		str[j++] = hex[x&15];
	}
	std::string ret(str, j);
	free(str); return ret;
}

std::string md5(const std::string &src, bool hex)
{
	md5_state_t ms;
	md5_byte_t digest[16];
	
	md5_init(&ms);
	md5_append(&ms, (const md5_byte_t *) src.c_str(), src.length());
	md5_finish(&ms, digest);
	
	if (hex) return hex_digest(digest, sizeof(digest));
	return std::string((char *)digest, sizeof(digest));
}

std::string md5(struct f64_t *fp, bool hex)
{
	md5_state_t ms;
	md5_init(&ms);
	
	char tmp[4096];
	int64_t offset = 0;
	
	for (;;) {
		int got = f64read(fp, tmp, sizeof(tmp), offset);
		if (got <= 0) break;
		md5_append(&ms, (const md5_byte_t*) tmp, got);
		offset += got;
		if (got != sizeof(tmp)) break;
	}
	
	md5_byte_t digest[16];
	md5_finish(&ms, digest);
	
	if (hex) return hex_digest(digest, sizeof(digest));
	return std::string((char *)digest, sizeof(digest));
}

std::string md5(const char *filename, bool hex)
{
	f64_t *fp = f64open(filename, 0);
	if (!fp) return std::string("");

	std::string ret = md5(fp, hex);
	f64close(fp); return ret;
}

std::string shorten(const std::string &url, size_t cnt)
{
	md5_state_t ms;
	md5_byte_t digest[16];
	
	md5_init(&ms);
	md5_append(&ms, (const md5_byte_t *) url.c_str(), url.length());
	md5_finish(&ms, digest);

	char str[32];
	int j = g_b64_encode(digest, sizeof(digest), 
		str, (++cnt > 32 ? 32 : cnt));
	return std::string(str, j);
}

std::string sha1(const std::string &src, bool hex)
{
	SHA1_CTX ctx;
	unsigned char digest[20];
	
	SHA1Init(&ctx);
	SHA1Update(&ctx, (const unsigned char*)src.c_str(), src.length());
	SHA1Final(&ctx, digest);
	
	if (hex) return hex_digest(digest, sizeof(digest));
	return std::string((char *)digest, sizeof(digest));
}

std::string sha1(struct f64_t *fp, bool hex)
{
	SHA1_CTX ctx;
	SHA1Init(&ctx);
	
	char tmp[4096];
	int64_t offset = 0;
	
	for (;;) {
		int got = f64read(fp, tmp, sizeof(tmp), offset);
		if (got <= 0) break;
		SHA1Update(&ctx, (const unsigned char*) tmp, got);
		offset += got;
		if (got != sizeof(tmp)) break;
	}
	
	unsigned char digest[20];
	SHA1Final(&ctx, digest);
	
	if (hex) return hex_digest(digest, sizeof(digest));
	return std::string((char *)digest, sizeof(digest));
}

std::string sha1(const char *filename, bool hex)
{
	f64_t *fp = f64open(filename, 0);
	if (!fp) return std::string("");

	std::string ret = sha1(fp, hex);
	f64close(fp); return ret;
}


uint32_t crc32(const std::string &src, uint32_t start)
{
	const uint8_t *buf = (const uint8_t *) src.c_str();
	uint32_t len = src.length();
	start ^= 0xFFffFFff;
	return (crc32_ieee_le(start, buf, len)^0xFFffFFff);
}

bool crc32(const char *filename, uint32_t &code)
{
	f64_t *fp = f64open(filename, 0);
	if (!fp) return false;

	code = crc32(fp);
	f64close(fp); return true;
}

uint32_t crc32(struct f64_t *fp)
{
	uint8_t tmp[4096];
	int64_t offset = 0;
	
	uint32_t code = 0xFFffFFff;
	for (;;) {
		int got = f64read(fp, tmp, sizeof(tmp), offset);
		if (got <= 0) break;
		code = crc32_ieee_le(code, tmp, got);
		offset += got;
		if (got != sizeof(tmp)) break;
	}
	return (code^0xFFffFFff);
}

uint32_t crc32_rev(uint32_t end, const std::string &src)
{
	const uint8_t *buf = (const uint8_t *) src.c_str();
	uint32_t len = src.length();
	end ^= 0xFFffFFff;
	return (crc32_ieee_le_rev(end, buf, len)^0xFFffFFff);
}

uint32_t adler32(const std::string &src, uint32_t start)
{
	const uint8_t *buf = (const uint8_t *) src.c_str();
	uint32_t len = src.length();
	return calc_adler32(start, buf, len);
}

uint32_t hash32(const std::string &src)
{
	return murmur_hash32(src.c_str(), src.length());
}

uint64_t hash64(const std::string &src)
{
	return murmur_hash64(src.c_str(), src.length());
}

std::string url_decode(const char *url, bool form_url_encoded)
{
	size_t len = strlen(url);
	size_t dst_len = len+1;
	char *dst = (char *) malloc(dst_len);
	int j = g_url_decode(url, len, dst, dst_len, form_url_encoded);
	
	std::string ret(dst, j);
	free(dst); return ret;
}

std::string url_encode(const char *url)
{
	size_t len = strlen(url);
	size_t dst_len = (3*len)+1;
	char *dst = (char *) malloc(dst_len);
	g_url_encode(url, dst, dst_len);
	
	std::string ret(dst);
	free(dst); return ret;
}

std::string b64_decode(const char *url, size_t len)
{
	size_t siz = (len>0 ? len : strlen(url));
	size_t dst_len = siz+1;
	unsigned char *dst = (unsigned char *) malloc(dst_len);
	int j = g_b64_decode(url, siz, dst, dst_len);
	
	std::string ret;
	if (j>0) ret.assign((char *)dst, j);
	free(dst); return ret;
}

std::string b64_encode(const void *src, size_t len)
{
	size_t dst_len = (4*len+2)/3+1;
	char *dst = (char *) malloc(dst_len);
	int j = g_b64_encode(
		(const unsigned char *) src, len, dst, dst_len);
	
	std::string ret(dst, j);
	free(dst); return ret;
}

std::string rand_str(size_t n, bool lower, bool upper)
{
	char *str = (char *) malloc(n);
	if (!str) return std::string();
	
	static const char char_list[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
	int start = (upper ? 0 : 26);
	int end = (lower ? 62 : 36);
	int total = (end - start);
	
	for (size_t i=0; i<n; i++) 
		str[i] = char_list[rand()%total + start];
	
	std::string ret(str, n);
	free(str); return ret;
}

std::string tohex(const std::string &str)
{
	const char *p = str.c_str();
	size_t n = str.length();
	return hex_digest((const md5_byte_t *) p, n);
}

bool save(const std::string &str, const char *fn)
{
	try {
		std::ofstream file(fn, std::ios_base::out|std::ios_base::binary);
		if (!file) return false;
		file.write(str.c_str(), str.length());
		file.close(); return true;
	}
	catch(...) {
		return false;
	}
}

bool load(std::string &str, const char *fn)
{
	try {
		std::ifstream file(fn, std::ios_base::in|std::ios_base::binary);
		if (!file) return false;
#if 0
		str.assign(std::istreambuf_iterator<char>(file), 
			std::istreambuf_iterator<char>());
#else
		std::ostringstream os;
		os << file.rdbuf();
		str.assign(os.str());
#endif
		file.close(); return true;
	}
	catch(...) {
		return false;
	}
}

} // namespace

