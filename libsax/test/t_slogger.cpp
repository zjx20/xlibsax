/*
 * t_slogger.cpp
 *
 *  Created on: 2012-8-14
 *      Author: x
 */

#include "sax/stage/slogger.h"
#include "gtest/gtest.h"

TEST(static_checker_helper, basic_test)
{
	using namespace sax::slogger;
	size_t is_static_size = sizeof(static_checker_helper<true>);

	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << 'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (uint8_t)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (int16_t)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (uint16_t)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (int32_t)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (uint32_t)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (int64_t)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (uint64_t)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (float)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (double)'a' << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << (long double)'a' << static_checker()));

	char char_array[100];
	const char const_char_array[100] = {0};
	char* char_pointer = char_array;
	const char* const_char_pointer = const_char_array;
	std::string std_string("hello");
	const std::string const_std_string("world");

	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << char_array << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << const_char_array << static_checker()));
	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<0>() << "const string" << static_checker()));
	ASSERT_NE(is_static_size, sizeof(static_log_size_helper<0>() << char_pointer << static_checker()));
	ASSERT_NE(is_static_size, sizeof(static_log_size_helper<0>() << const_char_pointer << static_checker()));
	ASSERT_NE(is_static_size, sizeof(static_log_size_helper<0>() << std_string << static_checker()));
	ASSERT_NE(is_static_size, sizeof(static_log_size_helper<0>() << const_std_string << static_checker()));
	ASSERT_NE(is_static_size, sizeof(static_log_size_helper<0>() << std::string("rvalue string") << static_checker()));
}

TEST(static_checker_helper, combine)
{
	using namespace sax::slogger;
	size_t is_static_size = sizeof(static_checker_helper<true>);

	ASSERT_EQ(is_static_size, sizeof(static_log_size_helper<true>() <<
			"a = " << 'a' << 123 << 23234.3242354 << static_checker()));

	const char* char_pointer = "char";

	ASSERT_NE(is_static_size, sizeof(static_log_size_helper<0>() <<
			"a = " << char_pointer << 'a' << 123 << 23234.3242354 << static_checker()));

	ASSERT_NE(is_static_size, sizeof(static_log_size_helper<0>() <<
			"a = " << std::string("std::string") << 'a' << 123 << 23234.3242354 << static_checker()));
}

TEST(static_log_size_helper, basic_test)
{
	using namespace sax::slogger;

	ASSERT_EQ(estimated_size::CHAR, sizeof((static_log_size_helper<0>() << 'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::UINT8, sizeof((static_log_size_helper<0>() << (uint8_t)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::INT16, sizeof((static_log_size_helper<0>() << (int16_t)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::UINT16, sizeof((static_log_size_helper<0>() << (uint16_t)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::INT32, sizeof((static_log_size_helper<0>() << (int32_t)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::UINT32, sizeof((static_log_size_helper<0>() << (uint32_t)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::INT64, sizeof((static_log_size_helper<0>() << (int64_t)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::UINT64, sizeof((static_log_size_helper<0>() << (uint64_t)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::FLOAT, sizeof((static_log_size_helper<0>() << (float)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::DOUBLE, sizeof((static_log_size_helper<0>() << (double)'a' << log_size_traits())));
	ASSERT_EQ(estimated_size::LONGDOUBLE, sizeof((static_log_size_helper<0>() << (long double)'a' << log_size_traits())));

	char char_array[100];
	const char const_char_array[200] = {0};
	char* char_pointer = char_array;
	const char* const_char_pointer = const_char_array;
	std::string std_string("hello");
	const std::string const_std_string("world");

	ASSERT_EQ(100, sizeof((static_log_size_helper<0>() << char_array << log_size_traits())));
	ASSERT_EQ(200, sizeof((static_log_size_helper<0>() << const_char_array << log_size_traits())));
	ASSERT_EQ(13, sizeof((static_log_size_helper<0>() << "hello world!" << log_size_traits())));
	ASSERT_EQ(sizeof(size_t), sizeof((static_log_size_helper<0>() << char_pointer << log_size_traits())));
	ASSERT_EQ(sizeof(size_t), sizeof((static_log_size_helper<0>() << const_char_pointer << log_size_traits())));
	ASSERT_EQ(sizeof(size_t), sizeof((static_log_size_helper<0>() << std_string << log_size_traits())));
	ASSERT_EQ(sizeof(size_t), sizeof((static_log_size_helper<0>() << const_std_string << log_size_traits())));
	ASSERT_EQ(sizeof(size_t), sizeof((static_log_size_helper<0>() << std::string("rvalue") << log_size_traits())));
}

TEST(dynamic_log_size_helper, basic_test)
{
	using namespace sax::slogger;
	ASSERT_EQ(estimated_size::CHAR, (dynamic_log_size_helper<0>() << 'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::UINT8, (dynamic_log_size_helper<0>() << (uint8_t)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::INT16, (dynamic_log_size_helper<0>() << (int16_t)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::UINT16, (dynamic_log_size_helper<0>() << (uint16_t)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::INT32, (dynamic_log_size_helper<0>() << (int32_t)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::UINT32, (dynamic_log_size_helper<0>() << (uint32_t)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::INT64, (dynamic_log_size_helper<0>() << (int64_t)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::UINT64, (dynamic_log_size_helper<0>() << (uint64_t)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::FLOAT, (dynamic_log_size_helper<0>() << (float)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::DOUBLE, (dynamic_log_size_helper<0>() << (double)'a' << log_size_traits()));
	ASSERT_EQ(estimated_size::LONGDOUBLE, (dynamic_log_size_helper<0>() << (long double)'a' << log_size_traits()));

	char char_array[100] = {"hello world"};
	const char const_char_array[200] = {"const_char_array"};
	char* char_pointer = char_array;
	const char* const_char_pointer = const_char_array;
	std::string std_string("hello");
	const std::string const_std_string("world!");

	ASSERT_EQ(100, dynamic_log_size_helper<0>() << char_array << log_size_traits());
	ASSERT_EQ(200, dynamic_log_size_helper<0>() << const_char_array << log_size_traits());
	ASSERT_EQ(13, dynamic_log_size_helper<0>() << "hello world!" << log_size_traits());
	ASSERT_EQ(11, dynamic_log_size_helper<0>() << char_pointer << log_size_traits());
	ASSERT_EQ(16, dynamic_log_size_helper<0>() << const_char_pointer << log_size_traits());
	ASSERT_EQ(5, dynamic_log_size_helper<0>() << std_string << log_size_traits());
	ASSERT_EQ(6, dynamic_log_size_helper<0>() << const_std_string << log_size_traits());
	ASSERT_EQ(16, dynamic_log_size_helper<0>() << std::string("rvalue something") << log_size_traits());
}

TEST(log_size_traits, combine)
{
	using namespace sax::slogger;

	ASSERT_EQ(5 + 1 + 11 + 14, sizeof(log_size_helper() <<
			"a = " << 'a' << 123 << 23234.3242354 << log_size_traits()));

	const char* char_pointer = "char";

	ASSERT_EQ(5 + 4 + 1 + 11 + 14, (log_size_helper() <<
			"a = " << char_pointer << 'a' << 123 << 23234.3242354 << log_size_traits()));

	ASSERT_EQ(5 + 11 + 1 + 11 + 14, (log_size_helper() <<
			"a = " << std::string("std::string") << 'a' << 123 << 23234.3242354 << log_size_traits()));
}

size_t get_file_size(const std::string& file)
{
	FILE* f = fopen(file.c_str(), "r");
	if (f == NULL) return (size_t) -1;
	fseek(f, 0, SEEK_END);
	size_t res = ftell(f);
	fclose(f);
	return res;
}

#define LOG_S(log, msg) log.log(msg, strlen(msg))

TEST(log_file_writer, max_size_limit)
{
	std::string logfile_name = "log_test1.txt";
	std::string clean_cmd = (std::string("rm -rf ") + logfile_name + "*").c_str();

	system(clean_cmd.c_str());

	char s[] = "01234567891";	// 11 chars

	{
		sax::slogger::log_file_writer<sax::dummy_lock> log(
				logfile_name, 100, 50, sax::slogger::SAX_TRACE);
		for (int i=0; i<6; i++) {
			LOG_S(log, s);
		}
	}

	ASSERT_EQ(strlen(s), get_file_size(logfile_name));

	system(clean_cmd.c_str());
}

TEST(log_file_writer, max_logfiles)
{
	std::string logfile_name = "log_test2.txt";
	std::string clean_cmd = (std::string("rm -rf ") + logfile_name + "*").c_str();

	system(clean_cmd.c_str());

	char s[] = "01234567891";	// 11 chars

	{
		sax::slogger::log_file_writer<sax::spin_type> log(
				logfile_name, 5, 10, sax::slogger::SAX_TRACE);
		for (int i=0; i<10; i++) {
			LOG_S(log, s);
		}
	}

	ASSERT_EQ(0, get_file_size(logfile_name));
	ASSERT_EQ(1, g_exists_file(logfile_name.c_str()));
	ASSERT_EQ(1, g_exists_file((logfile_name + ".1").c_str()));
	ASSERT_EQ(1, g_exists_file((logfile_name + ".2").c_str()));
	ASSERT_EQ(1, g_exists_file((logfile_name + ".3").c_str()));
	ASSERT_EQ(1, g_exists_file((logfile_name + ".4").c_str()));
	ASSERT_EQ(1, g_exists_file((logfile_name + ".5").c_str()));
	ASSERT_EQ(0, g_exists_file((logfile_name + ".6").c_str()));

	system(clean_cmd.c_str());
}

TEST(log_file_writer, append_file)
{
	std::string logfile_name = "log_test3.txt";
	std::string clean_cmd = (std::string("rm -rf ") + logfile_name + "*").c_str();

	system(clean_cmd.c_str());

	FILE* f = fopen(logfile_name.c_str(), "w");
	fputs("hello", f);
	fclose(f);

	{
		sax::slogger::log_file_writer<sax::spin_type> log(
				logfile_name, 100, 100, sax::slogger::SAX_TRACE);
		LOG_S(log, " world");
	}

	ASSERT_EQ(11, get_file_size(logfile_name));

	system(clean_cmd.c_str());
}

TEST(log_file_writer, curr_logfiles)
{
	std::string logfile_name = "log_test4.txt";
	std::string clean_cmd = (std::string("rm -rf ") + logfile_name + "*").c_str();

	system(clean_cmd.c_str());

	fclose(fopen((logfile_name + ".1").c_str(), "w"));
	fclose(fopen((logfile_name + ".2").c_str(), "w"));
	fclose(fopen((logfile_name + ".3").c_str(), "w"));
	FILE* f = fopen((logfile_name + ".4").c_str(), "w");
	fputs("hello world", f);
	fclose(f);

	{
		sax::slogger::log_file_writer<sax::spin_type> log(
				logfile_name, 100, 10, sax::slogger::SAX_TRACE);
		LOG_S(log, "01234567890");	// 11 chars
	}

	ASSERT_EQ(11, get_file_size(logfile_name + ".5"));

	system(clean_cmd.c_str());
}

TEST(log_file_writer, stderr)
{
	{
		sax::slogger::log_file_writer<sax::spin_type> log(
				"stderr", 10, 10, sax::slogger::SAX_TRACE);
		LOG_S(log, "log to stderr.\n");
		for (int i=0; i<10; i++) {
			LOG_S(log, "0123456789 abcdefghijklmnopqrstuvwxyz\n");
		}
	}
}

TEST(log_file_writer, stdout)
{
	{
		sax::slogger::log_file_writer<sax::spin_type> log(
				"stdout", 10, 10, sax::slogger::SAX_TRACE);
		LOG_S(log, "log to stdout.\n");
		for (int i=0; i<10; i++) {
			LOG_S(log, "0123456789 abcdefghijklmnopqrstuvwxyz\n");
		}
	}
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
