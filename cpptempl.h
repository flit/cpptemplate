/*
cpptempl
=================
This is a template engine for C++.

Copyright
==================
| Author: Ryan Ginstrom
| With additions by Martinho Fernandes and Chris Reed

MIT License

Requirements
==================
1. The boost library.
2. Compiler with C++11 support (this could be easily ifdef'd to support C++03).

Usage
=======================
Complete example::

    std::string text = "{% if item %}{$item}{% endif %}\n"
        "{% if thing %}{$thing}{% endif %}" ;
    cpptempl::data_map data ;
    data["item"] = "aaa" ;
    data["thing"] = "bbb" ;

    std::string result = cpptempl::parse(text, data) ;

There are two main functions that users will interact with, both called ``parse()``. They
are in the ``cpptempl`` namespace. The prototypes for these functions are as follows::

    std::string parse(std::string templ_text, data_map &data);
    void parse(std::ostream &stream, std::string templ_text, data_map &data) ;

Both take the template text as a ``std::string`` along with a data_map that has the variable
values to substitute. One form of ``parse()`` simply returns the result of the template
as another ``std::string``.

The other form takes a ``std::ostream`` reference as the first argument and writes the
template output to that stream. This form is actually more memory efficient, because it
does not build up the complete template output in memory as a string before returning it.

Syntax
=================
:Variables:
    ``{$variable_name}``
:Loops:
    ``{% for person in people %}Name: {$person.name}{% endfor %}``
:If:
    ``{% if person.name == "Bob" %}Full name: Robert{% endif %}``
:Def:
    ``{% def foobar %}template to store in the named variable{% enddef %}``

Anywhere a variable name is used, you may use a dotted key path to walk through the
keys of nested data_map objects. The elements of the key path used with a def statement
will be created if they do not exist.

Anywhere a key path is used, you may also use a quoted string. This is actually only
useful with if statements, as shown above.

Operators for if statement (x is a variable path):

==========  =======================================================
x           True if x is not empty
not x       True if x is empty
x == y      Equality
x != y      Inequality
==========  =======================================================

Inside a for statement block, a "loop" map variable is defined with these keys:

==========  =======================================================
``index``   Base-1 current index of loop
``index0``  Base-0 current index of loop
``last``    True if this is the last iteration through the loop
``count``   Total number of elements in the list
==========  =======================================================

Def statements evaluate the template between the def and enddef and store the results
in the named variable, which may be a path. The def template contents are evaluated
only once at the point of the def statement, not when the def variable is used.

There are a few pseudo-functions that may be used in if statements and variable
substitution. More may be added later.

===============  ===========================================================
``count(x)``     Returns the number of items in the specified list.
``defined(x)``   Returns true if the key path specifies an existing key.
``empty(x)``     True if the variable path x is the empty string.
===============  ===========================================================

Control statements inside ``{% %}`` may be commented at the end of the statement. Comments
are started with ``--`` and run to the close brace of the statement::

    {% for person in people %}
    Name: {$person.name
    {% endfor -- end the person loop %}

Control statements on a line by themselves will eat the newline following the statement.

Types
==================
All values are stored in a data_ptr variant object.

These are the built-in data types::

    bool
    std::string
    data_list
    data_map

All other types are converted to strings using ``boost::lexical_cast`` when set in
a data_ptr or data_map.

Handy Functions
========================
``make_data()`` : Feed it a bool, string, data_map, or data_list to create a data entry.
Example::

    data_map person ;
    person["name"] = make_data("Bob") ;
    person["occupation"] = make_data("Plumber") ;
    data_map data ;
    data["person"] = make_data(person) ;
    std::string result = parse(templ_text, data) ;

Note that using make_data() is only one method. You may also assign values directly to
data_map elements::

    data_map person;
    person["age"] = 35;
    person["name"] = "Fred";
    person["has_pet"] = true;

Errors
==================
Any template errors will result in a TemplateException being thrown.

The TemplateException class is a subclass of ``std::exception``, so it has a ``what()``
method. This method will return an error string describing the error. In most cases,
the message will be prefixed with the line number of the input template that caused the
error.

Known Issues
==================
- Quoted strings may not have spaces in them.

*/
#pragma once

#ifdef _WIN32
#pragma warning( disable : 4996 ) // 'std::copy': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#pragma warning( disable : 4512 ) // 'std::copy': Function call with parameters that may be unsafe - this call relies on the caller to check that the passed values are correct. To disable this warning, use -D_SCL_SECURE_NO_WARNINGS. See documentation on how to use Visual C++ 'Checked Iterators'
#endif

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <boost/lexical_cast.hpp>

#include <iostream>

namespace cpptempl
{
	// various typedefs
    class data_ptr;
    class data_map;
    class DataMap;
    class DataTemplate;

	typedef std::vector<data_ptr> data_list ;


	// data classes
	class Data
	{
	public:
        virtual ~Data()=default;
		virtual bool empty() = 0 ;
		virtual std::string getvalue();
		virtual data_list& getlist();
		virtual data_map& getmap() ;
	};

    class DataBool : public Data
    {
        bool m_value;
    public:
        DataBool(bool value) : m_value(value) {}
        std::string getvalue();
        bool empty();
    };

	class DataValue : public Data
	{
        std::string m_value ;
	public:
		DataValue(const std::string& value) : m_value(value){}
		DataValue(const std::string&& value) : m_value(value){}
        std::string getvalue();
		bool empty();
	};

	class DataList : public Data
	{
		data_list m_items ;
	public:
		DataList(const data_list &items) : m_items(items){}
		DataList(const data_list &&items) : m_items(items){}
		data_list& getlist() ;
		bool empty();
	};

	class data_ptr {
	public:
		data_ptr() {}
		template<typename T> data_ptr(const T& data) {
			this->operator =(data);
		}
        data_ptr(DataBool* data);
		data_ptr(DataValue* data);
        data_ptr(DataList* data);
        data_ptr(DataMap* data);
        data_ptr(DataTemplate* data);
		data_ptr(const data_ptr& data) {
			ptr = data.ptr;
		}
		template<typename T> void operator = (const T& data);
		template<typename T> void operator = (const T&& data);
		void push_back(const data_ptr& data);
		virtual ~data_ptr() {}
		Data* operator ->() {
			return ptr.get();
		}
        std::shared_ptr<Data> get() { return ptr; }
        bool isTemplate() const;
	private:
		std::shared_ptr<Data> ptr;
	};

	class data_map {
	public:
        class key_error : public std::runtime_error
        {
        public:
            key_error(const std::string & msg) : std::runtime_error(msg) {}
        };

		data_ptr& operator [](const std::string& key);
		bool empty();
		bool has(const std::string& key);
        data_ptr& parse_path(const std::string& key, bool create=false);
	private:
		std::unordered_map<std::string, data_ptr> data;
	};

	class DataMap : public Data
	{
		data_map m_items ;
	public:
		DataMap(const data_map &items) : m_items(items){}
		DataMap(const data_map &&items) : m_items(items){}
		data_map& getmap();
		bool empty();
	};

	template<> void data_ptr::operator = (const data_ptr& data);
	template<> void data_ptr::operator = (const bool& data);
	template<> void data_ptr::operator = (const std::string& data);
	template<> void data_ptr::operator = (const data_map& data);
	template<> void data_ptr::operator = (const data_list& data);
	template<typename T>
	void data_ptr::operator = (const T& data) {
		this->operator =(boost::lexical_cast<std::string>(data));
	}

	template<> void data_ptr::operator = (const data_ptr&& data);
	template<> void data_ptr::operator = (const bool&& data);
	template<> void data_ptr::operator = (const std::string&& data);
	template<> void data_ptr::operator = (const data_map&& data);
	template<> void data_ptr::operator = (const data_list&& data);
	template<typename T>
	void data_ptr::operator = (const T&& data) {
		this->operator =(boost::lexical_cast<std::string>(data));
	}

	// Custom exception class for library errors
	class TemplateException : public std::exception
	{
        uint32_t m_line;
        std::string m_reason;
	public:
		TemplateException(std::string reason) : std::exception(), m_line(0), m_reason(reason) {}
        TemplateException(size_t line, std::string reason);
        TemplateException(const TemplateException & other)=default;
        TemplateException& operator=(const TemplateException & other)=default;
        virtual ~TemplateException()=default;

        void set_reason(std::string reason) { m_reason = reason; }
        void set_line_if_missing(size_t line);

        virtual const char* what() const noexcept { return m_reason.c_str(); }
	};

	// convenience functions for making data objects
	inline data_ptr make_data(bool val)
	{
		return data_ptr(new DataBool(val)) ;
	}
	inline data_ptr make_data(std::string &val)
	{
		return data_ptr(new DataValue(val)) ;
	}
	inline data_ptr make_data(std::string &&val)
	{
		return data_ptr(new DataValue(val)) ;
	}
	inline data_ptr make_data(data_list &val)
	{
		return data_ptr(new DataList(val)) ;
	}
	inline data_ptr make_data(data_list &&val)
	{
		return data_ptr(new DataList(val)) ;
	}
	inline data_ptr make_data(data_map &val)
	{
		return data_ptr(new DataMap(val)) ;
	}
	inline data_ptr make_data(data_map &&val)
	{
		return data_ptr(new DataMap(val)) ;
	}
    template <typename T>
    data_ptr make_data(const T &val)
    {
        return data_ptr(boost::lexical_cast<std::string>(val));
    }

    data_ptr make_template(const std::string & templateText);

namespace impl {

	// token classes
	class Token ;
	typedef std::shared_ptr<Token> token_ptr ;
	typedef std::vector<token_ptr> token_vector ;

	typedef enum
	{
		TOKEN_TYPE_NONE,
		TOKEN_TYPE_TEXT,
		TOKEN_TYPE_VAR,
		TOKEN_TYPE_IF,
		TOKEN_TYPE_FOR,
        TOKEN_TYPE_DEF,
		TOKEN_TYPE_ENDIF,
		TOKEN_TYPE_ENDFOR,
        TOKEN_TYPE_ENDDEF
	} TokenType;

	// Template tokens
	// base class for all token types
	class Token
	{
        uint32_t m_line;
	public:
        Token(uint32_t line) : m_line(line) {}
		virtual TokenType gettype() = 0 ;
		virtual void gettext(std::ostream &stream, data_map &data) = 0 ;
		virtual void set_children(token_vector &children);
		virtual token_vector & get_children();
        uint32_t get_line() { return m_line; }
        void set_line(uint32_t line) { m_line = line; }
	};

	// token with children
	class TokenParent : public Token
	{
    protected:
		token_vector m_children ;
	public:
        TokenParent(uint32_t line) : Token(line) {}
		void set_children(token_vector &children);
		token_vector &get_children();
	};

	// normal text
	class TokenText : public Token
	{
        std::string m_text ;
	public:
		TokenText(std::string text, uint32_t line=0) : Token(line), m_text(text){}
		TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// variable
	class TokenVar : public Token
	{
        std::string m_key ;
	public:
		TokenVar(std::string key, uint32_t line=0) : Token(line), m_key(key){}
		TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// for block
	class TokenFor : public TokenParent
	{
        std::string m_key ;
        std::string m_val ;
	public:
		TokenFor(std::string expr, uint32_t line=0);
		TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// if block
	class TokenIf : public TokenParent
	{
        std::string m_expr ;
	public:
		TokenIf(std::string expr, uint32_t line=0) : TokenParent(line), m_expr(expr){}
		TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
		bool is_true(std::string expr, data_map &data);
	};

    // def block
    class TokenDef : public TokenParent
    {
        std::string m_name;
	public:
        TokenDef(std::string name, uint32_t line=0);
        TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
    };

	// end of block
	class TokenEnd : public Token // end of control block
	{
        TokenType m_type ;
	public:
		TokenEnd(std::string text, uint32_t line=0);
		TokenType gettype() { return m_type; }
		void gettext(std::ostream &stream, data_map &data);
	};

    std::string gettext(token_ptr token, data_map &data) ;

	void parse_tree(token_vector &tokens, token_vector &tree, TokenType until=TOKEN_TYPE_NONE) ;
	token_vector & tokenize(std::string text, token_vector &tokens) ;

} // namespace impl

    class DataTemplate : public Data
    {
        impl::token_vector m_tree;
    public:
		DataTemplate(const std::string & templateText);
		DataTemplate(const impl::token_vector &tree) : m_tree(tree) {}
		DataTemplate(const impl::token_vector &&tree) : m_tree(tree) {}
		virtual std::string getvalue();
		virtual bool empty();
		std::string parse(data_map & data);
    };

	// The big daddy. Pass in the template and data,
	// and get out a completed doc.
	void parse(std::ostream &stream, std::string templ_text, data_map &data) ;
    std::string parse(std::string templ_text, data_map &data);
}
