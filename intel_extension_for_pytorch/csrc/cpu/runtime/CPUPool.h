#pragma once
#include <dlfcn.h>
#include <omp.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <vector>

namespace torch_ipex {
namespace runtime {

typedef void* kmp_affinity_mask_t;
typedef void (*kmp_create_affinity_mask_p)(kmp_affinity_mask_t*);
typedef int (*kmp_set_affinity_mask_proc_p)(int, kmp_affinity_mask_t*);
typedef int (*kmp_set_affinity_p)(kmp_affinity_mask_t*);
typedef void (*kmp_destroy_affinity_mask_p)(kmp_affinity_mask_t*);
typedef int (*kmp_get_affinity_p)(kmp_affinity_mask_t*);
typedef int (*kmp_get_affinity_max_proc_p)();

class CPUPool {
 public:
  explicit CPUPool(const std::vector<int32_t>& cpu_core_list);
  explicit CPUPool(std::vector<kmp_affinity_mask_t>&& cpu_core_mask);
  CPUPool(CPUPool&& source_cpu_pool);

  const std::vector<int32_t>& get_cpu_core_list() const;
  const std::vector<kmp_affinity_mask_t>& get_cpu_affinity_mask() const;
  bool is_cpu_core_list_initialized() const;
  bool is_cpu_affinity_mask_initialized() const;
  ~CPUPool();

 private:
  // CPUPool has 2 kinds of expression: 1. cpu_core_list 2. cpu_affinity_mask
  // Notice: only one of these 2 expressions allow to use for specific CPUPool
  // object.
  std::vector<int32_t> cpu_core_list;
  bool cpu_core_list_initialized_{false};
  std::vector<kmp_affinity_mask_t> cpu_affinity_mask;
  bool cpu_affinity_mask_initialized_{false};

  // Put deleted function into private.
  CPUPool() = delete;
  CPUPool(const CPUPool& source_cpu_pool) =
      delete; // avoid potential risk of double destory masks.
  CPUPool& operator=(const CPUPool& source_cpu_pool) =
      delete; // avoid potential risk of double destory masks.
  CPUPool& operator=(CPUPool&& source_cpu_pool) = delete;
};

std::vector<int32_t> init_process_available_cores();
std::vector<int32_t> get_process_available_cores();
std::vector<int32_t> filter_cores_by_thread_affinity(
    const std::vector<int32_t>& cpu_core_list);
bool do_load_iomp_symbol();
bool is_runtime_ext_enabled();
void init_runtime_ext();
void _pin_cpu_cores(const torch_ipex::runtime::CPUPool& cpu_pool);
bool is_same_core_affinity_setting(const std::vector<int32_t>& cpu_core_list);
CPUPool get_cpu_pool_from_mask_affinity();
void set_mask_affinity_from_cpu_pool(const CPUPool& cpu_pool);

class WithCPUPool {
 public:
  explicit WithCPUPool(CPUPool&& cpu_pool)
      : previous_cpu_pool(
            torch_ipex::runtime::get_cpu_pool_from_mask_affinity()),
        current_cpu_pool(std::move(cpu_pool)) {
    torch_ipex::runtime::_pin_cpu_cores(current_cpu_pool);
  }

  ~WithCPUPool() {
    torch_ipex::runtime::set_mask_affinity_from_cpu_pool(
        this->previous_cpu_pool);
  }

 private:
  CPUPool previous_cpu_pool;
  CPUPool current_cpu_pool;

  WithCPUPool() = delete;
  WithCPUPool(const WithCPUPool& cpu_pool_guard) = delete;
  WithCPUPool& operator=(const WithCPUPool& cpu_pool_guard) = delete;
  WithCPUPool(WithCPUPool&& cpu_pool_guard) = delete;
  WithCPUPool& operator=(WithCPUPool&& cpu_pool_guard) = delete;
};

} // namespace runtime
} // namespace torch_ipex
