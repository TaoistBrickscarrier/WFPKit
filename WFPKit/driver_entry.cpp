#include "global.h"
#include "wfp_kits.h"

ULONGLONG g_guilty_module_ = NULL;
ULONGLONG g_module_end_ = 0;
WFP_CALLOUT_TABLE_ENTRY* g_hooked_callout_entry_ = NULL;
FWPS_CALLOUT_CLASSIFY_FN1 g_origin_classify_fn_ = NULL;

VOID DriverUnload(PDRIVER_OBJECT driver) {
}

void NTAPI WarpClassify(FWPS_INCOMING_VALUES0* inFixedValues,
                        const FWPS_INCOMING_METADATA_VALUES0* inMetaValues,
                        void* layerData, const void* classifyContext,
                        const FWPS_FILTER1* filter, UINT64 flowContext,
                        FWPS_CLASSIFY_OUT0* classifyOut) {
  KdPrint(("I'm in!\r\n"));
  return g_origin_classify_fn_(inFixedValues, inMetaValues, layerData,
                               classifyContext, filter,
                               flowContext, classifyOut);
}
bool RoutineInModule(PVOID routine,
                     ULONGLONG module_start, ULONGLONG module_end) {
  return ((ULONGLONG)routine >= module_start &&
    (ULONGLONG)routine < module_end);
}

bool __stdcall EnumAndDisable(WFP_CALLOUT_TABLE_ENTRY* entry) {
  if (RoutineInModule(entry->ClassifyFn, g_guilty_module_, g_module_end_) ||
      RoutineInModule(entry->NotifyFn, g_guilty_module_, g_module_end_) ||
      RoutineInModule(entry->FlowDeleteFn, g_guilty_module_, g_module_end_)) {
    entry->is_valid = false;
  }
  return true;
}

bool __stdcall EnumAndReplace(WFP_CALLOUT_TABLE_ENTRY* entry) {
  if (entry->version == 1 && 
      entry->ClassifyFn != NULL &&
      RoutineInModule(entry->ClassifyFn, g_guilty_module_, g_module_end_)) {
    g_origin_classify_fn_ = (FWPS_CALLOUT_CLASSIFY_FN1)entry->ClassifyFn;
    entry->ClassifyFn = (PVOID)WarpClassify;
    // Only replace the first one.
    return false;
  }
  return true;
}


#ifdef __cplusplus
extern "C"
#endif // __cplusplus
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path) {
  driver->DriverUnload = DriverUnload;

  if (WFPKitInitialize()) {
#if DBG
    WFPKitListCallbacks();
#endif // DBG
  }

  PVOID guilty_module = NULL;
  ULONG module_size = 0;
  guilty_module = KernelGetModuleHandle("SangforTcpDrv.sys", &module_size);
  if (guilty_module == NULL || module_size == 0) {
    KdPrint(("Guilty module not loaded!"));
    return STATUS_SUCCESS;
  }

  g_guilty_module_ = (ULONGLONG)guilty_module;
  g_module_end_ = g_guilty_module_ + module_size;

  // WFPKitEnumCallouts(EnumAndDisable);
  KdBreakPoint();
  WFPKitEnumCallouts(EnumAndReplace);

  return STATUS_SUCCESS; 
}