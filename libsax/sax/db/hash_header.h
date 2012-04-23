#ifndef hash_head_H__2011_09_01_
#define hash_head_H__2011_09_01_

#include <iostream>
using namespace std;
struct hash_header{
	int magic; //set it as 0x1a9999a1, check consistence.			
	size_t bucketSize;
	size_t dpow;
	size_t cacheSize;
	size_t numItems;
	size_t nBlock; //the number of bucket allocated, it indicates the file size: sizeof(ShFileHeader) + nblock*bucketSize
	
	hash_header()
	{
		magic = 0x1a9999a1;
		bucketSize = 1024;
		dpow = 16;
		cacheSize = 1024*200;
		initialize();
	}
	
	void display(std::ostream& os = std::cout) {
		os<<"magic: "<<magic<<endl;
		os<<"bucketSize: "<<bucketSize<<endl;
		os<<"dpow: "<<dpow<<endl;
		os<<"directorySize: "<<(1<<dpow)<<endl;
		os<<"cacheSize: "<<cacheSize<<endl;
		os<<"numItem: "<<numItems<<endl;
		os<<"nBlock: "<<nBlock<<endl;

		os<<endl;
		os<<"file size: "<<nBlock*bucketSize+sizeof(hash_header)<<"bytes"<<endl;
		if(nBlock != 0) {
			os<<"average items number in bucket: "<<double(numItems)/double(nBlock)<<endl;
			os<<"average length of bucket chain: "<< double(nBlock)/double(1<<dpow)<<endl;
		}
	}

	bool toFile(FILE* f)
	{
		if (!f) return false;
		if ( 0 != fseek(f, 0, SEEK_SET) ) return false;

		fwrite(this, sizeof(hash_header), 1, f);
		return true;
	}

	bool fromFile(FILE* f)
	{
		if (!f) return false;
		if ( 0 != fseek(f, 0, SEEK_SET) ) return false;
		fread(this, sizeof(hash_header), 1, f);
		return true;
	}

	void initialize() {
		numItems = 0;
		nBlock = 0;
	}

};

#endif
