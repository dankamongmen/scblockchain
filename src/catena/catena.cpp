#include <cstdlib>
#include <iostream>

int main(int argc, char **argv){
	for(auto i = 1 ; i < argc ; ++i){
		std::cout << argv[i] << std::endl;
	}
	return EXIT_SUCCESS;
}
