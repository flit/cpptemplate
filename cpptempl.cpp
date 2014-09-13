#ifdef _MSC_VER
#include "stdafx.h"
#endif

#include "cpptempl.h"

#include <sstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace cpptempl
{

    std::string strip_comment(const std::string & text);
    inline size_t count_newlines(const std::string & text);

	//////////////////////////////////////////////////////////////////////////
	// Data classes
	//////////////////////////////////////////////////////////////////////////

    data_ptr::data_ptr(DataBool* data) : ptr(data) {}
    data_ptr::data_ptr(DataValue* data) : ptr(data) {}
    data_ptr::data_ptr(DataList* data) : ptr(data) {}
    data_ptr::data_ptr(DataMap* data) : ptr(data) {}

	// data_map
	data_ptr& data_map::operator [](const std::string& key) {
		return data[key];
	}
	bool data_map::empty() {
		return data.empty();
	}
	bool data_map::has(const std::string& key) {
		return data.find(key) != data.end();
	}

	// data_ptr
	template<>
	void data_ptr::operator = (const data_ptr& data) {
		ptr = data.ptr;
	}

	template<>
	void data_ptr::operator = (const bool& data) {
		ptr.reset(new DataBool(data));
	}

	template<>
	void data_ptr::operator = (const std::string& data) {
		ptr.reset(new DataValue(data));
	}

	template<>
	void data_ptr::operator = (const data_map& data) {
		ptr.reset(new DataMap(data));
	}

	template<>
	void data_ptr::operator = (const data_list& data) {
		ptr.reset(new DataList(data));
	}

	template<>
	void data_ptr::operator = (const data_ptr&& data) {
		ptr = data.ptr;
	}

	template<>
	void data_ptr::operator = (const bool&& data) {
		ptr.reset(new DataBool(data));
	}

	template<>
	void data_ptr::operator = (const std::string&& data) {
		ptr.reset(new DataValue(data));
	}

	template<>
	void data_ptr::operator = (const data_map&& data) {
		ptr.reset(new DataMap(data));
	}

	template<>
	void data_ptr::operator = (const data_list&& data) {
		ptr.reset(new DataList(data));
	}

	void data_ptr::push_back(const data_ptr& data) {
        if (!ptr) {
            ptr.reset(new DataList(data_list()));
        }
        data_list& list = ptr->getlist();
        list.push_back(data);
	}

	// base data
    std::string Data::getvalue()
	{
		throw TemplateException("Data item is not a value") ;
	}

	data_list& Data::getlist()
	{
		throw TemplateException("Data item is not a list") ;
	}
	data_map& Data::getmap()
	{
		throw TemplateException("Data item is not a dictionary") ;
	}
    // data bool
    std::string DataBool::getvalue()
	{
		return m_value ? "true" : "false" ;
	}
	bool DataBool::empty()
	{
		return !m_value;
	}
	// data value
    std::string DataValue::getvalue()
	{
		return m_value ;
	}
	bool DataValue::empty()
	{
		return m_value.empty();
	}
	// data list
	data_list& DataList::getlist()
	{
		return m_items ;
	}

	bool DataList::empty()
	{
		return m_items.empty();
	}
	// data map
	data_map& DataMap:: getmap()
	{
		return m_items ;
	}
	bool DataMap::empty()
	{
		return m_items.empty();
	}
	//////////////////////////////////////////////////////////////////////////
	// parse_val
	//////////////////////////////////////////////////////////////////////////
	data_ptr& data_map::parse_path(const std::string & key, bool create)
	{
        if (key.empty())
        {
            throw key_error("empty map key");
        }

		// check for dotted notation, i.e [foo.bar]
		size_t index = key.find(".") ;
		if (index == std::string::npos)
		{
			if (!has(key))
			{
                if (!create)
                {
                    throw key_error("invalid map key");
                }
                data[key] = make_data("");
            }
			return data[key] ;
		}

        std::string sub_key = key.substr(0, index) ;
		if (!has(sub_key))
		{
			throw key_error("invalid map key");
		}

		return data[sub_key]->getmap().parse_path(key.substr(index+1), create) ;
	}

	data_ptr parse_val(std::string key, data_map &data)
	{
		// quoted string
		if (key[0] == '\"')
		{
			return make_data(boost::trim_copy_if(key, boost::is_any_of("\""))) ;
		}

        // handle functions
        std::string fn;
        size_t index = key.find("(");
        if (index != std::string::npos)
        {
            // Make sure there is a closing parenthesis at the end.
            if (key[key.size()-1] != ')')
            {
                throw TemplateException("Missing close parenthesis");
            }

            fn = key.substr(0, index);
            key = key.substr(index + 1, key.size() - index - 2);
        }

        // parse path and get resulting data object
        data_ptr result;
        try
        {
            result = data.parse_path(key);
        }
        catch (data_map::key_error & e)
        {
            return make_data("{$" + key + "}") ;
        }

        // evaluate function
        if (fn == "empty")
        {
            result = result->getlist().empty();
        }
        else if (fn == "count")
        {
            result = result->getlist().size();
        }

        return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// Token classes
	//////////////////////////////////////////////////////////////////////////

	// defaults, overridden by subclasses with children
	void Token::set_children( token_vector & )
	{
		throw TemplateException(get_line(), "This token type cannot have children") ;
	}

	token_vector & Token::get_children()
	{
		throw TemplateException(get_line(), "This token type cannot have children") ;
	}

	// TokenText
	TokenType TokenText::gettype()
	{
		return TOKEN_TYPE_TEXT ;
	}

	void TokenText::gettext( std::ostream &stream, data_map & )
	{
		stream << m_text ;
	}

	// TokenVar
	TokenType TokenVar::gettype()
	{
		return TOKEN_TYPE_VAR ;
	}

	void TokenVar::gettext( std::ostream &stream, data_map &data )
	{
        try
        {
            stream << parse_val(m_key, data)->getvalue() ;
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(get_line());
            throw e;
        }
	}

	// TokenVar
	void TokenParent::set_children( token_vector &children )
	{
		m_children.assign(children.begin(), children.end()) ;
	}

	token_vector & TokenParent::get_children()
	{
		return m_children;
	}

	// TokenFor
	TokenFor::TokenFor(std::string expr, uint32_t line)
    :   TokenParent(line)
	{
		std::vector<std::string> elements ;
		boost::split(elements, expr, boost::is_space()) ;
		if (elements.size() != 4u)
		{
			throw TemplateException(get_line(), "Invalid syntax in for statement") ;
		}
		m_val = elements[1] ;
		m_key = elements[3] ;
	}

	TokenType TokenFor::gettype()
	{
		return TOKEN_TYPE_FOR ;
	}

	void TokenFor::gettext( std::ostream &stream, data_map &data )
	{
        try
        {
            data_ptr value = parse_val(m_key, data) ;
            data_list &items = value->getlist() ;
            for (size_t i = 0 ; i < items.size() ; ++i)
            {
                data_map loop ;
                loop["index"] = make_data(i+1) ;
                loop["index0"] = make_data(i) ;
                loop["last"] = make_data(i == items.size() - 1);
                loop["count"] = make_data(items.size());
                data["loop"] = make_data(loop);
                data[m_val] = items[i] ;
                for(size_t j = 0 ; j < m_children.size() ; ++j)
                {
                    m_children[j]->gettext(stream, data) ;
                }
            }
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(get_line());
            throw e;
        }
	}

	// TokenIf
	TokenType TokenIf::gettype()
	{
		return TOKEN_TYPE_IF ;
	}

	void TokenIf::gettext( std::ostream &stream, data_map &data )
	{
		if (is_true(m_expr, data))
		{
			for(size_t j = 0 ; j < m_children.size() ; ++j)
			{
				m_children[j]->gettext(stream, data) ;
			}
		}
	}

	bool TokenIf::is_true( std::string expr, data_map &data )
	{
		std::vector<std::string> elements ;
		boost::split(elements, expr, boost::is_space()) ;

		if (elements[1] == "not")
		{
			return parse_val(elements[2], data)->empty() ;
		}
		if (elements.size() == 2)
		{
			return ! parse_val(elements[1], data)->empty() ;
		}
        std::string lhs;
        std::string rhs;
        try
        {
            lhs = parse_val(elements[1], data)->getvalue() ;
            rhs = parse_val(elements[3], data)->getvalue() ;
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(get_line());
            throw e;
        }
		if (elements[2] == "==")
		{
			return lhs == rhs ;
		}
        else if (elements[2] == "!=")
        {
            return lhs != rhs ;
        }
        else
        {
            throw TemplateException(get_line(), "Unknown/unsupported operator in if statement");
        }
	}

	// TokenDef
    TokenDef::TokenDef(std::string name, uint32_t line)
    :   TokenParent(line)
    {
		std::vector<std::string> elements ;
		boost::split(elements, name, boost::is_space()) ;
		if (elements.size() != 2u)
		{
			throw TemplateException(get_line(), "Invalid syntax in def statement") ;
		}
		m_name = elements[1] ;
    }

	TokenType TokenDef::gettype()
	{
		return TOKEN_TYPE_DEF ;
	}

	void TokenDef::gettext( std::ostream &stream, data_map &data )
	{
        try
        {
            data_ptr& target = data.parse_path(m_name, true);

            std::ostringstream buf;
            for(size_t j = 0 ; j < m_children.size() ; ++j)
            {
                m_children[j]->gettext(buf, data) ;
            }

            target = buf.str();
        }
        catch (data_map::key_error &)
        {
            // ignore exception
        }
	}

	// TokenEnd
    TokenEnd::TokenEnd(std::string text, uint32_t line)
    :   Token(line)
    {
		if (boost::starts_with(text, "endfor"))
        {
            m_type = TOKEN_TYPE_ENDFOR;
        }
        else if (boost::starts_with(text, "endif"))
        {
            m_type = TOKEN_TYPE_ENDIF;
        }
        else if (boost::starts_with(text, "enddef"))
        {
            m_type = TOKEN_TYPE_ENDDEF;
        }
        else
        {
            throw TemplateException(get_line(), "Unknown end of control statement");
        }
	}

	void TokenEnd::gettext( std::ostream &, data_map &)
	{
		throw TemplateException(get_line(), "End-of-control statements have no associated text") ;
	}

    TemplateException::TemplateException(size_t line, std::string reason)
    :   std::exception(),
        m_line(0),
        m_reason()
    {
        set_line_if_missing(line);
    }

    void TemplateException::set_line_if_missing(size_t line)
    {
        if (!m_line)
        {
            m_line = line;
            m_reason = std::string("Line ") + boost::lexical_cast<std::string>(line) + ": " + m_reason;
        }
    }

	// gettext
	// generic helper for getting text from tokens.

    std::string gettext(token_ptr token, data_map &data)
	{
		std::ostringstream stream ;
		token->gettext(stream, data) ;
		return stream.str() ;
	}
	//////////////////////////////////////////////////////////////////////////
	// parse_tree
	// recursively parses list of tokens into a tree
	//////////////////////////////////////////////////////////////////////////
	void parse_tree(token_vector &tokens, token_vector &tree, TokenType until)
	{
		while(! tokens.empty())
		{
			// 'pops' first item off list
			token_ptr token = tokens[0] ;
			tokens.erase(tokens.begin()) ;

			if (token->gettype() == TOKEN_TYPE_FOR)
			{
				token_vector children ;
				parse_tree(tokens, children, TOKEN_TYPE_ENDFOR) ;
				token->set_children(children) ;
			}
			else if (token->gettype() == TOKEN_TYPE_IF)
			{
				token_vector children ;
				parse_tree(tokens, children, TOKEN_TYPE_ENDIF) ;
				token->set_children(children) ;
			}
			else if (token->gettype() == TOKEN_TYPE_DEF)
			{
				token_vector children ;
				parse_tree(tokens, children, TOKEN_TYPE_ENDDEF) ;
				token->set_children(children) ;
			}
			else if (token->gettype() == until)
			{
				return ;
			}
			tree.push_back(token) ;
		}
	}

    std::string strip_comment(const std::string & text)
    {
        size_t pos = text.find("--");
        if (pos == std::string::npos)
        {
            return boost::trim_copy(text);
        }

        return boost::trim_copy(text.substr(0, pos));
    }

    inline size_t count_newlines(const std::string & text)
    {
        return std::count(text.begin(), text.end(), '\n');
    }

	//////////////////////////////////////////////////////////////////////////
	// tokenize
	// parses a template into tokens (text, for, if, variable)
	//////////////////////////////////////////////////////////////////////////
	token_vector & tokenize(std::string text, token_vector &tokens)
	{
        bool eolPrecedes = true;
        bool lastWasEol = true;
        uint32_t currentLine = 1;
		while(! text.empty())
		{
			size_t pos = text.find("{") ;
			if (pos == std::string::npos)
			{
				if (! text.empty())
				{
					tokens.push_back(token_ptr(new TokenText(text, currentLine))) ;
				}
				return tokens ;
			}
            std::string pre_text = text.substr(0, pos) ;
            currentLine += count_newlines(pre_text) ;
			if (! pre_text.empty())
			{
				tokens.push_back(token_ptr(new TokenText(pre_text, currentLine))) ;
			}

            // Track whether there was an EOL prior to this open brace.
            bool newLastWasEol = pos > 0 && text[pos-1] == '\n';
            eolPrecedes = (pos == 0 && lastWasEol) || newLastWasEol;
            if (pos > 0)
            {
                lastWasEol = newLastWasEol;
            }

			text = text.substr(pos+1) ;
			if (text.empty())
			{
				tokens.push_back(token_ptr(new TokenText("{", currentLine))) ;
				return tokens ;
			}

			// variable
			if (text[0] == '$')
			{
				pos = text.find("}") ;
				if (pos != std::string::npos)
				{
                    pre_text = boost::trim_copy(text.substr(1, pos-1));
					tokens.push_back(token_ptr (new TokenVar(pre_text, currentLine))) ;
					text = text.substr(pos+1) ;
				}

                lastWasEol = false ;
			}
			// control statement
			else if (text[0] == '%')
			{
				pos = text.find("}") ;
				if (pos != std::string::npos)
				{
                    pre_text = text.substr(1, pos-2) ;
                    currentLine += count_newlines(pre_text) ;
                    std::string expression = strip_comment(pre_text) ;

                    bool eolFollows = text.size()-1 > pos && text[pos+1] == '\n' ;

                    // Chop off following eol if this control statement is on a line by itself.
                    if (eolPrecedes && eolFollows)
                    {
                        text = text.substr(pos+2) ;
                        lastWasEol = true ;
                    }
                    else
                    {
                        text = text.substr(pos+1) ;
                    }

                    // Create control statement tokens.
					if (boost::starts_with(expression, "for "))
					{
						tokens.push_back(token_ptr (new TokenFor(expression, currentLine))) ;
					}
					else if (boost::starts_with(expression, "if "))
					{
						tokens.push_back(token_ptr (new TokenIf(expression, currentLine))) ;
					}
					else if (boost::starts_with(expression, "def "))
					{
						tokens.push_back(token_ptr (new TokenDef(expression, currentLine))) ;
					}
					else if (boost::starts_with(expression, "end"))
					{
						tokens.push_back(token_ptr (new TokenEnd(boost::trim_copy(expression), currentLine))) ;
					}
                    else
                    {
                        throw TemplateException(currentLine, "Unrecognized control statement");
                    }

                    // Increment current line to account for the newline we just chopped off.
                    if (eolPrecedes && eolFollows)
                    {
                        ++currentLine;
                    }
				}
                else
                {
                    tokens.push_back(token_ptr(new TokenText("{", currentLine))) ;
                }
			}
			else
			{
				tokens.push_back(token_ptr(new TokenText("{", currentLine))) ;
			}
		}
		return tokens ;
	}

	/************************************************************************
	* parse
	*
	*  1. tokenizes template
	*  2. parses tokens into tree
	*  3. resolves template
	*  4. returns converted text
	************************************************************************/
    std::string parse(std::string templ_text, data_map &data)
	{
		std::ostringstream stream ;
		parse(stream, templ_text, data) ;
		return stream.str() ;
	}
	void parse(std::ostream &stream, std::string templ_text, data_map &data)
	{
		token_vector tokens ;
		tokenize(templ_text, tokens) ;
		token_vector tree ;
		parse_tree(tokens, tree) ;

		for (size_t i = 0 ; i < tree.size() ; ++i)
		{
			// Recursively calls gettext on each node in the tree.
			// gettext returns the appropriate text for that node.
			// for text, itself;
			// for variable, substitution;
			// for control statement, recursively gets kids
			tree[i]->gettext(stream, data) ;
		}
	}
}
