#ifndef _WFP_KITS_H_
#define _WFP_KITS_H_

struct WFP_CALLOUT_TABLE_ENTRY {
  // 版本，决定了ClassifyFn/NotifyFn/FlowDeleteFn的工作模式。
  ULONG version;
  // ProcessCallout以这个参数来检查是否有效。
  ULONG is_valid;
  ULONG is_deleting;
  ULONG unkown_1;
  PVOID ClassifyFn; // FWPS_CALLOUT_CLASSIFY_FN0 / FWPS_CALLOUT_CLASSIFY_FN1
  PVOID NotifyFn; // FWPS_CALLOUT_NOTIFY_FN0 / FWPS_CALLOUT_NOTIFY_FN1
  PVOID FlowDeleteFn; // FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN0 / FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN1
  ULONG flags;
  ULONG external_flow_ref_count;
  ULONG active_callout_ref_count;
  ULONG unknown_2;
  PDEVICE_OBJECT device_object;
};

bool WFPKitInitialize();

#if DBG
void WFPKitListCallbacks();
#endif // DBG

typedef bool(__stdcall * WFPKitEnumRoutine_ptr)(WFP_CALLOUT_TABLE_ENTRY*);

void WFPKitEnumCallouts(WFPKitEnumRoutine_ptr check_routine);

#endif // !_WFP_KITS_H_
