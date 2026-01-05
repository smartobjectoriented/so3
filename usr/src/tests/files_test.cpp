

#include <fstream>
#include <iostream>

int main()
{
	std::ofstream file("example.txt");
	if (!file) {
		std::cerr << "Failed to open file\n";
		return 1;
	}

	file << "Hello from C++\n";
	// file closes automatically
	return 0;
}
