#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <range/v3/all.hpp>

#include <prox/prox.hpp>

#include "topology.hpp"
#include "types.hpp"

namespace syssnap
{
	class snapshot
	{
		template<typename... args>
		using fast_uset = std::unordered_set<args...>;

		template<typename... args>
		using fast_umap = std::unordered_map<args...>;

	private:
		topology topology_{};

		prox::process_tree processes_{};

		// To know where each PID is (in terms of CPUs and node)
		std::vector<fast_uset<pid_t>> cpu_pid_map_;  // input: CPU,  output: list of TIDs
		std::vector<fast_uset<pid_t>> node_pid_map_; // input: node, output: list of TIDs

		// Cache of the CPU and node of each PID
		fast_umap<pid_t, cpu_t>  pid_cpu_map_;  // input: TID, output: CPU
		fast_umap<pid_t, node_t> pid_node_map_; // input: TID, output: node

		// Load of each PID
		fast_umap<pid_t, float> pid_load_map_; // input: TID, output: load

		std::vector<float> cpu_use_;  // input: CPU,  output: use
		std::vector<float> node_use_; // input: node, output: use

		mutable bool dirty_{ false };

		// To know where each PID is (in terms of CPUs and node)
		std::vector<fast_uset<pid_t>> dirty_cpu_pid_map_;  // input: CPU,  output: list of TIDs
		std::vector<fast_uset<pid_t>> dirty_node_pid_map_; // input: node, output: list of TIDs

		// Cache of the CPU and node of each PID
		fast_umap<pid_t, cpu_t>  dirty_pid_cpu_map_;  // input: TID,  output: CPU
		fast_umap<pid_t, node_t> dirty_pid_node_map_; // input: TID,  output: node

		std::vector<float> dirty_cpu_use_;  // input: CPU,  output: use
		std::vector<float> dirty_node_use_; // input: node, output: use

		fast_umap<pid_t, cpu_t>  cpu_migrations_;  // input: PID, output: destination CPU
		fast_umap<pid_t, node_t> node_migrations_; // input: PID, output: destination node

		template<typename Map>
		void compute_load_sigmoid(const Map & pid_usage_map)
		{
			static const auto load = [](const float cpu_use, const float slice) {
				return std::min(1.0F, cpu_use / slice);
			};

			// S-shape function between 0 and 1
			static const auto weight = [](const float x) {
				// If x ~ 0, return 0
				if (x < std::numeric_limits<decltype(x)>::epsilon()) { return 0.0F; }
				// If x ~ 1, return 1
				if (x > 1.0F - std::numeric_limits<decltype(x)>::epsilon()) { return 1.0F; }
				// Else, compute the function
				constexpr auto beta = 3.0F;
				return 1.0F / (1.0F + std::pow((x / (1.0F - x)), -beta));
			};

			// Generate a vector of 101 values such that v[i] = s_shape(i / 100);
			static const auto weight_table = []() {
				std::array<float, 101> table{};

				for (size_t i = 0; i < table.size(); i++)
				{
					table.at(i) = weight(static_cast<float>(i) / 100.0F);
				}

				return table;
			}();

			const auto free_cpu_use =
			    std::clamp(100.0F - ranges::accumulate(pid_usage_map | ranges::views::values, 0.0F), 0.0F, 100.0F);
			const auto max_cpu_use = ranges::max(pid_usage_map | ranges::views::values);

			const auto cpu_index = static_cast<int>(std::round(free_cpu_use));

			const auto alpha = weight_table.at(idx(cpu_index));
			const auto beta  = 1 - alpha;

			for (const auto & [pid, cpu_use] : pid_usage_map)
			{
				const auto load_vs_free = load(cpu_use, free_cpu_use);
				const auto load_vs_max  = load(cpu_use, max_cpu_use);

				const auto pid_load = alpha * load_vs_free + beta * load_vs_max;

				pid_load_map_[pid] = pid_load;
			}
		}

		void compute_loads(const cpu_t cpu)
		{
			const auto & pids = cpu_pid_map_.at(idx(cpu));

			auto pid_usage_map = pids | ranges::views::transform([&](const auto pid) {
				                     return std::pair<pid_t, float>{ pid, processes_.cpu_use(pid) };
			                     });

			compute_load_sigmoid(pid_usage_map);
		}

		void compute_loads()
		{
			for (const auto cpu : topology_.cpus())
			{
				compute_loads(cpu);
			}
		}

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

				cpu_pid_map_.at(idx(cpu)).insert(pid);
				node_pid_map_.at(idx(node)).insert(pid);

				pid_cpu_map_[pid]  = cpu;
				pid_node_map_[pid] = node;

				cpu_use_.at(idx(cpu)) += proc.cpu_use();
				node_use_.at(idx(node)) += proc.cpu_use();
			}

			// Update the dirty stuff
			dirty_cpu_pid_map_  = cpu_pid_map_;
			dirty_node_pid_map_ = node_pid_map_;

			dirty_pid_cpu_map_  = pid_cpu_map_;
			dirty_pid_node_map_ = pid_node_map_;

			dirty_cpu_use_  = cpu_use_;
			dirty_node_use_ = node_use_;

			compute_loads();
		}

		void build()
		{
			const auto size_cpus  = static_cast<std::size_t>(topology::max_cpu()) + 1;
			const auto size_nodes = static_cast<std::size_t>(topology::max_node()) + 1;

			assert(std::cmp_greater(size_cpus, 0));
			assert(std::cmp_greater(size_nodes, 0));

			// Resize stuff
			cpu_pid_map_.resize(size_cpus, {});
			node_pid_map_.resize(size_nodes, {});

			dirty_cpu_pid_map_.resize(size_cpus, {});
			dirty_node_pid_map_.resize(size_nodes, {});

			cpu_use_.resize(size_cpus, 0.0F);
			node_use_.resize(size_nodes, 0.0F);

			dirty_cpu_use_.resize(size_cpus, 0.0F);
			dirty_node_use_.resize(size_nodes, 0.0F);

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

		[[nodiscard]] auto process(const pid_t pid) -> auto & { return processes_.find(pid); }

		[[nodiscard]] auto process(const pid_t pid) const -> const auto & { return processes_.find(pid); }

		[[nodiscard]] auto processor(const pid_t pid) const { return dirty_pid_cpu_map_.at(pid); }

		[[nodiscard]] auto original_processor(const pid_t pid) const { return pid_cpu_map_.at(pid); }

		[[nodiscard]] auto numa_node(const pid_t pid) const { return dirty_pid_node_map_.at(pid); }

		[[nodiscard]] auto original_numa_node(const pid_t pid) const { return pid_node_map_.at(pid); }

		[[nodiscard]] auto pids_in_cpu(const cpu_t cpu) const -> const auto &
		{
			return dirty_cpu_pid_map_.at(idx(cpu));
		}

		[[nodiscard]] auto pids_in_node(const node_t node) const -> const auto &
		{
			return dirty_node_pid_map_.at(idx(node));
		}

		[[nodiscard]] auto original_pids_in_cpu(const cpu_t cpu) const -> const auto &
		{
			return cpu_pid_map_.at(idx(cpu));
		}

		[[nodiscard]] auto original_pids_in_node(const node_t node) const -> const auto &
		{
			return node_pid_map_.at(idx(node));
		}

		[[nodiscard]] auto cpu_use(const cpu_t cpu) const { return cpu_use_.at(idx(cpu)); }

		[[nodiscard]] auto node_use(const node_t node) const { return node_use_.at(idx(node)); }

		[[nodiscard]] auto load_of(const pid_t pid) const { return pid_load_map_.at(pid); }

		[[nodiscard]] auto load_of_cpu(const cpu_t cpu) const
		{
			return ranges::accumulate(
			    pids_in_cpu(cpu) | ranges::views::transform([&](const auto pid) { return load_of(pid); }), 0.0F);
		}

		[[nodiscard]] auto load_of_node(const node_t node) const
		{
			return ranges::accumulate(
			    pids_in_node(node) | ranges::views::transform([&](const auto pid) { return load_of(pid); }), 0.0F);
		}

		[[nodiscard]] auto load_system() const
		{
			return ranges::accumulate(pid_load_map_ | ranges::views::values, 0.0F);
		}

		void migrate_to_cpu(const pid_t pid, const cpu_t cpu)
		{
			dirty_ = true;

			const auto node = topology_.node_from_cpu(cpu);

			const auto old_cpu  = dirty_pid_cpu_map_.at(pid);
			const auto old_node = dirty_pid_node_map_.at(pid);

			// Update the dirty maps
			dirty_cpu_pid_map_.at(idx(old_cpu)).erase(pid);
			dirty_node_pid_map_.at(idx(old_node)).erase(pid);

			dirty_cpu_pid_map_.at(idx(cpu)).insert(pid);
			dirty_node_pid_map_.at(idx(node)).insert(pid);

			// Update the dirty usage
			const auto cpu_use = processes_.cpu_use(pid) / 100.0F;

			dirty_cpu_use_.at(idx(old_cpu)) -= cpu_use;
			dirty_cpu_use_.at(idx(cpu)) += cpu_use;

			dirty_node_use_.at(idx(old_node)) -= cpu_use;
			dirty_node_use_.at(idx(node)) += cpu_use;

			cpu_migrations_[pid] = cpu;
		}

		void migrate_to_node(const pid_t pid, const node_t node)
		{
			dirty_ = true;

			const auto cpu = *(topology_.cpus_from_node(node) | ranges::views::sample(1)).begin();

			const auto old_cpu  = dirty_pid_cpu_map_.at(pid);
			const auto old_node = dirty_pid_node_map_.at(pid);

			// Update the dirty maps
			dirty_cpu_pid_map_.at(idx(old_cpu)).erase(pid);
			dirty_node_pid_map_.at(idx(old_node)).erase(pid);

			dirty_cpu_pid_map_.at(idx(cpu)).insert(pid);
			dirty_node_pid_map_.at(idx(node)).insert(pid);

			// Update the dirty usage
			const auto cpu_use = processes_.cpu_use(pid) / 100.0F;

			dirty_cpu_use_.at(idx(old_cpu)) -= cpu_use;
			dirty_cpu_use_.at(idx(cpu)) += cpu_use;

			dirty_node_use_.at(idx(old_node)) -= cpu_use;
			dirty_node_use_.at(idx(node)) += cpu_use;

			node_migrations_[pid] = node;
		}

		void unpin(const pid_t pid) { processes_.unpin(pid); }

		void unpin() { processes_.unpin(); }
	};
} // namespace syssnap