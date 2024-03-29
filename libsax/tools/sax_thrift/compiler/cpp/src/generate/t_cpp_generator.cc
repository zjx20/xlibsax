/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * Contains some contributions under the Thrift Software License.
 * Please see doc/old-thrift-license.txt in the Thrift distribution for
 * details.
 */

#include <cassert>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>

#include "platform.h"
#include "t_oop_generator.h"
using namespace std;


/**
 * C++ code generator. This is legitimacy incarnate.
 *
 */
class t_cpp_generator : public t_oop_generator {
 public:
  t_cpp_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
    : t_oop_generator(program)
  {
    (void) option_string;
    std::map<std::string, std::string>::const_iterator iter;

    out_dir_base_ = "gen-cpp";
  }

  virtual ~t_cpp_generator() {}

  /**
   * Init and close methods
   */

  void init_generator();
  void close_generator();
  void generate_program();

  /**
   * Program-level generation functions
   */

  void generate_typedef(t_typedef* ttypedef) {}
  void generate_enum(t_enum* tenum) {}
  void generate_struct(t_struct* tstruct) {}
  void generate_service(t_service* tservice) {
	  generate_auto_rpc_serial(tservice);
  }

  void generate_type_enum(std::ofstream& out);
  void generate_auto_serial_class(std::ofstream& out);
  void generate_auto_serial_class_impl(std::ofstream& out);
  void generate_auto_rpc_serial(t_service* tservice);

  /*
   * Helper rendering functions
   */

  std::string namespace_prefix(std::string ns);
  std::string namespace_open(std::string ns);
  std::string namespace_close(std::string ns);

 private:
  /**
   * Returns the include prefix to use for a file generated by program, or the
   * empty string if no include prefix should be used.
   */
  std::string get_include_prefix(const t_program& program) const;

  std::string struct_to_enum_item(std::string name) {
	  return "T_" + upcase_string(underscore(name));
  }

  std::string struct_to_buffer_func_name(std::string name) {
	  return lowercase(name) + "_to_buffer";
  }

  std::string buffer_to_struct_func_name(std::string name) {
	  return "buffer_to_" + lowercase(name);
  }

  std::string auto_serial_class_name() {
	  return program_name_ + "_auto_serial";
  }

  /**
   * Strings for namespace, computed once up front then used directly
   */

  std::string ns_open_;
  std::string ns_close_;

  /**
   * File streams, stored here to avoid passing them as parameters to every
   * function.
   */

  std::ofstream f_struct_typeid_;
  std::ofstream f_auto_serial_;

};


void t_cpp_generator::generate_program() {
	init_generator();

	generate_type_enum(f_struct_typeid_);
	generate_auto_serial_class(f_auto_serial_);
	generate_auto_serial_class_impl(f_auto_serial_);

	close_generator();

	const vector<t_service*> services = get_program()->get_services();
	for(size_t i = 0; i < services.size(); i++) {
		generate_service(services[i]);
	}
}

/**
 * Prepares for file generation by opening up the necessary file output
 * streams.
 */
void t_cpp_generator::init_generator() {
  // Make output directory
  MKDIR(get_out_dir().c_str());

  // Make output file
  string f_stru_type_name = get_out_dir()+program_name_+"_struct_typeid.h";
  f_struct_typeid_.open(f_stru_type_name.c_str());

  string f_auto_serial_name = get_out_dir()+program_name_+"_auto_serial.cpp";
  f_auto_serial_.open(f_auto_serial_name.c_str());

  // Print header
  f_struct_typeid_ <<
    autogen_comment();
  f_auto_serial_ <<
    autogen_comment();

  // Start ifndef
  f_struct_typeid_ <<
    "#ifndef " << "_" << program_name_ << "_STRUCT_TYPEID_H_" << endl <<
    "#define " << "_" << program_name_ << "_STRUCT_TYPEID_H_" << endl <<
    endl;

  // Include base types
  f_auto_serial_ <<
    "#include \"thrift/protocol/TBinaryProtocol.h\"" << endl <<
    "#include \"sax/net/t_buffer_transport.h\"" << endl <<
    "#include \"sax/net/thrift_utility.h\"" << endl <<
    "#include \"boost/shared_ptr.hpp\"" << endl <<
    "#include \"" << program_name_ << "_types.h\"" << endl <<
    "#include \"" << program_name_ << "_struct_typeid.h\"" << endl <<
    endl;

  // Include other Thrift includes
  const vector<t_program*>& includes = program_->get_includes();
  for (size_t i = 0; i < includes.size(); ++i) {
    f_struct_typeid_ <<
      "#include \"" << get_include_prefix(*(includes[i])) <<
      includes[i]->get_name() << "_struct_typeid.h\"" << endl;
	f_auto_serial_ <<
      "#include \"" << get_include_prefix(*(includes[i])) <<
      includes[i]->get_name() << "_auto_serial.h\"" << endl;
  }
  f_struct_typeid_ << endl;
  f_auto_serial_ << endl;

  // Open namespace
  ns_open_ = namespace_open(program_->get_namespace("cpp"));
  ns_close_ = namespace_close(program_->get_namespace("cpp"));

  f_struct_typeid_ <<
    ns_open_ << endl <<
    endl;

  f_auto_serial_ <<
    ns_open_ << endl <<
    endl;

}

/**
 * Closes the output files.
 */
void t_cpp_generator::close_generator() {
  // Close namespace
  f_struct_typeid_ <<
	ns_close_ << endl <<
	endl;
  f_auto_serial_ <<
    ns_close_ << endl <<
    endl;

  // Close ifndef
  f_struct_typeid_ <<
    "#endif" << endl;

  // Close output file
  f_struct_typeid_.close();
  f_auto_serial_.close();
}

void t_cpp_generator::generate_type_enum(ofstream& out) {
  const std::vector<t_struct*>& structs = get_program()->get_structs();

  indent(out) <<
    "struct TType {" << endl;

  indent_up();
  indent(out) <<
    "enum {" << endl;

  indent_up();
  indent(out) <<
    "T_TYPE_START = 0," << endl;

  for(size_t i = 0; i < structs.size(); i++) {
	indent(out) <<
      struct_to_enum_item(structs[i]->get_name()) << "," << endl;
  }

  indent(out) <<
    "T_TYPE_END" << endl;

  // end of enum
  indent_down();
  indent(out) <<
    "};" << endl;

  // end of struct
  indent_down();
  indent(out) <<
    "};" << endl;

  out << endl;
}

void t_cpp_generator::generate_auto_serial_class(ofstream& out) {
  const std::vector<t_struct*>& structs = get_program()->get_structs();

  out <<
    indent() << "class " << auto_serial_class_name() << " {" << endl <<
    indent() << "public:" << endl;

  indent_up();

  for(size_t i = 0; i < structs.size(); i++) {
    indent(out) <<
      "static void " << struct_to_buffer_func_name(structs[i]->get_name()) << "(sax::buffer* buf, void* obj);" << endl;
    indent(out) <<
      "static void* " << buffer_to_struct_func_name(structs[i]->get_name()) << "(sax::buffer* buf);" << endl;
  }

  indent(out) <<
    "static boost::shared_ptr< sax::TBufferTransport > _buffer_transport;" << endl;

  indent(out) <<
    "static boost::shared_ptr< apache::thrift::protocol::TBinaryProtocol > _protocol;" << endl;

  indent_down();

  indent(out) <<
    "private:" << endl;

  indent_up();

  out <<
    indent() << auto_serial_class_name() << "() {" << endl;

  indent_up();
  for(size_t i = 0; i < structs.size(); i++) {
    indent(out) << "sax::thrift_utility::thrift_register(TType::" <<
      struct_to_enum_item(structs[i]->get_name()) << ", " <<
      buffer_to_struct_func_name(structs[i]->get_name()) << ", " <<
      struct_to_buffer_func_name(structs[i]->get_name()) << ");" << endl;
  }

  // end of constructor
  indent_down();
  indent(out) << "}" << endl;

  indent(out) <<
    "static " << auto_serial_class_name() << " _registerer;" << endl;

  // end of class
  indent_down();
  indent(out) << "};" << endl;

}

void t_cpp_generator::generate_auto_serial_class_impl(ofstream& out) {
  const std::vector<t_struct*>& structs = get_program()->get_structs();

  indent(out) <<
    "boost::shared_ptr< sax::TBufferTransport > " << auto_serial_class_name() <<
    "::_buffer_transport(new sax::TBufferTransport());" << endl;

  indent(out) <<
    "boost::shared_ptr < apache::thrift::protocol::TBinaryProtocol > test_auto_serial::_protocol(" <<
    "new apache::thrift::protocol::TBinaryProtocol(" <<
    auto_serial_class_name() << "::_buffer_transport));" << endl;

  indent(out) <<
    auto_serial_class_name() << " " <<
    auto_serial_class_name() << "::_registerer;" << endl;

  out << endl;

  // generate struct to buffer funcs
  for(size_t i = 0; i < structs.size(); i++) {
    indent(out) <<
      "void " << auto_serial_class_name() << "::" <<
      struct_to_buffer_func_name(structs[i]->get_name()) <<
      "(sax::buffer* buf, void* obj) {" << endl;

    indent_up();
    indent(out) <<
      structs[i]->get_name() << "* t_obj = (" <<
      structs[i]->get_name() << "*) obj;" << endl;
    out <<
      indent() << "_buffer_transport->set_buffer(buf, false);" << endl <<
      indent() << "t_obj->write(_protocol.get());" << endl <<
      indent() << "_buffer_transport->flush();" << endl <<
      indent() << "delete t_obj;" << endl;

    // end of struct to buffer func
    indent_down();
    indent(out) << "}" << endl << endl;
  }

  // generate buffer to struct funcs
  for(size_t i = 0; i < structs.size(); i++) {
    indent(out) <<
      "void* " << auto_serial_class_name() << "::" <<
      buffer_to_struct_func_name(structs[i]->get_name()) <<
      "(sax::buffer* buf) {" << endl;

    indent_up();
    indent(out) <<
      structs[i]->get_name() << "* t_obj = new " <<
      structs[i]->get_name() << "();" << endl;
    indent(out) <<
      "if(t_obj != NULL) {" << endl;
    indent_up();
    out <<
      indent() << "_buffer_transport->set_buffer(buf, true);" << endl <<
      indent() << "t_obj->read(_protocol.get());" << endl;
    indent_down();
    indent(out) << "}" << endl;
    indent(out) << "return t_obj;" << endl;

    // end of buffer to struct func
    indent_down();
    indent(out) << "}" << endl << endl;
  }
}

void t_cpp_generator::generate_auto_rpc_serial(t_service* tservice) {
  ofstream out;
  string file_name = get_out_dir() + tservice->get_name() + "_auto_rpc_serial.cpp";
  out.open(file_name.c_str());

  out << autogen_comment();

  out <<
    indent() << "#include <map>" << endl <<
    indent() << "#include <string>" << endl <<
    indent() << "#include \"" << tservice->get_name() << ".h\"" << endl <<
    indent() << "#include \"" << program_name_ << "_struct_typeid.h\"" << endl <<
    indent() << "#include \"sax/net/buffer.h\"" << endl <<
    indent() << "#include \"sax/net/t_buffer_transport.h\"" << endl <<
    indent() << "#include \"sax/net/thrift_task.h\"" << endl <<
    indent() << "#include \"sax/net/thrift_utility.h\"" << endl <<
    indent() << "#include \"protocol/TBinaryProtocol.h\"" << endl <<
    endl;

  indent(out) << ns_open_ << endl <<
    endl;

  string class_name = tservice->get_name() + "_auto_rpc_serial";

  indent(out) <<
    "class " << class_name << " {" << endl <<
    "public:" << endl;

  indent_up();

  //////////////////////////////////////////
  ////// generating to_thrift_task
  //////////////////////////////////////////

  string to_thrift_task_fun_name = tservice->get_name() + "_to_thrift_task";

  indent(out) <<
    "static sax::thrift_task* " << to_thrift_task_fun_name <<
    "(sax::buffer* buf) {" << endl;

  indent_up();
  out <<
    indent() << "sax::thrift_task* ttask = new sax::thrift_task();" << endl <<
    indent() << "if(ttask == NULL) return ttask;" << endl <<
    endl <<
    indent() << "apache::thrift::protocol::TMessageType mtype;" << endl <<
    endl <<
    indent() << "_buffer_transport->set_buffer(buf, true);" << endl <<
    indent() << "_protocol->readMessageBegin(ttask->fname, mtype, ttask->seqid);" << endl << endl <<
    endl <<
    indent() << "if(mtype == apache::thrift::protocol::T_CALL) ttask->ttype = sax::thrift_task::T_CALL;" << endl <<
    indent() << "else if(mtype == apache::thrift::protocol::T_REPLY) ttask->ttype = sax::thrift_task::T_REPLY;" << endl <<
    indent() << "else if(mtype == apache::thrift::protocol::T_ONEWAY) ttask->ttype = sax::thrift_task::T_ONEWAY;" << endl <<
    endl <<
    indent() << "if(0) {}" << endl;

  const vector<t_function*> functions = tservice->get_functions();

  for(size_t i = 0; i < functions.size(); i++) {
    indent(out) << "else if(ttask->fname == \"" << functions[i]->get_name() << "\") {" << endl;
    indent_up();

    t_field* first_arg_field = functions[i]->get_arglist()->get_sorted_members()[0];
    string first_arg_type = first_arg_field->get_type()->get_name();
    string first_arg_name = first_arg_field->get_name();

    out <<
      indent() << tservice->get_name() + "_" << functions[i]->get_name() << "_args args;" << endl <<
      indent() << "args.read(_protocol.get());" << endl <<
      indent() << first_arg_type << "* t_obj = new " << first_arg_type << "();" << endl <<
      indent() << "if(t_obj != NULL) *t_obj = args." << first_arg_name << ";" << endl <<
      indent() << "ttask->obj = t_obj;" << endl <<
      indent() << "ttask->obj_type = TType::" << struct_to_enum_item(first_arg_type) << ";" << endl;

    indent_down();
    indent(out) << "}" << endl;
  }

  out << endl <<
    indent() << "_protocol->readMessageEnd();" << endl <<
    indent() << "return ttask;" << endl;

  indent_down();
  indent(out) << "}" << endl << endl;

  //////////////////////////////////////////
  ////// generating to_result_buffer
  //////////////////////////////////////////

  string to_result_buffer_fun_name = tservice->get_name() + "_to_result_buffer";

  indent(out) << "static void " << to_result_buffer_fun_name <<
		  "(sax::buffer* buf, sax::thrift_task *ttask) {" << endl;

  indent_up();
  indent(out) << "if(0) {}" << endl;

  for(size_t i = 0; i < functions.size(); i++) {
    indent(out) << "else if(ttask->fname == \"" << functions[i]->get_name() << "\") {" << endl;
    indent_up();

    t_type* returntype = functions[i]->get_returntype();

    out <<
      indent() << "_buffer_transport->set_buffer(buf, false);" << endl <<
      indent() << "apache::thrift::protocol::TMessageType mtype = apache::thrift::protocol::T_REPLY;" << endl <<
      indent() << "if(ttask->ttype == sax::thrift_task::T_CALL) mtype = apache::thrift::protocol::T_CALL;" << endl <<
      indent() << "else if(ttask->ttype == sax::thrift_task::T_REPLY) mtype = apache::thrift::protocol::T_REPLY;" << endl <<
      indent() << "else if(ttask->ttype == sax::thrift_task::T_ONEWAY) mtype = apache::thrift::protocol::T_ONEWAY;" << endl <<
      indent() << "_protocol->writeMessageBegin(ttask->fname, mtype, ttask->seqid);" << endl <<
      indent() << tservice->get_name() << "_" << functions[i]->get_name() << "_result result;" << endl <<
      indent() << "if(ttask->obj_type == TType::" << struct_to_enum_item(returntype->get_name()) << ") {" << endl;

    indent_up();
    out <<
      indent() << "result.success = *((" << returntype->get_name() << "*)ttask->obj);" << endl <<
      indent() << "result.__isset.success = true;" << endl <<
      indent() << "delete (" << returntype->get_name() << "*)(ttask->obj);" << endl <<
      indent() << "ttask->obj = NULL;" << endl;
    indent_down();

    indent(out) << "}" << endl;

    out <<
      //indent() << "delete ttask;" << endl <<
      indent() << "result.write(_protocol.get());" << endl <<
      indent() << "_protocol->writeMessageEnd();" << endl <<
      indent() << "_buffer_transport->flush();" << endl;

    indent_down();
    indent(out) << "}" << endl;
  }

  indent_down();
  indent(out) << "}" << endl << endl;

  //////////////////////////////////////////
  ////// generating static fields
  //////////////////////////////////////////

  indent(out) << "static boost::shared_ptr< sax::TBufferTransport > _buffer_transport;" << endl;
  indent(out) << "static boost::shared_ptr< apache::thrift::protocol::TBinaryProtocol > _protocol;" << endl;

  indent_down();
  indent(out) << "private:" << endl;

  indent_up();
  indent(out) << class_name << "() {" << endl;

  indent_up();
  indent(out) << "sax::thrift_utility::thrift_rpc_register(" <<
    to_thrift_task_fun_name << ", " <<
    to_result_buffer_fun_name << ");" << endl;

  indent_down();
  indent(out) << "}" << endl;

  indent(out) << "static " << class_name << " _registerer;" << endl;

  indent_down();
  indent(out) << "};" << endl << endl;	// end of class

  indent(out) << "boost::shared_ptr< sax::TBufferTransport > " << class_name <<
    "::_buffer_transport(new sax::TBufferTransport());" << endl;

  indent(out) << "boost::shared_ptr < apache::thrift::protocol::TBinaryProtocol > " <<
    class_name << "::_protocol(new apache::thrift::protocol::TBinaryProtocol(" <<
    class_name << "::_buffer_transport));" << endl;

  indent(out) << class_name << " " << class_name << "::_registerer;" << endl;

  out << endl;

  indent(out) << ns_close_ << endl <<
    endl;

  out.close();
}

/**
 * Makes a :: prefix for a namespace
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_prefix(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += ns.substr(0, loc);
    result += "::";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += ns + "::";
  }
  return result;
}

/**
 * Opens namespace.
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_open(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "";
  string separator = "";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += separator;
    result += "namespace ";
    result += ns.substr(0, loc);
    result += " {";
    separator = " ";
    ns = ns.substr(loc+1);
  }
  if (ns.size() > 0) {
    result += separator + "namespace " + ns + " {";
  }
  return result;
}

/**
 * Closes namespace.
 *
 * @param ns The namespace, w/ periods in it
 * @return Namespaces
 */
string t_cpp_generator::namespace_close(string ns) {
  if (ns.size() == 0) {
    return "";
  }
  string result = "}";
  string::size_type loc;
  while ((loc = ns.find(".")) != string::npos) {
    result += "}";
    ns = ns.substr(loc+1);
  }
  result += " // namespace";
  return result;
}


string t_cpp_generator::get_include_prefix(const t_program& program) const {
  string include_prefix = program.get_include_prefix();
  if ((include_prefix.size() > 0 && include_prefix[0] == '/')) {
    // if flag is turned off or this is absolute path, return empty prefix
    return "";
  }

  string::size_type last_slash = string::npos;
  if ((last_slash = include_prefix.rfind("/")) != string::npos) {
    return include_prefix.substr(0, last_slash) + "/" + out_dir_base_ + "/";
  }

  return "";
}


THRIFT_REGISTER_GENERATOR(cpp, "C++",
"    pure_enums:      Generate pure enums instead of wrapper classes.\n"
"    dense:           Generate type specifications for the dense protocol.\n"
"    include_prefix:  Use full include paths in generated files.\n"
)

