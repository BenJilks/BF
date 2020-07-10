#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <cassert>
#include <map>

//#define DEBUG

struct NodeStatement
{
    enum Type
    {
        Goto,
        Let,
        Source,
		Load,
		Unload,
		For,
		Print,
        Call,
        Scope,
    };

    Type type;
	int count;
    std::string operand;
    std::vector<std::string> args;
    std::vector<NodeStatement> sub_nodes;
};

struct NodeDef
{
    std::string name;
    std::vector<std::string> params;
    std::string body;
};

struct NodeProgram
{
    std::vector<NodeDef> defs;
    std::vector<NodeStatement> statements;
};

std::string replace_all(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

NodeProgram parse(std::string in)
{
    NodeProgram program;
    int curr_index = 0;

#ifdef DEBUG
    std::cout << "Parse: " << in << "\n";
#endif

    auto peek = [&](int count = 0) -> char
    {
        if (curr_index + count >= in.size())
            return 0;

        return in[curr_index + count];
    };

    auto consume = [&]() -> char
    {
        if (curr_index >= in.size())
            return 0;

        return in[curr_index++];
    };

    auto is_string_next = [&](const std::string &str) -> bool
    {
        for (int i = 0; i < str.size(); i++)
        {
            if (peek(i) != str[i])
                return false;
        }
        return true;
    };

    auto consume_string = [&](const std::string &str) -> bool
    {
        if (!is_string_next(str))
            return false;

        for (int i = 0; i < str.size(); i++)
            consume();
        return true;
    };

    auto skip_white_space = [&]()
    {
        for (;;)
        {
            if (!isspace(peek()))
                break;
            consume();
        }
    };

    auto parse_name = [&]() -> std::string
    {
        std::string buffer;
        for (;;)
        {
            auto c = peek();
            if (!isalnum(c) && c != '_' && c != '[' && c != ']')
                break;

            buffer += consume();
        }

        return buffer;
    };

	auto parse_let_name = [&]() -> std::string
    {
        std::string buffer;
        for (;;)
        {
            auto c = peek();
            if (!isalnum(c) && c != '_')
                break;

            buffer += consume();
        }

        return buffer;
    };

    auto parse_int = [&]() -> int
    {
		std::string buffer;
	    for (;;)
        {
            auto c = peek();
            if (!isdigit(c))
                break;

            buffer += consume();
        }

        return atoi(buffer.c_str());
    };

    auto parse_body = [&]()
    {
        std::string buffer;
        int depth = 1;
        assert (consume() == '{');
        for (;;)
        {
            if (peek() == '}')
                depth -= 1;
            else if (peek() == '{')
                depth += 1;

            if (depth <= 0)
                break;
            buffer += consume();
        }
        assert (consume() == '}');

        return buffer;
    };

    auto parse_def = [&]()
    {
        NodeDef def;

        assert (consume_string("def"));
        skip_white_space();
        def.name = parse_name();

        skip_white_space();
        assert(consume() == '(');
        for (;;)
        {
            skip_white_space();
            if (peek() == ')')
                break;

            def.params.push_back(parse_name());
            skip_white_space();

            if (peek() != ',')
                break;
            consume();
        }
        assert(consume() == ')');

        skip_white_space();
        def.body = parse_body();
        program.defs.push_back(def);
    };

    auto parse_let = [&]()
    {
		auto emit_let_name = [&]()
		{
			NodeStatement let;
			let.type = NodeStatement::Let;
			let.operand = parse_let_name();
			program.statements.push_back(let);
	
			auto name = let.operand;
			if (peek() == '[')
			{
				assert (consume() == '[');
				auto count = parse_int();
				assert (consume() == ']');
				for (int i = 0; i < count; i++)
				{
					let.operand = name + "[" + std::to_string(i) + "]";
					program.statements.push_back(let);
				}
			}
		};

        assert (consume_string("let"));
        skip_white_space();
		emit_let_name();

		skip_white_space();
		while (peek() == ',')
		{
			consume();
			skip_white_space();
			emit_let_name();
			skip_white_space();
		}
    };

    auto parse_goto = [&]()
    {
        NodeStatement goto_;

        assert (consume_string("goto"));
        goto_.type = NodeStatement::Goto;
        skip_white_space();
        goto_.operand = parse_name();
        program.statements.push_back(goto_);
    };

    auto parse_call = [&]()
    {
        NodeStatement call;

		auto skip_until = [&](char c)
		{
			std::string buffer;
	
			assert (consume() == c);
			while (peek() != c)
				buffer += consume();
			assert (consume() == c);
			return c + buffer + c;
		};

        call.type = NodeStatement::Call;
        call.operand = parse_name();
        skip_white_space();
        assert (consume() == '(');
        for (;;)
        {
            skip_white_space();
            if (peek() == ')')
                break;

            if (peek() == '{')
                call.args.push_back(parse_body());
		    else if (peek() == '\'')
				call.args.push_back(skip_until('\''));
			else if (isdigit(peek()))
				call.args.push_back(std::to_string(parse_int()));
            else
                call.args.push_back(parse_name());

            skip_white_space();
            if (peek() != ',')
                break;
            consume();
        }
        assert (consume() == ')');
        program.statements.push_back(call);
    };

    auto parse_source = [&]()
    {
        NodeStatement source;
        source.type = NodeStatement::Source;

        assert (consume() == '`');
        for (;;)
        {
            if (peek() == '`')
                break;
            source.operand += consume();
        }
        assert (consume() == '`');
        program.statements.push_back(source);
    };

    auto parse_load = [&]()
    {
		NodeStatement load;
		load.type = NodeStatement::Load;
	
		assert (consume_string("load"));
		skip_white_space();
		assert (consume() == '\'');
		load.operand = consume();
		assert (consume() == '\'');
		program.statements.push_back(load);
    };

    auto parse_unload = [&]()
    {
		NodeStatement unload;
		unload.type = NodeStatement::Unload;
		
		assert (consume_string("unload"));
		skip_white_space();
		assert (consume() == '\'');
		unload.operand = consume();
		assert (consume() == '\'');
		program.statements.push_back(unload);
    };

	auto parse_for = [&]()
	{
		NodeStatement for_;
		for_.type = NodeStatement::For;

		assert (consume_string("for"));
		skip_white_space();
		for_.operand = parse_name();
		skip_white_space();
		assert (consume_string("in"));
		skip_white_space();
		for_.count = parse_int();
		skip_white_space();
		for_.args.push_back(parse_body());
		program.statements.push_back(for_);
	};

    auto parse_print = [&]()
    {
        NodeStatement print;
        print.type = NodeStatement::Print;

		assert (consume_string("print"));
		skip_white_space();
        assert (consume() == '"');
        for (;;)
        {
            if (peek() == '"')
                break;
            print.operand += consume();
        }
        assert (consume() == '"');
        program.statements.push_back(print);
    };

    for (;;)
    {
        if (!peek() || peek() == -1)
            break;
        else if (isspace(peek()))
            consume();
        else if (is_string_next("def"))
            parse_def();
        else if (is_string_next("let"))
            parse_let();
        else if (is_string_next("goto"))
            parse_goto();
		else if (is_string_next("load"))
		    parse_load();
		else if (is_string_next("unload"))
		    parse_unload();
		else if (is_string_next("for"))
			parse_for();
		else if (is_string_next("print"))
		    parse_print();
        else if (peek() == '`')
            parse_source();
        else
            parse_call();
    }

    return program;
}

std::vector<NodeStatement> apply_defs(NodeProgram program)
{
    std::vector<NodeDef> defs = program.defs;

    auto apply_args = [&](const std::string &in, const std::map<std::string, std::string> &args)
    {
		std::string out;
		std::string buffer;

		for (int i = 0; i < in.size(); i++)
		{
			char c = in[i];
			if (!isalnum(c) && c != '_')
			{
				for (const auto &it : args)
				{
					if (it.first == buffer)
					{
						buffer = it.second;
						break;
					}
				}
	
				out += buffer + c;
				buffer = "";
				continue;
			}
	
			buffer += c;
		}

		return out + buffer;
    };

    auto parse_sub = [&](const std::string &in, const std::map<std::string, std::string> &args)
    {
        // Apply args
        std::string source = apply_args(in, args);
        auto sub_program = parse(source);
        for (const auto &def : sub_program.defs)
            defs.push_back(def);

        return sub_program.statements;
    };

    auto find_def = [&](std::string name) -> const NodeDef&
    {
        for (const auto &def : defs)
        {
            if (def.name == name)
                return def;
        }

        std::cout << name << "\n";
        assert (false);
    };

    std::function<NodeStatement(std::vector<NodeStatement>, std::string)> apply;
    apply = [&](std::vector<NodeStatement> in, std::string name)
    {
        std::vector<NodeStatement> sub_nodes;
        for (const auto &statement : in)
        {
            if (statement.type == NodeStatement::Call)
            {
                auto def = find_def(statement.operand);

                std::map<std::string, std::string> args;
                for (int i = 0; i < statement.args.size(); i++)
                    args[def.params[i]] = statement.args[i];

                auto sub_statement = apply(parse_sub(def.body, args), statement.operand);
                sub_nodes.push_back(sub_statement);
                continue;
            }
			
			if (statement.type == NodeStatement::For)
			{
				auto count = statement.count;
				auto body = statement.args[0];
				auto name = statement.operand;

				for (int i = 0; i < count; i++)
				{
					std::map<std::string, std::string> args;
					args[name] = std::to_string(i);
					auto sub_statement = apply(parse_sub(body, args), "for");
					sub_nodes.push_back(sub_statement);
				}
				continue;
			}

            sub_nodes.push_back(statement);
        }

        NodeStatement scope;
        scope.type = NodeStatement::Scope;
        scope.operand = name;
        scope.sub_nodes = sub_nodes;
        return scope;
    };

    return apply(program.statements, "Main").sub_nodes;
}

void compile(std::ostream &out, const std::vector<NodeStatement> &program)
{
	struct Scope
	{
		std::map<std::string, int> vars;
		Scope *parent;
		int allocator;
	};

	std::function<int(const Scope&, std::string)> find_var;
	find_var = [&](const Scope &scope, std::string name)
	{
		if (scope.vars.find(name) != scope.vars.end())
			return scope.vars.at(name);

		if (scope.parent)
			return find_var(*scope.parent, name);

		std::cout << name << "\n";
		assert(false);
	};

	int curr_pointer = 0;
	std::function<void(const std::vector<NodeStatement>&, Scope*)> compile_scope;
	compile_scope = [&](const std::vector<NodeStatement> &statements, Scope *parent)
	{
		Scope scope;
		scope.parent = parent;
		scope.allocator = parent ? parent->allocator : 0;
	
		auto goto_pos = [&](int pos)
		{
			if (pos < curr_pointer)
			{
				for (int i = pos; i < curr_pointer; i++)
					out << "<";
			}
			else if (pos > curr_pointer)
			{
				for (int i = curr_pointer; i < pos; i++)
					out << ">";
			}
			curr_pointer = pos;
		};

		for (const auto &statement : statements)
		{
			switch (statement.type)
			{
				case NodeStatement::Let:
				{
					scope.vars[statement.operand] = scope.allocator;
					scope.allocator += 1;
					break;
				}

				case NodeStatement::Goto:
				{
					auto pos = find_var(scope, statement.operand);
					goto_pos(pos);
					break;
				}

				case NodeStatement::Source:
				{
					out << statement.operand;
					break;
				}

				case NodeStatement::Scope:
				{
					compile_scope(statement.sub_nodes, &scope);
					break;
				}

				case NodeStatement::Load:
				{
					for (int i = 0; i < statement.operand[0]; i++)
						out << "+";
					break;
				}

				case NodeStatement::Unload:
				{
					for (int i = 0; i < statement.operand[0]; i++)
						out << "-";
					break;
				}


				case NodeStatement::Print:
				{
					auto temp_pos = scope.allocator;
					goto_pos(temp_pos);
					out << "[-]";

					int curr_value = 0;
					for (char target : statement.operand)
					{
						if (target < curr_value)
						{
							for (int i = target; i < curr_value; i++)
								out << "-";
						}
						else if (target > curr_value)
						{
							for (int i = curr_value; i < target; i++)
								out << "+";
						}

						out << ".";
						curr_value = target;
					}
					out << "[-]";
					break;
				}

				default:
				{
					assert (false);
					break;
				}
			}
		}
	};

	compile_scope(program, nullptr);
	out << "\n";
}

int main()
{
    std::stringstream stream;
    stream << std::cin.rdbuf();
    auto source = stream.str();
    auto program = apply_defs(parse(source));

#ifdef DEBUG
    std::function<void(std::vector<NodeStatement>,int)> log_program;
    log_program = [&](std::vector<NodeStatement> prog, int indent)
    {
        for (const auto &statement : prog)
        {
            for (int i = 0; i < indent; i++)
                std::cout << "\t";

            switch (statement.type)
            {
                case NodeStatement::Scope:
                    std::cout << "Scope " << statement.operand << ":\n";
                    log_program(statement.sub_nodes, indent + 1);
                    break;
                case NodeStatement::Goto:
                    std::cout << "goto " << statement.operand << "\n";
                    break;
                case NodeStatement::Let:
                    std::cout << "let " << statement.operand << "\n";
                    break;
                case NodeStatement::Call:
                    std::cout << statement.operand << "(";
                    for (const auto &arg : statement.args)
                        std::cout << arg << ", ";
                    std::cout << ")\n";
                    break;
                case NodeStatement::Source:
                    std::cout << "`" << statement.operand << "`\n";
                    break;
            }
        }
    };

    log_program(program, 0);
#endif

    compile(std::cout, program);
    return 0;
}
