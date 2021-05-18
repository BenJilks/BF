#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc != 2)
		return 1;

	std::stringstream stream;
	stream << std::cin.rdbuf();
	auto source = stream.str();
	auto input = std::string(argv[1]);

	unsigned char memory[256];
	int memory_ptr = 0;
	int source_ptr = 0;
	int input_ptr = 0;
	int max_mem_use = 0;
	std::vector<int> loop_stack;
	memset(memory, 0, sizeof(memory));

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

	while (source_ptr < source.size())
	{
#if 0
		dump_state();
		usleep(10000);
#endif

		char op = source[source_ptr++];
		switch (op)
		{
#if 1
			case '$':
				dump_state();
				sleep(1);
				break;
#endif
			case '>':
				memory_ptr += 1;
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
				std::cout << (int)memory[memory_ptr] << "\n";
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
					break;
				}
				source_ptr = loop_stack.back();
				break;
			default:
				break;
		}
	}

	std::cout << "\n";
	dump_state();
	return 0;
}
