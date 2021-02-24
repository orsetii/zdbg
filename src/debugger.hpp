#pragma once
#include <string>
#include <sys/wait.h>
#include <linenoise.h>
#include <iostream>
#include <sys/ptrace.h>
#include <unordered_map>
#include <iomanip>

#include "utils.hpp"
#include "registers.hpp"

class breakpoint {
public:
    breakpoint() = default;
    breakpoint(pid_t pid, std::intptr_t addr)
        : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{}
    {}

    void enable();
    void disable();

    auto is_enabled() const -> bool { return m_enabled; }
    auto get_address() const -> std::intptr_t { return m_addr; }

private:
    pid_t m_pid;
    std::intptr_t m_addr;
    bool m_enabled;
    uint8_t m_saved_data; //data which used to be at the breakpoint address
};

class debugger {
public:
    debugger (std::string prog_name, pid_t pid)
        : m_prog_name{std::move(prog_name)}, m_pid{pid} {}

    void run();

private:

    void set_breakpoint_at_addr(std::intptr_t addr);
    void handle_command(const std::string &line);
    void continue_execution();
    void dump_registers();
    uint64_t read_memory(uint64_t address);
    void write_memory(uint64_t address, uint64_t value);
    uint64_t get_pc();
    void set_pc(uint64_t pc);
    void step_over_breakpoint();
    void wait_for_signal();

    std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
    std::string m_prog_name;
    pid_t m_pid;
};
