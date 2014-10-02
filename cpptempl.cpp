#ifdef _MSC_VER
#include "stdafx.h"
#endif

#include "cpptempl.h"

#include <sstream>
#include <algorithm>
#include <stack>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace cpptempl
{

namespace impl
{
    // Types of tokens in control statements.
    enum TokenType
    {
        INVALID_TOKEN,
        KEY_PATH_TOKEN,
        STRING_LITERAL_TOKEN,
        TRUE_TOKEN,
        FALSE_TOKEN,
        FOR_TOKEN,
        IN_TOKEN,
        IF_TOKEN,
        ELIF_TOKEN,
        ELSE_TOKEN,
        DEF_TOKEN,
        ENDFOR_TOKEN,
        ENDIF_TOKEN,
        ENDDEF_TOKEN,
        AND_TOKEN,
        OR_TOKEN,
        NOT_TOKEN,
        EQ_TOKEN,
        NEQ_TOKEN,
        OPEN_PAREN_TOKEN = '(',
        CLOSE_PAREN_TOKEN = ')',
        COMMA_TOKEN = ',',
        END_TOKEN = 255
    };

    // Represents a token in a control statement.
    class Token
    {
        TokenType m_type;
        std::string m_value;
    public:
        Token(TokenType tokenType) : m_type(tokenType), m_value() {}
        Token(TokenType tokenType, const std::string & value) : m_type(tokenType), m_value(value) {}
        Token(const Token & other) : m_type(other.m_type), m_value(other.m_value) {}
        Token(Token && other) : m_type(other.m_type), m_value(std::move(other.m_value)) {}
        Token& operator=(const Token & other)=default;
        Token& operator=(Token && other)
        {
            m_type = other.m_type;
            m_value.assign(std::move(other.m_value));
            return *this;
        }
        ~Token()=default;

        TokenType get_type() const { return m_type; }
        const std::string & get_value() const { return m_value; }
    };

    typedef std::vector<Token> token_vector;

    // Helper class for parsing tokens.
    class TokenIterator
    {
        token_vector & m_tokens;
        size_t m_index;
        static Token s_endToken;
    public:
        TokenIterator(token_vector & seq) : m_tokens(seq), m_index(0) {}
        TokenIterator(const TokenIterator & other) : m_tokens(other.m_tokens), m_index(other.m_index) {}

        size_t size() const { return m_tokens.size(); }
        bool empty() const { return size() == 0; }
        bool is_valid() const { return m_index < size(); }

        Token * get();
        Token * next();
        Token * match(TokenType tokenType, const char * failure_msg=nullptr);

        TokenIterator& operator ++ ();

        Token * operator -> () { return get(); }
        Token & operator * () { return *get(); }
    };

    class ExprParser
    {
        TokenIterator & m_tok;
        data_map & m_data;
    public:
        ExprParser(TokenIterator & seq, data_map & data) : m_tok(seq), m_data(data) {}

        data_ptr parse_expr();
        data_ptr parse_bterm();
        data_ptr parse_bfactor();
        data_ptr parse_factor();
        data_ptr parse_var();
        data_ptr get_var_value(const std::string & path, data_list & params);
    };

	typedef enum
	{
		NODE_TYPE_NONE,
		NODE_TYPE_TEXT,
		NODE_TYPE_VAR,
		NODE_TYPE_IF,
        NODE_TYPE_ELIF,
        NODE_TYPE_ELSE,
		NODE_TYPE_FOR,
        NODE_TYPE_DEF,
		NODE_TYPE_ENDIF,
		NODE_TYPE_ENDFOR,
        NODE_TYPE_ENDDEF
	} NodeType;

	// Template nodes
	// base class for all node types
	class Node
	{
        uint32_t m_line;
	public:
        Node(uint32_t line) : m_line(line) {}
		virtual NodeType gettype() = 0 ;
		virtual void gettext(std::ostream &stream, data_map &data) = 0 ;
		virtual void set_children(node_vector &children);
		virtual node_vector & get_children();
        uint32_t get_line() { return m_line; }
        void set_line(uint32_t line) { m_line = line; }
	};

	// node with children
	class NodeParent : public Node
	{
    protected:
		node_vector m_children ;
	public:
        NodeParent(uint32_t line) : Node(line) {}
		void set_children(node_vector &children);
		node_vector &get_children();
	};

	// normal text
	class NodeText : public Node
	{
        std::string m_text ;
	public:
		NodeText(std::string text, uint32_t line=0) : Node(line), m_text(text){}
		NodeType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// variable
	class NodeVar : public Node
	{
        token_vector m_expr;
	public:
		NodeVar(token_vector & expr, uint32_t line=0) : Node(line), m_expr(expr) {}
		NodeType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// for block
	class NodeFor : public NodeParent
	{
        std::string m_key ;
        std::string m_val ;
	public:
		NodeFor(token_vector & tokens, uint32_t line=0);
		NodeType gettype();
		void gettext(std::ostream &stream, data_map &data);
	};

	// if block
	class NodeIf : public NodeParent
	{
        token_vector m_expr ;
        node_ptr m_else_if;
        NodeType m_if_type;
	public:
		NodeIf(token_vector & expr, uint32_t line=0);
		NodeType gettype();
        void set_else_if(node_ptr else_if);
		void gettext(std::ostream &stream, data_map &data);
		bool is_true(data_map &data);
        bool is_else();
	};

    // def block
    class NodeDef : public NodeParent
    {
        std::string m_name;
        string_vector m_params;
	public:
        NodeDef(token_vector & expr, uint32_t line=0);
        NodeType gettype();
		void gettext(std::ostream &stream, data_map &data);
    };

	// end of block
	class NodeEnd : public Node // end of control block
	{
        NodeType m_type ;
	public:
		NodeEnd(NodeType endType, uint32_t line=0) : Node(line), m_type(endType) {}
		NodeType gettype() { return m_type; }
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
    TokenType get_keyword_token(const std::string & s);
    void create_id_token(token_vector & tokens, const std::string & s);
    token_vector tokenize_statement(const std::string & text);
    std::string strip_comment(const std::string & text);
    inline size_t count_newlines(const std::string & text);
	node_vector & tokenize(std::string text, node_vector &nodes) ;

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
        if (!has(key) && parent)
        {
            return (*parent)[key];
        }
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
		impl::tokenize(templateText, m_tree);
    }

    std::string DataTemplate::getvalue()
	{
		return "";
	}

	bool DataTemplate::empty()
	{
		return false;
	}

    std::string DataTemplate::eval(data_map & data, data_list * param_values)
    {
        std::ostringstream stream;
        eval(stream, data, param_values);
		return stream.str() ;
    }

    void DataTemplate::eval(std::ostream &stream, data_map & data, data_list * param_values)
    {
        data_map * use_data = &data;

        // Build map of param names to provided param values. The params map's
        // parent is set to the main data_map. This will cause params to
        // override keys in the main map, without modifying the main map.
        data_map params_map;
        if (param_values)
        {
            // Check number of params.
            if (param_values->size() > m_params.size())
            {
                throw TemplateException("too many parameter(s) provided to subtemplate");
            }

            size_t i;
            for (i=0; i < std::min(m_params.size(), param_values->size()); ++i)
            {
                const std::string & key = m_params[i];
                data_ptr & value = (*param_values)[i];
                params_map[key] = value;
            }

            params_map.set_parent(&data);
            use_data = &params_map;
        }

        // Recursively calls gettext on each node in the tree.
        // gettext returns the appropriate text for that node.
		for (size_t i = 0 ; i < m_tree.size() ; ++i)
		{
			m_tree[i]->gettext(stream, *use_data) ;
		}
    }

    bool data_ptr::is_template() const
    {
        return (dynamic_cast<DataTemplate*>(ptr.get()) != nullptr);
    }

    TemplateException::TemplateException(size_t line, std::string reason)
    :   std::exception(),
        m_line(0),
        m_reason(reason)
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

    Token TokenIterator::s_endToken(END_TOKEN);

    Token * TokenIterator::get()
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

    Token * TokenIterator::next()
    {
        if (m_index < size())
        {
            ++m_index;
        }
        return get();
    }

    Token * TokenIterator::match(TokenType tokenType, const char * failure_msg)
    {
        Token * result = get();
        if (result->get_type() != tokenType)
        {
            throw TemplateException(failure_msg ? failure_msg : "unexpected token");
        }
        next();
        return result;
    }

    TokenIterator& TokenIterator::operator ++ ()
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

    struct KeywordDef
    {
        TokenType tok;
        const char * keyword;
    };

    const KeywordDef k_keywords[] = {
            { TRUE_TOKEN,   "true" },
            { FALSE_TOKEN,  "false" },
            { FOR_TOKEN,    "for" },
            { IN_TOKEN,     "in" },
            { IF_TOKEN,     "if" },
            { ELIF_TOKEN,   "elif" },
            { ELSE_TOKEN,   "else" },
            { DEF_TOKEN,    "def" },
            { ENDFOR_TOKEN, "endfor" },
            { ENDIF_TOKEN,  "endif" },
            { ENDDEF_TOKEN, "enddef" },
            { AND_TOKEN,    "and" },
            { OR_TOKEN,     "or" },
            { NOT_TOKEN,    "not" },
            { INVALID_TOKEN }
        };

    TokenType get_keyword_token(const std::string & s)
    {
        const KeywordDef * k = k_keywords;
        for (; k->tok != INVALID_TOKEN; ++k)
        {
            if (s == k->keyword)
            {
                return k->tok;
            }
        }
        return INVALID_TOKEN;
    }

    void create_id_token(token_vector & tokens, const std::string & s)
    {
        TokenType t = get_keyword_token(s);
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

    token_vector tokenize_statement(const std::string & text)
    {
        token_vector tokens;
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
                        tokens.emplace_back(static_cast<TokenType>(c));
                    }
                    else if (c == '\"' || c == '\'')
                    {
                        // Set the start pos to after the open quote.
                        pos = i + 1;
                        str_open_quote = c;
                        state = STRING_LITERAL_STATE;
                    }
                    else if (c == '=' || c == '!' || c == '&' || c == '|')
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
                    if (!is_key_path_char(c))
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
                                    throw TemplateException("unexpected '=' character");
                            }
                            break;
                        case '!':
                            switch (c)
                            {
                                case '=':
                                    tokens.emplace_back(NEQ_TOKEN);
                                    break;
                                default:
                                    tokens.emplace_back(NOT_TOKEN);

                                    // Reprocess this char in the initial state.
                                    --i;
                                    break;
                            }
                            break;
                        case '&':
                            switch (c)
                            {
                                case '&':
                                    tokens.emplace_back(AND_TOKEN);
                                    break;
                                default:
                                    throw TemplateException("unexpected '&' character");
                            }
                            break;
                        case '|':
                            switch (c)
                            {
                                case '|':
                                    tokens.emplace_back(OR_TOKEN);
                                    break;
                                default:
                                    throw TemplateException("unexpected '|' character");
                            }
                            break;
                        default:
                            throw TemplateException("internal error: unexpected first op char");
                    }
                    state = INIT_STATE;
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
        else if (state == OPERATOR_STATE)
        {
            throw TemplateException("unexpected character");
        }

        return tokens;
    }

	//////////////////////////////////////////////////////////////////////////
	// Expr parser
	//////////////////////////////////////////////////////////////////////////

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

    data_ptr ExprParser::get_var_value(const std::string & path, data_list & params)
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
                    assert(tmpl);
                    result = tmpl->eval(m_data, &params);
                }
            }

            return result;
        }
        catch (data_map::key_error & e)
        {
            // Return an empty string for invalid key so it will eval to false.
            return "";
        }
    }

    data_ptr ExprParser::parse_var()
    {
        Token * path = m_tok.match(KEY_PATH_TOKEN, "expected key path");

        data_list params;
        if (m_tok->get_type() == OPEN_PAREN_TOKEN)
        {
            m_tok.match(OPEN_PAREN_TOKEN);

            while (m_tok->get_type() != CLOSE_PAREN_TOKEN)
            {
                params.push_back(parse_expr());

                if (m_tok->get_type() != CLOSE_PAREN_TOKEN)
                {
                    m_tok.match(COMMA_TOKEN, "expected comma");
                }
            }
            m_tok.match(CLOSE_PAREN_TOKEN, "expected close paren");
        }

        return get_var_value(path->get_value(), params);
    }

    data_ptr ExprParser::parse_factor()
    {
        TokenType tokType = m_tok->get_type();
        data_ptr result;
        if (tokType == NOT_TOKEN)
        {
            m_tok.next();
            result = parse_expr()->empty();
        }
        else if (tokType == OPEN_PAREN_TOKEN)
        {
            m_tok.next();
            result = parse_expr();
            m_tok.match(CLOSE_PAREN_TOKEN, "expected close paren");
        }
        else if (tokType == STRING_LITERAL_TOKEN)
        {
            result = m_tok.match(STRING_LITERAL_TOKEN)->get_value();
        }
        else if (tokType == TRUE_TOKEN)
        {
            m_tok.next();
            result = true;
        }
        else if (tokType == FALSE_TOKEN)
        {
            m_tok.next();
            result = false;
        }
        else
        {
            result = parse_var();
        }
        return result;
    }

    data_ptr ExprParser::parse_bfactor()
    {
        data_ptr ldata = parse_factor();

        TokenType tokType = m_tok->get_type();
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
	// Node classes
	//////////////////////////////////////////////////////////////////////////

	// defaults, overridden by subclasses with children
	void Node::set_children( node_vector & )
	{
		throw TemplateException(get_line(), "This node type cannot have children") ;
	}

	node_vector & Node::get_children()
	{
		throw TemplateException(get_line(), "This node type cannot have children") ;
	}

	// NodeText
	NodeType NodeText::gettype()
	{
		return NODE_TYPE_TEXT ;
	}

	void NodeText::gettext( std::ostream &stream, data_map & )
	{
		stream << m_text ;
	}

	// NodeVar
	NodeType NodeVar::gettype()
	{
		return NODE_TYPE_VAR ;
	}

	void NodeVar::gettext( std::ostream &stream, data_map &data )
	{
        try
        {
            TokenIterator it(m_expr);
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

	// NodeVar
	void NodeParent::set_children( node_vector &children )
	{
		m_children.assign(children.begin(), children.end()) ;
	}

	node_vector & NodeParent::get_children()
	{
		return m_children;
	}

	// NodeFor
	NodeFor::NodeFor(token_vector & tokens, uint32_t line)
    :   NodeParent(line)
	{
        TokenIterator tok(tokens);
        tok.match(FOR_TOKEN, "expected 'for'");
        m_val = tok.match(KEY_PATH_TOKEN, "expected key path")->get_value();
        tok.match(IN_TOKEN, "expected 'in'");
        m_key = tok.match(KEY_PATH_TOKEN, "expected key path")->get_value();
        tok.match(END_TOKEN, "expected end of statement");
	}

	NodeType NodeFor::gettype()
	{
		return NODE_TYPE_FOR ;
	}

	void NodeFor::gettext( std::ostream &stream, data_map &data )
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

	// NodeIf
    NodeIf::NodeIf(token_vector & expr, uint32_t line)
    :   NodeParent(line),
        m_expr(expr),
        m_else_if(nullptr),
        m_if_type(NODE_TYPE_IF)
    {
        if (expr[0].get_type() == ELIF_TOKEN)
        {
            m_if_type = NODE_TYPE_ELIF;
        }
        else if (expr[0].get_type() == ELSE_TOKEN)
        {
            m_if_type = NODE_TYPE_ELSE;
        }
    }

	NodeType NodeIf::gettype()
	{
		return m_if_type ;
	}

    void NodeIf::set_else_if(node_ptr else_if)
    {
        m_else_if = else_if;
    }

	void NodeIf::gettext( std::ostream &stream, data_map &data )
	{
		if (is_true(data))
		{
			for(size_t j = 0 ; j < m_children.size() ; ++j)
			{
				m_children[j]->gettext(stream, data) ;
			}
		}
        else if (m_else_if)
        {
            m_else_if->gettext(stream, data);
        }
	}

	bool NodeIf::is_true( data_map &data )
	{
        try
        {
            TokenIterator it(m_expr);
            if (m_if_type == NODE_TYPE_IF)
            {
                it.match(IF_TOKEN, "expected 'if' keyword");
            }
            else if (m_if_type == NODE_TYPE_ELIF)
            {
                it.match(ELIF_TOKEN, "expected 'elif' keyword");
            }
            else if (m_if_type == NODE_TYPE_ELSE)
            {
                it.match(ELSE_TOKEN, "expected 'else' keyword");
                it.match(END_TOKEN, "expected end of statement");
                return true;
            }
            ExprParser parser(it, data);
            data_ptr d = parser.parse_expr();
            it.match(END_TOKEN, "expected end of statement");

            return !d->empty();
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(get_line());
            throw e;
        }
	}

	bool NodeIf::is_else()
    {
        return m_if_type == NODE_TYPE_ELSE;
    }

	// NodeDef
    NodeDef::NodeDef(token_vector & expr, uint32_t line)
    :   NodeParent(line)
    {
        TokenIterator tok(expr);
        tok.match(DEF_TOKEN, "expected 'def'");

        m_name = tok.match(KEY_PATH_TOKEN, "expected key path")->get_value();

        if (tok->get_type() == OPEN_PAREN_TOKEN)
        {
            tok.match(OPEN_PAREN_TOKEN);

            while (tok->get_type() != CLOSE_PAREN_TOKEN)
            {
                m_params.push_back(tok.match(KEY_PATH_TOKEN, "expected key path")->get_value());

                if (tok->get_type() != CLOSE_PAREN_TOKEN)
                {
                    tok.match(COMMA_TOKEN, "expected comma");
                }
            }
            tok.match(CLOSE_PAREN_TOKEN, "expected close paren");
        }
        tok.match(END_TOKEN, "expected end of statement");
    }

	NodeType NodeDef::gettype()
	{
		return NODE_TYPE_DEF ;
	}

	void NodeDef::gettext( std::ostream &stream, data_map &data )
	{
        try
        {
            // Follow the key path.
            data_ptr& target = data.parse_path(m_name, true);

            // Set the map entry's value to a newly created template. The nodes were already
            // parsed and set as our m_children vector. The names of the template's parameters
            // are set from the param names we parsed in the ctor.
            DataTemplate * tmpl = new DataTemplate(m_children);
            tmpl->params() = m_params;
            target = data_ptr(tmpl);
        }
        catch (data_map::key_error &)
        {
            // ignore exception
        }
	}

	void NodeEnd::gettext( std::ostream &, data_map &)
	{
		throw TemplateException(get_line(), "End-of-control statements have no associated text") ;
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
	// parses a template into nodes (text, for, if, variable, def)
	//////////////////////////////////////////////////////////////////////////
	node_vector & tokenize(std::string text, node_vector &nodes)
	{
        uint32_t currentLine = 1;
        try
        {
            std::stack<std::pair<node_ptr, TokenType>> node_stack;
            node_ptr current_node;
            node_vector * current_nodes = &nodes;
            bool eolPrecedes = true;
            bool lastWasEol = true;
            TokenType until = INVALID_TOKEN;
            while(! text.empty())
            {
                size_t pos = text.find("{") ;
                if (pos == std::string::npos)
                {
                    if (! text.empty())
                    {
                        current_nodes->push_back(node_ptr(new NodeText(text, currentLine))) ;
                    }
                    return nodes ;
                }
                std::string pre_text = text.substr(0, pos) ;
                currentLine += count_newlines(pre_text) ;
                if (! pre_text.empty())
                {
                    current_nodes->push_back(node_ptr(new NodeText(pre_text, currentLine))) ;
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
                    current_nodes->push_back(node_ptr(new NodeText("{", currentLine))) ;
                    return nodes ;
                }

                // variable
                if (text[0] == '$')
                {
                    pos = text.find("}") ;
                    if (pos != std::string::npos)
                    {
                        pre_text = text.substr(1, pos-1);
                        text = text.substr(pos+1) ;

                        token_vector stmt_tokens = tokenize_statement(pre_text);
                        current_nodes->push_back(node_ptr (new NodeVar(stmt_tokens, currentLine))) ;
                    }

                    lastWasEol = false ;
                }
                // control statement
                else if (text[0] == '%')
                {
                    pos = text.find("%}") ;
                    if (pos != std::string::npos)
                    {
                        pre_text = text.substr(1, pos-1) ;
                        uint32_t lineCount = count_newlines(pre_text) ;

                        bool eolFollows = text.size() > pos+2 && text[pos+2] == '\n' ;

                        // Tokenize the control statement.
                        token_vector stmt_tokens = tokenize_statement(strip_comment(pre_text));
                        TokenType first_token_type = stmt_tokens[0].get_type();

                        // Create control statement nodes.
                        switch (first_token_type)
                        {
                            case FOR_TOKEN:
                                node_stack.push(std::make_pair(current_node, until));
                                current_node = node_ptr(new NodeFor(stmt_tokens, currentLine));
                                current_nodes->push_back(current_node) ;
                                current_nodes = &current_node->get_children();
                                until = ENDFOR_TOKEN;
                                break;

                            case IF_TOKEN:
                                node_stack.push(std::make_pair(current_node, until));
                                current_node = node_ptr(new NodeIf(stmt_tokens, currentLine));
                                current_nodes->push_back(current_node) ;
                                current_nodes = &current_node->get_children();
                                until = ENDIF_TOKEN;
                                break;

                            case ELIF_TOKEN:
                            case ELSE_TOKEN:
                            {
                                auto current_if = dynamic_cast<NodeIf*>(current_node.get());
                                if (!current_if)
                                {
                                    throw TemplateException(currentLine, "else/elif without if");
                                }

                                if (current_if->is_else())
                                {
                                    throw TemplateException(currentLine, "if already has else");
                                }

                                current_node = node_ptr(new NodeIf(stmt_tokens, currentLine));
                                current_if->set_else_if(current_node);
                                current_nodes = &current_node->get_children();
                                break;
                            }

                            case DEF_TOKEN:
                                node_stack.push(std::make_pair(current_node, until));
                                current_node = node_ptr(new NodeDef(stmt_tokens, currentLine));
                                current_nodes->push_back(current_node) ;
                                current_nodes = &current_node->get_children();
                                until = ENDDEF_TOKEN;
                                break;

                            case ENDFOR_TOKEN:
                            case ENDIF_TOKEN:
                            case ENDDEF_TOKEN:
                                if (until == first_token_type)
                                {
                                    assert(!node_stack.empty());
                                    auto top = node_stack.top();
                                    node_stack.pop();
                                    if (top.first)
                                    {
                                        current_node = top.first;
                                        current_nodes = &top.first->get_children();
                                        until = top.second;
                                    }
                                    else
                                    {
                                        current_node.reset();
                                        current_nodes = &nodes;
                                        until = INVALID_TOKEN;
                                    }
                                }
                                else
                                {
                                    throw TemplateException(currentLine, "unexpected end statement");
                                }
                                break;

                            case END_TOKEN:
                                throw TemplateException(currentLine, "empty control statement");
                                break;

                            default:
                                throw TemplateException(currentLine, "Unrecognized control statement");
                        }

                        // Chop off following eol if this control statement is on a line by itself.
                        if (eolPrecedes && eolFollows)
                        {
                            ++pos ;
                            ++currentLine ;
                            lastWasEol = true ;
                        }

                        text = text.substr(pos+2) ;

                        currentLine += lineCount;
                    }
                    else
                    {
                        throw TemplateException(currentLine, "missing close statement block");
                    }
                }
                // comment
                else if (text[0] == '#')
                {
                    pos = text.find("#}") ;
                    if (pos != std::string::npos)
                    {
                        pre_text = text.substr(1, pos-1) ;
                        currentLine += count_newlines(pre_text) ;

                        bool eolFollows = text.size() > pos+2 && text[pos+2] == '\n' ;

                        // Chop off following eol if this comment is on a line by itself.
                        if (eolPrecedes && eolFollows)
                        {
                            ++pos ;
                            ++currentLine ;
                            lastWasEol = true ;
                        }

                        text = text.substr(pos+2) ;
                    }
                }
                else
                {
                    current_nodes->push_back(node_ptr(new NodeText("{", currentLine))) ;
                }
            }
            return nodes ;
        }
        catch (TemplateException e)
        {
            e.set_line_if_missing(currentLine);
            throw e;
        }
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
    std::string parse(const std::string &templ_text, data_map &data)
	{
		return DataTemplate(templ_text).eval(data);
	}
	void parse(std::ostream &stream, const std::string &templ_text, data_map &data)
	{
        DataTemplate(templ_text).eval(stream, data);
	}
}
