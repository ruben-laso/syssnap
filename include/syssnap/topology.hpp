#pragma once

#include <numa.h>

#include <concepts>
#include <string>
#include <utility>
#include <vector>

#include <range/v3/all.hpp>

#include <tabulate/table.hpp>

#include "types.hpp"

namespace syssnap
{
	class topology
	{
	private:
		std::vector<node_t> nodes_;
		std::vector<cpu_t>  cpus_;

		std::vector<std::vector<node_t>> nodes_by_distance_; // Contains the list of nodes sorted by distance
		// E.g. nodes_by_distance[1] = {1, 0, 2, 3} -> list of nodes, sorted by NUMA distance from node 1,
		// so 1 is the "closest" node (obviously), 0 is the closest neighbour, and 3 is the furthest neighbour

		// To know where each CPU is (in terms of memory node)
		std::vector<node_t>             cpu_node_map_; // input: CPU,  output: node
		std::vector<std::vector<cpu_t>> node_cpu_map_; // input: node, output: list of CPUs

		void detect_system_UMA()
		{
			nodes_ = { node_t{ 0 } };
			cpus_  = allowed_cpus();

			// disable implicit conversion temporarily
			// NOLINTBEGIN
			const auto size_cpus  = static_cast<std::size_t>(max_cpu() + 1);
			const auto size_nodes = static_cast<std::size_t>(max_node() + 1);
			// NOLINTEND

			node_cpu_map_.resize(size_nodes, {});
			node_cpu_map_.at(0) = cpus_;

			cpu_node_map_.resize(size_cpus, nodes_.front());

			// Compute the lists of nodes sorted by distance from a given node...
			nodes_by_distance_.resize(size_nodes, {});
			// libnuma is not available, so the distance is always 1
			nodes_by_distance_.at(0) = { node_t{ 1 } };
		}

		void detect_system_NUMA()
		{
			nodes_ = allowed_nodes();
			cpus_  = allowed_cpus();

			const auto size_cpus  = static_cast<std::size_t>(max_cpu() + 1);  // NOLINT
			const auto size_nodes = static_cast<std::size_t>(max_node() + 1); // NOLINT

			node_cpu_map_.resize(size_nodes, {});
			for (const auto node : nodes_)
			{
				node_cpu_map_.at(idx(node)) = detect_cpus_from_node(node);
				if (node_cpu_map_.at(idx(node)).empty())
				{
					std::stringstream error;
					error << "Error retrieving cpus from node " << node << ": " << strerror(errno);
					throw std::runtime_error(error.str());
				}
			}

			// For each CPU, reads topology file to get package (node) id
			cpu_node_map_.resize(size_cpus, 0);
			for (const auto cpu : cpus_)
			{
				cpu_node_map_.at(idx(cpu)) = numa_node_of_cpu(cpu);
			}

			// Compute the lists of nodes sorted by distance from a given node...
			nodes_by_distance_.resize(size_nodes, {});
			for (const auto node : nodes_)
			{
				std::multimap<int, int> distance_nodes_map{};

				for (const auto node_2 : nodes_)
				{
					distance_nodes_map.emplace(numa_distance(node, node_2), node_2);
				}

				// Vector of nodes sorted by distances
				std::vector<node_t> nodes_by_distance;
				nodes_by_distance.reserve(nodes_.size());

				for (const auto & node_2 : distance_nodes_map | ranges::views::values)
				{
					nodes_by_distance.emplace_back(node_2);
				}

				nodes_by_distance_.at(idx(node)) = nodes_by_distance;
			}
		}

		void detect_system()
		{
			if (std::cmp_less(numa_available(), 0)) { detect_system_UMA(); }
			else { detect_system_NUMA(); }
		}

	public:
		// Static functions
		[[nodiscard]] static auto max_node() -> node_t
		{
			static const auto MAX_NODE = node_t{ numa_max_node() };
			return MAX_NODE;
		}

		[[nodiscard]] static auto max_cpu() -> cpu_t
		{
			static const auto MAX_CPU = ranges::max(allowed_cpus());
			return MAX_CPU;
		}

		[[nodiscard]] static auto allowed_cpus() -> std::vector<cpu_t>
		{
			std::vector<cpu_t> allowed_cpus;

			for (const auto cpu : ranges::views::indices(0U, static_cast<uint32_t>(numa_all_cpus_ptr->size)))
			{
				if (std::cmp_not_equal(numa_bitmask_isbitset(numa_all_cpus_ptr, cpu), 0))
				{
					allowed_cpus.emplace_back(cpu);
				}
			}

			return allowed_cpus;
		}

		[[nodiscard]] static auto allowed_nodes() -> std::vector<node_t>
		{
			std::vector<node_t> allowed_nodes;

			auto * allowed_nodes_mask = numa_get_mems_allowed();

			for (const auto node : ranges::views::indices(0U, static_cast<uint32_t>(allowed_nodes_mask->size)))
			{
				if (std::cmp_not_equal(numa_bitmask_isbitset(allowed_nodes_mask, node), 0))
				{
					allowed_nodes.emplace_back(node);
				}
			}

			numa_bitmask_free(allowed_nodes_mask);

			return allowed_nodes;
		}

		[[nodiscard]] static auto detect_cpus_from_node(const node_t node) -> std::vector<cpu_t>
		{
			std::vector<cpu_t> cpus_in_node;

			bitmask * cpus_bm = numa_allocate_cpumask();

			if (std::cmp_equal(numa_node_to_cpus(static_cast<int>(node), cpus_bm), -1))
			{
				numa_free_cpumask(cpus_bm);
				std::stringstream error;
				error << "Error retrieving cpus from node " << node << ": " << strerror(errno);
				throw std::runtime_error(error.str());
			}

			const auto allowed_cpus = topology::allowed_cpus();

			for (const auto cpu : ranges::views::indices(0U, static_cast<uint32_t>(cpus_bm->size)))
			{
				if (std::cmp_not_equal(numa_bitmask_isbitset(cpus_bm, cpu), 0) and ranges::contains(allowed_cpus, cpu))
				{
					cpus_in_node.emplace_back(cpu);
				}
			}

			numa_free_cpumask(cpus_bm);

			return cpus_in_node;
		}

		topology() { detect_system(); }

		[[nodiscard]] auto num_of_cpus() const -> size_t { return cpus_.size(); }

		[[nodiscard]] auto num_of_nodes() const -> size_t { return nodes_.size(); }

		[[nodiscard]] auto cpus() const -> const std::vector<cpu_t> & { return cpus_; }

		[[nodiscard]] auto nodes() const -> const std::vector<node_t> & { return nodes_; }

		[[nodiscard]] auto node_cpu_map() const -> const std::vector<std::vector<cpu_t>> & { return node_cpu_map_; }

		[[nodiscard]] auto cpu_node_map() const -> const std::vector<node_t> & { return cpu_node_map_; }

		[[nodiscard]] auto nodes_by_distance() const -> const std::vector<std::vector<node_t>> &
		{
			return nodes_by_distance_;
		}

		[[nodiscard]] auto nodes_by_distance(const node_t node) const -> const std::vector<int> &
		{
			return nodes_by_distance_.at(idx(node));
		}

		[[nodiscard]] static auto node_distance(const node_t node_1, const node_t node_2) -> int
		{
			return numa_distance(node_1, node_2);
		}

		[[nodiscard]] auto cpus_from_node(const node_t node) const -> const auto &
		{
			return node_cpu_map_.at(idx(node));
		}

		[[nodiscard]] auto node_from_cpu(const cpu_t cpu) const -> node_t { return cpu_node_map_.at(idx(cpu)); }

		[[nodiscard]] auto ith_cpu_from_node(const node_t node, const std::unsigned_integral auto i) const -> int
		{
			return node_cpu_map_.at(node).at(i);
		}

		friend auto operator<<(std::ostream & os, const topology & topo) -> std::ostream &
		{
			os << "Detected system: " << topo.num_of_cpus() << " total CPUs, " << topo.num_of_nodes()
			   << " memory nodes." << '\n';

			os << '\n';

			// Print distance table
			os << "Nodes distance matrix:" << '\n';

			tabulate::Table distance_table;

			std::vector<std::string> nodes_str;
			nodes_str.reserve(topo.num_of_nodes() + 1);

			nodes_str.emplace_back("");

			for (const auto & node : topo.nodes())
			{
				nodes_str.emplace_back("Node " + std::to_string(node));
			}

			distance_table.add_row({ nodes_str.begin(), nodes_str.end() });

			for (const auto & n1 : topo.nodes())
			{
				std::vector<std::string> distances_str = {};
				distances_str.reserve(topo.nodes().size() + 1);

				distances_str.emplace_back("Node " + std::to_string(n1));

				for (const auto & n2 : topo.nodes())
				{
					distances_str.emplace_back(std::to_string(numa_distance(n1, n2)));
				}

				distance_table.add_row({ distances_str.begin(), distances_str.end() });
			}
			distance_table.format().hide_border();
			distance_table.print(os);

			os << '\n';

			// Print CPUs
			os << "NUMA node - CPU map: " << '\n';

			tabulate::Table node_cpus_table{};

			node_cpus_table.add_row({ "Node", "CPUs" });

			for (const auto & node : topo.nodes())
			{
				std::string cpus_str;

				for (const auto & cpu : topo.cpus_from_node(node))
				{
					cpus_str += std::to_string(cpu) + " ";
				}

				node_cpus_table.add_row({ std::to_string(node), cpus_str });
			}

			node_cpus_table.format().hide_border();
			node_cpus_table.print(os);

			os << '\n';

			return os;
		}
	};
} // namespace syssnap