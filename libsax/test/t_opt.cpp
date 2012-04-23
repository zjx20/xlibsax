#include <sax/c++/options.h>
#include <iostream>

template<bool v>
void dump(const sax::options<v> &opt)
{
	const typename sax::options<v>::args_type *args = opt.args();
	typename sax::options<v>::args_type::const_iterator i1;

	const typename sax::options<v>::pars_type *pars = opt.pars();
	typename sax::options<v>::pars_type::const_iterator i2;

	std::cout << "----------------------------" << std::endl;
	for(i1 = args->begin(); i1!=args->end(); i1++)
		std::cout << i1->first << "=" << i1->second << std::endl;
	
	std::cout << "-------------" << std::endl;
	
	for(i2 = pars->begin(); i2!=pars->end(); i2++)
		std::cout << *i2 << "|" ;
	std::cout << std::endl;
}

void test1( int argc, char *argv[] )
{
	sax::options_long opt;
	opt.init("t_opt.exe --sdf 23434 12 --33 455 556 -1122 dfd -a sdf");
	dump(opt);
}

void test2( int argc, char *argv[] )
{
	sax::options_short opt;
	opt.init(argc, argv);
	dump(opt);
	
	std::string val;
	if (opt.get("pi", val)) std::cout << "pi=" << val << std::endl;
	if (opt.get("a", val)) std::cout << "a=" << val << std::endl;
	if (opt.get("h", val)) std::cout << "h=" << val << std::endl;
}

void test3( int argc, char *argv[] )
{
	sax::options_short opt;
	opt.init("t_opt.ini", true);
	dump(opt);
}

int main( int argc, char *argv[] )
{
	test1(argc, argv);
	test2(argc, argv);
	test3(argc, argv);
}
