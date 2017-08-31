 /*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

static int comment_line = 0;
static std::string curr_str;
static int containsNull=0;
static int pos = 0;
/*
 *  Add Your own definitions here
 */

%}

/*
 * Define names for regular expressions here.
 */

DARROW          =>
DIGIT			[0-9]
INTEGER			 {DIGIT}+	
ESCAPE			\\
NEWLINE			\n
NULL_CHAR		\0
LEFT_PARAN		\(
RIGHT_PARAN		\)
STAR			\*
QUOTE			\"
DASH			-
TRUE			t(?i:rue)
FALSE			f(?i:alse)
WHITESPACE      [ \t\n\r\v\f]
 /* 
  * Not complete
  */
SINGLECHAR_TOKEN		[+-*/)(-]
TYPEID			[A-Z][a-zA-Z0-9_]*
OBJECTID		[a-z][a-zA-Z0-9_]*


%x COMMENT
%x COMMENT_IN_LINE
%x STRING

%%


<COMMENT_IN_LINE>\n {
	curr_lineno++;
	BEGIN INITIAL;
}
<COMMENT_IN_LINE><<EOF>> {
	BEGIN INITIAL;
}
<COMMENT_IN_LINE>. { }

 /*
  *  Nested comments
  */

<COMMENT>"(*" {
	comment_line++;
}
<COMMENT>"*)" {
	comment_line--;
	if(comment_line ==0)
		BEGIN 0;
}

<COMMENT>\n	{
	curr_lineno++;
}

<COMMENT><<EOF>> {
	
 	cool_yylval.error_msg="EOF in comment";
	BEGIN 0;
 	return ERROR;
} 

<COMMENT>. { }
 /*
  *  The multiple-character operators.
  */
"<-" {
	return ASSIGN;
}

"<=" {
	return LE;
}


 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
 /* Keywords */

{DARROW}		{ return (DARROW); }
(?i:class)  return CLASS;
(?i:else)   return ELSE;
(?i:fi)		return FI;
(?i:if)		return IF;
(?i:in)		return IN;
(?i:inherits) return INHERITS;
(?i:let)	return LET;
(?i:loop)	return LOOP;
(?i:pool)	return POOL;
(?i:then)	return THEN;
(?i:while)  return WHILE;
(?i:case)   return CASE;
(?i:esac)   return ESAC;
(?i:of)		return OF;
(?i:new)    return NEW;
(?i:isvoid) return ISVOID;
(?i:not)    return NOT;

  /* BOOL_CONST */
{TRUE} {
	cool_yylval.boolean = 1;
	return BOOL_CONST;
}

{FALSE} {
	cool_yylval.boolean = 0;
	return BOOL_CONST;
}

{INTEGER} {
	cool_yylval.symbol = inttable.add_string(yytext);
	return INT_CONST;
}

{TYPEID} {
	cool_yylval.symbol = idtable.add_string(yytext);
	return TYPEID;
}

{OBJECTID} {	
	cool_yylval.symbol = idtable.add_string(yytext);
	return OBJECTID;
}

{NEWLINE} {
	curr_lineno++;
}

{WHITESPACE} {
	
}	

{DASH}{DASH} {
	BEGIN COMMENT_IN_LINE;
}

"(*" {
	comment_line++;
	BEGIN COMMENT;
}

{STAR}{RIGHT_PARAN} {
	cool_yylval.error_msg="Unmatched *)";
	return ERROR;

}
{QUOTE} {
	BEGIN STRING;
	curr_str = "";
	containsNull = 0;
}


"-" {
	return int('-');
}

";" {
	return int(';');
}
"." {
	return int('.');
}

"+" {
	return int('+');
}

"*" { return int('*');}
"/" {return int('/'); }
"=" {return int('=');}
":" {return int(':');}
"{" {return int('{'); }
"}" {return int('}'); }
"@" {return int('@') ; }
"<" {return int('<'); }
"(" {return int('(');}
")" {return int(')');}
"," {return int(','); }
"~" {return int('~');}

 /* invalid  char encountered */
. {
    cool_yylval.error_msg = yytext;
    return ERROR;
}
 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */
<STRING>{QUOTE} {
	BEGIN INITIAL;
	if(curr_str.size() >= MAX_STR_CONST) {
		cool_yylval.error_msg = "String too long";
		return ERROR;
	
	}
	if(containsNull == 1) {
		cool_yylval.error_msg = "String contains null char";
		return ERROR;
	}
	cool_yylval.symbol = stringtable.add_string((char *)curr_str.c_str());
	return STR_CONST;
	
}



<STRING>{NULL_CHAR} {
	containsNull =  1;	
}

<STRING>{NEWLINE} {
	BEGIN INITIAL;
	curr_lineno++;
	cool_yylval.error_msg = "Unterminated string constant ";
	return ERROR;
}

<STRING>{ESCAPE}{NEWLINE} {
	curr_str += '\n';

}
<STRING>\\n {
	curr_str += '\n';
}

<STRING>\\t {
	curr_str += '\t';
}

<STRING>\\f {
	curr_str += '\f';

}

<STRING>\\b {
	curr_str += '\b';
}
<STRING>\\\0 {
	containsNull = 1;
}
<STRING>{ESCAPE}. {
	curr_str += yytext[1];
}
<STRING><<EOF>> {
	BEGIN INITIAL;
	cool_yylval.error_msg = "EOF in String";
	return ERROR;
}

<STRING>. {
	curr_str += yytext;
}
%%
