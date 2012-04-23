#ifndef __SMARTPTR_H_2008__
#define __SMARTPTR_H_2008__

/// @file smartptr.h
/// @brief smart pointers, have serveral applications:
///   -# deallocate objects when they go out of the scope,
///      eg. cleanup, autodel
///   -# automatically allocate/dealloc objects when needed,
///      eg. autoref, counted
///   -# utility functors for containers of pointers (adapted 
///      from Scott Meyers' <em>Effective STL</em>)
/// @author Qingshan.Luo
/// @date Mar. 2008, Aug. 2011

namespace sax {

/// @brief General class for remembering cleanup actions.
///
/// Declare a cleanup function as
/// <pre>
/// JpegImage *image = jpeg_open(...);
/// cleanup image_cleanup(jpeg_close,image);
/// </pre>
/// If you don't want the cleanup action to be executed, 
/// you can call the "forget" method, eg.
/// <pre>
/// image_cleanup.forget();
/// jpeg_close(image);
/// </pre>
class cleanup {
private:
	/// internal helper class.
	struct clean_base {
		virtual void clean() = 0;
		virtual ~clean_base() {}
	};
	
	/// an instance class
	template <class F, class T>
	class cleaner : public clean_base
	{
	private:
		F _f;
		T _t;
	public:
		cleaner(F f, T t):_f(f),_t(t) {}
		void clean() {_f(_t);}
	};
	
	clean_base *inst;
	cleanup() : inst(0) {}

public:
	template <class F, class T>
	cleanup(F f, T t) {
		inst = new cleaner<F,T>(f,t);
	}
	
	/// Tell the cleanup object to forget about cleaning
	/// up the target object.
	void forget() {
		if (inst) {
			delete inst;  inst = 0;
		}
	}

	/// Destroy the cleanup object, cleaning up the target 
	/// by calling the cleanup function if necessary.
	~cleanup() {
		if (inst) {
			inst->clean(); 
			delete inst;  inst = 0;
		}
	}
};

/// @brief Automatic deletion, linear assignment.
///
/// A smart pointer class that deletes the pointer it holds when it
/// goes out of scope.  Assignment is like it is for linear types: on
/// assignment, the pointer gets moved to the destination, and the
/// source gets set to NULL (this is convenient for returning values,
/// for use in lists, and similar settings).
template <class TYPE>
class autodel {
private:
	autodel(autodel<TYPE> &src);
	TYPE *inst;
public:
	/// Default constructor sets pointer to null.
	autodel() : inst(0) {}

	/// Destructor deletes any pointer held by the class.
	~autodel() {
		if (inst) {
			delete inst;  inst = 0;
		}
	}

	/// Initialization with a pointer,
	/// transfers ownership to the class.
	explicit autodel(TYPE *src) : inst(src) {}

	/// Assignment of a pointer, deletes the old pointer held 
	/// by the class and transfers ownership of the pointer.
	void operator = (TYPE *src) {
		if (inst != src) {
			if (inst) delete inst;
			inst = src;
		}
	}

	/// smart pointer dereference
	TYPE *operator->() const {
#if defined(_DEBUG)
		if (!inst) throw "sax::autodel(->): dereference null pointer";
#endif // _DEBUG
		return inst;
	}

	/// Explicit pointer dereference
	TYPE &operator*() const {
#if defined(_DEBUG)
		if (!inst) throw "sax::autodel(*): dereference null pointer";
#endif // _DEBUG
		return *inst;
	}

	/// Conversion to pointer.
	TYPE *get() const {
		return inst;
	}

	/// Testing whether the pointer is null.
	bool operator!() const {
		return !inst;
	}

	/// Testing whether the pointer is null.
	operator bool() const {
		return !!inst;
	}
	
	/// Linear assignment: get the pointer from the other 
	/// smart pointer, and set the other smart pointer to null.
	autodel<TYPE>& operator = (autodel<TYPE> &src)
	{
		TYPE *tmp = src.inst;
		src.inst = 0;
		if (inst != tmp) {
			if (inst) delete inst;
			inst = tmp;
		}
		return *this;
	}

	autodel<TYPE>& swap(autodel<TYPE> &src)
	{
		TYPE *tmp = src.inst;
		src.inst = this->inst;
		this->inst = tmp;
		return *this;
	}

	/// Take ownership away from this smart pointer and 
	/// set the smart pointer to other.
	TYPE *swap(TYPE *src)
	{
		TYPE *ret = inst;
		inst = src;
		return ret;
	}
};

/// @brief Automatic allocation and deletion.
///
/// A smart pointer class that automatically allocates an object
/// when an attempt is made to access and dereference the pointer.
/// Assignment is linear (ownership transfers from the source of
/// the assignment to the destination, and the source sets to null).
template <class TYPE>
class autoref {
private:
	TYPE *inst;
	autoref(autoref<TYPE> &);
	
public:
	/// Default initializer, sets object to null.
	autoref() : inst(0) {}

	/// Destructor deallocates object, if any.
	~autoref() { dealloc(); }

	/// smart pointer dereference allocates object if none is held, 
	/// then returns a pointer. This always succeeds.
	TYPE *operator->() {
		if (!inst) inst = new TYPE();
		return inst;
	}

	/// Pointer dereference allocates object if none is held, 
	/// then returns a pointer. This always succeeds.
	TYPE &operator*() {
		if (!inst) inst = new TYPE();
		return *inst;
	}

	/// Conversion to pointer.  Does not allocate an object.
	TYPE *get() {
		return inst;
	}

	/// Set to new pointer value, deleting any old object, 
	/// and transfering ownership to the class.
	autoref<TYPE> &operator=(TYPE *src) {
		if (inst != src) {
			if (inst) delete inst;
			inst = src;
		}
		return *this;
	}

	/// Linear assignment: get the pointer from other smart 
	/// pointer, and set the other smart pointer to null.
	autoref<TYPE> &operator=(autoref<TYPE> &src) {
		TYPE *tmp = src.inst;
		src.inst = 0;
		if (inst != tmp) {
			if (inst) delete inst;
			inst = tmp;
		}
		return *this;
	}

	/// Take ownership away from this smart pointer 
	/// and reset the smart pointer.
	TYPE *swap(TYPE *src) {
		TYPE *ret = inst;
		inst = src;
		return ret;
	}

	/// Deallocate the object.
	void dealloc() {
		if (inst) {
			delete inst;
			inst = 0;
		}
	}
};

/// @brief Automatic deletion based on reference count.
template <class TYPE>
class counted {
private:
	struct TC {
		TYPE* ptr;
		long refs;
		
		TC(TYPE *p, long r): ptr(p), refs(r) {}
		~TC() { if (ptr) delete ptr; }
	};
	mutable TC *inst;

	void incref() const 
	{
		if (inst) inst->refs++;
	}
	
	void decref() const 
	{
		if (inst && --inst->refs==0) {
			delete inst;
			inst = (TC *) 0;
		}
	}

public:
	counted() : inst(0) {
	}
	explicit counted(TYPE* ptr) : inst(0) {
		if (ptr) {
			inst = new TC(ptr, 1L);
		}
	}
	explicit counted(const counted<TYPE> &src) {
		src.incref();
		inst = src.inst;
	}
	explicit counted(counted<TYPE> &src) {
		src.incref();
		inst = src.inst;
	}
	~counted() {
		decref();
		inst = 0;
	}
	counted<TYPE> &operator=(const counted<TYPE> &src) {
		if(this==&src) return *this;
		src.incref();
		decref();
		inst = src.inst;
		return *this;
	}
	counted<TYPE> &operator=(TYPE *src) {
		if (inst==src) return *this;
		decref();
		inst = src;
		return *this;
	}
	void swap(const counted<TYPE> &src) {
		if (this != &src)
		{
			TC *tmp = inst;
			inst = src.inst;
			src.inst = tmp;
		}
	}
	TYPE* get() const {
		return (inst ? inst->ptr : (TYPE*)0);
	}

	operator bool() const {
		return (inst && inst->ptr);
	}
	void reset() {
		decref();
		inst = 0;
	}
	void reset(TYPE* ptr) {
		counted<TYPE>(ptr).swap(*this);
	}
	
	TYPE &operator*() {
		if (!inst) throw "sax::counted(): uninitialized";
		return *(inst->ptr);
	}
	TYPE *operator->() {
		if (!inst) throw "sax::counted(): uninitialized";
		return inst->ptr;
	}
	operator TYPE&() {
		if (!inst) throw "sax::counted(): uninitialized";
		return *(inst->ptr);
	}
	operator TYPE*() {
		if (!inst) throw "sax::counted(): uninitialized";
		return inst->ptr;
	}
	
	long use_count() const {
		return (inst ? inst->refs : 0);
	}
	
	bool unique() const {
		return (use_count()==1);
	}
};

/// @brief return objects pointed by a container of pointers.
/// A typical usage might be like:
/// @code
/// vector<Object*> v;
/// ...
/// transform(v.begin(), v.end(),
///           ostream_iterator<Object>(cout, " "),
///           dereference());
/// @endcode
struct dereference
{
	template <typename _Tp>
	const _Tp& operator()(const _Tp* __ptr) const
	{
		return *__ptr;
	}
};

/// @brief compare objects pointed by a container of pointers.
/// @code
/// vector<Object*> v;
/// ...
/// sort(v.begin(), v.end(), dereference_less());
/// @endcode
/// or
/// @code
/// set<Object*, dereference_less> s;
/// @endcode
struct dereference_less
{
	template <typename _Pointer>
	bool operator()(_Pointer __ptr1, _Pointer __ptr2) const
	{
		return *__ptr1 < *__ptr2;
	}
};

/// @brief delete objects pointed by a container of pointers.
/// A typical usage might be like:
/// @code
/// list<Object*> l;
/// ...
/// for_each(l.begin(), l.end(), delete_object());
/// @endcode
struct delete_object
{
	template <typename _Pointer>
	void operator()(_Pointer __ptr) const
	{
		delete __ptr;
	}
};

/// @brief output objects pointed by a container of pointers.
/// A typical usage might be like:
/// @code
/// list<Object*> l;
/// ...
/// for_each(l.begin(), l.end(), output_object<ostream>(cout, " "));
/// @endcode
template <typename _OutputStrm, typename _StringType = const char*>
struct output_object
{
	output_object(_OutputStrm& __outs, const _StringType& __sep)
		: _M_outs(__outs), _M_sep(__sep)
	{}

	template <typename _Tp>
	void operator()(const _Tp* __ptr) const
	{
		_M_outs << *__ptr << _M_sep;
	}

private:
	_OutputStrm& _M_outs;
	_StringType  _M_sep;
};


} // namespace sax

#endif//__SMARTPTR_H_2008__

