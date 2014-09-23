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
    ``{% def foobar %}template to store in the {$named} variable{% enddef %}``
:Comment:
    ``{# comment goes here #}``

Control statements on a line by themselves will eat the newline following the statement.

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

Def statements are described below under the Subtemplates section.

There are a few pseudo-functions that may be used in if statements and variable
substitution. More may be added later.

===============  ===========================================================
``count(x)``     Returns the number of items in the specified list.
``defined(x)``   Returns true if the key path specifies an existing key.
``empty(x)``     True if the variable path x is the empty string.
===============  ===========================================================

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

There are two ways to define a subtemplate. The first is to use the ``make_template()``
function. It takes a std::string and returns a subtemplate data_ptr, which may then
be stored in a data_map.

The second way to create a subtemplate is to use the def statement within a template.
Def statements define a subtemplate with the template contents between the def and
enddef statements. The subtemplate is stored in the named variable, which may be a path.
As with all subtemplates, the contents are evaluated at the point where the def variable
is used.

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

``make_template()`` : Creates a subtemplate from a std::string.

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
