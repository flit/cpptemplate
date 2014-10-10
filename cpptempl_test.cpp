// Copyright (c) 2010-2014 Ryan Ginstrom
// Copyright (c) 2014 Freescale Semiconductor, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define CPPTEMPL_UNIT_TEST
#include "cpptempl.h"
#include "cpptempl.cpp"

#define BOOST_TEST_ALTERNATIVE_INIT_API
// #ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE cpptemplTests
// #endif

#include <boost/test/unit_test.hpp>

#pragma warning( disable : 4996 ) // doesn't like wcstombs

#include "unit_testing.h"

using namespace boost::unit_test;
using namespace std ;
using namespace cpptempl;
using namespace cpptempl::impl;

std::string gettext(node_ptr node, data_map & data)
{
    ostringstream stream;
    node->gettext(stream, data);
    return stream.str();
}

BOOST_AUTO_TEST_SUITE( TestCppData )

	// DataMap
	BOOST_AUTO_TEST_CASE(test_DataMap_getvalue)
	{
		data_map items ;
		data_ptr data(new DataMap(items)) ;
		BOOST_CHECK_THROW( data->getvalue(), TemplateException ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataMap_getlist_throws)
	{
		data_map items ;
		data_ptr data(new DataMap(items)) ;

		BOOST_CHECK_THROW( data->getlist(), TemplateException ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataMap_getitem)
	{
		data_map items ;
		items["key"] = data_ptr(new DataValue("foo")) ;
		data_ptr data(new DataMap(items)) ;

		BOOST_CHECK_EQUAL( data->getmap()["key"]->getvalue(), "foo" ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataMap_parse_path)
	{
		data_map items ;
		data_map foo;
		data_map bar;
		data_map baz;
		items["a"] = "a";
		foo["b"] = "b";
		bar["c"] = "c";
		baz["d"] = "d";
		bar["baz"] = baz;
		foo["bar"] = bar;
		items["foo"] = foo;
		data_ptr data(new DataMap(items)) ;
		BOOST_CHECK_EQUAL( items.parse_path("a")->getvalue(), "a" );
		BOOST_CHECK_EQUAL( items.parse_path("foo.b")->getvalue(), "b" );
		BOOST_CHECK_EQUAL( items.parse_path("foo.bar.c")->getvalue(), "c" );
		BOOST_CHECK_EQUAL( items.parse_path("foo.bar.baz.d")->getvalue(), "d" );
		BOOST_CHECK_THROW( items.parse_path("xx.yy"), data_map::key_error ) ;
		BOOST_CHECK_THROW( items.parse_path("foo.bar.yy"), data_map::key_error ) ;
	}
	// DataList
	BOOST_AUTO_TEST_CASE(test_DataList_getvalue)
	{
		data_list items ;
		data_ptr data(new DataList(items)) ;

		BOOST_CHECK_THROW( data->getvalue(), TemplateException ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataList_getlist_throws)
	{
		data_list items ;
		items.push_back(make_data("bar")) ;
		data_ptr data(new DataList(items)) ;

		BOOST_CHECK_EQUAL( data->getlist().size(), 1u ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataList_getitem_throws)
	{
		data_list items ;
		data_ptr data(new DataList(items)) ;

		BOOST_CHECK_THROW( data->getmap(), TemplateException ) ;
	}
	// DataValue
	BOOST_AUTO_TEST_CASE(test_DataValue_getvalue)
	{
		data_ptr data(new DataValue("foo")) ;

		BOOST_CHECK_EQUAL( data->getvalue(), "foo" ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataValue_getlist_throws)
	{
		data_ptr data(new DataValue("foo")) ;

		BOOST_CHECK_THROW( data->getlist(), TemplateException ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataValue_getitem_throws)
	{
		data_ptr data(new DataValue("foo")) ;

		BOOST_CHECK_THROW( data->getmap(), TemplateException ) ;
	}
	// DataBool
	BOOST_AUTO_TEST_CASE(test_DataBool_true)
	{
		data_ptr data(new DataBool(true)) ;

		BOOST_CHECK_EQUAL( data->getvalue(), "true" ) ;
		BOOST_CHECK_EQUAL( data->empty(), false ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataBool_false)
	{
		data_ptr data(new DataBool(false)) ;

		BOOST_CHECK_EQUAL( data->getvalue(), "false" ) ;
		BOOST_CHECK_EQUAL( data->empty(), true ) ;
	}
	BOOST_AUTO_TEST_CASE(test_DataBool_getitem_throws)
	{
		data_ptr data(new DataBool(false)) ;

		BOOST_CHECK_THROW( data->getmap(), TemplateException ) ;
	}
BOOST_AUTO_TEST_SUITE_END()

// BOOST_AUTO_TEST_SUITE( TestCppParseVal )
//
// 	BOOST_AUTO_TEST_CASE(test_quoted)
// 	{
// 		data_map data ;
// 		data["foo"] = make_data("bar") ;
// 		data_ptr value = parse_val("\"foo\"", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getvalue(), "foo" ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_value)
// 	{
// 		data_map data ;
// 		data["foo"] = make_data("bar") ;
// 		data_ptr value = parse_val("foo", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getvalue(), "bar" ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_not_found)
// 	{
// 		data_map data ;
// 		data["foo"] = make_data("bar") ;
// 		data_ptr value = parse_val("kettle", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getvalue(), "{$kettle}" ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_not_found_dotted)
// 	{
// 		data_map data ;
// 		data["foo"] = make_data("bar") ;
// 		data_ptr value = parse_val("kettle.black", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getvalue(), "{$kettle.black}" ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_my_ax)
// 	{
// 		data_map data ;
// 		data["item"] = make_data("my ax") ;
// 		BOOST_CHECK_EQUAL( parse_val("item", data)->getvalue(), "my ax" ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_list)
// 	{
// 		data_map data ;
// 		data_list items ;
// 		items.push_back(make_data("bar")) ;
// 		data["foo"] = data_ptr(new DataList(items)) ;
// 		data_ptr value = parse_val("foo", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getlist().size(), 1u ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_dotted)
// 	{
// 		data_map data ;
// 		data_map subdata ;
// 		subdata["b"] = data_ptr(new DataValue("c")) ;
// 		data["a"] = data_ptr(new DataMap(subdata)) ;
// 		data_ptr value = parse_val("a.b", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getvalue(), "c" ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_double_dotted)
// 	{
// 		data_map data ;
// 		data_map sub_data ;
// 		data_map sub_sub_data ;
// 		sub_sub_data["c"] = data_ptr(new DataValue("d")) ;
// 		sub_data["b"] = data_ptr(new DataMap(sub_sub_data)) ;
// 		data["a"] = data_ptr(new DataMap(sub_data)) ;
// 		data_ptr value = parse_val("a.b.c", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getvalue(), "d" ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_dotted_to_list)
// 	{
// 		data_list friends ;
// 		friends.push_back(make_data("Bob")) ;
// 		data_map person ;
// 		person["friends"] = make_data(friends) ;
// 		data_map data ;
// 		data["person"] = make_data(person) ;
// 		data_ptr value = parse_val("person.friends", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getlist().size(), 1u ) ;
// 	}
// 	BOOST_AUTO_TEST_CASE(test_dotted_to_dict_list)
// 	{
// 		data_map bob ;
// 		bob["name"] = make_data("Bob") ;
// 		data_map betty ;
// 		betty["name"] = make_data("Betty") ;
// 		data_list friends ;
// 		friends.push_back(make_data(bob)) ;
// 		friends.push_back(make_data(betty)) ;
// 		data_map person ;
// 		person["friends"] = make_data(friends) ;
// 		data_map data ;
// 		data["person"] = make_data(person) ;
// 		data_ptr value = parse_val("person.friends", data) ;
//
// 		BOOST_CHECK_EQUAL( value->getlist()[0]->getmap()["name"]->getvalue(), "Bob" ) ;
// 	}
// BOOST_AUTO_TEST_SUITE_END()

 BOOST_AUTO_TEST_SUITE( TestCppNode )

 	// NodeVar
 	BOOST_AUTO_TEST_CASE(TestNodeVarType)
 	{
 	    token_vector tokens = impl::tokenize_statement("foo");
 		NodeVar node(tokens) ;
 		BOOST_CHECK_EQUAL( node.gettype(), NODE_TYPE_VAR ) ;
 	}
 	BOOST_AUTO_TEST_CASE(TestNodeVar)
 	{
 		node_ptr node(new NodeVar(tokenize_statement("foo"))) ;
 		data_map data ;
 		data["foo"] = make_data("bar") ;
 		BOOST_CHECK_EQUAL( gettext(node, data), "bar" ) ;
 	}
	BOOST_AUTO_TEST_CASE(TestNodeVarCantHaveChildren)
	{
		NodeVar node(tokenize_statement("foo")) ;
		node_vector children ;
		BOOST_CHECK_THROW(node.set_children(children), TemplateException) ;
	}
	// NodeText
	BOOST_AUTO_TEST_CASE(TestNodeTextType)
	{
		NodeText node("foo") ;
		BOOST_CHECK_EQUAL( node.gettype(), NODE_TYPE_TEXT ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeText)
	{
		node_ptr node(new NodeText("foo")) ;
		data_map data ;
		data["foo"] = make_data("bar") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "foo" ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeTextCantHaveChildrenSet)
	{
		NodeText node("foo") ;
		node_vector children ;
		BOOST_CHECK_THROW(node.set_children(children), TemplateException) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeTextCantHaveChildrenGet)
	{
		NodeText node("foo") ;
		node_vector children ;
		BOOST_CHECK_THROW(node.get_children(), TemplateException) ;
	}
	// NodeFor
	BOOST_AUTO_TEST_CASE(TestNodeForBadSyntax)
	{
		BOOST_CHECK_THROW(NodeFor node(tokenize_statement("foo"), true), TemplateException ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeForType)
	{
		NodeFor node(tokenize_statement("for item in items"), true) ;
		BOOST_CHECK_EQUAL( node.gettype(), NODE_TYPE_FOR ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeForTextEmpty)
	{
		node_ptr node(new NodeFor(tokenize_statement("for item in items"), true)) ;
		data_map data ;
		data_list items ;
		items.push_back(make_data("first"));
		data["items"] = make_data(items) ;
		BOOST_CHECK_EQUAL( gettext(node, data), "" ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeForTextOneVar)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		node_ptr node(new NodeFor(tokenize_statement("for item in items"), true)) ;
		node->set_children(children) ;
		data_map data ;
		data_list items ;
		items.push_back(make_data("first "));
		items.push_back(make_data("second "));
		data["items"] = make_data(items) ;
		BOOST_CHECK_EQUAL( gettext(node, data), "first second " ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeForTextOneVarLoop)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("loop.index")))) ;
		node_ptr node(new NodeFor(tokenize_statement("for item in items"), true)) ;
		node->set_children(children) ;
		data_map data ;
		data_list items ;
		items.push_back(make_data("first "));
		items.push_back(make_data("second "));
		data["items"] = make_data(items) ;
		BOOST_CHECK_EQUAL( gettext(node, data), "12" ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeForLoopTextVar)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("loop.index")))) ;
		children.push_back(node_ptr(new NodeText(". "))) ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		children.push_back(node_ptr(new NodeText(" "))) ;
		node_ptr node(new NodeFor(tokenize_statement("for item in items"), true)) ;
		node->set_children(children) ;
		data_map data ;
		data_list items ;
		items.push_back(make_data("first"));
		items.push_back(make_data("second"));
		data["items"] = make_data(items) ;
		BOOST_CHECK_EQUAL( gettext(node, data), "1. first 2. second " ) ;
	}
// 	BOOST_AUTO_TEST_CASE(TestNodeForLoopTextVarDottedKeyAndVal)
// 	{
// 		NodeFor node(tokenize_statement("for friend in person.friends"), true) ;
// 		BOOST_CHECK_EQUAL( node.m_key, "person.friends" ) ;
// 		BOOST_CHECK_EQUAL( node.m_val, "friend" ) ;
// 	}
	BOOST_AUTO_TEST_CASE(TestNodeForLoopTextVarDotted)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("loop.index")))) ;
		children.push_back(node_ptr(new NodeText(". "))) ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("friend.name")))) ;
		children.push_back(node_ptr(new NodeText(" "))) ;
		node_ptr node(new NodeFor(tokenize_statement("for friend in person.friends"), true)) ;
		node->set_children(children) ;

		data_map bob ;
		bob["name"] = make_data("Bob") ;
		data_map betty ;
		betty["name"] = make_data("Betty") ;
		data_list friends ;
		friends.push_back(make_data(bob)) ;
		friends.push_back(make_data(betty)) ;
		data_map person ;
		person["friends"] = make_data(friends) ;
		data_map data ;
		data["person"] = make_data(person) ;

		BOOST_CHECK_EQUAL( gettext(node, data), "1. Bob 2. Betty " ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeForTextOneText)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeText("{--}"))) ;
		node_ptr node(new NodeFor(tokenize_statement("for item in items"), true)) ;
		node->set_children(children) ;
		data_map data ;
		data_list items ;
		items.push_back(make_data("first "));
		items.push_back(make_data("second "));
		data["items"] = make_data(items) ;
		BOOST_CHECK_EQUAL( gettext(node, data), "{--}{--}" ) ;
	}

	//////////////////////////////////////////////////////////////////////////
	// NodeIf
	//////////////////////////////////////////////////////////////////////////

	BOOST_AUTO_TEST_CASE(TestNodeIfType)
	{
		NodeIf node(tokenize_statement("if items")) ;
		BOOST_CHECK_EQUAL( node.gettype(), NODE_TYPE_IF ) ;
	}
	// if not empty
	BOOST_AUTO_TEST_CASE(TestNodeIfText)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeText("{--}"))) ;
		node_ptr node(new NodeIf(tokenize_statement("if item"))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("foo") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "{--}" ) ;
		data["item"] = false;
		BOOST_CHECK_EQUAL( gettext(node, data), "" ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeIfVar)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		node_ptr node(new NodeIf(tokenize_statement("if item"))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("foo") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "foo" ) ;
		data["item"] = false;
		BOOST_CHECK_EQUAL( gettext(node, data), "" ) ;
	}

	// ==
	BOOST_AUTO_TEST_CASE(TestNodeIfEqualsTrue)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		node_ptr node(new NodeIf(tokenize_statement("if item == \"foo\""))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("foo") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "foo" ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeIfEqualsFalse)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		node_ptr node(new NodeIf(tokenize_statement("if item == \"bar\""))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("foo") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "" ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeIfEqualsTwoVarsTrue)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		node_ptr node(new NodeIf(tokenize_statement("if item == foo"))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("x") ;
		data["foo"] = make_data("x") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "x" ) ;
		data["foo"] = make_data("z") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "" ) ;
	}

	// !=
	BOOST_AUTO_TEST_CASE(TestNodeIfNotEqualsTrue)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		node_ptr node(new NodeIf(tokenize_statement("if item != \"foo\""))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("foo") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "" ) ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeIfNotEqualsFalse)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeVar(tokenize_statement("item")))) ;
		node_ptr node(new NodeIf(tokenize_statement("if item != \"bar\""))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("foo") ;
		dump_data(data);
		BOOST_CHECK_EQUAL( gettext(node, data), "foo" ) ;
	}

	// not
	BOOST_AUTO_TEST_CASE(TestNodeIfNotTrueText)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeText("{--}"))) ;
		node_ptr node(new NodeIf(tokenize_statement("if not item"))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("foo") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "") ;
	}
	BOOST_AUTO_TEST_CASE(TestNodeIfNotFalseText)
	{
		node_vector children ;
		children.push_back(node_ptr(new NodeText("{--}"))) ;
		node_ptr node(new NodeIf(tokenize_statement("if not item"))) ;
		node->set_children(children) ;
		data_map data ;
		data["item"] = make_data("") ;
		BOOST_CHECK_EQUAL( gettext(node, data), "{--}") ;
	}

BOOST_AUTO_TEST_SUITE_END()

 BOOST_AUTO_TEST_SUITE( TestCppParser )

 	BOOST_AUTO_TEST_CASE(test_empty)
 	{
 		string text = "" ;
 		node_vector nodes ;
 		impl::TemplateParser(text, nodes).parse() ;

 		BOOST_CHECK_EQUAL( 0u, nodes.size() ) ;
 	}
 	BOOST_AUTO_TEST_CASE(test_text_only)
 	{
 		string text = "blah blah blah" ;
 		node_vector nodes ;
 		impl::TemplateParser(text, nodes).parse() ;
 		data_map data ;

 		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
 		BOOST_CHECK_EQUAL( gettext(nodes[0], data), "blah blah blah" ) ;
 	}
	BOOST_AUTO_TEST_CASE(test_brackets_no_var)
	{
		string text = "{foo}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;
		data_map data ;

		BOOST_CHECK_EQUAL( 2u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0], data), "{" ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[1], data), "foo}" ) ;
	}
	BOOST_AUTO_TEST_CASE(test_ends_with_bracket)
	{
		string text = "blah blah blah{" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;
		data_map data ;

		BOOST_CHECK_EQUAL( 2u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0], data), "blah blah blah" ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[1], data), "{" ) ;
	}
	// var
	BOOST_AUTO_TEST_CASE(test_var)
	{
		string text = "{$foo}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;
		data_map data ;
		data["foo"] = make_data("bar") ;

		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0], data), "bar" ) ;
	}
	// for
	BOOST_AUTO_TEST_CASE(test_for)
	{
		string text = "{% for item in items %}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;

		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( nodes[0]->gettype(), NODE_TYPE_FOR ) ;
	}
	BOOST_AUTO_TEST_CASE(test_for_full)
	{
		string text = "{% for item in items %}{$item}{% endfor %}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;

		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( 1u, nodes[0]->get_children().size() ) ;
		BOOST_CHECK_EQUAL( nodes[0]->gettype(), NODE_TYPE_FOR ) ;
		BOOST_CHECK_EQUAL( nodes[0]->get_children()[0]->gettype(), NODE_TYPE_VAR ) ;
	}
	BOOST_AUTO_TEST_CASE(test_for_full_with_text)
	{
		string text = "{% for item in items %}*{$item}*{% endfor %}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;
		data_map data ;
		data["item"] = make_data("my ax") ;

		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( 3u, nodes[0]->get_children().size() ) ;
		BOOST_CHECK_EQUAL( nodes[0]->gettype(), NODE_TYPE_FOR ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0]->get_children()[0], data), "*" ) ;
		BOOST_CHECK_EQUAL( nodes[0]->get_children()[1]->gettype(), NODE_TYPE_VAR ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0]->get_children()[1], data), "my ax" ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0]->get_children()[2], data), "*" ) ;
	}
	// if
	BOOST_AUTO_TEST_CASE(test_if)
	{
		string text = "{% if foo %}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;

		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( nodes[0]->gettype(), NODE_TYPE_IF ) ;
	}
	BOOST_AUTO_TEST_CASE(test_if_full)
	{
		string text = "{% if item %}{$item}{% endif %}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;

		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( 1u, nodes[0]->get_children().size() ) ;
		BOOST_CHECK_EQUAL( nodes[0]->gettype(), NODE_TYPE_IF ) ;
		BOOST_CHECK_EQUAL( nodes[0]->get_children()[0]->gettype(), NODE_TYPE_VAR ) ;
	}
	BOOST_AUTO_TEST_CASE(test_if_full_with_text)
	{
		string text = "{% if item %}{{$item}}{% endif %}" ;
		node_vector nodes ;
		impl::TemplateParser(text, nodes).parse() ;
		data_map data ;
		data["item"] = make_data("my ax") ;

		BOOST_CHECK_EQUAL( 1u, nodes.size() ) ;
		BOOST_CHECK_EQUAL( 3u, nodes[0]->get_children().size() ) ;
		BOOST_CHECK_EQUAL( nodes[0]->gettype(), NODE_TYPE_IF ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0]->get_children()[0], data), "{" ) ;
		BOOST_CHECK_EQUAL( nodes[0]->get_children()[1]->gettype(), NODE_TYPE_VAR ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0]->get_children()[1], data), "my ax" ) ;
		BOOST_CHECK_EQUAL( gettext(nodes[0]->get_children()[2], data), "}" ) ;
	}

 BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestCppTemplateEval)

	BOOST_AUTO_TEST_CASE(test_empty)
	{
		string text = "" ;
		data_map data ;
		string actual = parse(text, data) ;
		string expected = "" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_no_vars)
	{
		string text = "foo" ;
		data_map data ;
		string actual = parse(text, data) ;
		string expected = "foo" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_var)
	{
		string text = "{$foo}" ;
		data_map data ;
		data["foo"] = make_data("bar") ;
		string actual = parse(text, data) ;
		string expected = "bar" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_var_surrounded)
	{
		string text = "aaa{$foo}bbb" ;
		data_map data ;
		data["foo"] = make_data("---") ;
		string actual = parse(text, data) ;
		string expected = "aaa---bbb" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_for)
	{
		string text = "{% for item in items %}{$item}{% endfor %}" ;
		data_map data ;
		data_list items ;
		items.push_back(make_data("0")) ;
		items.push_back(make_data("1")) ;
		data["items"] = make_data(items) ;
		string actual = parse(text, data) ;
		string expected = "01" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_if_false)
	{
		string text = "{% if item %}{$item}{% endif %}" ;
		data_map data ;
		data["item"] = make_data("") ;
		string actual = parse(text, data) ;
		string expected = "" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_if_true)
	{
		string text = "{% if item %}{$item}{% endif %}" ;
		data_map data ;
		data["item"] = make_data("foo") ;
		string actual = parse(text, data) ;
		string expected = "foo" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_nested_for)
	{
		string text = "{% for item in items %}{% for thing in things %}{$item}{$thing}{% endfor %}{% endfor %}" ;
		data_map data ;
		data_list items ;
		items.push_back(make_data("0")) ;
		items.push_back(make_data("1")) ;
		data["items"] = make_data(items) ;
		data_list things ;
		things.push_back(make_data("a")) ;
		things.push_back(make_data("b")) ;
		data["things"] = make_data(things) ;
		string actual = parse(text, data) ;
		string expected = "0a0b1a1b" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_nested_if_false)
	{
		string text = "{% if item %}{% if thing %}{$item}{$thing}{% endif %}{% endif %}" ;
		data_map data ;
		data["item"] = make_data("aaa") ;
		data["thing"] = make_data("") ;
		string actual = parse(text, data) ;
		string expected = "" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_nested_if_true)
	{
		string text = "{% if item %}{% if thing %}{$item}{$thing}{% endif %}{% endif %}" ;
		data_map data ;
		data["item"] = make_data("aaa") ;
		data["thing"] = make_data("bbb") ;
		string actual = parse(text, data) ;
		string expected = "aaabbb" ;
		BOOST_CHECK_EQUAL( expected, actual ) ;
	}
	BOOST_AUTO_TEST_CASE(test_usage_example)
	{
		string text = "{% if item %}{$item}{% endif %}\n"
			"{% if thing %}{$thing}{% endif %}" ;
		cpptempl::data_map data ;
		data["item"] = cpptempl::make_data("aaa") ;
		data["thing"] = cpptempl::make_data("bbb") ;

		string result = cpptempl::parse(text, data) ;

		string expected = "aaa\nbbb" ;
		BOOST_CHECK_EQUAL( result, expected ) ;
	}
	BOOST_AUTO_TEST_CASE(test_syntax_if)
	{
		string text = "{% if person.name == \"Bob\" %}Full name: Robert{% endif %}" ;
		data_map person ;
		person["name"] = make_data("Bob") ;
		person["occupation"] = make_data("Plumber") ;
		data_map data ;
		data["person"] = make_data(person) ;

		string result = cpptempl::parse(text, data) ;

		string expected = "Full name: Robert" ;
		BOOST_CHECK_EQUAL( result, expected ) ;
	}
	BOOST_AUTO_TEST_CASE(test_syntax_dotted)
	{
		string text = "{% for friend in person.friends %}"
			"{$loop.index}. {$friend.name} "
			"{% endfor %}" ;

		data_map bob ;
		bob["name"] = make_data("Bob") ;
		data_map betty ;
		betty["name"] = make_data("Betty") ;
		data_list friends ;
		friends.push_back(make_data(bob)) ;
		friends.push_back(make_data(betty)) ;
		data_map person ;
		person["friends"] = make_data(friends) ;
		data_map data ;
		data["person"] = make_data(person) ;

		string result = cpptempl::parse(text, data) ;

		string expected = "1. Bob 2. Betty " ;
		BOOST_CHECK_EQUAL( result, expected ) ;
	}
	BOOST_AUTO_TEST_CASE(test_example_okinawa)
	{
		// The text template
		string text = "I heart {$place}!" ;
		// Data to feed the template engine
		cpptempl::data_map data ;
		// {$place} => Okinawa
		data["place"] = cpptempl::make_data("Okinawa");
		// parse the template with the supplied data dictionary
		string result = cpptempl::parse(text, data) ;

		string expected = "I heart Okinawa!" ;
		BOOST_CHECK_EQUAL( result, expected ) ;
	}
	BOOST_AUTO_TEST_CASE(test_example_ul)
	{
		string text = "<h3>Locations</h3><ul>"
			"{% for place in places %}"
			"<li>{$place}</li>"
			"{% endfor %}"
			"</ul>" ;

		// Create the list of items
		cpptempl::data_list places;
		places.push_back(cpptempl::make_data("Okinawa"));
		places.push_back(cpptempl::make_data("San Francisco"));
		// Now set this in the data map
		cpptempl::data_map data ;
		data["places"] = cpptempl::make_data(places);
		// parse the template with the supplied data dictionary
		string result = cpptempl::parse(text, data) ;
		string expected = "<h3>Locations</h3><ul>"
			"<li>Okinawa</li>"
			"<li>San Francisco</li>"
			"</ul>" ;
		BOOST_CHECK_EQUAL(result, expected) ;
	}
	BOOST_AUTO_TEST_CASE(test_example_if_else)
	{
		// The text template
		string text = "{% if foo %}yes{% else %}no{% endif %}";
		// Data to feed the template engine
		cpptempl::data_map data ;
		data["foo"] = true;
		// parse the template with the supplied data dictionary
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "yes" ) ;

		data["foo"] = false;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "no" ) ;
	}
	BOOST_AUTO_TEST_CASE(test_example_if_elif)
	{
		// The text template
		string text = "{% if foo %}aa{% elif bar %}bb{% endif %}";
		// Data to feed the template engine
		cpptempl::data_map data ;
		data["foo"] = true;
		data["bar"] = false;
		// parse the template with the supplied data dictionary
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "aa" ) ;

		data["foo"] = false;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "" ) ;

		data["bar"] = true;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "bb" ) ;
	}
	BOOST_AUTO_TEST_CASE(test_example_if_elif_else)
	{
		// The text template
		string text = "{% if foo %}aa{% elif bar %}bb{% else %}cc{% endif %}";
		// Data to feed the template engine
		cpptempl::data_map data ;
		data["foo"] = true;
		data["bar"] = false;
		// parse the template with the supplied data dictionary
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "aa" ) ;

		data["foo"] = false;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "cc" ) ;

		data["bar"] = true;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "bb" ) ;
	}
	BOOST_AUTO_TEST_CASE(test_example_if_elif_x2)
	{
		// The text template
		string text = "{% if foo %}aa{% elif bar %}bb{% elif baz %}cc{% endif %}";
		// Data to feed the template engine
		cpptempl::data_map data ;
		data["foo"] = false;
		data["bar"] = false;
		data["baz"] = false;
		// parse the template with the supplied data dictionary
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "" ) ;

		data["foo"] = true;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "aa" ) ;

		data["foo"] = false;
		data["baz"] = true;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "cc" ) ;

		data["bar"] = true;
		BOOST_CHECK_EQUAL( cpptempl::parse(text, data), "bb" ) ;
	}
BOOST_AUTO_TEST_SUITE_END()

// According to the docs this main() should be provided by the boost unit test lib,
// but it wasn't linking until I added it.
int main(int argc, char* argv[] )
{
    return boost::unit_test::unit_test_main(init_unit_test, argc, argv);
}
