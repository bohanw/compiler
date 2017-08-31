#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"
#include <map>
#include <vector>
#include <algorithm>
#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  ostream& error_stream;

public:
  std::map<Symbol,Symbol> child_parent_map;
  std::map<Symbol, std::vector<Symbol> > parent_child_map;

  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);

  std::vector<Symbol> get_path(Symbol class_name);
  Symbol get_parent(Symbol class_name);
  void printClassTable();
  void add_pair(Symbol child, Symbol parent);

  //
  //std::ofstream &log;
};


struct FormalParam {
	Symbol name;
	Symbol type;

	FormalParam(Symbol n, Symbol t){
		this.name = n;
		this.type = t;
	}
};

struct MethodStruct {
	Symbol return_type;
	Symbol method_name;
	std::vector<FormalParam> formals;

	MethodStruct() {

	}
	explicit MethodStruct(Symbol t) {
		this.return_type = t;
	}
	MethodStruct(MethodStruct &m){
		this.return_type = m.return_type;
		this.method_name = m.method_name;
	}

	void add_param(FormalParam p) {
		formals.push_back(p);
	}

};
struct SymTable {
	SymbolTable<Symbol, Symbol> *object_table;
	SymbolTable<Symbol,MethodStruct> *method_table;

	SymTable(){
		object_table = new SymbolTable<Symbol, Symbol>();
		method_table = new SymbolTable<Symbol, MethodStruct>();
	}
};


#endif

