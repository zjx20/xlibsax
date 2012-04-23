#include "hashdb.h"
#include "btreedb.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace sax;

string hashfile = "hash_test.dat";
string btreefile = "btree_test.dat";
string indexFile = "hash_test.dat";
static string inputFile = "test.txt";
static int degree = 16;
static size_t cacheSize = 50000;
static size_t pageSize = 5120;
static bool trace = false;

template<typename T>
void run_insert(T& cm) {
	cout<<"run_insert"<<endl;
	int sum =0;
	int hit =0;
	clock_t t1 = clock();

	ifstream inf(inputFile.c_str());
	string ystr;
	string vvv;

	while (inf>>ystr>>vvv) {
		sum++;
		if (cm.insert(ystr.c_str(),ystr.size(),vvv.c_str(),vvv.size()) ) {
			hit++;
		}
		if (trace) {
			cout<<" After insert: key="<<ystr<<endl;
			cm.display();
			cout<<"\nnumItem: "<<cm.num_items()<<endl;
		}
	}
	cm.flush();
	cout<<"insert sum"<<sum<<" hit: "<<hit<<endl;
	printf("eclipse: %lf seconds\n", double(clock()- t1)/CLOCKS_PER_SEC);
	cm.display();
	cout<<"numItem: "<<cm.num_items()<<"\n"<<endl;
}

template<typename T>
void run_get(T& cm) {
	cout<<"run_get"<<endl;
	int sum =0;
	int hit =0;
	clock_t t1 = clock();

	ifstream inf(inputFile.c_str());
	string ystr, vvv;
	while (inf>>ystr>>vvv) {
		sum++;
		char buff[1024];
		int klen = ystr.size();
		int vsz = cm.find(ystr.c_str(), klen, buff);
		if (vsz != -1) {
			buff[vsz] = 0;
			if(vvv == buff)
			{
				hit++;
			}
		}
		else
		{
			//cout<<"false"<<endl;
		}
		if (trace) {
			cout<<" After insert: key="<<ystr<<endl;
			cm.display();
			cout<<"\nnumItem: "<<cm.num_items()<<endl;
		}
	}
	cm.flush();
	cm.display();
	
	printf("eclipse: %lf seconds\n", double(clock()- t1)/CLOCKS_PER_SEC);
	cout<<"get sum"<<sum<<" hit: "<<hit<<endl;
	cout<<"numItem: "<<cm.num_items()<<"\n"<<endl;
}

template<typename T>
void run_seq(T& cm) {

cout<<"run_seq"<<endl;
	clock_t t1 = clock();

	vector<std::string> result;
	typename T::Cursor locn;
	locn = cm.get_first_locn();
	char kbuff[1024];
	char vbuff[1024];
	size_t klen = 1024;
	size_t vlen = 1024;
	int a=0;
	while (cm.get(locn, kbuff, klen, vbuff, vlen) ) {
		a++;
		result.push_back(std::string(kbuff,klen));
		if( !cm.seq(locn) )
			break;
		if (trace)
			cout<<kbuff<<endl;
		klen = 1024;
		vlen = 1024;
	}
	
	cout<<"\nseq testing...."<<endl;
	
	cout<<"\nsepnum "<<a<<endl;
	for (unsigned int i=0; i<result.size(); i++) {
		if (trace) {
			cout<<result[i]<<endl;
		}
	}
	cm.flush();
	printf("eclipse: %lf seconds\n", double(clock()- t1)/CLOCKS_PER_SEC);
	cm.display();
	cout<< "finish getValue "<<endl;
	cout<<"numItem: "<<cm.num_items()<<"\n"<<endl;
}

template<typename T>
void run_del(T& cm) {
	cout<<"run_del"<<endl;
	clock_t t1 = clock();

	ifstream inf(inputFile.c_str());
	string ystr, vvv;
	//size_t items = cm.num_items();
	while (inf>>ystr>>vvv) {
		cm.del(ystr.c_str(), ystr.size());

		if (trace) {
			cout<< "after delete: key="<<ystr<<endl;
			cout<<"\nnumItem: "<<cm.num_items()<<endl;
			cm.display();
		}
	}
	cm.flush();
	printf("eclipse: %lf seconds\n", double(clock()- t1)/CLOCKS_PER_SEC);
	cm.display();
	cout<<"numItem: "<<cm.num_items()<<"\n"<<endl;

}


template<typename T>
void run(T& cm) {
	int op = 31;
	cout<<"op="<<op<<endl;
	if (op & 1)
		run_insert(cm);
	if (op & 2)
		run_get(cm);
	if (op & 4)
		run_seq(cm);
	if (op & 8)
		run_del(cm);
	cm.flush();
}

#define TEST(DB) \
do{\
	DB db(indexFile);\
	db.setDegree(degree);\
	db.setCacheSize(cacheSize);\
	db.setPageSize(pageSize);\
	db.open();\
	run(db);\
	if(argc == 3 && argv[2][0] == 'o')\
		db.optimize();\
	db.close();\
}\
while(0)


void showRange(std::vector<btreedb::DataPack> &ll)
{
	for(size_t ii = 0 ; ii < ll.size(); ++ii)
	{
		std::string key((char*)ll[ii].key, ll[ii].klen);
		std::string val((char*)ll[ii].val, ll[ii].vlen);
		std::cout<<key<<" -> "<<val<<endl;
	}
	cout<<endl;
}


void run_seq2(btreedb& cm) {

cout<<"run_seq"<<endl;
	clock_t t1 = clock();

	vector<std::string> result;
	btreedb::Cursor locn;
	locn = cm.get_last_locn();
	char kbuff[1024];
	char vbuff[1024];
	size_t klen = 1024;
	size_t vlen = 1024;
	int a=0;
	while (cm.get(locn, kbuff, klen, vbuff, vlen) ) {
		a++;
		result.push_back(std::string(kbuff,klen));
		if( !cm.seq(locn, false) )
			break;
		if (trace)
			cout<<kbuff<<endl;
		klen = 1024;
		vlen = 1024;
	}
	
	cout<<"\nlseq testing...."<<endl;
	
	cout<<"\nlsepnum "<<a<<endl;
	for (unsigned int i=0; i<result.size(); i++) {
		if (trace) {
			cout<<result[i]<<endl;
		}
	}
	cm.flush();
	printf("eclipse: %lf seconds\n", double(clock()- t1)/CLOCKS_PER_SEC);
	cm.display();
	cout<< "finish getValue "<<endl;
	cout<<"numItem: "<<cm.num_items()<<"\n"<<endl;
}


int main(int argc, char **argv)
{
	int type = 0;
	if(argc < 2)
	{
		printf("usage:%s 0|1 [o]\n0:hash\n1:btree\no:optimize", argv[0]);
		return 0;
	}
	type = atoi(argv[1]);
	if(type == 0)
	{
		indexFile = hashfile;
		TEST(hashdb);
	}
	else
	{
		indexFile = btreefile;
		TEST(btreedb);

		btreedb db(btreefile);
		db.open();
		run_insert(db);
		run_seq2(db);
		db.optimize();
		//run_insert(db);
		std::vector<btreedb::DataPack> ll;
		db.range(0,1,ll);
		showRange(ll);
		
		db.range(10,15,ll);
		showRange(ll);
		
		db.range(100,102,ll);
		showRange(ll);
		
		db.range(1000,1002,ll);
		showRange(ll);
		
		db.range(1000,1005,ll);
		showRange(ll);
		
		db.range(7990,8005,ll);
		showRange(ll);
		
		db.close();

	}
	
	return 0;
}
