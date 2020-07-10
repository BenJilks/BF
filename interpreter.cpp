#include <iostream>
#include <sstream>
#include <vector>

int main(int argc, char *argv[])
{
	if (argc != 2)
		return 1;

	std::stringstream stream;
	stream << std::cin.rdbuf();
	auto source = stream.str();
	auto input = std::string(argv[1]);

	char memory[256];
	int memory_ptr = 0;
	int source_ptr = 0;
	int input_ptr = 0;
	std::vector<int> loop_stack;
	while (source_ptr < source.size())
	{
		char op = source[source_ptr++];
		switch (op)
		{
			case '>':
				memory_ptr += 1;
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
				break;
			case ',':
				if (input_ptr < input.size())
					memory[memory_ptr] = input[input_ptr++];
				else
					source_ptr = source.size();
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
	return 0;
}
