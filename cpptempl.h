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

    void parse(std::ostream &stream, std::string templ_text, data_map &data);

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
    ``{% def foobar(x) %}The value of x is {$x} today{% enddef %}``
:Comment:
    ``{# comment goes here #}``

Control statements on a line by themselves will eat the newline following the statement.

Anywhere a variable name is used, you may use a dotted key path to walk through the
keys of nested data_map objects.

If statements and variable substitution blocks accept arbitrary expressions. This is
currently only useful for if statements, as the expressions are only Boolean.

Operators for expressions (x and y are subexpressions):

==============  =======================================================
!x              True if x is empty or false
x == y          Equality
x != y          Inequality
x && y          Boolean and
x || y          Boolean or
(x)             Parenthesized subexpression
sub(x,y,...)    Subtemplate invocation with parameters
==============  =======================================================

The keywords "not", "and", and "or" are also supported in place of the C-style operators.
Thus, "not (x and y)" is completely equivalent to "!(x && y)".

There are also a few pseudo-functions that may be used in expressions. More may be added
later.

===============  ===========================================================
``count(x)``     Returns the number of items in the specified list.
``defined(x)``   Returns true if the key path specifies an existing key.
``empty(x)``     True if the variable path x is the empty string.
===============  ===========================================================

Supported value types in expressions:

==========  ===============================================================
key         Name of key in top-level data_map (simple case of key path).
key.path    Dotted path of data_map keys.
true        Boolean true.
false       Boolean false.
"text"      Quoted string with double quotes.
'text'      Quoted string with single quotes.
==========  ===============================================================

If the expression in an if statement produces a non-Boolean value such as a string,
then the expression is considered true if the value is not empty.

Inside a for statement block, a "loop" map variable is defined with these keys:

==========  =======================================================
``index``   Base-1 current index of loop
``index0``  Base-0 current index of loop
``last``    True if this is the last iteration through the loop
``count``   Total number of elements in the list
==========  =======================================================

Def statements are described below under the Subtemplates section.

Control statements inside ``{% %}`` brackets may be commented at the end of the statement.
This style of comment is started with ``--`` and runs to the close bracket of the statement,
as demonstrated here::

    {% for person in people %}
    Name: {$person.name}
    {% endfor -- end the person loop %}

You may also put comments in ``{# #}`` brackets. These comments may span multiple lines.
They will not be copied to the output under any circumstances. And, as with all control
statements, if such a comment is on a line by itself, the newline following the comment
is absorbed and not reproduced in the output.

Types
==================
All values are stored in a data_ptr variant object.

These are the built-in data types::

    bool
    std::string
    data_list
    data_map
    subtemplate

All other types are converted to strings using ``boost::lexical_cast`` when set in
a data_ptr or data_map.

Bool values will result in either "true" or "false" when substituted. data_list or
data_map values will cause a TemplateException to the thrown if you attempt to
substitute them as a variable.

Subtemplates
==================
Subtemplates are a special type. They allow you to define a template once and reuse
it multiple times by substituting it as a variable. A subtemplate is completely
re-evaluated every time it is substituted, using the current values of any variables.
This is particularly useful within a loop.

Subtemplates may take parameters. These are defined when the subtemplate is created
via either of the methods described below. When a subtemplate is used in a variable
substitution in a template, you may pass values for its parameters just as you would
for a function call.

There are two ways to define a subtemplate. The first is to use the ``make_template()``
function. It takes a std::string and returns a subtemplate data_ptr, which may then
be stored in a data_map. It may also optionally be provided a vector of parameter
name strings.

The second way to create a subtemplate is to use the def statement within a template.
Def statements define a subtemplate with the template contents between the def and
enddef statements. The subtemplate is stored in the named variable, which may be a path.
The elements of the key path will be created if they do not exist. As with all
subtemplates, the contents are evaluated at the point where the def variable is used.

The parameters for a subtemplate may be specified in a def statement. This is done by
listing the parameter names in parentheses after the subtemplate's key path, as shown
in this example::

    {% def mytmpl(foo, bar) %}
    foo={$foo}
    bar={$bar}
    {% enddef %}

To use this subtemplate, you would do something like this::

    {$mytmpl("a", "b")}

This variable substitution expression will pass the string constants "a" and "b" for the
subtemplate parameters "foo" and "bar", respectively. During the evaluation of the
subtemplate, parameter variables will be set to the specified values. If there is
already a key in the global data map with the same name as a parameter, the parameter
will shadow the global key. The global data map is not modified permanently. Any
parameter keys will be restored to the original state, including being undefined, once the
subtemplate evaluation is completed. Any expression may be used to generate the parameter
values.

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

``make_template()`` : Creates a subtemplate from a std::string. The template string is
passed as the first parameter. An optional pointer to a std::string vector can be provided
as a second parameter to specify the names of subtemplate parameters.

Example of creating a subtemplate with params::

    string_vector params{"foo", "bar"};
    data_ptr subtmpl = make_template(template_text, &params);

Errors
==================
Any template errors will result in a TemplateException being thrown.

The TemplateException class is a subclass of ``std::exception``, so it has a ``what()``
method. This method will return an error string describing the error. In most cases,
the message will be prefixed with the line number of the input template that caused the
error.

Known Issues
==================
- "defined" pseudo-function is broken, always returning true.

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
		DataValue(std::string&& value) : m_value(value){}
        std::string getvalue();
		bool empty();
	};

	class DataList : public Data
	{
		data_list m_items ;
	public:
		DataList(const data_list &items) : m_items(items){}
		DataList(data_list &&items) : m_items(items){}
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
        data_ptr(data_ptr&& data) {
            ptr = std::move(data.ptr);
        }
        data_ptr& operator = (const data_ptr& data) {
            ptr = data.ptr;
            return *this;
        }
        data_ptr& operator = (data_ptr&& data) {
            ptr = std::move(data.ptr);
            return *this;
        }
        data_ptr& operator =(std::string&& data);
        data_ptr& operator =(data_map&& data);
        data_ptr& operator =(data_list&& data);
		template<typename T> void operator = (const T& data);
		void push_back(const data_ptr& data);
		virtual ~data_ptr() {}
		Data* operator ->() {
			return ptr.get();
		}
        std::shared_ptr<Data> get() { return ptr; }
        bool is_template() const;
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

        data_map() : data(), parent(nullptr) {}
		data_ptr& operator [](const std::string& key);
		bool empty();
		bool has(const std::string& key);
        data_ptr& parse_path(const std::string& key, bool create=false);
        void set_parent(data_map * p) { parent = p; }
	private:
		std::unordered_map<std::string, data_ptr> data;
        data_map * parent;
	};

	class DataMap : public Data
	{
		data_map m_items ;
	public:
		DataMap(const data_map &items) : m_items(items){}
		DataMap(data_map &&items) : m_items(items){}
		data_map& getmap();
		bool empty();
	};

	template<> void data_ptr::operator = (const bool& data);
	template<> void data_ptr::operator = (const std::string& data);
	template<> void data_ptr::operator = (const data_map& data);
	template<> void data_ptr::operator = (const data_list& data);
	template<typename T>
	void data_ptr::operator = (const T& data) {
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

namespace impl {

	// node classes
	class Node ;
	typedef std::shared_ptr<Node> node_ptr ;
	typedef std::vector<node_ptr> node_vector ;

} // namespace impl

    // List of param names.
    typedef std::vector<std::string> string_vector;

    class DataTemplate : public Data
    {
        impl::node_vector m_tree;
        string_vector m_params;
    public:
		DataTemplate(const std::string & templateText);
		DataTemplate(const impl::node_vector &tree) : m_tree(tree) {}
		DataTemplate(impl::node_vector &&tree) : m_tree(tree) {}
		virtual std::string getvalue();
		virtual bool empty();
		std::string eval(data_map & data, data_list * param_values=nullptr);
        string_vector & params() { return m_params; }
    };

    inline data_ptr make_template(const std::string & templateText, const string_vector * param_names=nullptr)
    {
        DataTemplate * t = new DataTemplate(templateText);
        if (param_names)
        {
            t->params() = *param_names;
        }
        return data_ptr(t);
    }

	// The big daddy. Pass in the template and data,
	// and get out a completed doc.
	void parse(std::ostream &stream, std::string templ_text, data_map &data) ;
    std::string parse(std::string templ_text, data_map &data);
}
