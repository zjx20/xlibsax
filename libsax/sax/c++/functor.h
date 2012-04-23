#ifndef __FUNCTOR_H_2008__
#define __FUNCTOR_H_2008__

/// @file functor.h
/// @brief smart functors
/// @author Qingshan.Luo
/// @date March, 2008 (port)
/// #note http://www.newty.de/fpt/functor.html#chapter4

namespace sax {

/// internal helper class.
template<class R>
class functor_base {
public:
	virtual R operator()() = 0;
	virtual R operator()() const =0;
	virtual ~functor_base(){}
};

template<class F=void()>
class functor;

// Part A: for generic pointer of funcs (ptr_func)

/// functor for zero args
template<class R>
class functor<R()> : public functor_base<R> {
public:
	typedef R(*func_type)();

	functor(func_type func)
		:func_(func){}
	
	R operator()(){
		return func_();
	}
	
	R operator()() const {
		return func_();
	}
	
private:
	func_type  func_;
};

/// functor for one args
template<class R, class A1>
class functor<R(A1)> : public functor_base<R> {
public:
	typedef R(*func_type)(A1);

	functor(func_type func, A1 a1)
		:func_(func), a1_(a1){}

	R operator()(){
		return func_(a1_);
	}
	
	R operator()() const{
		return func_(a1_);
	}

private:
	func_type  func_;
	A1  a1_;
};

/// functor for two args
template<class R,class A1,class A2>
class functor<R(A1,A2)> : public functor_base<R> {
public:
	typedef R(*func_type)(A1,A2);

	functor(func_type func, A1 a1, A2 a2)
      :func_ (func), a1_(a1), a2_(a2){}

	R operator()(){
		return func_(a1_, a2_);
	}

	R operator()() const{
		return func_(a1_, a2_);
	}
private:
	func_type  func_;
	A1  a1_;
	A2  a2_;
};


/// functor for three args
template<class R,class A1,class A2,class A3>
class functor<R(A1,A2,A3)> : public functor_base<R> {
public:
	typedef R(*func_type)(A1,A2,A3);

	functor(func_type func, A1 a1, A2 a2, A3 a3)
		:func_ (func), a1_(a1), a2_(a2), a3_(a3){}

	R operator()() {
		return func_(a1_, a2_, a3_);
	}

	R operator()() const {
		return func_(a1_, a2_, a3_);
	}
private:
	func_type  func_;
	A1  a1_;
	A2  a2_;
	A3  a3_;
};


// Part B: for member funcs (mem_func)

/// functor for zero args
template<class T, class R>
class functor<R (T::*)()> : public functor_base<R> {
public:
	typedef R(T::*func_type)();

	functor(func_type func, T *t)
		:t_(*t), func_(func){}

	R operator()(){
		return (t_.*func_)();
	}

	R operator()() const {
		return (t_.*func_)();
	}
private:
	T  &t_;
	func_type  func_;
};

/// functor for one args
template<class T, class R, class A1>
class functor<R (T::*)(A1)> : public functor_base<R> {
public:
	typedef R(T::*func_type)(A1);

	functor(func_type func, T *t, A1 a1)
		:t_(*t), func_(func),a1_(a1){}

	R operator()(){
		return (t_.*func_)(a1_);
	}

	R operator()() const{
		return (t_.*func_)(a1_);
	}
private:
	T  &t_;
	func_type  func_;
	A1  a1_;
};

/// functor for two args
template<class T, class R, class A1, class A2>
class functor<R (T::*)(A1,A2)> : public functor_base<R> {
public:
	typedef R(T::*func_type)(A1,A2);

	functor(func_type func, T *t, A1 a1, A2 a2)
		:t_(*t), func_(func), a1_(a1), a2_(a2){}

	R operator()(){
		return (t_.*func_)(a1_, a2_);
	}

	R operator()() const {
		return (t_.*func_)(a1_, a2_);
	}
private:
	T  &t_;
	func_type  func_;
	A1  a1_;
	A2  a2_;
};

/// functor for three args
template<class T, class R, class A1, class A2, class A3>
class functor<R (T::*)(A1,A2,A3)> : public functor_base<R> {
public:
	typedef R(T::*func_type)(A1,A2,A3);

	functor(func_type func, T *t, A1 a1, A2 a2, A3 a3)
		:t_(*t), func_(func), a1_(a1), a2_(a2), a3_(a3){}

	R operator()(){
		return (t_.*func_)(a1_, a2_, a3_);
	}

	R operator()() const {
		return (t_.*func_)(a1_, a2_, a3_);
	}
private:
	T  &t_;
	func_type  func_;
	A1  a1_;
	A2  a2_;
	A3  a3_;
};


// Part C: bind, the core helper function

template<class F>
functor<F> bind(F func)
{
	return functor<F>(func);
}

template<class F, class T1>
functor<F> bind(F func, T1 t1)
{
	return functor<F>(func, t1);
}

template<class F, class T1, class T2>
functor<F> bind(F func, T1 t1, T2 t2)
{
	return functor<F>(func, t1, t2);
}

template<class F, class T1, class T2, class T3>
functor<F> bind(F func, T1 t1, T2 t2, T3 t3)
{
	return functor<F>(func, t1, t2, t3);
}

template<class F, class T1, class T2, class T3, class T4>
functor<F> bind(F func, T1 t1, T2 t2, T3 t3, T4 t4)
{
	return functor<F>(func, t1, t2, t3, t4);
}

} // namespace sax

#endif  //__FUNCTOR_H_2008__

