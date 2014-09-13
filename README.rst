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

