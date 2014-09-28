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
    // Types of tokens in control statements.
    enum stmt_token_type_t
    {
        INVALID_TOKEN,
        KEY_PATH_TOKEN,
        FOR_TOKEN,
        IN_TOKEN,
        IF_TOKEN,
        DEF_TOKEN,
        ENDFOR_TOKEN,
        ENDIF_TOKEN,
        ENDDEF_TOKEN,
        NOT_TOKEN,
        EQ_TOKEN,
        NEQ_TOKEN,
        AND_TOKEN,
        OR_TOKEN,
        STRING_LITERAL_TOKEN,
        OPEN_PAREN_TOKEN = '(',
        CLOSE_PAREN_TOKEN = ')',
        COMMA_TOKEN = ',',
        END_TOKEN = 255
    };

    // Represents a token in a control statement.
    class StatementToken
    {
        stmt_token_type_t m_type;
        std::string m_value;
    public:
        StatementToken(stmt_token_type_t tokenType) : m_type(tokenType), m_value() {}
        StatementToken(stmt_token_type_t tokenType, const std::string & value) : m_type(tokenType), m_value(value) {}
        StatementToken(const StatementToken & other) : m_type(other.m_type), m_value(other.m_value) {}
        StatementToken(StatementToken && other) : m_type(other.m_type), m_value(std::move(other.m_value)) {}
        StatementToken& operator=(const StatementToken & other)=default;
        StatementToken& operator=(StatementToken && other)
        {
            m_type = other.m_type;
            m_value.assign(std::move(other.m_value));
            return *this;
        }
        ~StatementToken()=default;

        stmt_token_type_t get_type() const { return m_type; }
        const std::string & get_value() const { return m_value; }
    };

    typedef std::vector<StatementToken> stmt_token_vector_t;

    // Helper class for parsing tokens.
    class StmtTokenIterator
    {
        stmt_token_vector_t & m_tokens;
        size_t m_index;
        static StatementToken s_endToken;
    public:
        StmtTokenIterator(stmt_token_vector_t & seq) : m_tokens(seq), m_index(0) {}
        StmtTokenIterator(const StmtTokenIterator & other) : m_tokens(other.m_tokens), m_index(other.m_index) {}

        size_t size() const { return m_tokens.size(); }
        bool empty() const { return size() == 0; }
        bool is_valid() const { return m_index < size(); }

        StatementToken * get();
        StatementToken * next();
        StatementToken * match(stmt_token_type_t tokenType, const char * failure_msg=nullptr);

        StmtTokenIterator& operator ++ ();

        StatementToken * operator -> () { return get(); }
        StatementToken & operator * () { return *get(); }
    };

    class ExprParser
    {
        StmtTokenIterator & m_tok;
        data_map & m_data;
    public:
        ExprParser(StmtTokenIterator & seq, data_map & data) : m_tok(seq), m_data(data) {}

        data_ptr parse_expr();
        data_ptr parse_bterm();
        data_ptr parse_bfactor();
        data_ptr parse_factor();
        data_ptr parse_var();
        data_ptr get_var_value(const std::string & path, std::vector<data_ptr> & params);
    };

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
        stmt_token_vector_t m_expr;
	public:
		TokenVar(stmt_token_vector_t & expr, uint32_t line=0) : Token(line), m_expr(expr) {}
		TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// for block
	class TokenFor : public TokenParent
	{
        std::string m_key ;
        std::string m_val ;
	public:
		TokenFor(stmt_token_vector_t & tokens, uint32_t line=0);
		TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// if block
	class TokenIf : public TokenParent
	{
        stmt_token_vector_t m_expr ;
	public:
		TokenIf(stmt_token_vector_t & expr, uint32_t line=0) : TokenParent(line), m_expr(expr){}
		TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
		bool is_true(data_map &data);
	};

    // def block
    class TokenDef : public TokenParent
    {
        std::string m_name;
        string_vector_t m_params;
	public:
        TokenDef(stmt_token_vector_t & expr, uint32_t line=0);
        TokenType gettype();
		void gettext(std::ostream &stream, data_map &data);
    };

	// end of block
	class TokenEnd : public Token // end of control block
	{
        TokenType m_type ;
	public:
		TokenEnd(TokenType endType, uint32_t line=0) : Token(line), m_type(endType) {}
		TokenType gettype() { return m_type; }
		void gettext(std::ostream &stream, data_map &data);
	};

    // Lexer states for statement tokenizer.
    enum param_lexer_state_t
    {
        INIT_STATE,             //!< Initial state, skip whitespace, process single char tokens.
        KEY_PATH_STATE,         //!< Scan a key path.
        STRING_LITERAL_STATE,   //!< Scan a string literal.
        OPERATOR_STATE,         //!< Scan an operator.
    };

    inline bool is_key_path_char(char c);
    inline stmt_token_type_t get_keyword_token(const std::string & s);
    void create_id_token(stmt_token_vector_t & tokens, const std::string & s);
    stmt_token_vector_t tokenize_statement(const std::string & text);

    std::string strip_comment(const std::string & text);
    inline size_t count_newlines(const std::string & text);

	void parse_tree(token_vector &tokens, token_vector &tree, TokenType until=TOKEN_TYPE_NONE) ;
	token_vector & tokenize(std::string text, token_vector &tokens) ;

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

    bool data_ptr::is_template() const
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
	// parse_path
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
                    printf("invalid map key: %s\n", key.c_str());
                    throw key_error("invalid map key");
                }
                data[key] = make_data("");
            }
			return data[key] ;
		}

        std::string sub_key = key.substr(0, index) ;
		if (!has(sub_key))
		{
            printf("invalid map key: %s\n", sub_key.c_str());
			throw key_error("invalid map key");
		}

		return data[sub_key]->getmap().parse_path(key.substr(index+1), create) ;
	}

namespace impl
{

    StatementToken StmtTokenIterator::s_endToken(END_TOKEN);

    StatementToken * StmtTokenIterator::get()
    {
        if (is_valid())
        {
            return &m_tokens[m_index];
        }
        else
        {
            return &s_endToken;
        }
    }

    StatementToken * StmtTokenIterator::next()
    {
        if (m_index < size())
        {
            ++m_index;
        }
        return get();
    }

    StatementToken * StmtTokenIterator::match(stmt_token_type_t tokenType, const char * failure_msg)
    {
        StatementToken * result = get();
        if (result->get_type() != tokenType)
        {
            throw TemplateException(failure_msg ? failure_msg : "unexpected token");
        }
        next();
        return result;
    }

    StmtTokenIterator& StmtTokenIterator::operator ++ ()
    {
        if (m_index < size())
        {
            ++m_index;
        }
        return *this;
    }

    inline bool is_key_path_char(char c)
    {
        return (isalnum(c) || c == '.' || c == '_');
    }

    inline stmt_token_type_t get_keyword_token(const std::string & s)
    {
        if (s == "for")
        {
            return FOR_TOKEN;
        }
        else if (s == "in")
        {
            return IN_TOKEN;
        }
        else if (s == "if")
        {
            return IF_TOKEN;
        }
        else if (s == "def")
        {
            return DEF_TOKEN;
        }
        else if (s == "endfor")
        {
            return ENDFOR_TOKEN;
        }
        else if (s == "endif")
        {
            return ENDIF_TOKEN;
        }
        else if (s == "enddef")
        {
            return ENDDEF_TOKEN;
        }
        else if (s == "not")
        {
            return NOT_TOKEN;
        }
        else if (s == "and")
        {
            return AND_TOKEN;
        }
        else if (s == "or")
        {
            return OR_TOKEN;
        }
        else
        {
            return INVALID_TOKEN;
        }
    }

    void create_id_token(stmt_token_vector_t & tokens, const std::string & s)
    {
        stmt_token_type_t t = get_keyword_token(s);
        if (t == INVALID_TOKEN)
        {
            // Create key path token.
            tokens.emplace_back(KEY_PATH_TOKEN, s);
        }
        else
        {
            tokens.emplace_back(t);
        }
    }

    stmt_token_vector_t tokenize_statement(const std::string & text)
    {
        stmt_token_vector_t tokens;
        param_lexer_state_t state=INIT_STATE;
        int i=0;
        uint32_t pos=0;
        char str_open_quote=0;
        char first_op_char=0;

        for (; i < text.size(); ++i)
        {
            char c = text[i];

            switch (state)
            {
                case INIT_STATE:
                    // Skip any whitespace
                    if (isspace(c))
                    {
                        continue;
                    }
                    else if (is_key_path_char(c))
                    {
                        pos = i;
                        state = KEY_PATH_STATE;
                    }
                    else if (c == '(' || c == ')' || c == ',')
                    {
                        tokens.emplace_back(static_cast<stmt_token_type_t>(c));
                    }
                    else if (c == '\"' || c == '\'')
                    {
                        // Set the start pos to after the open quote.
                        pos = i + 1;
                        str_open_quote = c;
                        state = STRING_LITERAL_STATE;
                    }
                    else if (c == '=' || c == '!')
                    {
                        first_op_char = c;
                        state = OPERATOR_STATE;
                    }
                    else
                    {
                        throw TemplateException("unexpected character");
                    }
                    break;

                case KEY_PATH_STATE:
                    if (is_key_path_char(c))
                    {
                        continue;
                    }
                    else
                    {
                        create_id_token(tokens, text.substr(pos, i-pos));

                        // Reprocess this char in the initial state.
                        state = INIT_STATE;
                        --i;
                    }
                    break;

                case STRING_LITERAL_STATE:
                    if (c == str_open_quote)
                    {
                        // Create the string literal token and return to init state.
                        std::string x = text.substr(pos, i-pos);
                        tokens.emplace_back(STRING_LITERAL_TOKEN, x);
                        state = INIT_STATE;
                    }
                    else if (c == '\\')
                    {
                        // TODO: handle string literal escapes
                    }
                    break;

                case OPERATOR_STATE:
                    switch (first_op_char)
                    {
                        case '=':
                            switch (c)
                            {
                                case '=':
                                    tokens.emplace_back(EQ_TOKEN);
                                    break;
                                default:
                                    // Reprocess this char in the initial state.
                                    state = INIT_STATE;
                                    --i;
                                    break;
                            }
                            break;
                        case '!':
                            switch (c)
                            {
                                case '=':
                                    tokens.emplace_back(NEQ_TOKEN);
                                    break;
                                default:
                                    // Reprocess this char in the initial state.
                                    state = INIT_STATE;
                                    --i;
                                    break;
                            }
                            break;
                        default:
                            throw TemplateException("unexpected first op char");
                    }
                    break;
            }
        }

        // Handle a key path terminated by end of string.
        if (state == KEY_PATH_STATE)
        {
            create_id_token(tokens, text.substr(pos, i-pos));
        }
        else if (state == STRING_LITERAL_STATE)
        {
            throw TemplateException("unterminated string literal");
        }

        return tokens;
    }

    void print_tokens(const stmt_token_vector_t & tokens)
    {
        printf("tokens:\n");
        for (auto it : tokens)
        {
            printf("%d : %s\n", it.get_type(), it.get_value().c_str());
        }
        printf("--\n");
    }

    // Expression grammar
    //
    // expr         ::= bterm ( "or" bterm )*
    // bterm        ::= bfactor ( "and" bfactor )*
    // bfactor      ::= factor [ ( "==" | "!=" ) factor ]
    // factor       ::= "not" expr
    //              |   "(" expr ")"
    //              |   ( "true" | "false" )
    //              |   var
    // var          ::= KEY_PATH [ args ]
    // args         ::= "(" [ expr ( "," expr )* ")"

    data_ptr ExprParser::get_var_value(const std::string & path, std::vector<data_ptr> & params)
    {
        // Check if this is a pseudo function.
        bool is_fn = false;
        if (path == "count" || path == "empty" || path == "defined")
        {
            is_fn = true;
        }

        try
        {
            data_ptr result;
            if (is_fn)
            {
                if (params.size() != 1)
                {
                    throw TemplateException("function " + path + " requires 1 parameter");
                }

                if (path == "count")
                {
                    result = params[0]->getlist().size();
                }
                else if (path == "empty")
                {
                    result = params[0]->getlist().empty();
                }
                else if (path == "defined")
                {
                    // TODO: handle undefined case for defined fn
                    result = true;
                }
            }
            else
            {
                result = m_data.parse_path(path);

                // Handle subtemplates.
                if (result.is_template())
                {
                    std::shared_ptr<Data> tmplData = result.get();
                    DataTemplate * tmpl = dynamic_cast<DataTemplate*>(tmplData.get());
                    result = make_data(tmpl->parse(m_data));
                }
            }

            return result;
        }
        catch (data_map::key_error & e)
        {
            return make_data("{$" + path + "}");
        }
    }

    data_ptr ExprParser::parse_var()
    {
        StatementToken * path = m_tok.match(KEY_PATH_TOKEN);

        std::vector<data_ptr> params;
        if (m_tok.get()->get_type() == OPEN_PAREN_TOKEN)
        {
            m_tok.match(OPEN_PAREN_TOKEN);

            while (true)
            {
                params.push_back(parse_expr());

                if (m_tok.get()->get_type() == CLOSE_PAREN_TOKEN)
                {
                    m_tok.match(CLOSE_PAREN_TOKEN);
                    break;
                }
                else
                {
                    m_tok.match(COMMA_TOKEN);
                }
            }
        }

        return get_var_value(path->get_value(), params);
    }

    data_ptr ExprParser::parse_factor()
    {
        stmt_token_type_t tokType = m_tok->get_type();
        data_ptr result;
        if (tokType == NOT_TOKEN)
        {
            m_tok.next();
            result = !parse_expr()->empty();
        }
        else if (tokType == OPEN_PAREN_TOKEN)
        {
            m_tok.next();
            result = parse_expr();
            m_tok.match(CLOSE_PAREN_TOKEN);
        }
        else if (tokType == KEY_PATH_TOKEN)
        {
            result = parse_var();
        }
        else if (tokType == STRING_LITERAL_TOKEN)
        {
            result = m_tok.match(STRING_LITERAL_TOKEN)->get_value();
        }
        return result;
    }

    data_ptr ExprParser::parse_bfactor()
    {
        data_ptr ldata = parse_factor();

        stmt_token_type_t tokType = m_tok->get_type();
        if (tokType == EQ_TOKEN || tokType == NEQ_TOKEN)
        {
            m_tok.next();

            data_ptr rdata = parse_factor();

            std::string lhs = ldata->getvalue();
            std::string rhs = rdata->getvalue();
            switch (tokType)
            {
                case EQ_TOKEN:
                    ldata = (lhs == rhs);
                    break;
                case NEQ_TOKEN:
                    ldata = (lhs != rhs);
                    break;
                default:
                    break;
            }
        }
        return ldata;
    }

    data_ptr ExprParser::parse_bterm()
    {
        data_ptr ldata = parse_bfactor();

        while (m_tok->get_type() == AND_TOKEN)
        {
            m_tok.match(AND_TOKEN);

            data_ptr rdata = parse_bfactor();

            ldata = (!ldata->empty() && !rdata->empty());
        }
        return ldata;
    }

    data_ptr ExprParser::parse_expr()
    {
        data_ptr ldata = parse_bterm();

        while (m_tok->get_type() == OR_TOKEN)
        {
            m_tok.match(OR_TOKEN);

            data_ptr rdata = parse_bterm();

            ldata = (!ldata->empty() || !rdata->empty());
        }

        return ldata;
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
            StmtTokenIterator it(m_expr);
            ExprParser expr(it, data);
            data_ptr result = expr.parse_expr();
            stream << result->getvalue() ;
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
	TokenFor::TokenFor(stmt_token_vector_t & tokens, uint32_t line)
    :   TokenParent(line)
	{
        try
        {
            StmtTokenIterator tok(tokens);
            tok.match(FOR_TOKEN);
            m_val = tok.match(KEY_PATH_TOKEN)->get_value();
            tok.match(IN_TOKEN);
            m_key = tok.match(KEY_PATH_TOKEN)->get_value();
            tok.match(END_TOKEN);
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(get_line());
            throw e;
        }
	}

	TokenType TokenFor::gettype()
	{
		return TOKEN_TYPE_FOR ;
	}

	void TokenFor::gettext( std::ostream &stream, data_map &data )
	{
        try
        {
            data_ptr value = data.parse_path(m_key);
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
		if (is_true(data))
		{
			for(size_t j = 0 ; j < m_children.size() ; ++j)
			{
				m_children[j]->gettext(stream, data) ;
			}
		}
	}

	bool TokenIf::is_true( data_map &data )
	{
        try
        {
            StmtTokenIterator it(m_expr);
            it.match(IF_TOKEN);
            ExprParser parser(it, data);
            data_ptr d = parser.parse_expr();
            it.match(END_TOKEN);

            return !d->empty();
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(get_line());
            throw e;
        }
	}

	// TokenDef
    TokenDef::TokenDef(stmt_token_vector_t & expr, uint32_t line)
    :   TokenParent(line)
    {
        try
        {
            StmtTokenIterator tok(expr);
            tok.match(DEF_TOKEN);

            m_name = tok.match(KEY_PATH_TOKEN)->get_value();

            if (tok.get()->get_type() == OPEN_PAREN_TOKEN)
            {
                tok.match(OPEN_PAREN_TOKEN);

                while (tok.get()->get_type() != CLOSE_PAREN_TOKEN)
                {
                    m_params.push_back(tok.match(KEY_PATH_TOKEN)->get_value());

                    if (tok.get()->get_type() != CLOSE_PAREN_TOKEN)
                    {
                        tok.match(COMMA_TOKEN);
                    }
                }
                tok.match(CLOSE_PAREN_TOKEN);
            }
            tok.match(END_TOKEN);
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(get_line());
            throw e;
        }
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

	void TokenEnd::gettext( std::ostream &, data_map &)
	{
		throw TemplateException(get_line(), "End-of-control statements have no associated text") ;
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
                    pre_text = text.substr(1, pos-1);

                    stmt_token_vector_t stmt_tokens = tokenize_statement(pre_text);
//                    if (stmt_tokens.empty() || stmt_tokens[0].get_type() != KEY_PATH_TOKEN)
//                    {
//                        throw TemplateException(currentLine, "expected variable name");
//                    }

					tokens.push_back(token_ptr (new TokenVar(stmt_tokens, currentLine))) ;

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

                    stmt_token_vector_t stmt_tokens = tokenize_statement(expression);

                    // Create control statement tokens.
                    const StatementToken & keyword = stmt_tokens[0];
					if (keyword.get_type() == FOR_TOKEN)
                    {
						tokens.push_back(token_ptr (new TokenFor(stmt_tokens, savedLine))) ;
					}
					else if (keyword.get_type() == IF_TOKEN)
					{
						tokens.push_back(token_ptr (new TokenIf(stmt_tokens, savedLine))) ;
					}
					else if (keyword.get_type() == DEF_TOKEN)
					{
						tokens.push_back(token_ptr (new TokenDef(stmt_tokens, savedLine))) ;
					}
					else if (keyword.get_type() == ENDFOR_TOKEN)
					{
						tokens.push_back(token_ptr (new TokenEnd(TOKEN_TYPE_ENDFOR, savedLine))) ;
					}
					else if (keyword.get_type() == ENDIF_TOKEN)
					{
						tokens.push_back(token_ptr (new TokenEnd(TOKEN_TYPE_ENDIF, savedLine))) ;
					}
					else if (keyword.get_type() == ENDDEF_TOKEN)
					{
						tokens.push_back(token_ptr (new TokenEnd(TOKEN_TYPE_ENDDEF, savedLine))) ;
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
