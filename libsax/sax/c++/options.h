#ifndef __OPTIONS_H_2008__
#define __OPTIONS_H_2008__

/// @file options.h
/// @brief smart option segmenter
/// @author Qingshan.Luo
/// @date March, 2008

#include <string>
#include <vector>
#include <map>

#include <string.h>
#include <sax/os_api.h>
#include <sax/strutil.h>
#include "nocopy.h"

namespace sax {

template<bool short_cmd=false>
class options : public nocopy
{
public:
	typedef std::vector<std::string> pars_type;
	typedef std::map<std::string, std::string> args_type;
	typedef options<short_cmd> self_type;

	bool init(const char *cmd)
	{
		int argc = 0;
		char **argv = g_cmd_to_argv(cmd, &argc);
		if (argv) {
			bool ret = init(argc, argv);
			g_release(argv); 
			return ret;
		}
		return false;
	}
	
	bool init(int argc, char *argv[])
	{
		if (argc<1 || !argv) return false;
		
		_args.clear();
		_pars.clear();
		
		int i = (argv[0][0] != '-');
		while (i < argc) 
		{
			const char *a = argv[i++];
			if (a[0] != '-') {
				_pars.push_back(a);
				continue;
			}
			
			bool b_short;
			if (a[1] == '-') {
				if (a[2]=='\0' || a[2]=='=') continue;
				a += 2;  b_short = false;
			}
			else {
				if (a[1]=='\0' || a[1]=='=') continue;
				a += 1;  b_short = true;
			}
			
			std::string key, val;
			if (short_cmd && b_short && a[1]) {
				key.assign(a, 1);
				val.assign(a+1);
			}
			else {
				const char *p = strchr(a, '=');
				if (p) {
					key.assign(a, p-a);
					val.assign(p+1);
				}
				else {
					key.assign(a);
					if (i < argc && argv[i][0]!='-') val.assign(argv[i++]);
				}
			}
			sax::trim(key, true, true);
			sax::trim(val, true, true);
			_args[key] = val;
		}
		return (_args.size() > 0);
	}
	
	bool init(const char *ini, bool bDelQuote)
	{
		std::string tmp;
		if (!sax::load(tmp, ini)) return false;
		_args.clear();
		_pars.clear();
		
		sax::strings text = sax::split(tmp, "\r\n");
		for (size_t i=0; i<text.size(); i++)
		{
			std::string &line = text[i];
			sax::trimTails(line, "#");
			sax::trim(line, true, true);
			if (line.length()<3) continue;
			if (sax::startsWith(line, "[") && sax::endsWith(line, "]")) 
			{
				tmp = line.substr(1, line.length()-2);
				sax::trim(tmp, true, true);
				_pars.push_back(tmp);
				continue;
			}
			size_t j = line.find('=');
			if (j == std::string::npos) continue;
			std::string key = line.substr(0, j);
			std::string val = line.substr(j+1);
			sax::trim(key, true, true);
			sax::trim(val, true, true);
			if (bDelQuote && val.length() >= 2) 
			{
				char ch = val[0];
				if (ch == '"' || ch == '\'') {
					j = val.find(ch, 1);
					if (j == std::string::npos) continue;
					val = val.substr(1, j-1);
					sax::trim(val, true, true);
				}
			}
			_args[key] = val;
		}
		return (_args.size() > 0);
	}
	
	bool get(const char *key, std::string &val)
	{
		typename args_type::const_iterator it;
		it = _args.find(std::string(key));
		if (it == _args.end()) return false;
		val = it->second; 
		return true;
	}
	
	inline const args_type* args() const {return &_args;}
	inline const pars_type* pars() const {return &_pars;}
	inline size_t size() const {return _args.size();}
	
private:
	args_type _args;
	pars_type _pars;
};

typedef options<true> options_short;
typedef options<false> options_long;

} // namespace sax

#endif//__OPTIONS_H_2008__
