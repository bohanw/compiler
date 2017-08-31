
#include <iostream>
#include <map>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"

extern int semant_debug;
extern char *curr_filename;


ClassTable *classtbl;
typedef MethodStruct* Method;

//typedef SymbolTable<Symbol, method_class> methodTable;
//clas_name -> symtable(method_name, method_object)
//static std::map<Symbol, methodTable> methodsTable;
//static SymbolTable<Symbol, Symbol> attrList;
std::map<Symbol,SymTable> env;
static std::ofstream log; // semant checker info log

std::map<Symbol, Class_> classMapping;

//static ostream& semantic_error(Class_ c);
//static ostream& semantic_error(Symbol filename, tree_node *t);
static void enterscope(Symbol class_name);

static void exitscope(Symbol class_name);

static void enterscope_method(Symbol class_name);

static void exitscope_method(Symbol class_name);

static void add_method(Symbol name, Method method, Class_ c);

static void add_object(Symbol name, Symbol type, Class_ c);

static bool class_defined(Symbol class_name);
static Method find_method(Symbol method_name, Class_ c, bool current_scope);
static Symbol find_object(Symbol object_name, Class_ c, bool current_scope);
//

void print_path(std::vector<Symbol> path) {
	log << "inherit path ";
	for(std::vector<Symbol>::iterator it = path.begin();it != path.end();++it) {
		log << ' ' << *it;
	}
	log << '\n' << std::endl;
}

void enterscope(Symbol class_name){
	env[class_name].object_table->enterscope();
}
void exitscope(Symbol class_name){
	env[class_name].object_table->exitscope();
}
void enterscope_method(Symbol class_name){
	env[class_name].method_table->enterscope();
}

void exitscope_method(Symbol class_name){
	env[class_name].method_table->exitscope();
}

void add_method(Symbol name, Method method, Class_ c){
	env[c->get_name()].method_table->addid(name, method);
}

void add_object(Symbol name, Symbol type, Class_ c){
	env[c->get_name()].object_table->addid(name, new Symbol(type));
}
Method find_method(Symbol method_name, Class_ c, bool current_scope) {
	Method target = NULL;
	if(current_scope){
		target = env[c->get_name()].method_table->probe(method_name);
		return target;
	}
	else {
		//globally, excluding current scope
		if(env[c->get_name()].method_table->probe(method_name) == NULL &&
		   env[c->get_name()].method_table->lookup(method_name) != NULL) {
			target = env[c->get_name()].method_table->lookup(method_name);
			return target;
		}
	}
	return NULL;
}
bool class_defined(Symbol class_name){
	return((classMapping.find(class_name) != classMapping.end()) ||( class_name == SELF_TYPE));
}
//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}



/////////////////////
void ClassTable::printClassTable() {
	for(std::map<Symbol, Class_>::iterator it = classMapping.begin();it!=classMapping.end();++it) {
		Class_ c = it->second;
		log<< "Symbol " << it->first << "->  Class type :" << c->get_name() << std::endl;

	}

}
////////////////////



//
std::vector<Symbol> ClassTable::get_path(Symbol class_name) {
	std::vector<Symbol> path;
	path.push_back(class_name);
	Symbol s = get_parent(class_name);
	while( s != NULL) {
		path.push_back(s);
		s = get_parent(s);
	}
	print_path(path);

	return path;
}


Symbol ClassTable::get_parent(Symbol class_name) {
	Class_ c = classMapping[class_name];
	Symbol parent = c->get_parent();
	if(parent != NULL)
		return child_parent_map[class_name];
	else
		return NULL;
}

void ClassTable::add_pair(Symbol child, Symbol parent){
	child_parent_map[child] = parent;
	parent_child_map[parent].push_back(child);
}


ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    /* Fill this in */
	install_basic_classes();

	log << " build tree" << std::endl;

	//printClassTable();
	for(int i = classes->first();classes->more(i); i = classes->next(i)) {
		if(classes->nth(i)->get_name() == SELF_TYPE){
			//class not inserted in the map

			semant_error( classes->nth(i)) << "class name " << classes->nth(i)->get_name() << " cannot be SELF_TYPE" << std::endl;
		}

		if(classMapping.count(classes->nth(i)->get_name()) != 0){
			semant_error(classes->nth(i)) << "class name "<< classes->nth(i)->get_name() << " defined " << std::endl;
			return;
		}
		else {
			classMapping.insert(std::make_pair(classes->nth(i)->get_name(),classes->nth(i)));
			add_pair(classes->nth(i)->get_name(),classes->nth(i)->get_parent());
		}
	}

	if(classMapping.find(Main) == classMapping.end()) {
		semant_error() << "Class main not found" << std::endl;
	}

	//Check if any inheritance violation(cyclic graph)
	for(int i = classes->first();classes->more(i);i = classes->next(i)) {
		Class_ curr = classes->nth(i);
		Symbol parent = classes->nth(i)->get_parent();

		while( parent != Object && parent != curr->get_name()) {

			if(classMapping.find(parent) == classMapping.end()) {
				semant_error() << " parent class not defined" << std::endl;
				return;
			}
			if(parent == Int || parent == Str || parent == Bool || parent == SELF_TYPE) {
				semant_error() << "cannot inherit basic class" << std::endl;
				return;
			}
			log << curr->get_name() << "<-" << parent << std::endl;
			curr = classMapping[curr->get_parent()];
			parent = curr->get_parent();

		}

		if(parent == Object) {
			log << curr->get_name() << " < " << parent << std::endl;

		}
		else {
			semant_error() << "graph not acyclic" << std::endl;
			return;
		}
	}

	log << " build ClassTable complete " << std::endl;
}


void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);

    classMapping.insert(std::make_pair(Object,Object_class));
    classMapping.insert(std::make_pair(IO,IO_class));
    classMapping.insert(std::make_pair(Int,Int_class));
    classMapping.insert(std::make_pair(Str,Str_class));
    classMapping.insert(std::make_pair(Bool,Bool_class));
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 


// ----Add features(method/attr) to the current class
//  and enforce naming and scopes
void method_class::add_feature(Class_ c){
	Method meth = new Method(get_return_type());
	Formals fmls = get_formals();
	Symbol method_name = get_name();

	std:vector<Symbol> fml_names;

	if(find_method(method_name,c, true) != NULL){
		classtbl->semant_error(c) << "Method of same ID " << get_name() << " already defined in curr scope " << std::endl;
	}
	if(classMapping.find(get_return_type()) == classMapping.end()){
		classtbl->semant_error(c) << "REturn type " << get_return_type() << " not defined " << std::endl;

	}

	for(int i = fmls->first(); fmls->more(i);i = fmls->next(i)){
		Formal fml = fmls->nth(i);
		FormalPair fp(fml->get_name(),fml->get_type_decl());

		if(fml_names.find(fp.name) != fml_names.end()) {
			classtbl->semant_error(c) << "Duplicate fml param names "<< fp.name << std::endl;
		}
		fml_names.push_back(fp.name);

		if(fml->get_name() == self){
			classtbl->semant_error(c) << "Fml param name cannot be self "<< fp.name << std::endl;
		}

		if(fml->get_type_decl() == SELF_TYPE){
			classtbl->semant_error(c) << "fml param can't have type of SEFL_TYPE " << std::endl;
		}
		if(!class_defined(get_type_decl())){
			classtbl->semant_error(c) << "fml param return type undefined " << std::endl;
		}
		meth->add_param(fp);
	}

	//finish checking and add to symbol table
	add_method(method_name, meth, c);

}

void method_class::check_inherit(Class_ c){

}

void method_class::init_expr(Class_ c){

}
void attr_class::add_feature(Class_ c){

}

void attr_class::check_inherit(Class_ c){

}

void attr_class::init_expr(Class_ c){

}

//Typechecking functions
Symbol object_class::get_expr_type(Class_ class_) {

	return NULL;
}
Symbol no_expr_class::get_expr_type(Class_ class_) {
	Symbol type = No_type;
	return type;
}

Symbol isvoid_class::get_expr_type(Class_ class_) {
	Symbol type = get_e1()->get_expr_type(class_);
	return type;
}
Symbol new__class::get_expr_type(Class_ class_) { return NULL;}
Symbol string_const_class::get_expr_type(Class_ class_) {
	Symbol type = Str;
	return type;
}
Symbol bool_const_class::get_expr_type(Class_ class_) {
	Symbol type = Bool;
	return type;
}
Symbol int_const_class::get_expr_type(Class_ class_) {
	Symbol type = Int;
	return type;
}


Symbol comp_class::get_expr_type(Class_ class_) { return NULL;}
Symbol leq_class::get_expr_type(Class_ class_) { return NULL;}
Symbol eq_class::get_expr_type(Class_ class_) {return NULL;}
Symbol lt_class::get_expr_type(Class_ class_) {return NULL;}
Symbol neg_class::get_expr_type(Class_ class_) {return NULL;}
Symbol divide_class::get_expr_type(Class_ class_) {return NULL;}
Symbol mul_class::get_expr_type(Class_ class_) {return NULL;}
Symbol sub_class::get_expr_type(Class_ class_) {return NULL;}
Symbol plus_class::get_expr_type(Class_ class_) {return NULL;}
Symbol let_class::get_expr_type(Class_ class_) {return NULL;}
Symbol block_class::get_expr_type(Class_ class_) {return NULL;}
Symbol typcase_class::get_expr_type(Class_ class_) {return NULL;}

Symbol loop_class::get_expr_type(Class_ class_) {return NULL;}
Symbol cond_class::get_expr_type(Class_ class_) {return NULL;}
Symbol dispatch_class::get_expr_type(Class_ class_) {return NULL;}
Symbol static_dispatch_class::get_expr_type(Class_ class_) {return NULL;}
Symbol assign_class::get_expr_type(Class_ class_) {
	//Symbol type;
	if(get_name() == self){
		classtbl->semant_error(class_) << "assign cannot be type self" << std::endl;
		type = Object;
		return type;
	}

	//check if type is not defined

	return type;
}


/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    //**TESTING

    log.open("out.txt");
    //*********
    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    if (classtable->errors()) {
    	cerr << "Compilation halted due to static semantic errors." << endl;
    	exit(1);
    }


    //Traverse class maps con
    std::map<Symbol,Class_> classes = classMapping;

    for(std::map<Symbol, Class_>::iterator it = classes.begin();it!=classes.end();++it) {
    	env[it->first] = SymTable();
    	env[it->first].object_table->enterscope();
    	env[it->first].method_table->enterscope();

    	//add "self" with type "SELF_TYPE"
    	add_object(self, SELF_TYPE,it->second);



    	//add all features

    }

    //illegal method declaration
    log << "Check violation in method inhertiance" << std::endl;
    log << " "<< std::endl;

    for(std::map<Symbol, Class_>::iterator it = classes.begin();it!=classes.end();++it) {
    	Symbol curr_class_name = it->first;
    	Class_ curr_class = it->second;
    	Features feat_list = classMapping[curr_class_name]->get_features();





    }

    /* some semantic analysis code may go here */

    //add attr to the class

    log << " Check class type" << std::endl;


    if (classtable->errors()) {
    	cerr << "Compilation halted due to static semantic errors." << endl;
    	exit(1);
    }

}


