#ifndef __BINARY_H_2010__
#define __BINARY_H_2010__

/**
 * @file binary.h
 * @brief a read/write binary buffer for fast IO
 *
 * @author Qingshan
 * @date 2010-5-30
 */

namespace sax {

struct binary {

public:
	inline binary() : _start(0), _end(0), _data(0), _free(0)
	{
	}

	inline ~binary() 
	{
		this->destroy();
	}

	inline char *getData() 
	{
		return _data;
	}

	inline int getDataLen() 
	{
		return (_free - _data);
	}

	inline char *getFree() 
	{
		return _free;
	}

	inline int getFreeLen() 
	{
		return (_end - _free);
	}

	// move the data-reading cursor
	inline void drain(int len) 
	{
		_data += len;

		if (_data >= _free) {
			clear();
		}
	}

	// move the data-writing cursor
	inline bool pour(int len) 
	{
		if (_end - _free >= len)
		{
			_free += len;
			return true;
		}
		return false;
	}

	// roll back the poured
	inline bool strip(int len) 
	{
		if(_free - _data >= len)
		{
			_free -= len;
			return true;
		}
		return false;
	}

	/// reset pointers
	inline void clear() 
	{ 
		_data = _free = _start;
	}

	/// release memory
	void destroy();

	/// compact spaces
	void shrink();
	
	/// ensure (expand) the size of free space
	bool ensure(int need);
	
	// strstr(), binary-safe!
	int find(const void *pattern, int len);

private:
	char *_start;
	char *_end;	
	char *_data;
	char *_free;
};

} // namespace

#endif //__BINARY_H_2010__

