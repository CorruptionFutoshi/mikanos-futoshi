#include <Uefi.h>
#include <Library/UefiLib.h>

// *system_table is pointer of EFI_SYSTEM_TABLE variable
EFI_STATUS EFIAPI UefiMain(
		EFI_HANDLE image_handle,
		EFI_SYSTEM_TABLE *system_table){
	Print(L"Saiko no Egao de Kirinukeruyo");
	while(1);
	return EFI_SUCCESS;
}
