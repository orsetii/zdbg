#include "debugger.hpp"


void debugger::run() {

	// wait until child process has finished launching 
	int wait_status;
	auto options = 0;
	waitpid(m_pid, &wait_status, options);

	// read input from linenoise until we get an EOF (ctrl+d)
	char* line = nullptr;
	while ((line = linenoise("zdbg> ")) != nullptr) {

		handle_command(line);
		linenoiseHistoryAdd(line);
		free(line);
	}

}

void debugger::handle_command(const std::string &line) {

	auto args = split(line, ' ');
	auto command = args[0];

	if (is_prefix(command, "continue")) {
		continue_execution();
	} else {
		std::cerr << "Unknown Command\n";
	}
}

void debugger::continue_execution() {
	ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);

	int wait_status;
	auto options = 0;
	waitpid(m_pid, &wait_status, options);
}

