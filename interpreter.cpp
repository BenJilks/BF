#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>

#define MEMORY_SIZE 10 * 1024 // 10 Kb

int main(int argc, char *argv[])
{
	if (argc < 2)
		return 1;

	std::stringstream stream;
    
    {
        std::ifstream code_file(argv[1]);
	    stream << code_file.rdbuf();
    }

	auto source = stream.str();
	auto input = argc >= 3
        ? std::string(argv[2])
        : "";

	unsigned char *memory = new unsigned char[MEMORY_SIZE];
	int memory_ptr = 0;
	int source_ptr = 0;
	int input_ptr = 0;
	int max_mem_use = 1;
    bool is_stepping = false;
    bool break_on_exit = false;
    std::string last_command = "c";
	std::vector<int> loop_stack;
	memset(memory, 0, sizeof(memory));

    auto dump_code_position = [&]()
    {
        constexpr int source_view_size = 50;
        for (int i = source_ptr - source_view_size; i < source_ptr + source_view_size; i++)
        {
            if (i >= 0 && i < source.size() && source[i] != '\n')
                std::cout << source[i];
            else
                std::cout << " ";
        }
        std::cout << "\n";

        for (int i = 0; i < source_view_size - 1; i++)
            std::cout << " ";
        std::cout << "^\n\n";
    };

	auto dump_state = [&]()
	{
		static constexpr int row_size = 20;
		for (int row = 0; row < (max_mem_use / row_size) + 1; row++)
		{
			auto index = row * row_size;
			for (int i = index; i < std::min(index + row_size, max_mem_use); i++)
				printf("%4u ", memory[i]);
			std::cout << "\n";
			for (int i = index; i < std::min(index + row_size, max_mem_use); i++)
				printf("%4s ", i == memory_ptr ? "^" : " ");
			std::cout << "\n";
		}
		std::cout << "\n";
	};

    auto do_command = [&](const std::string &line)
    {
        std::stringstream stream(line);
        std::string command;
        stream >> command;

        if (command == "c" || command == "continue")
        {
            is_stepping = false;
        }
        else if (command == "q" || command == "quit")
        {
            source_ptr = source.size();
        }
        else if (command == "e" || command == "exit")
        {
            break_on_exit = true;
        }
        else
        {
            is_stepping = true;
        }
    };

    auto debugger = [&]()
    {
        break_on_exit = false;

        std::cout << "\n";
        dump_code_position();
        dump_state();
        
        std::string line;
        std::cout << "debugger >> ";
        std::getline(std::cin, line);

        if (line.empty())
        {
            do_command(last_command);
        }
        else
        {
            do_command(line);
            last_command = line;
        }
    };

	while (source_ptr < source.size())
	{
        if (is_stepping)
        {
            is_stepping = false;
            debugger();
        }

		char op = source[source_ptr++];
		switch (op)
		{
			case '*':
                if (!is_stepping)
                    debugger();
				break;
			case '>':
				memory_ptr += 1;
                if (memory_ptr > MEMORY_SIZE)
                {
                    std::cout << "Out of memory!!\n";
                    return 1;
                }

				max_mem_use = std::max(max_mem_use, memory_ptr + 1);
				break;
			case '<':
				memory_ptr -= 1;
				break;
			case '+':
				memory[memory_ptr] += 1;
				break;
			case '-':
				memory[memory_ptr] -= 1;
				break;
			case '.':
				std::cout << memory[memory_ptr];
				std::cout.flush();
				break;
			case ',':
				if (input_ptr < input.size())
					memory[memory_ptr] = input[input_ptr++];
				else
					memory[memory_ptr] = 0;
				break;
			case '[':
				if (memory[memory_ptr] == 0)
				{
					// If it's 0 going in, skip to the end of the loop
					int depth = 1;
					while (source_ptr < source.size())
					{
						char c = source[source_ptr++];
						if (c == '[') 
							depth += 1;
						else if (c == ']') 
							depth -= 1;
						if (depth <= 0)
							break;
					}
					break;
				}
				loop_stack.push_back(source_ptr);
				break;
			case ']':
				if (memory[memory_ptr] == 0)
				{
					loop_stack.pop_back();
                    if (break_on_exit)
                        debugger();
					break;
				}
				source_ptr = loop_stack.back();
				break;
			default:
				break;
		}
	}

	std::cout << "\n";
	//dump_state();
    delete[] memory;
	return 0;
}
