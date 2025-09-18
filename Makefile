none:
	@echo "targets available are:"
	@echo " - example_gcc"
	@echo " - example_clang"
	@echo " - example_mingw"

example_gcc:
	gcc ./example.c -Wall -Wextra -Wswitch-enum -o example_gcc
example_clang:
	clang ./example.c -Wall -Wextra -Wswitch-enum -o example_clang
example_mingw:
	x86_64-w64-mingw32-gcc ./example.c -Wall -Wextra -Wswitch-enum -o example_mingw