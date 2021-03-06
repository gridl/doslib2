
#include <efi.h>
#include <efilib.h>
#include <efistdarg.h>

#include <efi/doslib/efidoslib_base.h>
#include <efi/doslib/efidoslib_utf.h>
#include <efi/doslib/efidoslib_printf.h>
#include <efi/doslib/efidoslib_assert.h>

/* NTS: Do NOT mark your efi_main as type EFIAPI, because the EFI BIOS does not
 *      directly call your function, the gnu-efi stub does */
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	InitializeLib(ImageHandle,SystemTable);
	doslib_init(ImageHandle,SystemTable);
	Print(L"Hello world (gnu-efi Print)\r\n");
	SystemTable->ConOut->OutputString(SystemTable->ConOut,L"Hello world (direct ConOut call)\r\n");
	puts("Hello world (puts)\r\n");
	printf("Hello world (printf) %s\r\n","Hello world string");
	return EFI_SUCCESS;
}

