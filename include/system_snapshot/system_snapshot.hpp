#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "topology.hpp"

#include "proc_watcher/proc_watcher.hpp"

namespace system_snapshot
{
	class snapshot
	{
		template<typename... args>
		using fast_uset = std::unordered_set<args...>;

		template<typename... args>
		using fast_umap = std::unordered_map<args...>;

	private:
		topology topology_{};

		proc_watcher::process_tree processes_{};

		// To know where each PID is (in terms of CPUs and node)
		std::vector<fast_uset<pid_t>> cpu_pid_map_;  // input: CPU,  output: list of TIDs
		std::vector<fast_uset<pid_t>> node_pid_map_; // input: node, output: list of TIDs

		// Cache of the CPU and node of each PID
		fast_umap<pid_t, int> pid_cpu_map_;  // input: TID,  output: CPU
		fast_umap<pid_t, int> pid_node_map_; // input: TID,  output: node

		std::vector<float> cpu_use_;  // input: CPU,  output: use
		std::vector<float> node_use_; // input: node, output: use

		mutable bool dirty_{ false };

		// To know where each PID is (in terms of CPUs and node)
		std::vector<fast_uset<pid_t>> dirty_cpu_pid_map_;  // input: CPU,  output: list of TIDs
		std::vector<fast_uset<pid_t>> dirty_node_pid_map_; // input: node, output: list of TIDs

		// Cache of the CPU and node of each PID
		fast_umap<pid_t, int> dirty_pid_cpu_map_;  // input: TID,  output: CPU
		fast_umap<pid_t, int> dirty_pid_node_map_; // input: TID,  output: node

		std::vector<float> dirty_cpu_use_;  // input: CPU,  output: use
		std::vector<float> dirty_node_use_; // input: node, output: use

		fast_umap<pid_t, int> cpu_migrations_;  // input: PID, output: destination CPU
		fast_umap<pid_t, int> node_migrations_; // input: PID, output: destination node

		void rebuild()
		{
			// Reset variables
			ranges::fill(cpu_pid_map_, fast_uset<pid_t>{});
			ranges::fill(node_pid_map_, fast_uset<pid_t>{});

			pid_cpu_map_.clear();
			pid_node_map_.clear();

			ranges::fill(cpu_use_, 0.0F);
			ranges::fill(node_use_, 0.0F);

			// Build the maps
			for (const auto & proc : processes_)
			{
				const auto pid  = proc.pid();
				const auto cpu  = proc.processor();
				const auto node = proc.numa_node();

				cpu_pid_map_[cpu].insert(pid);
				node_pid_map_[node].insert(pid);

				pid_cpu_map_[pid]  = cpu;
				pid_node_map_[pid] = node;

				cpu_use_[cpu] += proc.cpu_use();
				node_use_[node] += proc.cpu_use();
			}

			// Update the dirty stuff
			dirty_cpu_pid_map_  = cpu_pid_map_;
			dirty_node_pid_map_ = node_pid_map_;

			dirty_pid_cpu_map_  = pid_cpu_map_;
			dirty_pid_node_map_ = pid_node_map_;

			dirty_cpu_use_  = cpu_use_;
			dirty_node_use_ = node_use_;
		}

		void build()
		{
			// Resize stuff
			cpu_pid_map_.resize(topology::max_cpu() + 1, {});
			node_pid_map_.resize(topology::max_node() + 1, {});

			dirty_cpu_pid_map_.resize(topology::max_cpu() + 1, {});
			dirty_node_pid_map_.resize(topology::max_node() + 1, {});

			cpu_use_.resize(topology::max_cpu() + 1, 0.0F);
			node_use_.resize(topology::max_node() + 1, 0.0F);

			dirty_cpu_use_.resize(topology::max_cpu() + 1, 0.0F);
			dirty_node_use_.resize(topology::max_node() + 1, 0.0F);

			rebuild();
		}

		void pin_pid_to_cpu(const pid_t pid, const int cpu) { processes_.pin_processor(pid, cpu); }

		void pin_pid_to_node(const pid_t pid, const int node) { processes_.pin_numa_node(pid, node); }

	public:
		// ----------------
		// Static functions
		// ----------------
		static auto priority_to_weight(const size_t priority = 20)
		{
			static constexpr std::array<int, 40> sched_prio_to_weight = {
				/* -20 */ 88761, 71755, 56483, 46273, 36291,
				/* -15 */ 29154, 23254, 18705, 14949, 11916,
				/* -10 */ 9548,  7620,  6100,  4904,  3906,
				/*  -5 */ 3121,  2501,  1991,  1586,  1277,
				/*   0 */ 1024,  820,   655,   526,   423,
				/*   5 */ 335,   272,   215,   172,   137,
				/*  10 */ 110,   87,    70,    56,    45,
				/*  15 */ 36,    29,    23,    18,    15,
			};

			return sched_prio_to_weight.at(priority);
		}

		// ----------------

		snapshot() { build(); }

		[[nodiscard]] auto system_topology() const -> const auto & { return topology_; }

		[[nodiscard]] auto processes() const -> const auto & { return processes_; }

		void update()
		{
			// Update the process tree
			processes_.update();

			// Rebuild the snapshot
			rebuild();
		}

		void commit()
		{
			// If the snapshot is not dirty (nothing to change), do nothing
			if (not dirty_) { return; }

			for (const auto & [pid, cpu] : cpu_migrations_)
			{
				pin_pid_to_cpu(pid, cpu);
			}

			for (const auto & [pid, node] : node_migrations_)
			{
				pin_pid_to_node(pid, node);
			}

			cpu_migrations_.clear();
			node_migrations_.clear();

			dirty_ = false;

			update();
		}

		void rollback()
		{
			cpu_migrations_.clear();
			node_migrations_.clear();

			dirty_cpu_pid_map_  = cpu_pid_map_;
			dirty_node_pid_map_ = node_pid_map_;

			dirty_pid_cpu_map_  = pid_cpu_map_;
			dirty_pid_node_map_ = pid_node_map_;

			dirty_cpu_use_  = cpu_use_;
			dirty_node_use_ = node_use_;

			dirty_ = false;
		}

		[[nodiscard]] auto processor(const pid_t pid) const { return dirty_pid_cpu_map_.at(pid); }

		[[nodiscard]] auto original_processor(const pid_t pid) const { return pid_cpu_map_.at(pid); }

		[[nodiscard]] auto numa_node(const pid_t pid) const { return dirty_pid_node_map_.at(pid); }

		[[nodiscard]] auto original_numa_node(const pid_t pid) const { return pid_node_map_.at(pid); }

		[[nodiscard]] auto pids_in_cpu(const int cpu) const -> const auto & { return dirty_cpu_pid_map_.at(cpu); }

		[[nodiscard]] auto pids_in_node(const int node) const -> const auto & { return dirty_node_pid_map_.at(node); }

		[[nodiscard]] auto original_pids_in_cpu(const int cpu) const -> const auto & { return cpu_pid_map_.at(cpu); }

		[[nodiscard]] auto original_pids_in_node(const int node) const -> const auto &
		{
			return node_pid_map_.at(node);
		}

		[[nodiscard]] auto cpu_use(const int cpu) const { return cpu_use_.at(cpu); }

		[[nodiscard]] auto node_use(const int node) const { return node_use_.at(node); }

		void migrate_to_cpu(const pid_t pid, const int cpu)
		{
			dirty_ = true;

			const auto node = topology_.node_from_cpu(cpu);

			const auto old_cpu  = dirty_pid_cpu_map_.at(pid);
			const auto old_node = dirty_pid_node_map_.at(pid);

			// Update the dirty maps
			dirty_cpu_pid_map_.at(old_cpu).erase(pid);
			dirty_node_pid_map_.at(old_node).erase(pid);

			dirty_cpu_pid_map_.at(cpu).insert(pid);
			dirty_node_pid_map_.at(node).insert(pid);

			// Update the dirty usage
			const auto cpu_use = processes_.cpu_use(pid) / 100.0F;

			dirty_cpu_use_.at(old_cpu) -= cpu_use;
			dirty_cpu_use_.at(cpu) += cpu_use;

			dirty_node_use_.at(old_node) -= cpu_use;
			dirty_node_use_.at(node) += cpu_use;

			cpu_migrations_[pid] = cpu;
		}

		void migrate_to_node(const pid_t pid, const int node)
		{
			dirty_ = true;

			const auto cpu = *(topology_.cpus_from_node(node) | ranges::view::sample(1)).begin();

			const auto old_cpu  = dirty_pid_cpu_map_.at(pid);
			const auto old_node = dirty_pid_node_map_.at(pid);

			// Update the dirty maps
			dirty_cpu_pid_map_.at(old_cpu).erase(pid);
			dirty_node_pid_map_.at(old_node).erase(pid);

			dirty_cpu_pid_map_.at(cpu).insert(pid);
			dirty_node_pid_map_.at(node).insert(pid);

			// Update the dirty usage
			const auto cpu_use = processes_.cpu_use(pid) / 100.0F;

			dirty_cpu_use_.at(old_cpu) -= cpu_use;
			dirty_cpu_use_.at(cpu) += cpu_use;

			dirty_node_use_.at(old_node) -= cpu_use;
			dirty_node_use_.at(node) += cpu_use;

			node_migrations_[pid] = node;
		}

		void unpin(const pid_t pid) { processes_.unpin(pid); }

		void unpin() { processes_.unpin(); }
	};
} // namespace system_snapshot