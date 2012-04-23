#include "c++/stage.h"
#include "logger.h"
#include "os_api.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace sax {

//------------------------------------------------------------------------
// define events involved
struct msg64_type: public sax::event_base<1>{int len; char msg[64];};
struct msg96_type: public sax::event_base<2>{int len; char msg[96];};

struct msg128_type: public sax::event_base<3>{int len; char msg[128];};
struct msg256_type: public sax::event_base<4>{int len; char msg[256];};
struct msg512_type: public sax::event_base<5>{int len; char msg[512];};

struct msg1024_type: public sax::event_base<6>{int len; char msg[1024];};
struct msg2048_type: public sax::event_base<7>{int len; char msg[2048];};
struct msg4096_type: public sax::event_base<8>{int len; char msg[4096];};
struct msg8192_type: public sax::event_base<9>{int len; char msg[8192];};

struct mark_begin_type : public sax::event_base<11> 
{
	uint64_t stamp;
	char cmd[36];
	uint64_t trans_key;
};

struct mark_done_type : public sax::event_base<12> 
{
	uint64_t stamp;
	uint64_t trans_key;
	bool succeed;
};

//------------------------------------------------------------------------
// define parameter structure, and implement the main logic of IO
struct log_param {
	char path[256];
	size_t size;
	size_t num;
	logger *owner;
};

static void do_fname(
	char buf[], int sz, const char *path, const char *ext, int index)
{
	if (index <= 0) g_snprintf(buf, sz, "%s.%s", path, ext);
	else g_snprintf(buf, sz, "%s.%s.%d", path, ext, index);
}

static void do_rotate(const char *path, size_t num, const char *ext)
{
	char fn[2][256];
	size_t i = num;
	do_fname(fn[i%2], sizeof(fn[i%2]), path, ext, i);
	while (--i > 0) {
		do_fname(fn[i%2], sizeof(fn[i%2]), path, ext, i);
		if (g_exists_file(fn[i%2])) {
			g_move_file(fn[i%2], fn[(i+1)%2]);
		}
	}
	do_fname(fn[0], sizeof(fn[0]), path, ext, -1);
	g_move_file(fn[0], fn[1]);
}

static bool do_saveto(const sax::log_param &par,
	size_t len, const char *msg, FILE *&out, const char *ext)
{
	if (!out) {
		char buf[256];
		do_fname(buf, sizeof(buf), par.path, ext, -1);
		if (!(out = fopen(buf, "ab"))) return false;
	}
	
	// write msg, but may be in the buffer
	if (!len) {
		len = strlen(msg); 
		if (!len) return false;
	}
	size_t got = fwrite(msg, 1, len, out);
	if (got != len) return false;
	
	size_t fsize = ftell(out);
	if (fsize >= par.size) {
		fclose(out);
		out = NULL;
		do_rotate(par.path, par.num, ext);
	}
	return true;
}

//------------------------------------------------------------------------
// define the main logic for stage
class log_handler : public sax::handler_base
{
public:
	enum {TIMER_FLUSH=1, TIMER_STAT=2};
	
	log_handler() : _log(0), _stat(0) {}
	
	bool init(sax::timer_base *timer, void* param) 
	{
		this->_timer = timer;
		this->_timer->init(8, 256, 50);
		memcpy(&_param, param, sizeof(_param));
		return true;
	}
	void on_start(int thread_id) 
	{
		_timer->start(3*1000, (void *) TIMER_FLUSH);
		_timer->start(5*1000, (void *) TIMER_STAT);
	}
	void on_event(const sax::event_type *ev, int thread_id) 
	{
		switch (ev->get_type())
		{
			case msg64_type::ID:
			case msg96_type::ID: 
			case msg128_type::ID:
			case msg256_type::ID:
			case msg512_type::ID:
			case msg1024_type::ID:
			case msg2048_type::ID:
			case msg4096_type::ID:
			case msg8192_type::ID: {
				msg64_type *p = (msg64_type *) ev;
				do_saveto(_param, p->len, p->msg, _log, "log");
				break;
			}
			case mark_begin_type::ID: {
				mark_begin_type *p = (mark_begin_type *) ev;
				this->do_mark_begin(p); break;
			}
			case mark_done_type::ID: {
				mark_done_type *p = (mark_done_type *) ev;
				this->do_mark_done(p); break;
			}
			default: {
				do_saveto(_param, 0, "Bad event ID", _log, "log");
			}
		}
	}
	void on_timeout(uint32_t id, void *param)
	{
		switch ((long) param)
		{
		case TIMER_FLUSH: 
			if (_log) fflush(_log);
			_timer->start(3*1000, param);
			break;
		case TIMER_STAT: 
			this->do_cmd_stat();
			_timer->start(5*1000, param);
			break;
		}
	}
	~log_handler() {
		if (_log) fclose(_log);
		if (_stat) fclose(_stat);
	}
	
protected:
	void do_cmd_stat();
	void do_mark_begin(mark_begin_type *ev);
	void do_mark_done(mark_done_type *ev);
	
private:
	sax::log_param _param;
	sax::timer_base *_timer;
	FILE *_log, *_stat;
};

void log_handler::do_cmd_stat()
{
	LOG_ERROR(_param.owner, "stat");
}

void log_handler::do_mark_begin(mark_begin_type *ev)
{
}

void log_handler::do_mark_done(mark_done_type *ev)
{
}

//------------------------------------------------------------------------
// implement the logic of logger
logger::~logger()
{
	if (_biz) {_biz->halt(); delete _biz;}
}

bool logger::start(const char *path, size_t size, size_t num)
{
	if (_biz) {_biz->halt(); delete _biz;}
	
	sax::log_param param;
	memset(param.path, 0, sizeof(param.path));
	g_strlcpy(param.path, path, sizeof(param.path));
	param.size = (size>1024 ? size : 1024);
	param.num = (num>1 ? num : 1);
	param.owner = this;
	
	const int cap = 8*1024; // queue capacity
	_biz = sax::create_stage<log_handler>(
		"logger", cap, 1, true, cap*256, &param);
	return (_biz != NULL);
}

void logger::halt()
{
	if (_biz) _biz->halt();
}

void logger::setLevel(LEVEL_ENUM neu)
{
	if (neu < LEVEL_TRACE) _level = LEVEL_TRACE;
	else if (neu > LEVEL_ERROR) _level = LEVEL_OFF;
	else _level = neu;
}

uint64_t logger::markBegin(const char *cmd)
{
	if (!_biz) return 0;
	
	uint64_t key = 10000;
	
	mark_begin_type *ev = _biz->new_event<mark_begin_type>();
	if (!ev) return 0;
	
	ev->stamp = g_now_us();
	g_strlcpy(ev->cmd, cmd, sizeof(ev->cmd));
	ev->trans_key = key;
	
	if (_biz->push_event(ev, 0.1)) return key;
	_biz->del_event(ev); return 0;
}

bool logger::markDone(uint64_t key, bool succeed)
{
	if (!_biz) return false;
	
	mark_done_type *ev = _biz->new_event<mark_done_type>();
	if (!ev) return false;
	
	ev->stamp = g_now_us();
	ev->trans_key = key;
	ev->succeed = succeed;
	
	if (_biz->push_event(ev, 0.1)) return true;
	_biz->del_event(ev); return false;
}

bool logger::dispatch(const char *msg, size_t len)
{
	if (!_biz) return false;
	
	if (!len) len = strlen(msg);
	event_type *ev = NULL;
	
	if (len <= 64) ev = _biz->new_event<msg64_type>();
	else if (len <= 96) ev = _biz->new_event<msg96_type>();
	else if (len <= 128) ev = _biz->new_event<msg128_type>();
	else if (len <= 256) ev = _biz->new_event<msg256_type>();
	else if (len <= 512) ev = _biz->new_event<msg512_type>();
	else if (len <= 1024) ev = _biz->new_event<msg1024_type>();
	else if (len <= 2048) ev = _biz->new_event<msg2048_type>();
	else if (len <= 4096) ev = _biz->new_event<msg4096_type>();
	else if (len <= 8192) ev = _biz->new_event<msg8192_type>();
	
	if (!ev) return false;
	
	((msg64_type *)ev)->len = len;
	memcpy(((msg64_type *)ev)->msg, msg, len);
	
	if (_biz->push_event(ev, 1.0)) return true;
	_biz->del_event(ev); return false;
}

#ifdef _MSC_VER
#define vsnprintf(a,b,c,d) _vsnprintf(a,b,c,d)
#endif

static inline int outp_head(
	char buf[], LEVEL_ENUM level, const char *file, int line)
{
	static const char *names[] = {
		"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "OFF"
	}; // coincide with LEVEL declaration
	
	uint64_t now = g_now_ms();
	int ms = now % 1000;
	time_t tick = (time_t) (now / 1000);
	
	struct tm st;
	g_localtime(&tick, &st, g_timezone()*3600);
	
	return sprintf(buf, 
		"[%04d/%02d/%02d %02d:%02d:%02d.%03d] %-5s (%s:%d) ", 
		st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, 
		st.tm_min, st.tm_sec, ms, names[(int)level], file, line);
}

void logger::output(LEVEL_ENUM level, 
	const char *file, int line, const char *fmt, ...)
{
	if (level >= this->_level) {
		if (level > LEVEL_OFF) level = LEVEL_OFF;
		char buf[8*1024]; //coincide with msg8192_type
		int got = outp_head(buf, level, file, line);
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(&buf[got], sizeof(buf)-got, fmt, ap);
		va_end(ap);
		g_strlcat(buf, "\n", sizeof(buf));
		this->dispatch(buf, 0);
	}
}

//------------------------------------------------------------------------
// override the operator () for logger_helper
void logger_helper::operator ()(logger *log, char *fmt, ...)
{
	if (_level >= log->_level) {
		if (_level > LEVEL_OFF) _level = LEVEL_OFF;
		char buf[8*1024]; //coincide with msg8192_type
		int got = outp_head(buf, _level, _file, _line);
		va_list ap;
		va_start(ap, fmt);
		vsnprintf(&buf[got], sizeof(buf)-got, fmt, ap);
		va_end(ap);
		g_strlcat(buf, "\n", sizeof(buf));
		log->dispatch(buf, 0);
	}
}

//------------------------------------------------------------------------
// constructor: initialize the headers etc.
logger_stream::logger_stream(
	LEVEL_ENUM level, int line, const char *file, logger *log) :
	 _level(level), _line(line), _file(file), _log(log)
{
	if (_level >= _log->_level) {
		int got = outp_head(_buf, _level, _file, _line);
		int size = sizeof(_buf) - got - 1; // 1 byte free
		_os = new std::ostrstream(&_buf[got], size);
	}
	else {
		_os = new std::ostrstream(_buf, 1); // size > 0!
	}
}

// deconstructor: send data immediately
logger_stream::~logger_stream()
{
	if (_level >= _log->_level) {
		char *ptr = _os->str();
		ptr += _os->pcount();
		*ptr++ = '\n'; // using 1 byte
		_log->dispatch(_buf, ptr-_buf);
	}
	delete _os;
}

//------------------------------------------------------------------------

} // namespace
