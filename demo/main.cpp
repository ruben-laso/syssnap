#include <chrono>
#include <thread>

#include <csignal>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <range/v3/all.hpp>

#include <CLI/CLI.hpp>

#include <system_snapshot/system_snapshot.hpp>

// Options structure
struct Options
{
	static constexpr auto DEFAULT_DEBUG     = false;
	static constexpr auto DEFAULT_MIGRATION = false;

	static constexpr auto DEFAULT_TIME = 30.0;
	static constexpr auto DEFAULT_DT   = 1.0;

	bool debug     = DEFAULT_DEBUG;
	bool migration = DEFAULT_MIGRATION;

	double time = DEFAULT_TIME;
	double dt   = DEFAULT_DT;

	std::string child_process{};
};

Options options;

struct Global
{
	std::chrono::time_point<std::chrono::high_resolution_clock> start_time = std::chrono::high_resolution_clock::now();

	system_snapshot::snapshot snapshot;

	pid_t child_pid = 0;
};

Global global;

void clean_end(const int signal, [[maybe_unused]] siginfo_t * const info, [[maybe_unused]] void * const context)
{
	if (std::cmp_equal(signal, SIGCHLD))
	{
		spdlog::info("Child process (PID {}) ended.", global.child_pid);
		global.child_pid = 0;
		exit(EXIT_SUCCESS);
	}
}

auto run_child(const std::string & command)
{
	const auto pid = fork();

	if (pid == 0)
	{
		// Child process
		execlp(command.c_str(), command.c_str(), nullptr);
	}
	else if (pid > 0)
	{
		// Parent process
		global.child_pid = pid;
		spdlog::info("Child process (PID {}) started.", global.child_pid);
		// Register signal handler
		struct sigaction action
		{};
		action.sa_sigaction = clean_end;
		sigemptyset(&action.sa_mask);
		action.sa_flags = SA_SIGINFO;
		sigaction(SIGCHLD, &action, nullptr);
	}
	else
	{
		// Error
		const auto err = errno;
		const auto msg = fmt::format("Failed to fork child process: Error {} ({})", err, strerror(err));
		spdlog::error(msg);
		throw std::runtime_error(msg);
	}
}

auto parse_options(const int argc, const char * argv[])
{
	CLI::App app{ "Demo of system_snapshot" };

	app.add_flag("-d,--debug", options.debug, "Debug output");
	app.add_flag("-m,--migration", options.migration, "Migrate child process to random CPU");

	app.add_option("-t,--time", options.time, "Time to run the demo for");
	app.add_option("-s,--dt", options.dt, "Time step for the demo");

	app.add_option("-r,--run", options.child_process, "Child process to run");

	CLI11_PARSE(app, argc, argv)

	if (options.debug) { spdlog::set_level(spdlog::level::debug); }

	if (not options.child_process.empty()) { run_child(options.child_process); }

	if (options.debug)
	{
		spdlog::debug("Options:");
		spdlog::debug("\tDebug: {}", options.debug);
		spdlog::debug("\tTime: {}", options.time);
		spdlog::debug("\tTime step: {}", options.dt);
		if (options.child_process.empty()) { spdlog::debug("\tChild process: None"); }
		else { spdlog::debug("\tChild process (PID {}): {}", global.child_pid, options.child_process); }
	}

	return EXIT_SUCCESS;
}

template<typename T>
auto format_seconds(const T & seconds)
{
	if (seconds > 100) { return fmt::format("{:.0f}s", seconds); }
	if (seconds > 10) { return fmt::format("{:.1f}s", seconds); }
	if (seconds > 1) { return fmt::format("{:.2f}s", seconds); }
	if (seconds > 1e-3) { return fmt::format("{:.0f}ms", seconds * 1e3); }
	if (seconds > 1e-6) { return fmt::format("{:.0f}us", seconds * 1e6); }
	return fmt::format("{:.0f}ns", seconds * 1e9);
}

auto keep_running()
{
	// Keep running until time is up
	const auto now     = std::chrono::high_resolution_clock::now();
	const auto elapsed = std::chrono::duration<double>(now - global.start_time).count();
	return elapsed < options.time;
}

template<typename F>
auto measure(F && f)
{
	const auto start = std::chrono::high_resolution_clock::now();
	f();
	const auto end = std::chrono::high_resolution_clock::now();
	return std::chrono::duration<double>(end - start).count();
}

void update_snapshot()
{
	const auto seconds = measure([&] { global.snapshot.update(); });
	spdlog::info("Snapshot update took {}.", format_seconds(seconds));
}

void show_NUMA_state()
{
	for (const auto node : global.snapshot.system_topology().nodes())
	{
		const auto pids_node = global.snapshot.pids_in_node(node) | ranges::to_vector | ranges::actions::sort;

		const auto node_use = global.snapshot.node_use(node);

		spdlog::info("Node {}: {} processes -> {:>.2f}% CPU use", node, pids_node.size(), node_use);
		spdlog::debug("\tPIDs: {}", fmt::join(pids_node, ", "));
	}
}

void show_CPU_state()
{
	for (const auto cpu : global.snapshot.system_topology().cpus())
	{
		const auto pids_cpu = global.snapshot.pids_in_cpu(cpu) | ranges::to_vector | ranges::actions::sort;

		const auto cpu_use = global.snapshot.cpu_use(cpu);

		spdlog::info("CPU {}: {} processes -> {}% CPU use", cpu, pids_cpu.size(), cpu_use);
		spdlog::debug("\tPIDs: {}", fmt::join(pids_cpu, ", "));
	}
}

void print_children_info()
{
	// If the child process does NOT exist, return -> Nothing to do...
	if (std::cmp_equal(global.child_pid, 0)) { return; }

	// Print information about the child process(es)
	spdlog::info("Child process(es):");
	const auto & processes = global.snapshot.processes();

	const auto & opt_child = processes.get(global.child_pid);

	if (not opt_child)
	{
		spdlog::info("\tPID {} does not exist anymore.", global.child_pid);
		return;
	}

	const auto & child         = opt_child->get();
	const auto & children_pids = child->children_and_tasks();

	for (const auto & child_pid : children_pids)
	{
		const auto & opt_proc = processes.get(child_pid);

		if (not opt_proc)
		{
			spdlog::info("\tPID {} does not exist anymore.", child_pid);
			continue;
		}

		const auto & proc = opt_proc->get();
		spdlog::info("\tPID {}. CPU {} at {:>.2f}%. \"{}\"", proc->pid(), proc->processor(), proc->cpu_use(),
		             proc->cmdline());
	}
}

void migrate_random_child()
{
	const auto & processes = global.snapshot.processes();

	const auto & proc_opt = processes.get(global.child_pid);

	if (not proc_opt)
	{
		spdlog::info("Child process (PID {}) does not exist anymore.", global.child_pid);
		return;
	}

	const auto & children_pids = proc_opt->get()->children_and_tasks();

	// Select a random CPU
	const auto cpu = static_cast<int>(*(global.snapshot.system_topology().cpus() | ranges::views::sample(1)).begin());
	const auto pid = children_pids.empty() ? global.child_pid : *(children_pids | ranges::views::sample(1)).begin();

	spdlog::info("Migrating child process (PID {}) to CPU {}...", pid, cpu);

	// Migrate the child process to the selected CPU
	global.snapshot.migrate_to_cpu(pid, cpu);
	global.snapshot.commit();

	spdlog::info("Child process (PID {}) migrated to CPU {}", pid, cpu);
}

auto main(const int argc, const char * argv[]) -> int
{
	try
	{
		std::ignore = parse_options(argc, argv);

		spdlog::info("Demo of system_snapshot");

		auto sleep_time = options.dt;

		while (keep_running())
		{
			using namespace std::literals::chrono_literals;
			std::this_thread::sleep_for(sleep_time * 1s);

			const auto loop_time = measure([&] {
				update_snapshot();

				show_NUMA_state();

				show_CPU_state();

				print_children_info();

				if (options.migration) { migrate_random_child(); }
			});

			sleep_time = options.dt - loop_time;
		}
	}
	catch (const std::exception & e)
	{
		spdlog::error("Exception: {}", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
