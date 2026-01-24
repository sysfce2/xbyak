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
	mask.set(0);
	mask.set(2);
	mask.set(5);
	mask.set(7);
	
	printf("Created CpuMask with CPUs: 0, 2, 5, 7\n");
	printf("Iterating through set CPUs:\n");
	for (CpuMask::iterator it = mask.begin(); it != mask.end(); ++it) {
		if (mask.get(*it)) {
			printf("  CPU %u is set\n", *it);
		}
	}
	printf("Total CPUs in mask: %u\n", mask.size());
	printf("\n");
}

void printSystemTopology()
{
	printSeparator();
	printf("CpuTopology Class - System CPU topology\n");
	printSeparator();
	
	Cpu cpu;
	CpuTopology cpuTopo(cpu);
	
	printf("System Configuration:\n");
	printf("  Logical CPUs:   %zu\n", cpuTopo.getLogicalCpuNum());
	printf("  Physical Cores: %zu\n", cpuTopo.getPhysicalCoreNum());
	printf("  Sockets:        %zu\n", cpuTopo.getSocketNum());
	printf("  Hybrid System:  %s\n", cpuTopo.isHybrid() ? "Yes (P-cores + E-cores)" : "No");
	printf("\n");
}

void printLogicalCpuDetails()
{
	printSeparator();
	printf("LogicalCpu Class - Per-CPU topology information\n");
	printSeparator();
	
	Cpu cpu;
	CpuTopology cpuTopo(cpu);
	
	printf("Detailed CPU Topology (showing upto 32 Logical CPUs):\n");
	size_t maxCpusToPrint = 32;
	size_t numCpus = cpuTopo.getLogicalCpuNum();
	
	for (size_t i = 0; i < numCpus && i < maxCpusToPrint; i++) {
		const LogicalCpu& logCpu = cpuTopo.getLogicalCpu(i);
		
		// Determine core type string
		const char* coreTypeStr;
		switch (logCpu.coreType) {
			case Unknown:     coreTypeStr = "Unknown"; break;
			case Performance: coreTypeStr = "P-core"; break;
			case Efficient:   coreTypeStr = "E-core"; break;
			case Standard:    coreTypeStr = "Standard"; break;
			default:          coreTypeStr = "Unknown"; break;
		}
		
		printf("  CPU %-2u: Socket=%u Core=%u Type=%-11s Siblings=[", 
			logCpu.index, logCpu.physicalId, logCpu.coreId, coreTypeStr);
		
		bool first = true;
		for (CpuMask::iterator it = logCpu.siblingIndices.begin(); 
		     it != logCpu.siblingIndices.end(); ++it) {
			if (logCpu.siblingIndices.get(*it)) {
				if (!first) printf(",");
				printf("%u", *it);
				first = false;
			}
		}
		printf("]\n");
	}
	
	if (numCpus > maxCpusToPrint) {
		printf("  ... (%zu more CPUs not shown)\n", numCpus - maxCpusToPrint);
	}
	printf("\n");
}

void printCacheHierarchy()
{
	printSeparator();
	printf("CpuCache Class - Cache hierarchy and sharing\n");
	printSeparator();
	
	Cpu cpu;
	CpuTopology cpuTopo(cpu);
	
	if (cpuTopo.getLogicalCpuNum() == 0) {
		printf("No CPU information available\n\n");
		return;
	}
	
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
		
		// Include core type in signature
		const char* coreTypeStr = "";
		switch (logCpu.coreType) {
			case Performance: coreTypeStr = "P-core"; break;
			case Efficient:   coreTypeStr = "E-core"; break;
			case Standard:    coreTypeStr = "Standard"; break;
			default:          coreTypeStr = "Unknown"; break;
		}
		offset += snprintf(signature + offset, sizeof(signature) - offset, "%s:", coreTypeStr);
		
		// Add cache properties to signature
		for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
			const CpuCache& cache = cpuTopo.getCache(cpuIdx, (CacheType)cType);
			offset += snprintf(signature + offset, sizeof(signature) - offset, 
				"%d-%u-%u-%u-%zu;", 
				cType, cache.size, cache.associativity, cache.lineSize, 
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
		const char* coreTypeStr = "";
		switch (logCpu.coreType) {
			case Performance: coreTypeStr = "P-core"; break;
			case Efficient:   coreTypeStr = "E-core"; break;
			case Standard:    coreTypeStr = "Standard"; break;
			default:          coreTypeStr = "Unknown"; break;
		}
		
		printf("\n%s (CPUs [", coreTypeStr);
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
				printf(" | %2u-way | Line: %2u bytes", 
					cache.associativity, cache.lineSize);
				if (cache.isShared) {
					printf(" | Shared by %zu CPUs", cache.getSharedCpuNum());
				}
				printf("\n");
			}
		}
	}
	printf("\n");
}

void printCacheSharingDetails()
{
	printSeparator();
	printf("Cache Sharing Analysis\n");
	printSeparator();
	
	Cpu cpu;
	CpuTopology cpuTopo(cpu);
	
	if (cpuTopo.getLogicalCpuNum() == 0) {
		printf("No CPU information available\n\n");
		return;
	}
	
	const char* cacheNames[] = {"L1i", "L1d", "L2", "L3"};
	
	// Create a map to group CPUs by their cache topology (same logic as printCacheHierarchy)
	std::map<std::string, std::vector<size_t> > topologyGroups;
	
	// Build cache topology signatures for each CPU
	for (size_t cpuIdx = 0; cpuIdx < cpuTopo.getLogicalCpuNum(); cpuIdx++) {
		const LogicalCpu& logCpu = cpuTopo.getLogicalCpu(cpuIdx);
		
		// Create a signature string that uniquely identifies the cache topology
		char signature[512];
		int offset = 0;
		
		// Include core type in signature
		const char* coreTypeStr = "";
		switch (logCpu.coreType) {
			case Performance: coreTypeStr = "P-core"; break;
			case Efficient:   coreTypeStr = "E-core"; break;
			case Standard:    coreTypeStr = "Standard"; break;
			default:          coreTypeStr = "Unknown"; break;
		}
		offset += snprintf(signature + offset, sizeof(signature) - offset, "%s:", coreTypeStr);
		
		// Add cache properties to signature
		for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
			const CpuCache& cache = cpuTopo.getCache(cpuIdx, (CacheType)cType);
			offset += snprintf(signature + offset, sizeof(signature) - offset, 
				"%d-%u-%u-%u-%zu;", 
				cType, cache.size, cache.associativity, cache.lineSize, 
				cache.getSharedCpuNum());
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
		
		// Print core type
		const char* coreTypeStr = "";
		switch (logCpu.coreType) {
			case Performance: coreTypeStr = "P-core"; break;
			case Efficient:   coreTypeStr = "E-core"; break;
			case Standard:    coreTypeStr = "Standard"; break;
			default:          coreTypeStr = "Unknown"; break;
		}
		
		printf("%s Topology (representative CPU %zu):\n", coreTypeStr, firstCpu);
		
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
				
				if (cache.isShared) {
					printf("    Shared by %zu CPUs: ", cache.getSharedCpuNum());
					bool first = true;
					int count = 0;
					for (CpuMask::iterator it = cache.sharedCpuIndices.begin(); 
					     it != cache.sharedCpuIndices.end(); ++it) {
						if (cache.sharedCpuIndices.get(*it)) {
							if (!first) printf(",");
							printf("%u", *it);
							first = false;
							count++;
							if (count > 20) {
								printf("...");
								break;
							}
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
{
	printf("\n");
	printf("Xbyak CPU Cache Topology API Test\n");
	printf("==================================\n");
	printf("\n");
	
	try {
		// Test each class
		printCpuMaskTest();
		printSystemTopology();
		printLogicalCpuDetails();
		printCacheHierarchy();
		printCacheSharingDetails();
		
		printSeparator();
		printf("All tests completed successfully!\n");
		printSeparator();
		printf("\n");
		
		return 0;
	} catch (...) {
		printf("Error: Exception occurred during testing\n");
		return 1;
	}
}
