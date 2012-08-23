#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "compiler.h"

// some compilers do not define NDEBUG automatically
// when release! we patch this for assert(.)!
#if (!defined(_DEBUG) && !defined(NDEBUG))
#define NDEBUG
#endif // NDEBUG
#include <assert.h> // assert(.) is ignored by 'NDEBUG'!

#include "os_api.h"


//-------------------------------------------------------------------------
//------------------ (1) common part for string routines ------------------
//-------------------------------------------------------------------------
/**
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
int g_strlcpy(char *dst, const char *src, int siz)
{
	char *d = dst;
	const char *s = src;
	int n = siz;

	/* Copy as many bytes as will fit */
	if (--n > 0) {
		do {
			if ((*d++ = *s++) == '\0') break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0) *d = '\0';		/* NUL-terminate dst */
		while (*s++);
	}

	return (s - src - 1);	/* count does not include NUL */
}

/**
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
int g_strlcat(char *dst, const char *src, int siz)
{
	char *d = dst;
	const char *s = src;
	int n = siz;
	int dlen;
	if (n < 0) return -1;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0') d++;

	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));

	while (*s != '\0') {
		if (n != 1) {*d++ = *s;	n--;}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

int g_strncmp(const char *s1, const char *s2, int n)
{
	while (n-- > 0)
	{
		int u1 = (unsigned char) *s1++;
		int u2 = (unsigned char) *s2++;
		if (u1 != u2) return u1 - u2;
		if (u1 == '\0') return 0;
	}
	return 0;
}

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static const int16_t charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int g_stricmp(const char *s1, const char *s2)
{
	for (;;) {
		int u1 = (unsigned char) *s1++;
		int u2 = (unsigned char) *s2++;
		if (charmap[u1] != charmap[u2]) {
			return charmap[u1] - charmap[u2];
		}
		if (u1 == '\0')  return 0;
	}
}


int g_strnicmp(const char *s1, const char *s2, int n)
{
	while (n-- > 0) {
		int u1 = (unsigned char) *s1++;
		int u2 = (unsigned char) *s2++;
		if (charmap[u1] != charmap[u2]) {
			return charmap[u1] - charmap[u2];
		}
		if (u1 == '\0') return 0;
	}
	return 0;
}


char *g_strdup(const char *s)
{
	char *result = (char*) malloc(strlen(s)+1);
	if (!result) return NULL;
	strcpy(result, s);  return result;
}

void g_release(void *p)
{
	if (p) free(p);
}

char *g_strrchr(const char *s, char c)
{
	const char *last = NULL;
	while((s=strchr(s,c))) {last=s; s++;}
	return (char *)last;
}

void g_strrtrim(char *s)
{
	int j = strlen(s)-1;
	while (j>=0 && isspace(s[j])) s[j--]='\0';
}

void g_strtrim(char *s)
{
	int j = strlen(s)-1;
	char *p = s;
	while (j>=0 && isspace(s[j])) s[j--]='\0';
	while (isspace(*p)) p++;
	if (p != s) {
		do { *s++ = *p; } while (*p++);
	}
}

/// A simple wildcard text-matching algorithm in a single while loop
/// compares text strings, one of which can have wildcards ('*').
/// http://drdobbs.com/architecture-and-design/210200888?pgno=2
/// @param str A string without wildcards
/// @param pat A (potentially) corresponding string with wildcards
/// @param is_case_sensitive By default, match on 'X' vs 'x'
/// @param str For function names, for example, you can stop at the first '('
int g_strmatch(const char *str, const char *pat, 
	int is_case_sensitive, char terminator)
{
	// location after the last '*', if we've encountered one
	const char *pAfterLastWild = NULL;
	
	// location in the tame string, from which we started after last wildcard
	const char *pAfterLastTame = NULL;
	
	// Walk the text strings one character at a time.
	while (1)
	{
		char t = *str;
		char w = *pat;

		// How do you match a unique text string?
		if (!t || t == terminator)
		{
			// Easy: unique up on it!
			if (!w || w == terminator) return 1;
			if (w == '*') {pat++; continue;} // "x*" matches "x" or "xy"
			if (pAfterLastTame)
			{
				if (!(*pAfterLastTame) || *pAfterLastTame == terminator) return 0;
				str = pAfterLastTame++;
				pat = pAfterLastWild;
				continue;
			}
			return 0; // "x" doesn't match "xy"
		}
		else
		{
			if (!is_case_sensitive)
			{
				// Lowercase the characters to be compared.
				if (t >= 'A' && t <= 'Z') t += ('a' - 'A');
				if (w >= 'A' && w <= 'Z') w += ('a' - 'A');
			}

			// How do you match a tame text string?
			if (t != w)
			{
				// The tame way: unique up on it!
				if (w == '*')
				{
					pAfterLastWild = ++pat;
					pAfterLastTame = str;
					continue; // "*y" matches "xy"
				}
				if (pAfterLastWild)
				{
					pat = pAfterLastWild;
					w = *pat;
					if (!w || w == terminator) return 1; // "*" matches "x"
					
					if (!is_case_sensitive && w >= 'A' && w <= 'Z') w += ('a' - 'A');
					if (t == w) pat++;
					str++; continue; // "*sip*" matches "mississippi"
				}
				return 0; // "x" doesn't match "y"
			 }
		}
		str++; pat++;
	}
}

int g_snprintf(char *buf, int sz, const char *fmt, ...)
{
	va_list ap;
	int got;
	va_start(ap, fmt);
#ifdef _MSC_VER
	got = _vsnprintf(buf, sz, fmt, ap);
#else
	got = vsnprintf(buf, sz, fmt, ap);
#endif
	va_end(ap);
	return got;
}

// URL-decode input buffer into destination buffer.
// 0-terminate the destination buffer. Return the length of decoded data.
// form-url-encoded data differs from URI encoding in a way that it
// uses '+' as character for space, see RFC 1866 section 8.2.1
// http://ftp.ics.uci.edu/pub/ietf/html/rfc1866.txt
int g_url_decode(const char *src, int src_len, char *dst,
	int dst_len, int is_form_url_encoded)
{
	int i, j, a, b;
	if (src_len <= 0) src_len = strlen(src);
	
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

	for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) 
	{
		if (src[i] == '%' &&
			isxdigit(* (unsigned char *) (src + i + 1)) &&
			isxdigit(* (unsigned char *) (src + i + 2))) 
		{
			a = tolower(* (unsigned char *) (src + i + 1));
			b = tolower(* (unsigned char *) (src + i + 2));
			dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
			i += 2;
		}
		else if (is_form_url_encoded && src[i] == '+') {
			dst[j] = ' ';
		}
		else {
			dst[j] = src[i];
		}
	}
	
#undef HEXTOI
	
	dst[j] = '\0'; /* Null-terminate the destination */
	return j;
}

int g_url_encode(const char *src, char *dst, int dst_len) 
{
	static const char *dont_escape = "._-$,;~()";
	static const char *hex = "0123456789abcdef";
	const char *begin = dst;
	const char *end = dst + dst_len - 1;

	for (; *src != '\0' && dst < end; src++, dst++) 
	{
		if (isalnum(*(unsigned char *) src) ||
			strchr(dont_escape, * (unsigned char *) src) != NULL) 
		{
			*dst = *src;
		}
		else if (dst + 2 < end) 
		{
			dst[0] = '%';
			dst[1] = hex[(* (unsigned char *) src) >> 4];
			dst[2] = hex[(* (unsigned char *) src) & 0xf];
			dst += 2;
		}
	}
	*dst = '\0';
	return (dst - begin);
}

static int b64_decode(const char *src, int src_len, 
	unsigned char dst[], int dst_len)
{
	static const char _t256[256] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,63,-1,
		52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
		15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
		-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
		41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
	};
	char s[4] = {0, 0, 0, 0};
	int cnt = 0;
	const unsigned char *p;
	
	p = (const unsigned char *) src;
	switch (src_len)
	{
	case 4: if ((s[3]=_t256[(int)p[3]])==-1) return -1;
	case 3: if ((s[2]=_t256[(int)p[2]])==-1) return -1;
	case 2: if ((s[1]=_t256[(int)p[1]])==-1) return -1;
	case 1: if ((s[0]=_t256[(int)p[0]])==-1) return -1;
	}
	
	p = (const unsigned char *) s;
	if (dst_len>0) {
		dst[0] = (p[0] << 2) | (p[1] >> 4); 
		cnt = 1;
	}
	if (dst_len>1) {
		dst[1] = ((p[1] << 4) & 0xf0) | (p[2] >> 2);
		cnt = 2;
	}
	if (dst_len>2) {
		dst[2] = ((p[2] << 6) & 0xc0) | (p[3]);
		cnt = 3;
	}
	if (src_len==4) src_len = 3;
	return (cnt < src_len ? cnt : src_len);
}

int g_b64_decode(const char *src, int src_len, 
	unsigned char *dst, int dst_len)
{
	int j=0;
	if (src_len <= 0) src_len = strlen(src);
	
	while (src_len > 0 && j < dst_len)
	{
		int len = (src_len>=4 ? 4 : src_len);
		int ret = b64_decode(src, len, dst+j, dst_len-j);
		if (ret <= 0) return -1; // invalid
		j += ret;
		src += len; 
		src_len -= len;
	}
	return j;
}

// only alphanumerics, the special characters "$-_.+!*'(),", 
// and reserved characters used for their reserved purposes 
// may be used unencoded within a URL.
// @see URLs: http://www.rfc-editor.org/rfc/rfc1738.txt
static int b64_encode(const unsigned char *src, 
	int src_len, char dst[], int dst_len)
{
	static const char _t64[64] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M',
		'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
		'a','b','c','d','e','f','g','h','i','j','k','l','m',
		'n','o','p','q','r','s','t','u','v','w','x','y','z',
		'0','1','2','3','4','5','6','7','8','9','-','.'
	};
	int cnt = 0;
	unsigned char s[3] = {0, 0, 0};
	
	switch (src_len)
	{
	case 3: s[2] = src[2];
	case 2: s[1] = src[1];
	case 1: s[0] = src[0];
	}
	
	// QS: adopt 6 bits for encoding
	if (dst_len>0) {
		dst[0] = _t64[((s[0]>>2))];
		cnt = 1;
	}
	if (dst_len>1) {
		dst[1] = _t64[((s[0]&0x03)<<4)|(s[1]>>4)];
		cnt = 2;
	}
	if (dst_len>2) {
		dst[2] = _t64[((s[1]&0x0f)<<2)|(s[2]>>6)];
		cnt = 3;
	}
	if (dst_len>3) {
		dst[3] = _t64[((s[2]&0x3f))];
		cnt = 4;
	}
	src_len ++;
	return (cnt < src_len ? cnt : src_len);
}

int g_b64_encode(const unsigned char *src, 
	int src_len, char *dst, int dst_len)
{
	const char *begin = dst;
	const char *end = dst + dst_len - 1;
	
	while (src_len > 0 && dst < end)
	{
		int len = (src_len>=3 ? 3 : src_len);
		int ret = b64_encode(src, len, dst, end-dst);
		dst += ret;
		src += len; 
		src_len -= len;
	}
	*dst = '\0';
	return (dst - begin);
}


static char *do_escape_slash(char *str)
{
	for(; *str; str++) {
		if(*str=='\\') *str = '/';
	}
	return str;
}

int g_path_to_url(const char *path, char url[], int size)
{
	char full[512];
	return (g_full_path(path, full, sizeof(full))
		&&  g_strlcpy(url, "file://", size)
		&&  (!strchr(full, ':') || g_strlcat(url, "/", size))
		&&  g_strlcat(url, full, size)
		&&  do_escape_slash(url));
}

int g_url_to_path(const char *url, char path[], int size)
{
	if (0==g_strnicmp(url, "file://", 7))
	{
		const char *p = url + 7;
		if (*p != '/') p = strchr(p, '/');
		if (p) {
			if (strchr(p, ':')) p++;
			return g_strlcpy(path, p, size);
		}
	}
	return 0;
}


// simulates the CommandLineToArgvW() but with ANSI version
char **g_cmd_to_argv(const char *cmd, int* argc)
{
	int len = strlen(cmd);
	int i = ((len+2)/2) * sizeof(void *) + sizeof(void *);
	char**argv = (char**)malloc(i + (len+2)*sizeof(char));
	char *line = (char *)(((char *)argv)+i);
	int j, c, in_QUOT = 0, in_SPACE = 1;
	
	for (argv[c=0]=line, j=0; *cmd; cmd++) 
	{
		if (in_QUOT) {
			if (*cmd == '\"') in_QUOT = 0;
			else line[j++] = *cmd;
		}
		else {
			switch (*cmd) {
			case '\"':
				in_QUOT = 1;
				if (in_SPACE) argv[c++] = line+j;
				in_SPACE = 0; break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				if (!in_SPACE) line[j++] = '\0';
 				in_SPACE = 1; break;
			default:
				if (in_SPACE) argv[c++] = line+j;
				line[j++] = *cmd;
				in_SPACE = 0; break;
			}
		}
	}
	line[j] = '\0';
	argv[c] = (char *) 0;
	
	*argc = c;
	return argv;
}

int g_mkdir_recur(const char *dir)
{
	char *p, tmp[1024];
	
	if (!g_strlcpy(tmp, dir, sizeof(tmp))) return 0;
	
	for (p=tmp; *p; p++) {
		if(*p=='\\') *p = '/';
	}
	
	for (p=tmp; (p=strchr(p+1,'/')) && *(p+1); )
	{
		*p = '\0';
		if (!g_exists_dir(tmp) && 
			!g_create_dir(tmp)) return 0;
		*p = '/';
	}
	return g_create_dir(tmp);
}
//-------------------------------------------------------------------------
//------------------- (2) some OS-dependent API/Object --------------------
//-------------------------------------------------------------------------

#if (defined(WIN32) || defined(_WIN32))

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <io.h>
// ################################################################
/// Some microsoft compilers lack this definition.
#ifndef INVALID_FILE_ATTRIBUTES
# define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

int g_create_dir(const char *dir)
{
	return (CreateDirectory(dir, NULL)!=FALSE);
}

int g_delete_dir(const char *dir)
{
	return (RemoveDirectory(dir)!=FALSE);
}

int g_exists_dir(const char *dir)
{
	DWORD dwAttrs = GetFileAttributes(dir);
	if (dwAttrs==INVALID_FILE_ATTRIBUTES) return 0;
	return (dwAttrs & FILE_ATTRIBUTE_DIRECTORY);
}

int g_exists_file(const char *fn)
{
	DWORD dwAttrs = GetFileAttributes(fn);
	if (dwAttrs==INVALID_FILE_ATTRIBUTES) return 0;
	return (dwAttrs & FILE_ATTRIBUTE_ARCHIVE) &&
		!(dwAttrs & FILE_ATTRIBUTE_DIRECTORY);
}

int g_delete_file(const char *fn)
{
	return (DeleteFile(fn)!=FALSE);
}

int g_copy_file(const char *src, const char *dst)
{
	return (CopyFile(src, dst, FALSE)!=FALSE);
}

int g_move_file(const char *src, const char *dst)
{
	return (MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING)!=FALSE);
}

int g_full_path(const char *rel, char full[], int size)
{
	int nByte = GetFullPathName(rel, 0, 0, 0);
	if( size < nByte ) return 0;

	GetFullPathName(rel, size, full, 0);
	return 1;
}

// Implementation of POSIX opendir/closedir/readdir for Windows.
struct DIR_HANDLE
{
	WIN32_FIND_DATAW info;
	HANDLE handle;
	struct g_dirent_t result;
};

DIR_HANDLE *g_opendir(const char *name)
{
	DIR_HANDLE *ctx = NULL;
	wchar_t wpath[260];
	DWORD attrs;

	if (name == NULL) {
		SetLastError(ERROR_BAD_ARGUMENTS);
		return ctx;
	}
	
	ctx = (DIR_HANDLE *) malloc(sizeof(*ctx));
	if (ctx == NULL) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return ctx;
	}
	
	(void) MultiByteToWideChar(CP_UTF8, 0, name, -1, 
		wpath, ARRAY_SIZE(wpath));
	attrs = GetFileAttributesW(wpath);
	if (attrs != 0xFFFFFFFF && (attrs & FILE_ATTRIBUTE_DIRECTORY)) 
	{
		(void) wcscat(wpath, L"\\*");
		ctx->handle = FindFirstFileW(wpath, &ctx->info);
		ctx->result.dname[0] = '\0';
		return ctx;
	}
	free(ctx); return NULL;
}

int g_closedir(DIR_HANDLE *ctx)
{
	if (ctx) {
		HANDLE h = ctx->handle;
		free(ctx);
		return (h==INVALID_HANDLE_VALUE || FindClose(h));
	}
	SetLastError(ERROR_BAD_ARGUMENTS);
	return 0;
}

static time_t sys2unix(DWORD hi, DWORD lo)
{
	uint64_t ts = (uint64_t) hi << 32;
	ts |= lo;
	ts -= UINT64_C(0x019db1ded53e8000);
	return (time_t)(ts/10000000);
}

struct g_dirent_t *g_readdir(DIR_HANDLE *ctx)
{
	struct g_dirent_t *ret;
	
	if (ctx == NULL) {
		SetLastError(ERROR_BAD_ARGUMENTS);
		return NULL;
	}
	
	if (ctx->handle == INVALID_HANDLE_VALUE)
	{
		SetLastError(ERROR_FILE_NOT_FOUND);
		return NULL;
	}

	ret = &ctx->result;
	(void) WideCharToMultiByte(CP_UTF8, 0, 
		ctx->info.cFileName, -1, 
		ret->dname, sizeof(ret->dname), 0, 0);
	
	ret->fsize = (uint64_t) ctx->info.nFileSizeHigh << 32;
	ret->fsize |= ctx->info.nFileSizeLow;
	ret->mtime = sys2unix(
		ctx->info.ftLastWriteTime.dwHighDateTime,
		ctx->info.ftLastWriteTime.dwLowDateTime);
	ret->is_dir = (0 != (ctx->info.dwFileAttributes & 
		FILE_ATTRIBUTE_DIRECTORY));
	
	if (!FindNextFileW(ctx->handle, &ctx->info))
	{
		(void) FindClose(ctx->handle);
		ctx->handle = INVALID_HANDLE_VALUE;
	}
	return ret;
}



int g_disk_info(const char *dir, uint64_t *_free, uint64_t *_total)
{
	ULARGE_INTEGER ul_free, ul_total;
	if (!GetDiskFreeSpaceEx(dir, &ul_free, &ul_total, 0)) return 0;
	if (_free) *_free = ul_free.QuadPart;
	if (_total) *_total = ul_total.QuadPart;
	return 1;
}

int g_mem_phys(uint64_t *_free, uint64_t *_total)
{
	MEMORYSTATUSEX st;
	st.dwLength = sizeof (st);
	if (!GlobalMemoryStatusEx(&st)) return 0;
	
	if (_free) *_free = st.ullAvailPhys;
	if (_total) *_total = st.ullTotalPhys;
	return 1;
}

int g_mem_virt(uint64_t *_free, uint64_t *_total)
{
	MEMORYSTATUSEX st;
	st.dwLength = sizeof (st);
	if (!GlobalMemoryStatusEx(&st)) return 0;
	
	if (_free) *_free = st.ullAvailVirtual;
	if (_total) *_total = st.ullTotalVirtual;
	return 1;
}

int g_load_avg(void)
{
	return 0;
}

int g_cpu_usage(char tick[16], int *out)
{
	typedef struct {
		DWORD   dwUnknown1;
		ULONG   uKeMaximumIncrement;
		ULONG   uPageSize;
		ULONG   uMmNumberOfPhysicalPages;
		ULONG   uMmLowestPhysicalPage;
		ULONG   uMmHighestPhysicalPage;
		ULONG   uAllocationGranularity;
		PVOID   pLowestUserAddress;
		PVOID   pMmHighestUserAddress;
		ULONG   uKeActiveProcessors;
		BYTE    bKeNumberProcessors;
		BYTE    bUnknown2;
		WORD    wUnknown3;
	} SYSTEM_BASIC_INFORMATION;

	typedef struct {
		LARGE_INTEGER   liIdleTime;
		DWORD           dwSpare[76];
	} SYSTEM_PERFORMANCE_INFORMATION;

	typedef struct {
		LARGE_INTEGER liKeBootTime;
		LARGE_INTEGER liKeSystemTime;
		LARGE_INTEGER liExpTimeZoneBias;
		ULONG         uCurrentTimeZoneId;
		DWORD         dwReserved;
	} SYSTEM_TIME_INFORMATION;	
	
	static int NumberOfProcessors = 0;
	SYSTEM_PERFORMANCE_INFORMATION _perf;
	SYSTEM_TIME_INFORMATION _time;
	
	int64_t *_tick = (int64_t *) tick;

	typedef LONG (WINAPI *PROC)(UINT, PVOID, ULONG, PULONG);
	PROC NtQuerySystemInformation = (PROC) GetProcAddress (
		GetModuleHandle("ntdll"), "NtQuerySystemInformation");
	if (!NtQuerySystemInformation) return 0;

	if (NO_ERROR != NtQuerySystemInformation(
		2, &_perf, sizeof(_perf), NULL)) return 0;

	if (NO_ERROR != NtQuerySystemInformation(
		3, &_time, sizeof(_time), NULL)) return 0;
	
	if (0 == NumberOfProcessors) {
		SYSTEM_BASIC_INFORMATION _base;
		if (NO_ERROR != NtQuerySystemInformation(
			0, &_base, sizeof(_base), NULL)) return 0;
		NumberOfProcessors = _base.bKeNumberProcessors;
	}

	// if it's a first call (out=NULL) - skip it!
	if (out) {
		// diff = NewValue - OldValue
		double idle = _perf.liIdleTime.QuadPart - _tick[0];
		double syst = _time.liKeSystemTime.QuadPart - _tick[1];
		
		// CCpuUsage = 1 - (CpuIdle%) / NumberOfProcessors
		double rate = 1 - (idle/syst)/NumberOfProcessors;
		*out = (int) (rate*10000.0 + 0.5); // %%%
	}

	// store new CPU's idle and system time
	_tick[0] = _perf.liIdleTime.QuadPart;
	_tick[1] = _time.liKeSystemTime.QuadPart;
	return 1;
}
// ################################################################
int g_flock(int fd, int op)
{
	HANDLE hh = (HANDLE) _get_osfhandle(fd);
	DWORD low = 1, high = 0;
	OVERLAPPED offset;
	DWORD dwFlags = (op & LOCK_NB) ? LOCKFILE_FAIL_IMMEDIATELY : 0;
	ZeroMemory(&offset, sizeof(offset));

	// error in file descriptor?
	if (hh < 0) return 0;

	// bug for bug compatible with Unix
	UnlockFileEx(hh, 0, low, high, &offset);

	switch (op & ~LOCK_NB) {
		case LOCK_EX: // exclusive
			dwFlags |= LOCKFILE_EXCLUSIVE_LOCK;
		case LOCK_SH: // shared
			if (LockFileEx(hh,
				dwFlags, 0, low, high, &offset)) return 1;
			break;
		case LOCK_UN: // unlock, always succeeds
			return 1;
	}
	return 0;
}

int g_flock2(void* fp, int op)
{
	return g_flock(_fileno((FILE *)fp), op);
}

// ################################################################
/// a structure for containing all mutex information
struct g_mutex_t {
	long refs;  ///< Number of enterances
	CRITICAL_SECTION cs; ///< Mutex controlling the lock
	DWORD owner;         ///< Thread holding this mutex
};

g_mutex_t *g_mutex_init(void)
{
	void *t = calloc(1, sizeof(g_mutex_t));
	g_mutex_t *p = (g_mutex_t *) t;
	if (p) InitializeCriticalSection(&p->cs);
	return p;
}

void g_mutex_free(g_mutex_t *p)
{
	assert(p && p->refs==0);
	DeleteCriticalSection(&p->cs);
	free(p); // release all!
}

int  g_mutex_held(g_mutex_t *p)
{
	assert(p);
	return (p->refs>0 && p->owner==GetCurrentThreadId());
}

void g_mutex_enter(g_mutex_t *p)
{
	assert(p);
	EnterCriticalSection(&p->cs);
	p->owner = GetCurrentThreadId();
	p->refs++;
}

int g_mutex_try_enter(g_mutex_t *p, double sec)
{
	double end = time(NULL) + sec;
	assert(p);
	do {
		if (TryEnterCriticalSection(&p->cs))
		{
			p->owner = GetCurrentThreadId();
			return (++p->refs);
		}
	} while(Sleep(1), time(NULL)<end);

	return 0; // failed to lock!
}

void g_mutex_leave(g_mutex_t *p)
{
	assert(p && g_mutex_held(p));
	p->refs--;
	LeaveCriticalSection(&p->cs);
}

// ################################################################
g_thread_t g_thread_start(void* (*func)(void *), void *user)
{
	HANDLE h = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE) func, user, 0, NULL);
	return (g_thread_t) h;
}

int g_thread_join(g_thread_t th, long *ret)
{
	HANDLE h = (HANDLE) th;
	assert(th);
	if (WaitForSingleObject(h, INFINITE)==WAIT_FAILED) return 0;
	if (ret) {
		DWORD ec = 0;
		GetExitCodeThread(h, &ec);
		*ret = ec;
	}
	CloseHandle(h); return 1;
}

int g_thread_start_detached(void* (*func)(void *), void *user)
{
	HANDLE h = CreateThread(NULL, 0,
		(LPTHREAD_START_ROUTINE) func, user, 0, NULL);
	if (!h) return 0;
	CloseHandle(h); return 1; // ref--
}

int g_thread_sleep(double sec)
{
	if (sec <= 0) {Sleep(0); return 1;}
	sec *= 1000;
	if (sec > INT32_MAX) sec = INT32_MAX;
	Sleep((int32_t)sec); return 1;
}

void g_thread_yield() {Sleep(0);}

void g_thread_pause() {
#warning "g_thread_pause() is not implemented."
}

void g_thread_exit(long ret) {ExitThread(ret);}

long g_thread_id() {return (long)GetCurrentThreadId();}

int g_thread_bind(int cpu, const char *name) {return 0;}

long g_process_id() { return (long)GetCurrentProcessId();}

// ################################################################
long g_lock_set(long *ptr, long new_val)
{
	return InterlockedExchange(ptr, new_val);
}

long g_lock_add(long *ptr, long inc_val)
{
	return InterlockedExchangeAdd(ptr, inc_val);
}

long g_ifeq_set(long *ptr, long cmp, long new_val)
{
	return InterlockedCompareExchange(ptr, new_val, cmp);
}

// ################################################################
int64_t g_now_hr()
{
	static time_t start = 0;
	static LARGE_INTEGER freq;
	static LARGE_INTEGER tick;
	LARGE_INTEGER count;

	if (start == 0)
	{
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&tick);
		start = time(NULL);
		Sleep(0); // yield!
	}
	
	QueryPerformanceCounter(&count);
	return (int64_t)start * (int64_t)1000000 +
		(count.QuadPart - tick.QuadPart) * (int64_t)1000000 / freq.QuadPart;
}

// refer: http://gears.googlecode.com/svn/trunk/gears/base/common/
int64_t g_now_us()
{
	uint64_t utc;
	FILETIME ft;
#if defined(_WIN32_WCE)
	SYSTEMTIME st;
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
#else
	GetSystemTimeAsFileTime(&ft);
#endif
	utc = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	return (utc - UINT64_C(116444736000000000)) / 10;
}

int64_t g_now_ms()
{
	return g_now_us() / 1000;
}

int g_timezone()
{
	TIME_ZONE_INFORMATION tz;
	GetTimeZoneInformation(&tz);
	return tz.Bias/-60;
}

int gettimeofday(struct timeval* tv, void *tz)
{
	int64_t usec = g_now_us();
	tv->tv_sec =  (int)(usec/1000000);
	tv->tv_usec = (int)(usec%1000000);
	UNUSED_PARAMETER(tz);
	return 0;
}
// ################################################################
struct g_share_t {
	HANDLE mutex;
	HANDLE r_evt;
	HANDLE w_evt;
	volatile int32_t tp;
	volatile int32_t rc;
	volatile int32_t wc;
};

g_share_t *g_share_init(void)
{
	g_share_t *s = (g_share_t *) malloc(sizeof(g_share_t));
	assert(s);
	s->mutex = CreateMutex(NULL, FALSE, NULL);
	s->r_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
	s->w_evt = CreateEvent(NULL, FALSE, FALSE, NULL);
	s->tp = 0;
	s->rc = s->wc = 0;
	return s;
}

void g_share_free(g_share_t *s)
{
	assert(s);
	CloseHandle(s->mutex);
	CloseHandle(s->r_evt);
	CloseHandle(s->w_evt);
	free(s);
}

void g_share_lockw(g_share_t *s)
{
	g_share_try_lockw(s, -1);
}

int  g_share_try_lockw(g_share_t *s, double sec)
{
	DWORD dw = (sec<0 ? INFINITE : (DWORD)(sec*1000));
	WaitForSingleObject(s->mutex, INFINITE);
	++s->wc;
	if (s->tp != 0)
	{
		DWORD ret = SignalObjectAndWait(
			s->mutex, s->w_evt, dw, FALSE);
		if (ret == WAIT_OBJECT_0) return 1;
		if (ret == WAIT_TIMEOUT) SetLastError(WAIT_TIMEOUT);
		return 0;
	}
	s->tp = 2;
	ReleaseMutex(s->mutex);
	return 1;
}

void g_share_lockr(g_share_t *s)
{
	g_share_try_lockr(s, -1);
}

int  g_share_try_lockr(g_share_t *s, double sec)
{
	DWORD dw = (sec<0 ? INFINITE : (DWORD)(sec*1000));
	WaitForSingleObject(s->mutex, INFINITE);
	++s->rc;
	if (s->tp == 2)
	{
		DWORD ret = SignalObjectAndWait(
			s->mutex, s->r_evt, dw, FALSE);
		if (ret == WAIT_OBJECT_0) return 1;
		if (ret == WAIT_TIMEOUT) SetLastError(WAIT_TIMEOUT);
		return 0;
	}
	s->tp = 1;
	ReleaseMutex(s->mutex);
	return 1;
}

void g_share_unlock(g_share_t *s)
{
	WaitForSingleObject(s->mutex, INFINITE);
	if (s->tp == 2)
	{
		--s->wc;
		if (s->wc > 0) SetEvent(s->w_evt);
		else if (s->rc > 0) { s->tp = 1; PulseEvent(s->r_evt); }
		else s->tp = 0;
	}
	else if (s->tp == 1)
	{
		--s->rc;
		if (s->rc == 0)
		{
			if (s->wc > 0) { s->tp = 2; SetEvent(s->w_evt); }
			else s->tp = 0;
		}
	}
	ReleaseMutex(s->mutex);
}

// ################################################################
struct g_atom_t {
	LONG   sem; ///< semaphore
	volatile int32_t wc; ///< count of writers
	volatile int32_t rc; ///< count of readers
};

void g_atom_enter(g_atom_t *s)
{
	while (InterlockedCompareExchange(
		&s->sem, 1, 0) != 0) Sleep(0);
}

void g_atom_leave(g_atom_t *s)
{
	InterlockedExchange(&s->sem, 0);
}

// ################################################################
struct g_sema_t {HANDLE sem;};

g_sema_t *g_sema_init(uint32_t cap, uint32_t cnt)
{
	HANDLE h;
	if (cnt > cap) cap = cnt;
	
	h = CreateSemaphore(
		NULL,  // default security attributes
		cnt,   // initial count
		cap,   // maximum count
		NULL); // unnamed semaphore

 	if (h) {
		g_sema_t *s = (g_sema_t *) malloc(sizeof(g_sema_t));
		assert(s);
		s->sem = h;
		return s;
 	}
 	return NULL;
}

void g_sema_free(g_sema_t *s)
{
	if (s) {
		CloseHandle(s->sem);
		free(s);
	}
}

int	g_sema_post(g_sema_t *s)
{
	assert(s);
	return (ReleaseSemaphore(s->sem, 1, NULL)!=FALSE);
}

int	g_sema_wait(g_sema_t *s, double sec)
{
	DWORD dw = (sec<0 ? INFINITE : (DWORD)(sec*1000));
	assert(s);
	return (WaitForSingleObject(s->sem, dw)==WAIT_OBJECT_0);
}
// ################################################################
struct g_cond_t {
	CRITICAL_SECTION mutex; ///< mutex object
	uint32_t wait;          ///< wait count
	uint32_t wake;          ///< wake count
	HANDLE sev;             ///< signal event handle
	HANDLE fev;             ///< finish event handle
};

/// @brief initialize
g_cond_t *g_cond_init(void)
{
	g_cond_t *gc =
		(g_cond_t *) calloc(1, sizeof(g_cond_t));
	InitializeCriticalSection(&gc->mutex);
	gc->wait = 0;
	gc->wake = 0;
	gc->sev = CreateEvent(NULL, TRUE, FALSE, NULL);
	assert(gc->sev);
	gc->fev = CreateEvent(NULL, FALSE,FALSE, NULL);
	assert(gc->fev);
	return gc;
}

/// @brief deconstruct
void g_cond_free(g_cond_t *gc)
{
	assert(gc);
	CloseHandle(gc->fev);
	CloseHandle(gc->sev);
	DeleteCriticalSection(&gc->mutex);
	free(gc);
}

/// @brief Wait for the signal.
int g_cond_wait(g_cond_t *gc, g_mutex_t *mtx, double sec)
{
	assert(gc);

	gc->wait++;
	LeaveCriticalSection(&mtx->cs);
	while (1) {
		if (sec >= 0) {
			DWORD dw = (DWORD)(sec*1000);
			if (WaitForSingleObject(gc->sev, dw)==WAIT_TIMEOUT)
			{
				EnterCriticalSection(&mtx->cs);
				gc->wait--; return 0;
			}
		}
		else {
			WaitForSingleObject(gc->sev, INFINITE);
		}
		EnterCriticalSection(&gc->mutex);
		if (gc->wake > 0) {
			gc->wait--;
			gc->wake--;
			if (gc->wake < 1) {
				ResetEvent(gc->sev);
				SetEvent(gc->fev);
			}
			LeaveCriticalSection(&gc->mutex);
			break;
		}
		LeaveCriticalSection(&gc->mutex);
	}
	EnterCriticalSection(&mtx->cs);
	return 1;
}

/// @brief Send the wake-up signal to another waiting thread.
void g_cond_signal(g_cond_t *gc)
{
	assert(gc);
	if (gc->wait > 0) {
		gc->wake = 1;
		SetEvent(gc->sev);
		WaitForSingleObject(gc->fev, INFINITE);
	}
}

/// @brief Send the wake-up signal to all waiting threads.
void g_cond_broadcast(g_cond_t *gc)
{
	assert(gc);
	if (gc->wait > 0) {
		gc->wake = gc->wait;
		SetEvent(gc->sev);
		WaitForSingleObject(gc->fev, INFINITE);
	}
}
// ################################################################

#ifndef INVALID_SET_FILE_POINTER
# define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

#ifndef LOCKFILE_FAIL_IMMEDIATELY
# define LOCKFILE_FAIL_IMMEDIATELY 1
#endif

struct f64_t {
	HANDLE  fd; ///< handle for accessing the file
	g_mutex_t *m;  ///< mutex locker for W/R
	int32_t  r; ///< a flag for read-only access
	uint32_t e; ///< number for GetLastError()
};

#if _WIN32_WCE
#define isNT()  (1)
#else
static int isNT(void)
{
	static int os_type = 0;
	if (os_type==0) {
		OSVERSIONINFO ver;
		ver.dwOSVersionInfoSize = sizeof(ver);
		GetVersionEx(&ver);
		os_type = ver.dwPlatformId==VER_PLATFORM_WIN32_NT ? 2 : 1;
	}
	return (os_type==2);
}
#endif

/// translate UTF8 to Unicode
static WCHAR *utf8_to_uni(const char *fn)
{
	WCHAR *wz;
	int nChar;
	nChar = MultiByteToWideChar(CP_UTF8, 0, fn, -1, NULL, 0);

	wz = (WCHAR *) malloc(nChar*sizeof(wz[0]));
	if( wz==0 ) return 0;

	nChar = MultiByteToWideChar(CP_UTF8, 0, fn, -1, wz, nChar);
	if (nChar==0) {	free(wz); wz = 0;}
	return wz;
}

static f64_t *do_alloc(HANDLE h, int ro)
{
	f64_t *f = (f64_t *) malloc(sizeof(f64_t));
	assert (f!=NULL);
	f->fd = h;   f->r = ro;
	f->e = NO_ERROR;
	f->m = g_mutex_init();
	assert (f->m); return f;
}

f64_t *f64open(const char *fn, int create)
{
	DWORD dwCreation =
		(create ? OPEN_ALWAYS : OPEN_EXISTING);
	DWORD dwFlags =
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	WCHAR *wz;
	if (isNT() && (wz=utf8_to_uni(fn))!=0)
	{
		int read_only = 0;
		HANDLE h = CreateFileW(
			wz,
			GENERIC_READ | GENERIC_WRITE, // Desired Access,
			FILE_SHARE_READ | FILE_SHARE_WRITE, // Sharing Mode
			NULL,
			dwCreation, // Creation Disposition
			dwFlags,    // Flags And Attributes
			NULL);

		if (h==INVALID_HANDLE_VALUE)
		{
			h = CreateFileW(
				wz,
				GENERIC_READ, // Desired Access,
				FILE_SHARE_READ | FILE_SHARE_WRITE, // Sharing Mode
				NULL,
				dwCreation, // Creation Disposition
				dwFlags,    // Flags And Attributes
				NULL);
			read_only = 1;
		}
		free(wz);
		return (h==INVALID_HANDLE_VALUE ?
			NULL : do_alloc(h, read_only));
	}
	return NULL;
}

/// @brief close a file.
/// It is reported that an attempt to close a handle might sometimes
/// fail.  This is a very unreasonable result, but windows is notorious
/// for being unreasonable so I do not doubt that it might happen.  If
/// the close fails, we pause for 100 milliseconds and try again.
int f64close(f64_t *f)
{
	const int MAX_TRY = 3;
	int cnt = 0;
	do {
		if (CloseHandle(f->fd)) {
			g_mutex_free(f->m);
			free(f); return 1;
		}
	} while (++cnt < MAX_TRY && (Sleep(100),1));
	return 0;
}

int f64read(f64_t *f, void *ptr, int amt, int64_t offset)
{
	LONG upper = (LONG)((offset>>32) & 0x7fffffff);
	LONG lower = (LONG)((offset) & 0xffffffff);
	DWORD got = 0, rc;

	g_mutex_enter(f->m);
	rc = SetFilePointer(f->fd, lower, &upper, FILE_BEGIN);
	if (rc==INVALID_SET_FILE_POINTER &&
		(f->e=GetLastError())!=NO_ERROR)
	return (g_mutex_leave(f->m), 0);

	rc = ReadFile(f->fd, ptr, amt, &got, 0);
	g_mutex_leave(f->m);
	return (rc ? got : (f->e=GetLastError(),0));
}

int f64write(f64_t *f, const void *ptr, int amt, int64_t offset)
{
	LONG upper = (LONG)((offset>>32) & 0x7fffffff);
	LONG lower = (LONG)((offset) & 0xffffffff);
	DWORD got, wrote=0, rc;

	g_mutex_enter(f->m);
	rc = SetFilePointer(f->fd, lower, &upper, FILE_BEGIN);
	if (rc==INVALID_SET_FILE_POINTER &&
		(f->e=GetLastError())!=NO_ERROR)
	return (g_mutex_leave(f->m), 0);

	while (amt>0 &&
		(rc=WriteFile(f->fd,ptr,amt,&got,0))!=0 && got>0)
	{
		wrote += got;
		amt -= got;
		ptr = &((char*)ptr)[got];
	}
	g_mutex_leave(f->m);
	if (!rc) f->e = GetLastError();
	return wrote;
}

/// MACRO: GetVLFilePointer()
/// @note no meaningful for multiple thread!
int f64tell(f64_t *f, int64_t *offset)
{
	LONG upper = 0;
	DWORD rc = SetFilePointer(f->fd, 0, &upper, FILE_CURRENT);
	if (rc==INVALID_SET_FILE_POINTER &&
		(f->e=GetLastError())!=NO_ERROR) return 0;
	*offset = (((int64_t)upper)<<32) | rc;
	return 1;
}

int f64sync(f64_t *f, int flag)
{
	UNUSED_PARAMETER(flag);
	return (FlushFileBuffers(f->fd)?1:(f->e=GetLastError(),0));
}

// SetEndOfFile will fail if "size" is negative!
int f64trunc(f64_t *f, int64_t size)
{
	LONG upper = (LONG)((size>>32) & 0x7fffffff);
	LONG lower = (LONG)((size) & 0xffffffff);
	DWORD rc;

	g_mutex_enter(f->m);
	rc = SetFilePointer(f->fd, lower, &upper, FILE_BEGIN);
	if (rc==INVALID_SET_FILE_POINTER &&
		(f->e=GetLastError())!=NO_ERROR)
	return (g_mutex_leave(f->m), 0);

	rc = SetEndOfFile(f->fd);
	g_mutex_leave(f->m);
	return (rc ? 1:(f->e=GetLastError(),0));
}

/// find the size of a specific file
/// @note no meaningful for multiple thread!
int64_t f64size(f64_t *f)
{
	DWORD upper = 0;
	DWORD rc = GetFileSize(f->fd, &upper);
	if (rc==INVALID_SET_FILE_POINTER &&
		(f->e=GetLastError())!=NO_ERROR) return 0;
	return (((int64_t)upper)<<32) | rc;
}

int f64lock(f64_t *f, int op)
{
	DWORD low=1, high=0;
	DWORD flag=(op&LOCK_NB)?LOCKFILE_FAIL_IMMEDIATELY:0;
	OVERLAPPED lap;
	
	// bug for bug compatible with Unix
	ZeroMemory(&lap, sizeof(lap)); 
	UnlockFileEx(f->fd, 0, low, high, &lap);

	switch (op & ~LOCK_NB)
	{
	case LOCK_EX: // exclusive
		flag |= LOCKFILE_EXCLUSIVE_LOCK;
	case LOCK_SH: // shared
		if (LockFileEx(f->fd,flag,0,low,high,&lap)) return 1;
		break;
	case LOCK_UN: return 1; // unlock, always succeeds
	}
	return 0;
}

// ################################################################
// we use unnamed mapping objects to be compatiable for Unix/Linux
static shm_t *shm_open_impl(
	HANDLE hFile, void *start, uint32_t len, int forced)
{
	SYSTEM_INFO info;
	HANDLE hMapping;
	void *ptr;
	shm_t *map;

	// MAKE the returned handle can be inherited by child processes.
	SECURITY_ATTRIBUTES sa;
	DWORD unit;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	GetSystemInfo(&info);
	unit = info.dwPageSize;

	if (hFile) {
		DWORD cc, lSizeHigh = 0;
		cc = GetFileSize(hFile, &lSizeHigh); // file size

		if (start == SHM_LOAD) { // struct mb_man_t will be happy!
			DWORD got = 0;
			DWORD siz = sizeof(void *);
			if (lSizeHigh) return NULL;

			start = NULL; // SHM_NULL
			if (cc >= siz) {
				SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
				ReadFile(hFile, &start, siz, &got, NULL);
				if (got!=siz) start = NULL;
			}
			if (!start) {
				if (forced) {CloseHandle(hFile); return NULL;}
			}
			if((DWORD)len < cc) len = cc;
		}

		if (!len) { //default: filesize or pagesize
			if (lSizeHigh) return NULL;
			if(!(len=cc)) len = unit;
		}

		// opens an unnamed file mapping object for a specified file.
		hMapping = CreateFileMapping(
			hFile,            // use paging file
			&sa,              // security
			PAGE_READWRITE,   // read/write access
			0,
			len,
			NULL);            // unnamed mapping object
	}
	else {
		if (!len) len = unit;

		// creates a file mapping object of a specified size that is backed
		// by the system paging file instead of by a file in the file system
		hMapping = CreateFileMapping(
			INVALID_HANDLE_VALUE, // use paging file
			&sa,                  // security
			PAGE_READWRITE,       // read/write access
			0,
			len,
			NULL);                // unnamed mapping object
	}

	// DO NOT check: GetLastError()==ERROR_ALREADY_EXISTS.
	// we use CreateFileMapping instead of OpenFileMapping.
	// If the function fails, the return value is NULL.
	if (hMapping==NULL) return NULL;

	// Create a file mapping view
	ptr = MapViewOfFileEx(hMapping,
			FILE_MAP_ALL_ACCESS, 0, 0, len, start);

	if (!ptr && !forced) { // not initialized with an address
		DWORD ag = info.dwAllocationGranularity;
		char *base = (char *) start;

		// Note: The Base Address must be a multiple of system's
		// memory allocation granularity, or the function fails.
		base = (char *) (((ULONG) base + ag-1) & (~(ag-1)));
		if (base == start) base += ag;

		while (base) {
			ptr = MapViewOfFileEx(hMapping,
				FILE_MAP_ALL_ACCESS, 0, 0, len, base);
			if (ptr) break;
			else base += ag;
		}
	}
	if (!ptr) {
		CloseHandle(hMapping); return NULL;
	}

	map = (shm_t *) malloc(sizeof(shm_t));
	assert(map!=NULL);
	map->ptr = (uint8_t *) ptr;
	map->len = len;
	map->owner = hMapping;
	return map;
}

shm_t *g_shm_open(
	const char *fn, void *start, uint32_t len, int forced)
{
	shm_t *map;
	WCHAR *wz;
	if (fn && isNT() && (wz=utf8_to_uni(fn))!=0) {
		HANDLE hFile = CreateFileW(
			wz,
			GENERIC_WRITE|GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_FLAG_SEQUENTIAL_SCAN,
			NULL);

		free(wz);
		if (hFile==INVALID_HANDLE_VALUE) return NULL;
		map = shm_open_impl(hFile, start, len, forced);
		CloseHandle(hFile); // release this original, DO: nRef--
	}
	else {
		map = shm_open_impl(NULL, start, len, forced);
	}
	return map;
}

int g_shm_close(shm_t *map)
{
	if (!map || !map->ptr) return 0;
	if (!UnmapViewOfFile(map->ptr)) return 0;
	CloseHandle(map->owner);
	free(map);  return 1;
}

int g_shm_sync(const shm_t *map, uint32_t s, uint32_t c)
{
	if (!map || !map->ptr) return 0;
	if (c<=0) {s=0; c=map->len;}
	else {
		if (s >= map->len) return 0;
		if (c > map->len - s) c = map->len - s;
	}
	return(FALSE!=FlushViewOfFile(map->ptr+s, c));
}

int g_shm_unit(void)
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwAllocationGranularity;
}


#else // OS=!WIN32

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <sys/syscall.h>
#include <sys/vfs.h>
#include <sys/prctl.h>

#define __USE_GNU 1
#include <sched.h>
#include <pthread.h>
// ################################################################
#ifndef O_LARGEFILE
#  define O_LARGEFILE 0
#endif
#ifndef O_BINARY
# define O_BINARY 0
#endif

int g_create_dir(const char *dir)
{
#if defined(__MINGW32__)
	return (mkdir(dir)==0);
#else
	return (mkdir(dir,0775)==0);
#endif
}

int g_delete_dir(const char *dir)
{
	return (rmdir(dir)==0);
}

int g_exists_dir(const char *dir)
{
	struct stat buf;
	return (stat(dir,&buf)==0 && S_ISDIR(buf.st_mode));
}

int g_exists_file(const char *fn)
{
	return (access(fn, F_OK)==0);
}

int g_delete_file(const char *fn)
{
	return (unlink(fn)==0);
}

int g_copy_file(const char *src, const char *dst)
{
	char buf[8*1024];
	int s, d, ret=1;
	s = open(src, O_RDONLY|O_LARGEFILE|O_BINARY);
	if (s<0) {return 0;}
	d = open(dst, O_WRONLY|O_CREAT|O_TRUNC|O_LARGEFILE|O_BINARY, S_IREAD|S_IWRITE);
	if (d<0) {close(s); return 0;}
	for(;;) {
		int got = read(s, buf, sizeof(buf));
		if (got< 0) {ret=0; break;}
		if (got==0) {break;}
		if (got!=write(d, buf, got)) {ret=0; break;}
	}
	close(s); close(d);	return ret;
}

int g_move_file(const char *src, const char *dst)
{
	return (rename(src, dst)==0);
}

int g_full_path(const char *rel, char full[], int size)
{
	if (rel[0] == '/') {
		g_strlcpy(full, rel, size);
	}
	else {
	    if( getcwd(full, size-1)==0 ) return 0;
	    g_strlcat(full, "/", size);
	    g_strlcat(full, rel, size);
    }
    return 1;
}


#include <dirent.h>
struct DIR_HANDLE
{
	DIR *dir;
	struct g_dirent_t result;
	char path[260];
};

DIR_HANDLE *g_opendir(const char *name)
{
	DIR_HANDLE *ctx = NULL;
	DIR *dir = opendir(name);
	if (!dir) return NULL;
	
	ctx = (DIR_HANDLE *) malloc(sizeof(*ctx));
	ctx->dir = dir;
	ctx->result.dname[0] = '\0';
	
	if (g_strlcpy(ctx->path, name, sizeof(ctx->path)))
	{
		int i = strlen(ctx->path)-1;
		if (ctx->path[i] == '/') ctx->path[i] = 0;
	}
	
	return ctx;
}

int g_closedir(DIR_HANDLE *ctx)
{
	if (ctx) {
		DIR *dir = ctx->dir;
		free(ctx);
		return (dir==NULL || closedir(dir)==0);
	}
	return 0;
}

struct g_dirent_t *g_readdir(DIR_HANDLE *ctx)
{
	struct stat st;
	struct g_dirent_t *ret;
	struct dirent *ent;
	char tmp[512];
	
	if (!ctx || !ctx->dir) return NULL;
	
	ent = readdir(ctx->dir);
	if (!ent) {
		ctx->result.dname[0] = '\0';
		return NULL;
	}
	
	ret = &ctx->result;
	strcpy(ret->dname, ent->d_name);
	
	sprintf(tmp, "%s/%s", ctx->path, ent->d_name);
	if (stat(tmp, &st) == 0) {
		ret->fsize = st.st_size;
		ret->mtime = st.st_mtime;
		ret->is_dir = S_ISDIR(st.st_mode);
	}
	else {
		ret->fsize = ret->mtime = ret->is_dir = 0;
	}
	return ret;
}


int g_disk_info(const char *dir, uint64_t *_free, uint64_t *_total)
{
	struct statfs buf;
	if (statfs(dir, &buf) != 0) return 0;

	if (_free) *_free = (uint64_t)(buf.f_bfree) * buf.f_bsize;
	if (_total) *_total = (uint64_t)(buf.f_blocks) * buf.f_bsize;
	return 1;
}

static int mem_read_key(
	const char *buf, const char *key, uint64_t *val)
{
	char name[64], unit[64]="";
	long long x = 0;
	char *ptr = strstr((char *)buf, key);
	if (!ptr) return 0;

	if (sscanf(ptr, "%s %lld %s", name, &x, unit)<2) return 0;
	else if (0==g_stricmp(unit, "KB")) *val = (x<<10);
	else if (0==g_stricmp(unit, "MB")) *val = (x<<20);
	else if (0==g_stricmp(unit, "GB")) *val = (x<<30);
	return 1;
}

int g_mem_phys(uint64_t *_free, uint64_t *_total)
{
	char buf[8192];
	FILE *fp = fopen("/proc/meminfo", "rb");
	if (fp) {
		size_t got = fread(buf, 1, sizeof(buf)-1, fp);
		fclose(fp);
		buf[got] = '\0';

		if (_free && !mem_read_key(buf, "MemFree:", _free)) return 0;
		if (_total && !mem_read_key(buf, "MemTotal:", _total)) return 0;
		return 1;
	}
	return 0;
}

int g_mem_virt(uint64_t *_free, uint64_t *_total)
{
	char buf[8192];
	FILE *fp = fopen("/proc/meminfo", "rb");
	if (fp) {
		uint64_t total = 0, used = 0;
		size_t got = fread(buf, 1, sizeof(buf)-1, fp);
		fclose(fp);
		buf[got] = '\0';

		if (!mem_read_key(buf, "VmallocTotal:", &total)) return 0;
		if (!mem_read_key(buf, "VmallocUsed:", &used)) return 0;
		if (_total) *_total = total;
		if (_free) *_free = total - used;
		return 1;
	}
	return 0;
}

int g_load_avg(void)
{
	int val = 0;
	double d;
	if (getloadavg(&d, 1) == 1)
	{
		val = (int) (d * 100);
		if (val < 10) val = 10;
	}
	return val;
}

static int read_cpu_time(uint32_t t[4])
{
	int ret = 0;
	FILE *fp = fopen("/proc/stat", "rb");
	if (!fp) return 0;
	ret = (4 == fscanf(fp, "%*s%u%u%u%u", &t[0], &t[1], &t[2], &t[3]));
	fclose(fp);
	return ret;
}

int g_cpu_usage(char tick[16], int *out)
{
	uint32_t *_neu = (uint32_t *) tick;
	if (out) {
		uint32_t _old[4], z1, z2, z3;
		memcpy(_old, _neu, 16);
		if (!read_cpu_time(_neu)) return 0;
		
		// total = user + nice + system + idle
		z1 = _old[0] + _old[1] + _old[2] + _old[3];
		z2 = _neu[0] + _neu[1] + _neu[2] + _neu[3];
		
		z3 = (_neu[0]+_neu[2]) - (_old[0]+_old[2]);
		*out = (z3*100000/(1 + z2-z1)+5)/10; //%%%
		
		return 1;
	}
	return read_cpu_time(_neu);
}
// ################################################################
#if defined(F_RDLCK) && defined(F_WRLCK) && defined(F_UNLCK) && \
	defined(F_SETLK) && defined(F_SETLKW)
int g_flock(int fd, int op)
{
	struct flock fck;
	int ret;

	fck.l_start = 0;
	fck.l_len = 0;
	fck.l_whence = SEEK_SET;
	fck.l_pid = getpid();

	if (op & LOCK_SH) fck.l_type = F_RDLCK;
	else if (op & LOCK_EX) fck.l_type = F_WRLCK;
	else if (op & LOCK_UN) fck.l_type = F_UNLCK;
	else return 0;

	ret = fcntl(fd,
		(op & LOCK_NB) ? F_SETLK : F_SETLKW, &fck);
	return (ret<0?0:1);
}
#else
#warning "no proper fcntl/flock support."
int g_flock(int fd, int op){return 0;}
#endif

int g_flock2(void* fp, int op)
{
	return g_flock(fileno((FILE *)fp), op);
}


// ################################################################
static void to_abs_ts(double sec, struct timespec *ts)
{
	struct timeval tv;
	assert(sec >= 0);
	if (gettimeofday(&tv, NULL) == 0)
	{
		double inte, frac;
		frac = modf(sec, &inte);
		ts->tv_sec = tv.tv_sec + (time_t) inte;
		ts->tv_nsec = tv.tv_usec*1000 + (long)(frac*1000000000);
		if (ts->tv_nsec >= 1000000000) {
			ts->tv_sec++;
			ts->tv_nsec -= 1000000000;
		}
	}
	else {
		time_t det = (time_t)(sec + 0.5);
		if (det == 0) det = 1;
		ts->tv_sec = time(NULL) + det;
		ts->tv_nsec = 0;
	}
}

struct g_mutex_t {
	long refs;    ///< Number of enterances
	pthread_mutex_t mutex; ///< Mutex controlling the lock
	pthread_t owner;       ///< Thread holding this mutex
};

// If recursive mutexes are not available, then we have to grow
// our own.  This implementation assumes that pthread_equal()
// is atomic - that it cannot be deceived into thinking self
// and p->owner are equal if p->owner changes between two values
// that are not equal to self while the comparison is taking place.
// This implementation also assumes a coherent cache - that
// separate processes cannot read different values from the same
// address at the same time.  If either of these two conditions
// are not met, then the mutexes will fail and problems will result.

g_mutex_t *g_mutex_init(void)
{
	void *t = calloc(1, sizeof(g_mutex_t));
	g_mutex_t *p = (g_mutex_t *) t;
	if (p) {
#if defined(PTHREAD_MUTEX_RECURSIVE)
		pthread_mutexattr_t recursiveAttr;
		pthread_mutexattr_init(&recursiveAttr);
		pthread_mutexattr_settype(&recursiveAttr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&p->mutex, &recursiveAttr);
		pthread_mutexattr_destroy(&recursiveAttr);
#else
		pthread_mutex_init(&p->mutex, 0);
#endif // PTHREAD_MUTEX_RECURSIVE
	}
	return p;
}

void g_mutex_free(g_mutex_t *p)
{
	assert(p && p->refs==0);
	pthread_mutex_destroy(&p->mutex);
	free(p); // release all!
}

int  g_mutex_held(g_mutex_t *p)
{
	assert(p);
	return (p->refs!=0 && pthread_equal(p->owner, pthread_self()));
}

void g_mutex_enter(g_mutex_t *p)
{
	assert(p);
#if !defined(PTHREAD_MUTEX_RECURSIVE)
	if( p->refs>0 && pthread_equal(p->owner,pthread_self()))
	{
		p->refs++;
	}
	else {
		pthread_mutex_lock(&p->mutex);
		assert(p->refs==0);
		p->refs = 1;
		p->owner = pthread_self();
	}
#else
	pthread_mutex_lock(&p->mutex);
	assert( p->refs>0 || p->owner==0 );
	p->refs++;
	p->owner = pthread_self();
#endif // PTHREAD_MUTEX_RECURSIVE
}

static int exec_try_lock(pthread_mutex_t *mt, double sec)
{
	int ret = pthread_mutex_trylock(mt);
	if (0 == ret) return 0;

	if (sec > 0) {
		struct timespec ts;
		to_abs_ts(sec, &ts);
		ret = pthread_mutex_timedlock(mt, &ts);
	}
	return ret;
}

int g_mutex_try_enter(g_mutex_t *p, double sec)
{
	assert(p);

#if !defined(PTHREAD_MUTEX_RECURSIVE)
	if (p->refs>0 && pthread_equal(p->owner, pthread_self()))
	{
		return (++p->refs);
	}
	else if (exec_try_lock(&p->mutex, sec)==0)
	{
		assert(p->refs==0);
		p->owner = pthread_self();
		return (p->refs=1);
	}
#else
	if (exec_try_lock(&p->mutex, sec)==0)
	{
		p->owner = pthread_self();
		return (++p->refs);
	}
#endif // PTHREAD_MUTEX_RECURSIVE

	return 0; // failed!
}

void g_mutex_leave(g_mutex_t *p)
{
	assert(p && g_mutex_held(p));
	p->refs--;
	if( p->refs==0 ) p->owner = 0;
#if !defined(PTHREAD_MUTEX_RECURSIVE)
	if( p->refs==0 ){
		pthread_mutex_unlock(&p->mutex);
	}
#else
	pthread_mutex_unlock(&p->mutex);
#endif // PTHREAD_MUTEX_RECURSIVE
}

// ################################################################
g_thread_t g_thread_start(void* (*func)(void *), void *user)
{
	pthread_t id;
	int i = pthread_create(&id, NULL, func, user);
	return (i==0 ? (g_thread_t) id : NULL);
}

int g_thread_join(g_thread_t th, long *ret)
{
	pthread_t id = (pthread_t) th;
	assert(th);
	return (pthread_join(id, (void **)ret)==0);
}

int g_thread_start_detached(void* (*func)(void *), void *user)
{
	pthread_attr_t attr;
	pthread_t id;
	(void) pthread_attr_init(&attr);
	(void) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	return (0==pthread_create(&id, &attr, func, user));
}

int g_thread_sleep(double sec)
{
	if (sec <= 0.0) {sched_yield(); return 1;}
	else if (sec > INT32_MAX) sec = INT32_MAX;

	do {
		struct timespec req;
		struct timespec rem;
		long   inte = (long) sec;
		double frac = (sec - inte);
		req.tv_sec = (time_t) inte;
		req.tv_nsec = (long) (frac*1000000000);
		while (nanosleep(&req, &rem) != 0)
		{
			if (errno != EINTR) return 0;
			req = rem;
		}
		return 1;
	} while (0);
}

void g_thread_yield() {sched_yield();}

void g_thread_pause() {asm ("pause");}

void g_thread_exit(long ret) {pthread_exit((void *)ret);}

#ifndef SYS_gettid
#define SYS_gettid __NR_gettid
#endif
long g_thread_id() {return (long)syscall(SYS_gettid);} //gettid()

int g_thread_bind(int cpu, const char *name) 
{
	if (cpu >= 0) {
		int num = sysconf(_SC_NPROCESSORS_CONF);
		if (cpu < num) {
			cpu_set_t _set;
			cpu_set_t _get;
			
			CPU_ZERO(&_set);
			CPU_SET(cpu, &_set);
			sched_setaffinity(0, sizeof(_set), &_set);
			
			CPU_ZERO(&_get);
			sched_getaffinity(0, sizeof(_get), &_get);
			
			return CPU_ISSET(cpu, &_get);
		}
		return 0;
	}
	if (name && *name) {
		prctl(PR_SET_NAME, name);
	}
	return 1;
}

long g_process_id() { return (long)getpid();}

// ################################################################
long g_lock_set(long *ptr, long new_val)
{
	return __sync_lock_test_and_set(ptr, new_val);
}

long g_lock_add(long *ptr, long inc_val)
{
	return __sync_fetch_and_add(ptr, inc_val);
}

long g_ifeq_set(long *ptr, long cmp, long new_val)
{
	return __sync_val_compare_and_swap(ptr, cmp, new_val);
}

// ################################################################
static int64_t get_cpu_hz()
{
	FILE *fp = fopen("/proc/cpuinfo", "rt");
	double mhertz = -1;
	if (fp) {
		char buf[128];
		while(fgets(buf, sizeof buf, fp))
		{
			if (sscanf (buf, "cpu MHz : %lf", &mhertz) == 1) break;
		}
		fclose(fp);
	}
	return (int64_t) (mhertz*1000000);
}

int64_t g_now_hr()
{
	static int64_t start = 0;
	static int64_t freq;
	static int64_t tick;
	uint64_t now;
	
#if defined (__amd64__) || defined (__x86_64__)
	uint32_t eax, edx;
	asm volatile ("rdtsc" : "=a" (eax), "=d" (edx) : : "memory");
	now = (((uint64_t) eax) | (((uint64_t) edx) << 32));
#else
	asm volatile ("rdtsc" : "=A" (now) : : "memory");
#endif

	if (start == 0) {
		start = g_now_us();
		tick = now;
		freq = get_cpu_hz();
		return start;
	}
	return start + (now-tick)*1000000/freq;
}

int64_t g_now_us()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (int64_t)tp.tv_sec * 1000000 + tp.tv_usec;
}

int64_t g_now_ms()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (int64_t)tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

int g_timezone()
{
	struct timezone zone;
	struct timeval tp;
	gettimeofday(&tp, &zone);
	return zone.tz_minuteswest/-60;
}
// ################################################################
struct g_share_t {
	pthread_rwlock_t lock;
};

g_share_t *g_share_init(void)
{
	g_share_t *s = (g_share_t *) malloc(sizeof(g_share_t));
	assert(s);
	pthread_rwlock_init(&s->lock, NULL);
	return s;
}

void g_share_free(g_share_t *s)
{
	assert(s);
	pthread_rwlock_destroy(&s->lock);
	free(s);
}

void g_share_lockw(g_share_t *s)
{
	pthread_rwlock_wrlock(&s->lock);
}

int  g_share_try_lockw(g_share_t *s, double sec)
{
	int ret = pthread_rwlock_trywrlock(&s->lock);
	if (0 == ret) return 1;

	if (sec > 0) {
		struct timespec ts;
		to_abs_ts(sec, &ts);
		ret = pthread_rwlock_timedwrlock(&s->lock, &ts);
	}
	return (ret==0);
}

void g_share_lockr(g_share_t *s)
{
	pthread_rwlock_rdlock(&s->lock);
}

int  g_share_try_lockr(g_share_t *s, double sec)
{
	int ret = pthread_rwlock_tryrdlock(&s->lock);
	if (0 == ret) return 1;

	if (sec > 0) {
		struct timespec ts;
		to_abs_ts(sec, &ts);
		ret = pthread_rwlock_timedrdlock(&s->lock, &ts);
	}
	return (ret==0);
}

void g_share_unlock(g_share_t *s)
{
	pthread_rwlock_unlock(&s->lock);
}
// ################################################################

#if !defined(SPIN_DEBUG)
	#if !defined(_DEBUG)
	#define SPIN_DEBUG 0
	#else
	#define SPIN_DEBUG 1
	#endif
#endif


#if SPIN_DEBUG

#define SPIN_DEBUG_CHECK(s) \
	do {	\
		assert(s->wc <= 1);	\
		assert((s->wc == 1 && s->rc == 0) || (s->wc == 0 && s->rc >= 0));	\
	} while(0)

#define SPIN_DEBUG_INC_READER(s) __sync_add_and_fetch(&s->rc, 1)
#define SPIN_DEBUG_DEC_READER(s) __sync_sub_and_fetch(&s->rc, 1)
#define SPIN_DEBUG_INC_WRITER(s) __sync_add_and_fetch(&s->wc, 1)
#define SPIN_DEBUG_DEC_WRITER(s) __sync_sub_and_fetch(&s->wc, 1)

#else
#define SPIN_DEBUG_CHECK(s)
#define SPIN_DEBUG_INC_READER(s)
#define SPIN_DEBUG_DEC_READER(s)
#define SPIN_DEBUG_INC_WRITER(s)
#define SPIN_DEBUG_DEC_WRITER(s)
#endif

#define SPIN(s, spin, cond, ret)						\
	do {												\
		int n = 1, i;									\
		do {											\
			for (i = 0; i < n; i++) g_thread_pause();	\
		} while (!(ret = (cond)) && (n <<= 1) < spin);	\
	} while(0)


struct g_spin_t {
	volatile int32_t sem;	///< semaphore
#if SPIN_DEBUG
	volatile int32_t rc;
	volatile int32_t wc;
#endif
};

#define SPIN_PURE_INIT_VALUE		0x00000000
#define SPIN_PURE_LOCK_VALUE		0x00000001

g_spin_t* g_spin_init()
{
	g_spin_t* s = malloc(sizeof(g_spin_t));
	if (s == NULL) return NULL;

	memset(s, 0, sizeof(g_spin_t));

	s->sem = SPIN_PURE_INIT_VALUE;

	return s;
}

void g_spin_free(g_spin_t* s)
{
	assert(s);
	assert(s->sem == SPIN_PURE_INIT_VALUE);
	free(s);
}


#define SPIN_PURE_TRYLOCK(s) (s->sem == SPIN_PURE_INIT_VALUE && \
		__sync_bool_compare_and_swap(&s->sem, SPIN_PURE_INIT_VALUE, SPIN_PURE_LOCK_VALUE))

#define SPIN_PURE_UNLOCK(s) s->sem = SPIN_PURE_INIT_VALUE

void g_spin_enter(g_spin_t* s, int spin)
{
	int spin_ret;
	while (!SPIN_PURE_TRYLOCK(s)) {
		SPIN(s, spin, SPIN_PURE_TRYLOCK(s), spin_ret);
		if (spin_ret) break;
		g_thread_yield();
	}

	SPIN_DEBUG_INC_WRITER(s);
	SPIN_DEBUG_CHECK(s);
}

void g_spin_leave(g_spin_t* s)
{
	SPIN_DEBUG_CHECK(s);
	SPIN_DEBUG_DEC_WRITER(s);

	SPIN_PURE_UNLOCK(s);
}

// ################################################################
struct g_spin_rw_t {
	volatile int32_t sem;	///< semaphore
	volatile int32_t w;		///< writers need lock
	int32_t prefer_writer;
#if SPIN_DEBUG
	volatile int32_t rc;
	volatile int32_t wc;
#endif
};

#define SPIN_WRITER_LOCK_FLAG		0x40000000
#define SPIN_RWLOCK_INIT_VALUE		0x00000000
#define SPIN_WRITER_LOCK_VALUE		(SPIN_RWLOCK_INIT_VALUE | SPIN_WRITER_LOCK_FLAG)

g_spin_rw_t* g_spin_rw_init(int prefer_writer)
{
	g_spin_rw_t* s = malloc(sizeof(g_spin_rw_t));
	if (s == NULL) return NULL;

	memset(s, 0, sizeof(g_spin_rw_t));

	s->sem = SPIN_RWLOCK_INIT_VALUE;
	if (prefer_writer) {
		s->prefer_writer = 1;
	}

	return s;
}

void g_spin_rw_free(g_spin_rw_t* s)
{
	assert(s);
	assert(s->sem == SPIN_RWLOCK_INIT_VALUE);
	assert((s->w) == 0);
	free(s);
}

#define SPIN_WRITER_TRYLOCK(s) (s->sem == SPIN_RWLOCK_INIT_VALUE && \
		__sync_bool_compare_and_swap(&s->sem, SPIN_RWLOCK_INIT_VALUE, SPIN_WRITER_LOCK_VALUE))

#define SPIN_WRITER_UNLOCK(s) (__sync_and_and_fetch(&s->sem, ~SPIN_WRITER_LOCK_FLAG))

void g_spin_lockw(g_spin_rw_t* s, int spin)
{
	if (SPIN_WRITER_TRYLOCK(s)) {
		SPIN_DEBUG_INC_WRITER(s);
		SPIN_DEBUG_CHECK(s);
		return;
	}

	if (s->prefer_writer) __sync_add_and_fetch(&s->w, 1);

	int spin_ret;
	while (!SPIN_WRITER_TRYLOCK(s)) {
		SPIN(s, spin, SPIN_WRITER_TRYLOCK(s), spin_ret);
		if (spin_ret) break;
		g_thread_yield();
	}

	if (s->prefer_writer) __sync_sub_and_fetch(&s->w, 1);

	SPIN_DEBUG_INC_WRITER(s);
	SPIN_DEBUG_CHECK(s);
}

int g_spin_try_lockw(g_spin_rw_t* s, double sec, int spin)
{
	if (SPIN_WRITER_TRYLOCK(s)) {
		SPIN_DEBUG_INC_WRITER(s);
		SPIN_DEBUG_CHECK(s);
		return 0;
	}

	if (sec <= 0) return 1;

	if (s->prefer_writer) __sync_add_and_fetch(&s->w, 1);

	int ret = 0;
	int64_t limit = g_now_us() + (int64_t)(sec * 1000000);

	int spin_ret;
	while (!SPIN_WRITER_TRYLOCK(s)) {
		SPIN(s, spin, SPIN_WRITER_TRYLOCK(s), spin_ret);
		if (spin_ret) break;
		if (g_now_us() > limit) { ret = 1; break; }
		g_thread_yield();
	}

	if (s->prefer_writer) __sync_sub_and_fetch(&s->w, 1);

#if SPIN_DEBUG
	if (ret == 0) {
		SPIN_DEBUG_INC_WRITER(s);
		SPIN_DEBUG_CHECK(s);
	}
#endif

	return ret;
}

void g_spin_unlockw(g_spin_rw_t* s)
{
	SPIN_DEBUG_CHECK(s);
	SPIN_DEBUG_DEC_WRITER(s);

	SPIN_WRITER_UNLOCK(s);
}

#define SPIN_READER_TRYLOCK(s) \
	/* s->w > 0 if there are some writers want to get the lock */	\
	( (s->w == 0) && \
	/* check whether the lock has been allocated by a writer */	\
	(s->sem & SPIN_WRITER_LOCK_FLAG) == 0 && \
	((__sync_fetch_and_add(&s->sem, 1) & SPIN_WRITER_LOCK_FLAG) ? \
			(__sync_sub_and_fetch(&s->sem, 1), 0)/* comma expression, returning 0 */ : 1) )

#define SPIN_READER_UNLOCK(s) __sync_sub_and_fetch(&s->sem, 1)

void g_spin_lockr(g_spin_rw_t* s, int spin)
{
	int spin_ret;
	while (!SPIN_READER_TRYLOCK(s)) {
		SPIN(s, spin, SPIN_READER_TRYLOCK(s), spin_ret);
		if (spin_ret) break;
		g_thread_yield();
	}

	SPIN_DEBUG_INC_READER(s);
	SPIN_DEBUG_CHECK(s);
}

int g_spin_try_lockr(g_spin_rw_t* s, double sec, int spin)
{
	if (SPIN_READER_TRYLOCK(s)) {
		SPIN_DEBUG_INC_READER(s);
		SPIN_DEBUG_CHECK(s);
		return 0;
	}

	if (sec <= 0) return 1;

	int ret = 0;
	int64_t limit = g_now_us() + (int64_t)(sec * 1000000);

	int spin_ret;
	while (!SPIN_READER_TRYLOCK(s)) {
		SPIN(s, spin, SPIN_READER_TRYLOCK(s), spin_ret);
		if (spin_ret) break;
		if (g_now_us() > limit) { ret = 1; break; }
		g_thread_yield();
	}

#if SPIN_DEBUG
	if (ret == 0) {
		SPIN_DEBUG_INC_READER(s);
		SPIN_DEBUG_CHECK(s);
	}
#endif

	return ret;
}

void g_spin_unlockr(g_spin_rw_t* s)
{
	SPIN_DEBUG_CHECK(s);
	SPIN_DEBUG_DEC_READER(s);

	SPIN_READER_UNLOCK(s);
}

// ################################################################
#include <semaphore.h>
struct g_sema_t {sem_t sem;};

g_sema_t *g_sema_init(uint32_t cap, uint32_t cnt)
{
	UNUSED_PARAMETER(cap);
	
	g_sema_t *s = (g_sema_t *) malloc(sizeof(g_sema_t));
	if (s == NULL) return NULL;

	if (sem_init(&s->sem, 0, cnt) != 0) {
		free(s);
		return NULL;
	}

	return s;
}

void g_sema_free(g_sema_t *s)
{
	if (s) {
		sem_destroy(&s->sem);
		free(s);
	}
}

int	g_sema_post(g_sema_t *s)
{
	assert(s);
	return (sem_post(&s->sem)==0);
}

int	g_sema_wait(g_sema_t *s, double sec)
{
	assert(s);
	if (sec < 0) {
		return (sem_wait(&s->sem)==0);
	}
	else if (sec > 0) {
		struct timespec ts;
		to_abs_ts(sec, &ts);
		return (sem_timedwait(&s->sem, &ts)==0);
	}
	return (sem_trywait(&s->sem)==0);
}

// ################################################################
struct g_cond_t {pthread_cond_t cond;};

/// @brief initialize
g_cond_t *g_cond_init(void)
{
	g_cond_t *gc =
		(g_cond_t *) calloc(1, sizeof(g_cond_t));
	if (pthread_cond_init(&gc->cond, NULL)==0) return gc;
	free(gc); return NULL;
}

/// @brief deconstruct
void g_cond_free(g_cond_t *gc)
{
	assert(gc);
	pthread_cond_destroy(&gc->cond);
	free(gc);
}

/// @brief Wait for the signal.
int g_cond_wait(g_cond_t *gc, g_mutex_t *mtx, double sec)
{
	assert(gc);
	if (sec < 0) {
		return (pthread_cond_wait(
			&gc->cond, &mtx->mutex)==0);
	}
	else {
		struct timespec ts;
		to_abs_ts(sec, &ts);
		return (pthread_cond_timedwait(
			&gc->cond, &mtx->mutex, &ts)==0);
	}
}

/// @brief Send the wake-up signal to another waiting thread.
void g_cond_signal(g_cond_t *gc)
{
	assert(gc);
	if(pthread_cond_signal(&gc->cond)!=0) assert(1);
}

/// @brief Send the wake-up signal to all waiting threads.
void g_cond_broadcast(g_cond_t *gc)
{
	assert(gc);
	if(pthread_cond_broadcast(&gc->cond)!=0) assert(1);
}
// ################################################################
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct f64_t {
	int fd; ///< file descriptor returned by open()
	int dd; ///< directory file descriptor
	int  r; ///< a flag for read-only access
	int  e; ///< number for GetLastError()
	g_mutex_t *m;  ///< mutex locker for W/R
};

#ifndef O_LARGEFILE
#  define O_LARGEFILE 0
#endif
#ifndef O_BINARY
# define O_BINARY 0
#endif

static f64_t *do_alloc(int fd, int dd, int ro)
{
	f64_t *f = (f64_t *) malloc(sizeof(f64_t));
	assert (f!=NULL);
	f->fd = fd;  f->dd = dd;
	f->r = ro;   f->e = 0;
	f->m = g_mutex_init();
	assert (f->m); return f;
}

f64_t *f64open(const char *fn, int create)
{
	int  i, fd, dd=-1;
	int  read_only = 0;
	int  flag = O_RDWR|O_LARGEFILE|O_BINARY;

	if (create) flag |= O_CREAT;
	fd = open(fn, flag, 0644);
	if (fd<0) {
		fd = open(fn, O_RDONLY|O_LARGEFILE|O_BINARY, 0644);
		read_only = 1;
		if(fd<0) return NULL;
	}

#ifdef FD_CLOEXEC
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD, 0) | FD_CLOEXEC);
#endif//FD_CLOEXEC

	if (create==2) {
		char path[512+1];
		strcpy(path, fn);
		for (i=strlen(path)-1; i>0 && path[i]!='/'; i--);
		if (i>0 ) {
	    	path[i] = '\0';
			dd = open(path, O_RDONLY|O_BINARY, 0);
		}
		if (dd<0) {
			close(fd); return NULL;
		}
#ifdef FD_CLOEXEC
		fcntl(dd, F_SETFD, fcntl(dd, F_GETFD, 0) | FD_CLOEXEC);
#endif//FD_CLOEXEC
	}
	return do_alloc(fd, dd, read_only);
}

int f64close(f64_t *f)
{
	if (f->dd >= 0) {
		int err = close(f->dd);
		if (err) { f->e = errno;  return 0;}
		else f->dd=-1;
	}
	if (f->fd >= 0) {
		int err = close(f->fd);
		if (err) { f->e = errno; return 0;}
	}
	g_mutex_free(f->m);
	free(f);  return 1;
}

int f64read(f64_t *f, void *ptr, int amt, int64_t offset)
{
	int got = -1;
#ifdef __USE_UNIX98
	got = pread(f->fd, ptr, amt, offset);
#else
	int64_t newOffset;
	g_mutex_enter(f->m);
	newOffset = lseek(f->fd, offset, SEEK_SET);

	if (newOffset==offset) {
		got = read(f->fd, ptr, amt);
	}
	g_mutex_leave(f->m);
#endif
	return (got<0 ? (f->e=errno, 0) : got);
}

int f64write(f64_t *f, const void *ptr, int amt, int64_t offset)
{
	int wrote = 0;
	int got = -1;
#ifdef __USE_UNIX98
	while (amt>0 && (got=pwrite(f->fd, ptr, amt, offset))>=0)
	{
		offset += got;
		amt -= got;
		wrote += got;
		ptr = &((char*)ptr)[got];
	}
#else
	int64_t newOffset;
	g_mutex_enter(f->m);
	newOffset = lseek(f->fd, offset, SEEK_SET);

	if (newOffset==offset) {
		while (amt>0 && (got=write(f->fd, ptr, amt))>=0)
		{
			amt -= got;
			wrote += got;
			ptr = &((char*)ptr)[got];
		}
	}
	g_mutex_leave(f->m);
#endif
	return (got<0 ? (f->e=errno, 0) : wrote);
}

int f64tell(f64_t *f, int64_t *offset)
{
	int64_t x = lseek(f->fd, 0L, SEEK_CUR);
	return (x<0?(f->e=errno,0):(*offset=x,1));
}

int f64sync(f64_t *f, int data_only)
{
#if !defined(__MINGW32__)
	int fd = f->fd;
#if defined(F_FULLFSYNC)
	int rc = fcntl(fd, F_FULLFSYNC, 0);
	if (rc) rc = fsync(fd); // failed? try fsync()
#else
	int rc = data_only ? fdatasync(fd) : fsync(fd);
#endif // F_FULLFSYNC
	if (rc) { f->e = errno; return 0;}

	if (f->dd >= 0) {
		// ignore errors for directory sync: Ticket #1657
		fsync(f->dd);

		// Only need to sync once, close the directory now
		rc = close(f->dd);
		return (rc ? (f->e=errno, 0) : (f->dd=-1, 1));
	}
#endif // __MINGW32__
	return 1;
}

// Truncate an open file to a specified size
int f64trunc(f64_t *f, int64_t size)
{
	return (ftruncate(f->fd, (off_t)size) ?
		(f->e=errno,0) : 1);
}

int64_t f64size(f64_t *f)
{
	struct stat buf;
	return (fstat(f->fd, &buf) ?
		(f->e=errno,0) : buf.st_size);
}

// flock and fcntl aren't part of Standard C++ per ISO/IEC 14882:2003.
// Note that flock doesn't support NFS, in contrast, fcntl function
// satisfy POSIX Standard, which creates a daemon to support NFS too.
// We define a new flock (based on fcntl) here:
#if defined(F_RDLCK) && defined(F_WRLCK) && \
	defined(F_UNLCK) && defined(F_SETLK) && defined(F_SETLKW)
int f64lock(f64_t *f, int op)
{
	struct flock fck;
	int rc, flag = (op & LOCK_NB) ? F_SETLK : F_SETLKW;
	fck.l_start = 0;
	fck.l_len = 0;
	fck.l_whence = SEEK_SET;
	fck.l_pid = getpid();

	if (op & LOCK_SH) fck.l_type = F_RDLCK;
	else if (op & LOCK_EX) fck.l_type = F_WRLCK;
	else if (op & LOCK_UN) fck.l_type = F_UNLCK;
	else {return 0;}

	rc = fcntl(f->fd, flag, &fck);
	return (rc<0 ? 0 : 1);
}
#endif //F_RDLCK,F_WRLCK,F_UNLCK


// ################################################################
#include <sys/mman.h>
#include <sys/types.h>

static shm_t *shm_open_impl(
	int fd, void *start, uint32_t len, int forced)
{
	int flags = MAP_SHARED;
	void *ptr;
	shm_t *map;
	int unit = sysconf(_SC_PAGESIZE);

	if (fd >= 0) {
		uint32_t cc;
		cc = lseek(fd, 0, SEEK_END); // file size

		// struct mb_man_t will be happy!
		if (start == SHM_LOAD) {
			uint32_t got = 0;
			uint32_t siz = sizeof(void *);
			start = NULL; // SHM_NULL
			if (cc >= siz) {
				lseek(fd, 0, SEEK_SET);
				got = read(fd, &start, siz);
				if (got!=siz) start = NULL;
			}
			if (!start) {
				if (forced) {close(fd); return NULL;}
			}
			if (len < cc) len = cc;
		}

		if (!len) { //default: filesize or pagesize
			if(!(len=cc)) len = unit;
		}

		if (cc < len) { // patch to stuff some bytes, work as Win32
			lseek(fd, len-1, SEEK_SET);
			if (1!=write(fd, "", 1)) {
				close(fd); return NULL;
			}
		}
	}
	else {
		if (!len) len = unit;
		flags |= MAP_ANONYMOUS;
	}

	// not initialized with an address
	ptr = mmap(start, len, PROT_READ|PROT_WRITE, flags, fd, 0);

	if (ptr==MAP_FAILED) return NULL; // okay?

	// since MAP_FIXED is not recommanded to be used, we do some
	// checks, to avoid Linux automatically use MAP_VARIABLE!
	if (forced && start && start != ptr) {
		munmap(ptr, len); return NULL;
	}

	map = (shm_t *) malloc(sizeof(shm_t));
	assert(map!=NULL);
	map->ptr = (uint8_t *) ptr;
	map->len = len;
	map->owner = NULL;	return map;
}

shm_t *g_shm_open(
	const char *fn, void *start, uint32_t len, int forced)
{
	shm_t *map;
	if (fn) {
		int fd = open(fn, O_CREAT|O_RDWR|O_BINARY, S_IREAD|S_IWRITE);
		if (fd < 0) return NULL;
		map = shm_open_impl(fd, start, len, forced);
		close(fd); // release this original
	}
	else {
		map = shm_open_impl(-1, start, len, forced);
	}
	return map;
}

int g_shm_close(shm_t *map)
{
	if (!map || !map->ptr) return 0;
	if (map->ptr==MAP_FAILED) return 0;
	if (0!=munmap(map->ptr, map->len)) return 0;
	free(map);  return 1;
}

int g_shm_sync(const shm_t *map, uint32_t s, uint32_t c)
{
	if (!map || !map->ptr) return 0;
	if (map->ptr==MAP_FAILED) return 0;
	if (c<=0) {s=0; c=map->len;}
	else {
		if (s >= map->len) return 0;
		if (c > map->len - s) c = map->len - s;
	}
	return(0==msync(map->ptr+s, c, MS_SYNC));
}

int g_shm_unit(void)
{
	return sysconf(_SC_PAGESIZE);
}

#endif // OS=!WIN32


//-------------------------------------------------------------------------
//---------------- (3) common part for g_atom_t routines ----------------
//-------------------------------------------------------------------------
uint64_t g_now_uid()
{
	static uint64_t _tic = 0;
	uint64_t now = g_now_us();
	
	static long _ack = 0; //locker
	while (g_ifeq_set(&_ack, 0, 1)) g_thread_yield();
	
	// "now" equal to "_tic" in all cases
	if (_tic < now) _tic = now;
	else now = ++_tic; 
	
	g_lock_set(&_ack, 0); //unlock
	return now; 
}

#define SECS_PER_HOUR (60 * 60)
#define SECS_PER_DAY (SECS_PER_HOUR * 24)
#define IS_LEAP_YEAR(y) \
	((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0))
#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV(y, 4) - DIV(y, 100) + DIV(y, 400))

struct tm* g_localtime(time_t _sec, struct tm *_tp, long offset)
{
	/* How many days come before each month (0-12). */
	static const short __mon_yday[2][13] =
	{
		/* Normal years. */
    	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
		/* Leap years. */
		{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
	};

	long days, rem, y;
	time_t sec;
	const short *ip;
	struct tm *tp;

	tp = (struct tm *) _tp;
	sec = _sec;
	days = sec / SECS_PER_DAY;
	rem  = sec % SECS_PER_DAY;
	rem += offset;
	
	while (rem < 0) { rem += SECS_PER_DAY; --days;}
	while (rem >= SECS_PER_DAY) { rem -= SECS_PER_DAY; ++days;}
	
	tp->tm_hour = rem / SECS_PER_HOUR;
	rem %= SECS_PER_HOUR;
	tp->tm_min = rem / 60;
	tp->tm_sec = rem % 60;

	/* January 1, 1970 was a Thursday. */
	tp->tm_wday = (4 + days) % 7;
	if (tp->tm_wday < 0) tp->tm_wday += 7;
	
	y = 1970;
	while (days < 0 || days >= (IS_LEAP_YEAR(y) ? 366 : 365))
    {
		/* Guess a corrected year, assuming 365 days per year. */
		long yg = y + days / 365 - (days % 365 < 0);

		/* Adjust DAYS and Y to match the guessed year. */
		days -= ((yg - y) * 365 + 
			LEAPS_THRU_END_OF (yg - 1)- LEAPS_THRU_END_OF (y - 1));
		
		y = yg;
    }
    
	tp->tm_year = y - 1900;
	tp->tm_yday = days;
	ip = __mon_yday[IS_LEAP_YEAR(y)];
	
	for (y = 11; days < (long int) ip[y]; --y) continue;
	
	days -= ip[y];
	tp->tm_mon = y;
	tp->tm_mday = days + 1;
	tp->tm_isdst = 0;

	return tp;
}

//-------------------------------------------------------------------------
//------------------- (4) common part for m64_t routines ------------------
//-------------------------------------------------------------------------


struct m64_t {
	f64_t *f64;
	shm_t *shm;
};

m64_t *m64open(const char *fn, uint32_t xmap)
{
	f64_t *f64 = f64open(fn, !g_exists_file(fn));
	shm_t *shm = NULL;
	m64_t *mix;
	if (!f64) return NULL;
	if (xmap) {
		shm = shm_open_impl(f64->fd, NULL, xmap, 0);
		if (!shm) {f64close(f64); return NULL;}
	}

	mix = (m64_t *)malloc(sizeof(m64_t));
	mix->f64 = f64;
	mix->shm = shm;
	return mix;
}

int m64close(m64_t *m)
{
	if (m) {
		if (m->shm) {
			if (!g_shm_close(m->shm)) return 0;
			m->shm = NULL;
		}
		if (f64close(m->f64)) {
			m->f64 = NULL;
			free(m); return 1;
		}
	}
	return 0;
}

int m64read(m64_t *m, void *ptr, int amt, int64_t offset)
{
	int got = 0;
	assert(m && ptr);
	assert(amt > 0);
	assert(offset>=0);

	if (m->shm) {
		int64_t siz = (int64_t) m->shm->len;
		int64_t end = (offset + amt);
		if (end <= siz) {
			memcpy(ptr, m->shm->ptr+offset, amt);
			return amt;
		}
		else if (offset < siz) {
			got = (int) (siz-offset);
			memcpy(ptr, m->shm->ptr+offset, got);
			amt -= got;
			offset += got;
			ptr = &((char*)ptr)[got];
		}
	}
	return (got + f64read(m->f64, ptr, amt, offset));
}

int m64write(m64_t *m, const void *ptr, int amt, int64_t offset)
{
	int got = 0;
	assert(m && ptr);
	assert(amt > 0);
	assert(offset>=0);

	if (m->shm) {
		int64_t siz = (int64_t) m->shm->len;
		int64_t end = (offset + amt);
		if (end <= siz) {
			memcpy(m->shm->ptr+offset, ptr, amt);
			return amt;
		}
		else if (offset < siz) {
			got = (int) (siz-offset);
			memcpy(m->shm->ptr+offset, ptr, got);
			amt -= got;
			offset += got;
			ptr = &((char*)ptr)[got];
		}
	}
	return (got + f64write(m->f64, ptr, amt, offset));
}

int m64tell(m64_t *m, int64_t *offset)
{
	return f64tell(m->f64, offset);
}

int m64sync(m64_t *m, int flag)
{
	int i = (m->shm ?
		g_shm_sync(m->shm, 0, 0) : 0);
	return (f64sync(m->f64, flag) + i);
}

int m64trunc(m64_t *m, int64_t size)
{
	return f64trunc(m->f64, size);
}

int64_t m64size(m64_t *m)
{
	return f64size(m->f64);
}

int m64lock(m64_t *m, int op)
{
	return f64lock(m->f64, op);
}
//-------------------------------------------------------------------------

