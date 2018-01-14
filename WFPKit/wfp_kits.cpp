#include "global.h"
#include "wfp_kits.h"

namespace {

ULONG g_callout_count_ = 0;
WFP_CALLOUT_TABLE_ENTRY* g_callout_array_ = NULL;

}

bool WFPKitInitialize() {
  //
  // 从NETIO!KfdGetOffloadEpoch中获取NETIO!gWfpGlobal。
  // KfdGetOffloadEpoch代码如下：
  // 488b05 XX XX XX XX mov rax, gWfpGlobal
  //

  PUCHAR ptr_KfdGetOffloadEpoch = NULL;
  PUCHAR netio_gWfpGlobal = NULL;
  ULONG image_size = 0;
  PVOID netio_sys = KernelGetModuleHandle("netio.sys", &image_size);
  if (netio_sys == NULL) {
    return false;
  }
  ptr_KfdGetOffloadEpoch = (PUCHAR)KernelGetProcAddress(netio_sys, image_size,
                                                        "KfdGetOffloadEpoch");
  if (ptr_KfdGetOffloadEpoch == NULL) {
    KdPrint(("NO KfdGetOffloadEpoch found!\r\n"));
    return false;
  }
  if (ptr_KfdGetOffloadEpoch[0] != 0x48 ||
      ptr_KfdGetOffloadEpoch[1] != 0x8b ||
      ptr_KfdGetOffloadEpoch[2] != 0x05) {
    KdPrint(("KfdGetOffloadEpoch not match!\r\n"));
    return false;
  }

  netio_gWfpGlobal = ptr_KfdGetOffloadEpoch +
    *(PLONG32)(ptr_KfdGetOffloadEpoch + 3) + 7;
  if ((ULONGLONG)netio_gWfpGlobal >= (ULONGLONG)netio_sys + image_size) {
    return false;
  } else {
    netio_gWfpGlobal = *(PUCHAR*)netio_gWfpGlobal;
  }

  //
  // gWfpGlobal +548处为CallOut的数量，
  //            +550处为CallOut的列表。
  //
  g_callout_count_ = *(PULONG)(netio_gWfpGlobal + 0x548);
  g_callout_array_ = *(WFP_CALLOUT_TABLE_ENTRY**)(netio_gWfpGlobal + 0x550);

  return true;
}

#if DBG
void WFPKitListCallbacks() {
  if (g_callout_count_ == 0 || g_callout_array_ == NULL) {
    return;
  }

  WFP_CALLOUT_TABLE_ENTRY* callouts = g_callout_array_;
  for (ULONG i = 0; i < g_callout_count_; ++i, ++callouts) {
    if (callouts->is_valid == 0) {
      continue;
    }

    PVOID class_fn = callouts->ClassifyFn;
    if (class_fn == NULL) {
      continue;
    }

    DbgPrint("WFP version : %d routine : %llx\r\n",
             callouts->version, class_fn);
  }
}
#endif // DBG

void WFPKitEnumCallouts(WFPKitEnumRoutine_ptr check_routine) {
  if (check_routine == NULL ||
      g_callout_count_ == 0 ||
      g_callout_array_ == NULL) {
    return;
  }

  WFP_CALLOUT_TABLE_ENTRY* callouts = g_callout_array_;
  for (ULONG i = 0; i < g_callout_count_; ++i, ++callouts) {
    if (callouts->is_valid == 0 || callouts->is_deleting != 0) {
      continue;
    }

    if (!check_routine(callouts)) {
      break;
    }
  }
}