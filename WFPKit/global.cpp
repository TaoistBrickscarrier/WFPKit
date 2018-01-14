
#include "global.h"

PVOID KernelGetModuleHandle(char* module_name, ULONG* image_size) {
  NTSTATUS status = STATUS_SUCCESS;
  PVOID buffer = NULL;
  ULONG buffer_size = 4096;
  ULONG return_length = 0;

  do {
    buffer = ExAllocatePool(NonPagedPool, buffer_size);
    if (buffer == NULL) {
      return false;
    }
    status = ZwQuerySystemInformation(11, buffer, buffer_size, &return_length);
    if (status == STATUS_INFO_LENGTH_MISMATCH) {
      ExFreePool(buffer);
      buffer_size = return_length;
      continue;
    }
    if (!NT_SUCCESS(status)) {
      ExFreePool(buffer);
      return NULL;
    } else {
      break;
    }
  } while (true);

  PRTL_PROCESS_MODULES process_modules = (PRTL_PROCESS_MODULES)buffer;
  RTL_PROCESS_MODULE_INFORMATION* modules = process_modules->Modules;
  ULONGLONG start_address = 0;
  for (ULONG i = 0; i < process_modules->NumberOfModules; ++i) {
    if (_stricmp((char*)modules->FullPathName + modules->OffsetToFileName,
                 module_name) == 0) {
      start_address = (ULONGLONG)process_modules->Modules[i].ImageBase;
      *image_size = process_modules->Modules[i].ImageSize;
      break;
    } else {
      ++modules;
    }
  }
  ExFreePool(buffer);
  return (PVOID)start_address;
}

PVOID KernelGetProcAddress(PVOID module, ULONG size, char* proc_name) {
  if (module == NULL) {
    return NULL;
  }

  ULONGLONG module_end = (ULONGLONG)module + size;

  ULONG need_size = 0;

  IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)module;
  need_size = dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS64);
  if (dos_header->e_magic != IMAGE_DOS_SIGNATURE || need_size > size) {
    return NULL;
  }

  IMAGE_NT_HEADERS64* nt_header = (IMAGE_NT_HEADERS64*)((PUCHAR)module +
                                                        dos_header->e_lfanew);
  if (nt_header->Signature != IMAGE_NT_SIGNATURE) {
    return NULL;
  }

  ULONG dir_index = IMAGE_DIRECTORY_ENTRY_EXPORT;

  IMAGE_EXPORT_DIRECTORY* export_dir = (IMAGE_EXPORT_DIRECTORY*)(
    (PUCHAR)module +
    nt_header->OptionalHeader.DataDirectory[dir_index].VirtualAddress);
  ULONG export_dir_size = nt_header->OptionalHeader.DataDirectory[dir_index].Size;

  if ((ULONGLONG)export_dir + export_dir_size > module_end) {
    return NULL;
  }

  ULONG* name_addr_array = (ULONG*)(
    (PUCHAR)module + export_dir->AddressOfNames);
  for (ULONG i = 0; i < export_dir->NumberOfNames; ++i) {
    char* function_name = (char*)module + name_addr_array[i];
    if ((ULONGLONG)function_name > module_end) {
      continue;
    }
    if (_stricmp(function_name, proc_name) == 0) {
      USHORT* name_ordinals = (USHORT*)((PUCHAR)module +
                                        export_dir->AddressOfNameOrdinals);
      if ((ULONGLONG)name_ordinals > module_end) {
        return NULL;
      }
      ULONG* functions = (ULONG*)((PUCHAR)module +
                                  export_dir->AddressOfFunctions);
      if ((ULONGLONG)functions > module_end) {
        return NULL;
      }

      ULONGLONG FoundFunction = (ULONGLONG)module + functions[name_ordinals[i]];
      if (FoundFunction > module_end) {
        return NULL;
      } else {
        return (PVOID)FoundFunction;
      }
    }
  }
  return NULL;
}