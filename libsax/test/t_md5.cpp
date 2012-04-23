#include <iostream>
#include <string>
#include <stdio.h>

#include <sax/strutil.h>
#include <sax/os_api.h>
#include <sax/c++/fifo.h>

void test_info()
{
	uint64_t  f, t;
	g_disk_info("/", &f, &t);
	printf("g_disk_info: %lu, %lu\n", (long)(f/1024/1024), (long)(t/1024/1024));

	/// @brief retrieve the memory information
	g_mem_phys(&f, &t);
	printf("g_mem_phys: %lu, %lu\n", (long)(f/1024/1024), (long)(t/1024/1024));

	g_mem_virt(&f, &t);
	printf("g_mem_virt: %lu, %lu\n", (long)(f/1024/1024), (long)(t/1024/1024));

	/// @brief retrieve the system load average
	int z = g_load_avg();
	printf("g_load_avg: %u\n", z);

	char tick[16]; // magic buffer
	g_cpu_usage(tick, NULL); // init
	g_thread_sleep(1);
	g_cpu_usage(tick, &z);   // calc
	printf("g_cpu_usage: %u\n", z);
}

#include <time.h>
void test_dirent()
{
	DIR_HANDLE *x = g_opendir(".");
	g_dirent_t *t;
	while ((t = g_readdir(x)))
	{
		time_t tm = (time_t) t->mtime;
		char mod[32];
		strftime(mod, sizeof(mod), "%d-%b-%Y %H:%M", localtime(&tm));
		printf( "[%s]: %s -- %lu (%d)\n", t->dname, mod, (long) t->fsize, t->is_dir);
	}
	g_closedir(x);
}

void test_fifo()
{
	const int *p=0, z[]={0,1,2};
	bool b;
	sax::fifo<const int *> que(2);
	b=que.pop_front(p, 0.5);
	printf("pop_front():%d\n", b?*p:-1);
	
	b=que.push_back(z+0, 0.5);
	printf("push_back():%s\n", b?"ok":"fail");
	
	b=que.push_back(z+1, 0.5);
	printf("push_back():%s\n", b?"ok":"fail");
	
	b=que.push_back(z+2, 0.5);
	printf("push_back():%s\n", b?"ok":"fail");
	
	b=que.pop_front(p, 0.5);
	printf("pop_front():%d\n", b?*p:-1);

	b=que.push_back(z+2, 0.5);
	printf("push_back():%s\n", b?"ok":"fail");

	b=que.push_back(z+0, 0.5);
	printf("push_back():%s\n", b?"ok":"fail");
	
	b=que.pop_front(p, 0.5);
	printf("pop_front():%d\n", b?*p:-1);
	
	b=que.pop_front(p, 0.5);
	printf("pop_front():%d\n", b?*p:-1);
	
	b=que.pop_front(p, 0.5);
	printf("pop_front():%d\n", b?*p:-1);
}

int main( int argc, char *argv[] )
{
	test_fifo();
	std::string z = "0123456789";
	std::cout << "md5: " << sax::md5(z) << std::endl;
	std::cout << "sha: " << sax::sha1(z)  << std::endl;
	std::cout << "crc: " << sax::crc32(z) << std::endl;
	
	std::string s = "Hello World!";
	if (argc>1) s = argv[1];
	
	std::string a = sax::md5(s,true);
	std::string b = sax::shorten(s, 6);
	std::string c = sax::shorten(s,10);
	std::string d = sax::shorten(s,25);
	
	std::cout << s << std::endl;
	std::cout << a << std::endl;
	std::cout << b << std::endl;
	std::cout << c << std::endl;
	std::cout << d << std::endl;
	
	sax::strings x = sax::split("abc\ndfefe\tdfdfdf sdfsdf");
	std::cout << sax::join(x, "|") << std::endl;
	std::cout << "match: " << sax::match("abcpcd", "ab*d") << std::endl;
	std::cout << "match: " << sax::match("abcdcd", "ab*d") << std::endl;
	std::cout << "match: " << sax::match("abcpcdx", "ab*dx") << std::endl;
	std::cout << "match: " << sax::match("abcdcdx", "ab*dx") << std::endl;
	test_dirent();
	test_info();
	
	char t1[] = "abcdefghi";
	
	a = sax::b64_encode(t1, 1);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 2);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 3);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 4);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 5);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 6);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 7);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 8);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	a = sax::b64_encode(t1, 9);
	std::cout << a << std::endl;
	b = sax::b64_decode(a.c_str());
	std::cout << b << std::endl;
	
	std::cout << sax::toString(1.2345)+ sax::toString(4323)<< std::endl;
	std::cout << sax::toString(1.2345==12) << std::endl;
	
	double xx = sax::parseString<double>("1.2345");
	bool bb = sax::parseString<bool>("true");
	std::cout << xx << std::endl;
	std::cout << bb << std::endl;
	return 0;
}
