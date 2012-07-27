#ifndef __SYSUTIL_H_QS__
#define __SYSUTIL_H_QS__

/**
 * @file sysutil.h
 * @brief c++ wrapper for some objects defined in os_api.h
 *
 * @author Qingshan
 * @date 2011-3-28
 */

#include <stdlib.h>
#include <memory.h>

#include "os_api.h"
#include "timer.h"
#include "blowfish.h"
#include "mt_rand.h"

#if defined(__cplusplus) || defined(c_plusplus)

namespace sax {

/// @brief a wrapper for thread pool via g_thread_t
/// @remark derive the function on_routine(), and check
/// the stop_flag by is_stopped() in its iterations
class thread_mgr
{
public:
	thread_mgr();
	virtual ~thread_mgr();
	int run(int n); // n>0: number of threads
	void stop();
protected:
	virtual void on_routine(int i)=0;
	bool is_stopped();
private:
	volatile long _stop_flag;
	volatile long _num_alive;
	static void* proc(void *user);
};

///// @brief a wrapper for Single Timer Wheel Timer
///// @remark derive the function on_timeout()
//class timer_base
//{
//public:
//	inline timer_base() : _stw(0) {}
//	virtual ~timer_base() {
//		if (_stw) g_timer_destroy(_stw);
//	}
//	inline uint32_t start(uint32_t delay, void *param) {
//		return (_stw ? g_timer_start(_stw,
//			delay, &timer_base::proc, this, param) : 0);
//	}
//	inline bool cancel(uint32_t id) {
//		return _stw && g_timer_cancel(_stw, id);
//	}
//	inline bool init(uint32_t capacity,
//		uint32_t wheel_size=1024, uint32_t granularity=10)
//	{
//		if (_stw) g_timer_destroy(_stw);
//		_stw = g_timer_create(capacity, wheel_size, granularity);
//		return (NULL != _stw);
//	}
//	inline void poll() {
//		if (_stw) g_timer_loop(_stw);
//	}
//protected:
//	virtual void on_timeout(uint32_t id, void *param)=0;
//private:
//	g_timer_t *_stw;
//	static void proc(uint32_t id, void *client, void *param);
//};

class dummy_lock
{
public:
	inline dummy_lock() {}
	inline ~dummy_lock() {}
	inline void enter() {}
	inline void leave() {}
};

/// an inline-wrapper for g_mutex_t
class mutex_type
{
public:
	friend class cond_type;
	friend class auto_mutex;
	inline  mutex_type() {_pi=g_mutex_init();}
	inline ~mutex_type() {g_mutex_free(_pi);}
	inline void enter() {g_mutex_enter(_pi);}
	inline void leave() {g_mutex_leave(_pi);}
	inline int try_enter(double sec) {return g_mutex_try_enter(_pi, sec);}
private:
	g_mutex_t *_pi;
};

/// an inline-wrapper for g_share_t
class share_type
{
public:
	friend class share_rwlock;
	inline  share_type() {_pi=g_share_init();}
	inline ~share_type() {g_share_free(_pi);}
	inline void lockw() {g_share_lockw(_pi);}
	inline int try_lockw(double sec) {return g_share_try_lockw(_pi, sec);}
	inline void lockr() {g_share_lockr(_pi);}
	inline int try_lockr(double sec) {return g_share_try_lockr(_pi, sec);}
	inline void unlock() {g_share_unlock(_pi);}
private:
	g_share_t *_pi;
};

/// an inline-wrapper for g_spin_t
class spin_type
{
public:
	friend class auto_rwlock;
	inline  spin_type(int spin_times = 16) {_pi=g_spin_init(); _spin_times = spin_times;}
	inline ~spin_type() {g_spin_free(_pi);}
	inline void enter() {g_spin_enter(_pi, _spin_times);}
	inline void leave() {g_spin_leave(_pi);}
private:
	g_spin_t *_pi;
	int _spin_times;
};

/// an inline-wrapper for g_sema_t
class sema_type
{
public:
	inline sema_type(uint32_t cap=1024, uint32_t cnt=0) 
	{
		_pi = g_sema_init(cap, cnt);
	}
	inline ~sema_type() {g_sema_free(_pi);}
	inline int post() {return g_sema_post(_pi);}
	inline int wait(double sec) {return g_sema_wait(_pi, sec);}
private:
	g_sema_t *_pi;
};

/// an inline-wrapper for g_cond_t
class cond_type
{
public:
	inline  cond_type() {_pi=g_cond_init();}
	inline ~cond_type() {g_cond_free(_pi);}
	inline void signal() {g_cond_signal(_pi);}
	inline int wait(g_mutex_t *mtx, double sec) {return g_cond_wait(_pi, mtx, sec);}
	inline int wait(mutex_type *mtx, double sec) {return g_cond_wait(_pi, mtx->_pi, sec);}
	inline void broadcast() {g_cond_broadcast(_pi);}
private:
	g_cond_t *_pi;
};

/**
* @brief a simple class encapsulating lock/unlock among threads
*        support mutex_type, spin_type and dummy_lock
*/
template <typename T>
class auto_lock
{
public:
	inline auto_lock(T& lock) : _lock(lock) {
		_lock.enter();
	}

	inline auto_lock(T* lock) : _lock(*lock) {
		_lock.enter();
	}

	inline ~auto_lock() {
		_lock.leave();
	}

private:
	T& _lock;
};

/**
* @brief a simple class encapsulating mutex lock/unlock among threads
* @see g_mutex_enter(), g_mutex_leave().
*/
class auto_mutex
{
public:
	/// @brief using constructor to enter a mutex.
	/// @param p the pointer of a g_mutex_t.
	inline auto_mutex(g_mutex_t *p) : _mtx(p)
	{
		if (_mtx) g_mutex_enter(_mtx);
	}

	inline auto_mutex(mutex_type *p) : _mtx(p->_pi)
	{
		if (_mtx) g_mutex_enter(_mtx);
	}

	/// @brief using deconstructor to leave a mutex.
	inline ~auto_mutex() {
		if(_mtx) g_mutex_leave(_mtx); 
	}

private:
	g_mutex_t *_mtx; ///> holding the g_mutex_t handle.
};

// rwlock based on g_share_xxx:
class share_rwlock
{
public:
	inline share_rwlock(g_share_t *sh, int ex) : _sh(sh)
	{
		if (_sh) {
			if (ex) g_share_lockw(_sh);
			else    g_share_lockr(_sh);
		}
	}
	
	inline share_rwlock(share_type *p, int ex) : _sh(p->_pi)
	{
		if (_sh) {
			if (ex) g_share_lockw(_sh);
			else    g_share_lockr(_sh);
		}
	}

	/// @brief using deconstructor to unlock s file.
	inline ~share_rwlock() {
		if (_sh) g_share_unlock(_sh);
	}
private:
	g_share_t *_sh; ///> a pointer holding the g_share_t handle.
};

/**
* @brief a simple class encapsulating file lock/unlock among processes
* @see g_flock(), g_flock2().
*/
class auto_flock
{
public:
	/// @brief using constructor to lock a file.
	/// @param fp the locking file pointer (FILE *).
	/// @param ex exclusive flag: 1 for LOCK_EX and 0 for LOCK_SH.
	inline  auto_flock(void *fp, int ex) : _fp(fp)
	{
		if(_fp) g_flock2(_fp, ex?LOCK_EX:LOCK_SH);
	}

	/// @brief using deconstructor to unlock a file.
	inline ~auto_flock() {
		if(_fp) g_flock2(_fp, LOCK_UN);
	}

private:
	void *_fp; ///> a pointer holding the FILE handle.
};

class auto_f64lock
{
public:
	/// @brief using constructor to lock a file.
	/// @param fp the locking file pointer (f64_t *).
	/// @param ex exclusive flag: 
	///   - 1 for LOCK_EX
	///   - 0 for LOCK_SH
	inline  auto_f64lock(f64_t *fp, int ex) : _fp(fp)
	{
		if(_fp) f64lock(_fp, ex ? LOCK_EX : LOCK_SH);
	}

	/// @brief using deconstructor to unlock a file.
	inline ~auto_f64lock() {
		if(_fp) f64lock(_fp, LOCK_UN);
	}

private:
	f64_t *_fp; ///> a pointer holding the file handle.
};

class auto_m64lock
{
public:
	/// @brief using constructor to lock a file.
	/// @param fp the locking file pointer (m64_t *).
	/// @param ex exclusive flag: 
	///   - 1 for LOCK_EX
	///   - 0 for LOCK_SH
	inline  auto_m64lock(m64_t *fp, int ex) : _fp(fp)
	{
		if(_fp) m64lock(_fp, ex ? LOCK_EX : LOCK_SH);
	}

	/// @brief using deconstructor to unlock a file.
	inline ~auto_m64lock() {
		if(_fp) m64lock(_fp, LOCK_UN);
	}

private:
	m64_t *_fp; ///> a pointer holding the file handle.
};

/// an inline-wrapper for blowfish algorithm:
class blowfish
{
public:
	inline blowfish() 
	{
		memset(&_ctx, 0, sizeof(_ctx));
	}
	inline blowfish(const uint8_t *key, int len)
	{
		blowfish_init(&_ctx, key, len);
	}
	inline void init(const uint8_t *key, int len)
	{
		blowfish_init(&_ctx, key, len);
	}
	inline void encrypt(uint32_t *xl, uint32_t *xr)
	{
		blowfish_encrypt(&_ctx, xl, xr);
	}
	inline void decrypt(uint32_t *xl, uint32_t *xr)
	{
		blowfish_decrypt(&_ctx, xl, xr);
	}
	inline int encode(uint8_t *s, int size)
	{
		return blowfish_enc(&_ctx, s, size);
	}
	inline int decode(uint8_t *s, int size)
	{
		return blowfish_dec(&_ctx, s, size);
	}

private:
	blowfish_ctx _ctx;
};

/// an inline-wrapper for Mersenne Twister RNG:
class rand_type
{
public:
	inline rand_type(uint32_t seed = 0) 
	{
		_mt = mt_seed(seed);
	}
	inline rand_type(const uint32_t *seed, int len)
	{
		_mt = mt_seed_ex(seed, len);
	}
	inline ~rand_type()
	{
		mt_kill(_mt);
	}
	inline int32_t rand()
	{
		return mt_rand(_mt);
	}
	inline uint32_t rand_u32()
	{
		return mt_rand_u32(_mt);
	}
	inline double rand_r53()
	{
		return mt_rand_r53(_mt);
	}
	inline double rand_r1()
	{
		return mt_rand_r1(_mt);
	}
	inline double rand_r2()
	{
		return mt_rand_r2(_mt);
	}
	inline double rand_r3()
	{
		return mt_rand_r3(_mt);
	}
	inline int32_t range(int32_t tmin, int32_t tmax)
	{
		return mt_range(_mt, tmin, tmax);
	}

private:
	mt_str_t  *_mt;
};

} // namespace

#endif//__cplusplus

#endif //__SYSUTIL_H_QS__

