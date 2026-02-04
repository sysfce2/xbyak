/*
 * Test program for Xbyak CPU Cache Topology API
 * Demonstrates the CpuTopology, CpuCache, LogicalCpu, and CpuMask classes
 */

#include <stdio.h>
#include <string.h>
#include <map>
#include <vector>
#include "xbyak/xbyak_util.h"

using namespace Xbyak::util;

void printSeparator()
{
	printf("========================================\n");
}

void printCpuMaskTest()
{
	printSeparator();
	printf("CpuMask Class - CPU affinity mask operations\n");
	printSeparator();

	CpuMask mask;
	mask.append(0);
	mask.append(2);
	mask.append(5);
	mask.append(7);

	printf("Created CpuMask with CPUs: 0, 2, 5, 7\n");
	printf("Iterating through set CPUs:\n");
	for (CpuMask::iterator it = mask.begin(); it != mask.end(); ++it) {
		printf("  CPU %u is set\n", *it);
	}
	printf("Total CPUs in mask: %zu\n", mask.size());
	printf("\n");
}

void printSystemTopology(const CpuTopology& cpuTopo)
{
	printSeparator();
	printf("CpuTopology Class - System CPU topology\n");
	printSeparator();

	printf("System Configuration:\n");
	printf("  Logical CPUs:   %zu\n", cpuTopo.getLogicalCpuNum());
	printf("  Physical Cores: %zu\n", cpuTopo.getPhysicalCoreNum());
	printf("  Cache Line Size:%u bytes\n", cpuTopo.getLineSize());
	printf("  Hybrid System:  %s\n", cpuTopo.isHybrid() ? "Yes (P-cores + E-cores)" : "No");
	printf("\n");
}

const char *type2str(int coreType)
{
	switch (coreType) {
	case Performance: return "P-core";
	case Efficient: return "E-core";
	case Standard: return "Standard";
	default: return "Unknown";
	}
}

void printLogicalCpuDetails(const CpuTopology& cpuTopo)
{
	printSeparator();
	printf("LogicalCpu Class - Per-CPU topology information\n");
	printSeparator();

	printf("Detailed CPU Topology (showing upto 32 Logical CPUs):\n");
	size_t maxCpusToPrint = 32;
	size_t numCpus = cpuTopo.getLogicalCpuNum();

	for (size_t i = 0; i < numCpus && i < maxCpusToPrint; i++) {
		const LogicalCpu& logCpu = cpuTopo.getLogicalCpu(i);
		printf("  CPU %3zu: Core=%u Type=%s Siblings=", i, logCpu.coreId, type2str(logCpu.coreType));
		logCpu.getSiblings().put();
	}

	if (numCpus > maxCpusToPrint) {
		printf("  ... (%zu more CPUs not shown)\n", numCpus - maxCpusToPrint);
	}
	printf("\n");
}

void printCacheHierarchy(const CpuTopology& cpuTopo)
{
	printSeparator();
	printf("CpuCache Class - Cache hierarchy and sharing\n");
	printSeparator();

	const char* cacheNames[] = {"L1i", "L1d", "L2", "L3"};

	// Create a map to group CPUs by their cache topology
	// Key: cache topology signature, Value: list of CPUs with that topology
	std::map<std::string, std::vector<size_t> > topologyGroups;

	// Build cache topology signatures for each CPU
	for (size_t cpuIdx = 0; cpuIdx < cpuTopo.getLogicalCpuNum(); cpuIdx++) {
		const LogicalCpu& logCpu = cpuTopo.getLogicalCpu(cpuIdx);

		// Create a signature string that uniquely identifies the cache topology
		char signature[512];
		int offset = 0;
		offset += snprintf(signature + offset, sizeof(signature) - offset, "%s:", type2str(logCpu.coreType));

		// Add cache properties to signature
		for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
			const CpuCache& cache = cpuTopo.getCache(cpuIdx, (CacheType)cType);
			offset += snprintf(signature + offset, sizeof(signature) - offset,
				"%d-%u-%u-%zu;",
				cType, cache.size, cache.associativity,
				cache.getSharedCpuNum());
		}

		topologyGroups[signature].push_back(cpuIdx);
	}

	// Print each unique cache topology group
	printf("Cache Hierarchy by Topology:\n");

	for (std::map<std::string, std::vector<size_t> >::iterator groupIt = topologyGroups.begin();
	     groupIt != topologyGroups.end(); ++groupIt) {

		const std::vector<size_t>& cpuList = groupIt->second;
		if (cpuList.empty()) continue;

		size_t firstCpu = cpuList[0];
		const LogicalCpu& logCpu = cpuTopo.getLogicalCpu(firstCpu);

		// Print core type and CPU list
		printf("\n%s (CPUs [", type2str(logCpu.coreType));
		for (size_t i = 0; i < cpuList.size(); i++) {
			if (i > 0) printf(", ");
			printf("%zu", cpuList[i]);
			// Limit output to ~20 CPUs for readability
			if (i >= 19 && cpuList.size() > 20) {
				printf(", ...");
				break;
			}
		}
		printf("]):\n");

		// Print cache details for this topology
		for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
			const CpuCache& cache = cpuTopo.getCache(firstCpu, (CacheType)cType);
			if (cache.size > 0) {
				printf("  %s: ", cacheNames[cType]);
				if (cache.size >= 1024 * 1024) {
					printf("%6.2f MB", cache.size / (1024.0 * 1024.0));
				} else if (cache.size >= 1024) {
					printf("%6.2f KB", cache.size / 1024.0);
				} else {
					printf("%6u B ", cache.size);
				}
				printf(" | %2u-way", cache.associativity);
				if (cache.isShared()) {
					printf(" | Shared by %zu CPUs", cache.getSharedCpuNum());
				}
				printf("\n");
			}
		}
	}
	printf("\n");
}

void printCacheSharingDetails(const CpuTopology& cpuTopo)
{
	printSeparator();
	printf("Cache Sharing Analysis\n");
	printSeparator();

	const char* cacheNames[] = {"L1i", "L1d", "L2", "L3"};

	// Create a map to group CPUs by their cache topology (same logic as printCacheHierarchy)
	std::map<std::string, std::vector<size_t> > topologyGroups;

	// Build cache topology signatures for each CPU
	for (size_t cpuIdx = 0; cpuIdx < cpuTopo.getLogicalCpuNum(); cpuIdx++) {
		const LogicalCpu& logCpu = cpuTopo.getLogicalCpu(cpuIdx);

		// Create a signature string that uniquely identifies the cache topology
		char signature[512];
		int offset = 0;
		offset += snprintf(signature + offset, sizeof(signature) - offset, "%s:", type2str(logCpu.coreType));

		// Add cache properties to signature
		for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
			const CpuCache& cache = cpuTopo.getCache(cpuIdx, (CacheType)cType);
			offset += snprintf(signature + offset, sizeof(signature) - offset,
				"%d-%u-%u-%zu;",
				cType, cache.size, cache.associativity, cache.getSharedCpuNum());
		}

		topologyGroups[signature].push_back(cpuIdx);
	}

	// Print cache sharing analysis for each unique topology
	for (std::map<std::string, std::vector<size_t> >::iterator groupIt = topologyGroups.begin();
	     groupIt != topologyGroups.end(); ++groupIt) {

		const std::vector<size_t>& cpuList = groupIt->second;
		if (cpuList.empty()) continue;

		size_t firstCpu = cpuList[0];
		const LogicalCpu& logCpu = cpuTopo.getLogicalCpu(firstCpu);

		printf("%s Topology (representative CPU %zu):\n", type2str(logCpu.coreType), firstCpu);

		// Analyze each cache level
		for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
			const CpuCache& cache = cpuTopo.getCache(firstCpu, (CacheType)cType);
			if (cache.size > 0) {
				printf("  %s Cache:\n", cacheNames[cType]);
				printf("    Size: ");
				if (cache.size >= 1024 * 1024) {
					printf("%.2f MB\n", cache.size / (1024.0 * 1024.0));
				} else if (cache.size >= 1024) {
					printf("%.2f KB\n", cache.size / 1024.0);
				} else {
					printf("%u B\n", cache.size);
				}

				if (cache.isShared()) {
					printf("    Shared by %zu CPUs: ", cache.getSharedCpuNum());
					int count = 0;
					for (CpuMask::const_iterator i = cache.sharedCpuIndices.begin(), ie = cache.sharedCpuIndices.end(); i != ie; ++i) {
						if (count > 0) printf(",");
						printf("%u", *i);
						count++;
						if (count > 20) {
							printf("...");
							break;
						}
					}
					printf("\n");
				} else {
					printf("    Private (not shared)\n");
				}
			}
		}
		printf("\n");
	}
}

int main()
	try
{
	printf("\n");
	printf("Xbyak CPU Cache Topology API Test\n");
	printf("==================================\n");
	printf("\n");

	Cpu cpu;
	CpuTopology cpuTopo(cpu);

	printCpuMaskTest();
	printSystemTopology(cpuTopo);
	printLogicalCpuDetails(cpuTopo);
	printCacheHierarchy(cpuTopo);
	printCacheSharingDetails(cpuTopo);

	printSeparator();
	printf("All tests completed successfully!\n");
	printSeparator();
	printf("\n");
} catch (std::exception& e) {
	printf("Error: %s\n", e.what());
	return 1;
}
