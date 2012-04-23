#include <string>
#include <iostream>

#include <sax/c++/expire.h>

int main( int argc, char *argv[] )
{
	sax::expire<std::string> exp(2);
	exp.add("a");
	exp.add("b");
	exp.add("c");
	std::cout << exp.size() << std::endl;
	
	std::cin.get();
	exp.add("a");
	exp.add("d");
	
	exp.purge();
	std::cout << exp.size() << std::endl;
	
	std::cin.get();
	
	exp.purge();
	std::cout << exp.size() << std::endl;
	return 0;
}

