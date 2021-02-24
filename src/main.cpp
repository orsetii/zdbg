#include <iostream>
#include <sys/ptrace.h>
#include <unistd.h>
#include "debugger.hpp"

int main(int argc, char* argv[]) {

	if (argc < 2) {

		std::cerr << "Program name not specified";
		return 1;
	}

	auto prog = argv[1];

	auto pid = fork();

	if (pid == 0) {
		// we're in child process
		// exec debugee
		ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
		execl(prog, prog, nullptr);
	} else if (pid >= 1) {
		// we're in paraent process
		// exec debugger
		auto dbger = debugger(prog, pid);
		dbger.run();
	}
	return 0;
}
