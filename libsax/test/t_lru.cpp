#include <stdio.h>
#include <string.h>
#include <time.h>

#include <string>
#include <stdlib.h>
#include <iostream>

#include <sax/c++/lru_map.h>

/// lru type for use in the unit tests
typedef sax::LRU_map<std::string,std::string> unit_lru_type;
/// lru POD type for use in the unit tests
typedef sax::LRU_map<int,int> unit_lru_type2;
/// Data class for testing the scoping issues with const refs
class test_big_data {
	public:
		char buffer[1000];
};
/// lru with large data for use in the unit tests
typedef sax::LRU_map<int,test_big_data> unit_lru_type3;

/// Dumps the cache for debugging.
std::string dump( unit_lru_type *L ) {
	unit_lru_type::iter_t it = L->begin();
	std::string ret("");
	for(; it != L->end(); it++) {
		ret.append(it->first);
		ret.append( ":" );
		ret.append(it->second);
		ret.append( "\n" );
	}
	//std::cout << "Dump---------------" << std::endl << ret << "-------------------" << std::endl;
	return ret;
}


#define unit_assert( msg, cond ) \
  std::cout << "  " << msg << ": " << std::flush; \
  if( !cond ) { std::cout << "FAILED" << std::endl; return false; } \
  std::cout << "PASSED" << std::endl;


/** @test Basic creation and desctruction test */
bool lru_cache_1cycle() 
{
	const std::string unit_data_1cycle_a("foo:4\n");
	const std::string unit_data_1cycle_b("bar:flower\nfoo:4\n");
	const std::string unit_data_1cycle_c("foo:4\nbar:flower\n");
	const std::string unit_data_1cycle_d("foo:moose\nbaz:Stalin\nbar:flower\n");
	const std::string unit_data_1cycle_e("foo:moose\nbar:flower\n");
	const std::string unit_data_1cycle_f("quz:xyzzy\nbaz:monkey\nfoo:moose\n");
	const std::string unit_data_1cycle_g("coat:mouse\npants:cat\nsocks:bear\n");

	unit_lru_type *L = new unit_lru_type(3);
	unit_assert( "size==0", (L->size() == 0) );

	// Checking a bogus key shouldn't alter the cache.
	L->find( "foo" );
	unit_assert( "exists() doesn't increase size", (L->size() == 0) );

	// Check insert() and exists()
	L->insert( "foo", "4" );
	unit_assert( "size==1 after insert(foo,4)", (L->size() == 1) );
	unit_assert( "check exists(foo)", L->find( "foo" ) );
	unit_assert( "contents check a)", unit_data_1cycle_a.compare( dump( L ) ) == 0 );

	// Check second insert and ordering
	L->insert( "bar", "flower" );
	unit_assert( "size==2 after insert(bar,flower)", (L->size() == 2) );
	unit_assert( "contents check b)", unit_data_1cycle_b.compare( dump( L ) ) == 0 );

	// Check touching
	L->find( "foo" );
	unit_assert( "contents check c)", unit_data_1cycle_c.compare( dump( L ) ) == 0 );

	// Insert of an existing element should result in only a touch
	L->insert( "bar", "flower" );
	unit_assert( "verify insert touches", unit_data_1cycle_b.compare( dump( L ) ) == 0 );

	// Verify that fetch works
	unit_assert( "verify fetch(bar)", ( std::string("flower").compare( *(L->find("bar")) ) == 0 ) );

	// Insert of an existing element with new data should replace and touch
	L->insert( "baz", "Stalin" );
	L->insert( "foo", "moose" );
	unit_assert( "verify insert replaces", unit_data_1cycle_d.compare( dump( L ) ) == 0 );

	// Test removal of an existing member.
	L->remove( "baz" );
	unit_assert( "verify remove works", unit_data_1cycle_e.compare( dump( L ) ) == 0 );

	// Test LRU_map removal as we add more members than max_size()
	L->insert( "baz", "monkey" );
	L->insert( "quz", "xyzzy" );
	unit_assert( "verify LRU_map semantics", unit_data_1cycle_f.compare( dump( L ) ) == 0 );

	// Stress test the implementation a little..
	const char *names[10] = { "moose", "dog", "bear", "cat", "mouse", "hat", "mittens", "socks", "pants", "coat" };
	for( int i = 0; i < 50; i++ ) {
		L->insert( names[ i % 10 ], names[ i % 9 ] );
	}
	unit_assert( "stress test a little", unit_data_1cycle_g.compare( dump( L ) ) == 0 );

	// Setup a little for the third test which verifies that scoped references inserted into the cache don't disappear.
	unit_lru_type3 *L3 = new unit_lru_type3(2);
	for( int i = 0; i < 10; i++ ) {
		test_big_data B;
		snprintf( B.buffer, 1000, "%d\n", i );
		L3->insert( i, B );
	}
	
	test_big_data* B = L3->find( 9 );
	unit_assert( "scope check element L3[1]", ( strncmp( B->buffer, "9\n", 1000 ) == 0 ) );
	B = L3->find( 8 );
	unit_assert( "scope check element L3[2]", ( strncmp( B->buffer, "8\n", 1000 ) == 0 ) );
	delete L3;
	
#define TRANSACTIONS 500000
	unit_lru_type2 *L2 = new unit_lru_type2(5);
	double t0 = clock();
	for( int i = 0; i < TRANSACTIONS; i++ ) {
		L2->insert( i, i-1 );
	}
	double t1 = clock();
	delete L2;
	std::cout << "(int,int) inserts: " << (int)(CLOCKS_PER_SEC/(t1-t0)*TRANSACTIONS) << std::endl;
	return true;
}

#include <sax/c++/lru_set.h>
void lru_set_test()
{
	sax::LRU_set<std::string> t;
	t.resize(5);
	t.insert("11");
	t.insert("22");
	t.insert("33");
	t.insert("44");
	t.insert("55"); std::cout << "t.size(): " << t.size() << std::endl;
	t.insert("66"); std::cout << "t.size(): " << t.size() << std::endl;
	std::cout << "11: " << t.find("11") << std::endl;
	std::cout << "22: " << t.find("22") << std::endl;
	std::cout << "55: " << t.find("55") << std::endl;
	t.insert("77"); std::cout << "t.size(): " << t.size() << std::endl;
	
	sax::LRU_set<std::string>::iter_t it;
	for(it = t.begin(); it != t.end(); it++) {
		std::cout << *it << std::endl;
	}
	
	t.remove("22");
	t.insert("88"); std::cout << "t.size(): " << t.size() << std::endl;
	for(it = t.begin(); it != t.end(); it++) {
		std::cout << *it << std::endl;
	}
}

int main( int argc, char *argv[] )
{
	lru_set_test();
	lru_cache_1cycle();
	return 0;
}
