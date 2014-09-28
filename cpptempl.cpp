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

namespace impl
{

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

	// get a data value from a data map
	// e.g. foo.bar => data["foo"]["bar"]
	data_ptr parse_val(std::string key, data_map &data) ;

    std::string strip_comment(const std::string & text);
    inline size_t count_newlines(const std::string & text);

}

	//////////////////////////////////////////////////////////////////////////
	// Data classes
	//////////////////////////////////////////////////////////////////////////

    // These ctors are defined here to resolve definition ordering issues with clang.
    data_ptr::data_ptr(DataBool* data) : ptr(data) {}
    data_ptr::data_ptr(DataValue* data) : ptr(data) {}
    data_ptr::data_ptr(DataList* data) : ptr(data) {}
    data_ptr::data_ptr(DataMap* data) : ptr(data) {}
    data_ptr::data_ptr(DataTemplate* data) : ptr(data) {}

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

	data_ptr& data_ptr::operator = (std::string&& data) {
		ptr.reset(new DataValue(data));
        return *this;
	}

	data_ptr& data_ptr::operator = (data_map&& data) {
		ptr.reset(new DataMap(data));
        return *this;
	}

	data_ptr& data_ptr::operator = (data_list&& data) {
		ptr.reset(new DataList(data));
        return *this;
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
	data_map& DataMap::getmap()
	{
		return m_items ;
	}
	bool DataMap::empty()
	{
		return m_items.empty();
	}

    // data template
    DataTemplate::DataTemplate(const std::string & templateText)
    {
        // Parse the template
		impl::token_vector tokens ;
		impl::tokenize(templateText, tokens) ;
		impl::parse_tree(tokens, m_tree) ;
    }

    std::string DataTemplate::getvalue()
	{
		return "";
	}

	bool DataTemplate::empty()
	{
		return false;
	}

    std::string DataTemplate::parse(data_map & data)
    {
        std::ostringstream stream;
		for (size_t i = 0 ; i < m_tree.size() ; ++i)
		{
			// Recursively calls gettext on each node in the tree.
			// gettext returns the appropriate text for that node.
			// for text, itself;
			// for variable, substitution;
			// for control statement, recursively gets kids
			m_tree[i]->gettext(stream, data) ;
		}
		return stream.str() ;
    }

    bool data_ptr::isTemplate() const
    {
        return (dynamic_cast<DataTemplate*>(ptr.get()) != nullptr);
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

    data_ptr make_template(const std::string & templateText)
    {
        return data_ptr(new DataTemplate(templateText));
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

namespace impl
{

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
        try
        {
            data_ptr result = data.parse_path(key);

            // evaluate function
            if (fn == "empty")
            {
                result = result->getlist().empty();
            }
            else if (fn == "count")
            {
                result = result->getlist().size();
            }
            else if (fn == "defined")
            {
                result = true;
            }
            // Template have to be handled specially since they need the data map.
            else if (fn.empty() && result.isTemplate())
            {
                std::shared_ptr<Data> tmplData = result.get();
                DataTemplate * tmpl = dynamic_cast<DataTemplate*>(tmplData.get());
                return make_data(tmpl->parse(data));
            }

            return result;
        }
        catch (data_map::key_error & e)
        {
            // Check if we got the error inside a defined() fn.
            if (fn == "defined")
            {
                return data_ptr(false);
            }

            return make_data("{$" + key + "}") ;
        }
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
            // Follow the key path.
            data_ptr& target = data.parse_path(m_name, true);

            // Set the map entry's value to a template. The tokens were already
            // parsed and set as our m_children vector.
            target = data_ptr(new DataTemplate(m_children));
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
                    uint32_t savedLine = currentLine;
                    pre_text = text.substr(1, pos-2) ;
                    currentLine += count_newlines(pre_text) ;
                    std::string expression = strip_comment(pre_text) ;

                    bool eolFollows = text.size()-1 > pos && text[pos+1] == '\n' ;

                    // Chop off following eol if this control statement is on a line by itself.
                    if (eolPrecedes && eolFollows)
                    {
                        ++pos ;
                        ++currentLine ;
                        lastWasEol = true ;
                    }

                    text = text.substr(pos+1) ;

                    // Create control statement tokens.
					if (boost::starts_with(expression, "for "))
					{
						tokens.push_back(token_ptr (new TokenFor(expression, savedLine))) ;
					}
					else if (boost::starts_with(expression, "if "))
					{
						tokens.push_back(token_ptr (new TokenIf(expression, savedLine))) ;
					}
					else if (boost::starts_with(expression, "def "))
					{
						tokens.push_back(token_ptr (new TokenDef(expression, savedLine))) ;
					}
					else if (boost::starts_with(expression, "end"))
					{
						tokens.push_back(token_ptr (new TokenEnd(boost::trim_copy(expression), savedLine))) ;
					}
                    else
                    {
                        throw TemplateException(savedLine, "Unrecognized control statement");
                    }
				}
			}
			// comment
			else if (text[0] == '#')
			{
				pos = text.find("}") ;
				if (pos != std::string::npos)
				{
                    pre_text = text.substr(1, pos-2) ;
                    currentLine += count_newlines(pre_text) ;

                    bool eolFollows = text.size()-1 > pos && text[pos+1] == '\n' ;

                    // Chop off following eol if this comment is on a line by itself.
                    if (eolPrecedes && eolFollows)
                    {
                        ++pos ;
                        ++currentLine ;
                        lastWasEol = true ;
                    }

                    text = text.substr(pos+1) ;
				}
            }
			else
			{
				tokens.push_back(token_ptr(new TokenText("{", currentLine))) ;
			}
		}
		return tokens ;
	}

} // namespace impl

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
		impl::token_vector tokens ;
		impl::tokenize(templ_text, tokens) ;
		impl::token_vector tree ;
		impl::parse_tree(tokens, tree) ;

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
