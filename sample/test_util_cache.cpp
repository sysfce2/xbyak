/*
 * Test program for Xbyak CPU Cache Topology API
 * Demonstrates the CpuSet, CpuCache, LogicalCpu, and CpuMask classes
 */

#include <stdio.h>
#include <string.h>
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
	printf("CpuSet Class - System CPU topology\n");
	printSeparator();
	
	Cpu cpu;
	CpuSet cpuSet(cpu);
	
	printf("System Configuration:\n");
	printf("  Logical CPUs:   %zu\n", cpuSet.getLogicalCpuNum());
	printf("  Physical Cores: %zu\n", cpuSet.getPhysicalCoreNum());
	printf("  Sockets:        %zu\n", cpuSet.getSocketNum());
	printf("  Hybrid System:  %s\n", cpuSet.isHybrid() ? "Yes (P-cores + E-cores)" : "No");
	printf("\n");
}

void printLogicalCpuDetails()
{
	printSeparator();
	printf("LogicalCpu Class - Per-CPU topology information\n");
	printSeparator();
	
	Cpu cpu;
	CpuSet cpuSet(cpu);
	
	printf("Detailed CPU Topology:\n");
	for (size_t i = 0; i < cpuSet.getLogicalCpuNum(); i++) {
		const LogicalCpu& logCpu = cpuSet.getLogicalCpu(i);
		
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
	printf("\n");
}

void printCacheHierarchy()
{
	printSeparator();
	printf("CpuCache Class - Cache hierarchy and sharing\n");
	printSeparator();
	
	Cpu cpu;
	CpuSet cpuSet(cpu);
	
	if (cpuSet.getLogicalCpuNum() == 0) {
		printf("No CPU information available\n\n");
		return;
	}
	
	const char* cacheNames[] = {"L1i", "L1d", "L2", "L3"};
	
	// Print cache information for first CPU of each type
	printf("Cache Hierarchy Summary:\n");
	
	bool printedPcore = false;
	bool printedEcore = false;
	
	for (size_t cpuIdx = 0; cpuIdx < cpuSet.getLogicalCpuNum(); cpuIdx++) {
		const LogicalCpu& logCpu = cpuSet.getLogicalCpu(cpuIdx);
		
		// Print P-core cache once
		if (logCpu.coreType == Performance && !printedPcore) {
			printf("\nP-core (CPU %zu):\n", cpuIdx);
			printedPcore = true;
			
			for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
				const CpuCache& cache = cpuSet.getCache(cpuIdx, (CacheType)cType);
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
		
		// Print E-core cache once
		if (logCpu.coreType == Efficient && !printedEcore) {
			printf("\nE-core (CPU %zu):\n", cpuIdx);
			printedEcore = true;
			
			for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
				const CpuCache& cache = cpuSet.getCache(cpuIdx, (CacheType)cType);
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
		
		// For non-hybrid systems, print first CPU only
		if (logCpu.coreType == Standard && cpuIdx == 0) {
			printf("\nStandard CPU (CPU 0):\n");
			
			for (int cType = L1i; cType < CACHE_TYPE_NUM; cType++) {
				const CpuCache& cache = cpuSet.getCache(0, (CacheType)cType);
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
			break; // Only print once for standard systems
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
	CpuSet cpuSet(cpu);
	
	if (cpuSet.getLogicalCpuNum() == 0) {
		printf("No CPU information available\n\n");
		return;
	}
	
	// Analyze L3 cache sharing
	const CpuCache& l3 = cpuSet.getCache(0, L3);
	if (l3.size > 0) {
		printf("L3 Cache Information:\n");
		printf("  Size: ");
		if (l3.size >= 1024 * 1024) {
			printf("%.2f MB\n", l3.size / (1024.0 * 1024.0));
		} else {
			printf("%.2f KB\n", l3.size / 1024.0);
		}
		
		if (l3.isShared) {
			printf("  Shared by %zu CPUs: ", l3.getSharedCpuNum());
			bool first = true;
			int count = 0;
			for (CpuMask::iterator it = l3.sharedCpuIndices.begin(); 
			     it != l3.sharedCpuIndices.end(); ++it) {
				if (l3.sharedCpuIndices.get(*it)) {
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
			printf("  Private (not shared)\n");
		}
	}
	printf("\n");
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
