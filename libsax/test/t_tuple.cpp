#include <iostream>

#include <sax/c++/tuple.h>

int main( int argc, char *argv[] )
{
	sax::tuple<int> t(10);
	t.push_back(30);
	t.push_back(40);
	t.push_back(50);
	std::cout << "size=" << t.size() << std::endl;
	
	sax::tuple<int>::iterator it;
	
	std::cout << "testing iterator: it++ " << std::endl;
	for (it = t.begin(); it != t.end(); it++)
		std::cout << *it << std::endl;

	std::cout << "testing iterator: ++it " << std::endl;
	for (it = t.begin(); it != t.end(); ++it)
		std::cout << *it << std::endl;

	std::cout << "testing iterator: --it " << std::endl;
	for (it = t.end(); it != t.begin(); )
		std::cout << *--it << std::endl;
	
	return 0;
}

