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
	} else if ((is_prefix(command, "break")) || is_prefix(command, "b")) {

		// naively assume that the user has written 0xADDRESS
		std::string addr { args[1], 2 };

		set_breakpoint_at_addr(std::stol(addr, 0, 16));
	} else if (is_prefix(command, "register")) {
		if (is_prefix(args[1], "dump")) {
		    dump_registers();
		} else if (is_prefix(args[1], "read")) {
		    std::cout << get_reg_value(m_pid, get_register_from_name(args[2])) << std::endl;
		} else if (is_prefix(args[1], "write")) {
		    std::string val {args[3], 2}; //assume 0xVAL
		    set_reg_value(m_pid, get_register_from_name(args[2]), std::stol(val, 0, 16));
		}

	} else if(is_prefix(command, "memory")) {
		std::string addr {args[2], 2}; //assume 0xADDRESS

		if (is_prefix(args[1], "read")) {
		    std::cout << std::hex << read_memory(std::stol(addr, 0, 16)) << std::endl;
		}
		if (is_prefix(args[1], "write")) {
		    std::string val {args[3], 2}; //assume 0xVAL
		    write_memory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
		}
	} else {
		std::cerr << "Unknown Command\n";
	}
}

void debugger::continue_execution() {
    step_over_breakpoint();
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    wait_for_signal();
}

void debugger::set_breakpoint_at_addr(std::intptr_t addr) {

	std::cout << "Breakpoint set at 0x" << std::hex << addr << std::endl;
	breakpoint bp {m_pid, addr};
	bp.enable();
	m_breakpoints[addr] = bp;
}

void debugger::dump_registers() {
    for (const auto& rd : g_register_descriptors) {
        std::cout << rd.name << " 0x"
                  << std::setfill('0') << std::setw(16) << std::hex << get_reg_value(m_pid, rd.r) << std::endl;
    }
}

uint64_t debugger::read_memory(uint64_t address) {
    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void debugger::write_memory(uint64_t address, uint64_t value) {
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

uint64_t debugger::get_pc() {
    return get_reg_value(m_pid, reg::rip);
}

void debugger::set_pc(uint64_t pc) {
    set_reg_value(m_pid, reg::rip, pc);
}

void debugger::wait_for_signal() {
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}

void debugger::step_over_breakpoint() {
    // - 1 because execution will go past the breakpoint
    auto possible_breakpoint_location = get_pc() - 1;

    if (m_breakpoints.count(possible_breakpoint_location)) {
        auto& bp = m_breakpoints[possible_breakpoint_location];
        if (bp.is_enabled()) {
            auto previous_instruction_address = possible_breakpoint_location;
            set_pc(previous_instruction_address);

            bp.disable();
            ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
            wait_for_signal();
            bp.enable();
        }
    }
}



void breakpoint::enable() {

	// grab the word at *addr
	auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
	// save bottom byte
	m_saved_data = static_cast<uint8_t>(data & 0xff);

	// int 3, aka software breakpoint
	uint64_t int3 = 0xcc;

	// set bottom byte to 0xcc
	uint64_t data_with_int3 = ((data & ~0xff) | int3);

	ptrace(PTRACE_PEEKDATA, m_pid, m_addr, data_with_int3);

	m_enabled = true;
}


void breakpoint::disable() {

	// Since we are opearting on words not bytes,
	// we need to save the word, and overwrite the 0xcc byte
	// with m_saved_data; then save that changed word back.
	
	auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
	auto restored_data = ((data & ~0xff) | m_saved_data);
	ptrace(PTRACE_POKEDATA, m_pid, m_addr, restored_data);

	m_enabled = false;
}

