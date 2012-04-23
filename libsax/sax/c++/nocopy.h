#ifndef __NOCOPY_H_2008__
#define __NOCOPY_H_2008__

/**
 * @file nocopy.h
 * same as boost::noncopyable
 */
namespace sax {

class nocopy
{
protected:
	nocopy() {}
	~nocopy() {}
private:
	nocopy(const nocopy&);
	const nocopy& operator=(const nocopy&);
};

} //namespace

#endif //__NOCOPY_H_2008__
