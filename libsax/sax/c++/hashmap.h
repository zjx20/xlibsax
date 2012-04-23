#ifndef __HASHMAP_H_2011__
#define __HASHMAP_H_2011__

/**
 * adaptations of hash-map and hash-set for generic use
 */

#ifdef __GNUC__

#define GCC_VERSION_ID \
	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION_ID >= 40102  // gcc-4.1.2

#include <tr1/unordered_map>
#include <tr1/unordered_set>

namespace sax {

template<class KEY, class VAL>
struct hashmap : public std::tr1::unordered_map<KEY, VAL>
{
};

template<class VAL>
struct hashset : public std::tr1::unordered_set<VAL>
{
};

} // namespace

#else  // gcc-4.1.2

#include <ext/hash_set>
#include <ext/hash_map>

#include <string>
namespace __gnu_cxx
{
template<> struct hash<std::string>
{
	size_t operator()(const std::string &x) const
	{
		size_t __length = x.length();
		const char *__first = x.c_str();
		size_t __result = 0;
		for (; __length > 0; --__length)
			__result = (__result * 131) + *__first++;
		return __result;
	}
};
}

namespace sax {

template<class KEY, class VAL>
struct hashmap : public __gnu_cxx::hash_map<KEY, VAL>
{
	void rehash(size_t n) { this->resize(n); }
};

template<class VAL>
struct hashset : public __gnu_cxx::hash_set<VAL>
{
	void rehash(size_t n) { this->resize(n); }
};

} // namespace

#endif  // gcc-4.1.2

#elif defined(_MSC_VER) && (_MSC_VER >= 1500) // VC9.0 and later

#include <unordered_map>
#include <unordered_set>

namespace sax {

template<class KEY, class VAL>
struct hashmap : public std::tr1::unordered_map<KEY, VAL>
{
};

template<class VAL>
struct hashset : public std::tr1::unordered_set<VAL>
{
};

#else

#include <hash_set>
#include <hash_map>

namespace sax {

template<class KEY, class VAL>
struct hashmap : public stdext::hash_map<KEY, VAL>
{
	void rehash(size_t n) {}
};

template<class VAL>
struct hashset : public stdext::hash_set<VAL>
{
	void rehash(size_t n) {}
};

} // namespace

#endif

#endif  //__HASHMAP_H_2011__
