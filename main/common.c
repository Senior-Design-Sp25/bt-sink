#include "esp_heap_caps.h"

void print_initial_memory() {
    printf("Initial Memory Status:\n");
    printf("Total Free DRAM: %d KB\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL)/1024);
    printf("Largest Free DRAM Block: %d KB\n", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)/1024);

    printf("Total Free PSRAM: %d KB\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/1024);
    printf("Largest Free PSRAM Block: %d KB\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)/1024);
}


void print_memory_usage() {
    size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL)/1024;
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM)/1024;
    size_t total_free = free_dram + free_psram;

    printf("\nCurrent Memory Usage:\n");
    printf("Free DRAM: %d KB\n", free_dram);
    printf("Free PSRAM: %d KB\n", free_psram);
    printf("Total Free Memory: %d KB\n", total_free);
}

void print_max_ram() {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  printf("\nRam high-water mark:\n");
  printf("Min free DRAM: %u KB\n", info.minimum_free_bytes/1024);
}