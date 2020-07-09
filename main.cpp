#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <cassert>
#include <map>

/*

def input(x) { goto x `.` }
def print(x) { goto x `,` }
def inc(x) { goto x `+` }
def dec(x) { goto x `-` }
def inc(x, by) { goto x `+`*by }
def dec(x, by) { goto x `-`*by }

def for(x, do) {
    goto x
    `[`
        do
        dec(x)
    `]`
}

def copy(a, b) {
    let temp
    for(a, {
        inc(temp)
        inc(b)
    })

    for(temp, {
        inc(a)
    })
}

def add(a, b, c)
{
    copy(a, c)
    copy(b, c)
}

let a, b, c
input(a)
input(b)
add(a, b, c)
print(c)

*/

struct NodeStatement
{
    enum Type
    {
        Goto,
        Let,
        Source,
        Call,
        Scope,
    };

    Type type;
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
            if (!isalnum(c) && c != '_')
                break;

            buffer += consume();
        }

        return buffer;
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
        NodeStatement let;

        assert (consume_string("let"));
        let.type = NodeStatement::Let;
        skip_white_space();
        let.operand = parse_name();
        program.statements.push_back(let);
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

    auto parse_sub = [&](const std::string &in, const std::map<std::string, std::string> &args)
    {
        // Apply args
        std::string source = in;
        for (const auto &arg : args)
            source = replace_all(source, arg.first, arg.second);

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

int main()
{
    std::ifstream in("test.bfs");
    std::stringstream stream;
    stream << in.rdbuf();
    auto source = stream.str();

    auto program = parse(source);
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
    log_program(apply_defs(program), 0);
}
