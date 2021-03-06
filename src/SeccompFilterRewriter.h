/* -*- Mode: C++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#ifndef RR_SECCOMP_FILTER_REWRITER_H_
#define RR_SECCOMP_FILTER_REWRITER_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "core.h"

/**
 * When seccomp decides not to execute a syscall the kernel returns to userspace
 * without modifying the registers. There is no negative return value to
 * indicate that whatever side effects the syscall would happen did not take
 * place. This is a problem for rr, because for syscalls that require special
 * handling, we'll be performing that handling even though the syscall didn't
 * actually happen.
 *
 * To get around this we can use the same mechanism that is used to skip the
 * syscall in the kernel to skip it ourselves: original_syscallno. We can't
 * use the traditional value of -1 though, because the kernel initializes
 * original_syscallno to -1 when delivering signals, and exiting sigreturn
 * will restore that. Not recording the side effects of sigreturn would be
 * bad. Instead we use -2, which still causes skipping the syscall when
 * given to the kernel as original_syscallno, but is never generated by the
 * kernel itself.
 */
#define SECCOMP_MAGIC_SKIP_ORIGINAL_SYSCALLNO -2

namespace rr {

class RecordTask;

/**
 * Object to support install_patched_seccomp_filter.
 */
class SeccompFilterRewriter {
public:
  /**
   * Assuming |t| is set up for a prctl or seccomp syscall that
   * installs a seccomp-bpf filter, patch the filter to signal the tracer
   * instead of silently delivering an errno, and install it.
   */
  void install_patched_seccomp_filter(RecordTask* t);

  uint32_t map_filter_data_to_real_result(uint16_t value) {
    DEBUG_ASSERT(value < index_to_result.size());
    return index_to_result[value];
  }

private:
  /**
   * Seccomp filters can return 32-bit result values. We need to map all of
   * them into a single 16 bit data field. Fortunately (so far) all the
   * filters we've seen return constants, so there aren't too many distinct
   * values we need to deal with. For each constant value that gets returned,
   * we'll add it as the key in |result_map|, with the corresponding value
   * being the 16-bit data value that our rewritten filter returns.
   */
  std::unordered_map<uint32_t, uint16_t> result_to_index;
  std::vector<uint32_t> index_to_result;
};

} // namespace rr

#endif // RR_SECCOMP_FILTER_REWRITER_H_
