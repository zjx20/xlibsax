#ifndef __TUPLE_H_QS__
#define __TUPLE_H_QS__

/**
 * @file tuple.h a simple and fast template array
 * @author (ported from mongodb: mongo/util/array.h)
 * @note please call hasSpace() before push_back(), getNext()
 */

namespace sax {

template<typename T>
class tuple {
public:
	typedef tuple<T> self_type;

	tuple( int capacity=10000 )
		: _capacity( capacity ) , _size(0) , _end(this, capacity)
	{
		_data = new T[capacity];
	}

	~tuple() {
		delete[] _data;
	}

	void clear() {
		_size = 0;
	}

	T& operator[]( int x ) {
		return _data[x];
	}

	T& getNext() {
		return _data[_size++];
	}

	void push_back( const T& t ) {
		_data[_size++] = t;
	}

	int size() {
		return _size;
	}

	bool hasSpace() {
		return _size < _capacity;
	}
	
	class iterator {
	public:
		iterator() {
			_it = 0;
			_pos = 0;
		}

		iterator( tuple * it , int pos=0 ) {
			_it = it;
			_pos = pos;
		}

		bool operator==(const iterator& other ) const {
			return _pos == other._pos;
		}

		bool operator!=(const iterator& other ) const {
			return _pos != other._pos;
		}

		iterator &operator++() {
			_pos++;
			return *this;
		}
		
		iterator operator++(int) {
			iterator tmp = *this;
			_pos++;
			return tmp;
		}

		iterator &operator--() {
			_pos--;
			return *this;
		}
		
		iterator operator--(int) {
			iterator tmp = *this;
			_pos--;
			return tmp;
		}

		T& operator*() {
			return _it->_data[_pos];
		}

	private:
		tuple * _it;
		int _pos;
		friend class tuple;
	};

	iterator begin() {
		return iterator(this);
	}

	iterator end() {
		_end._pos = _size;
		return _end;
	}

private:
	int _capacity;
	int _size;
	iterator _end;
	T * _data;
};

} // namespace

#endif //__TUPLE_H_QS__
