#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>

struct MemoryMap {
	// represent first get memory size.
	UINTN buffer_size;
	// VOID means any type like var of Java. represent memoryAddress that point memory area stored this first get struct memorymap
	VOID* bufferptr;
	// represent size of this struct memorymap
	UINTN map_size;
	// represent variable to identify this struct memorymap 
	UINTN map_key;
	// represent size of descriptor. descriptor means individual row of this struct memorymap
	UINTN descriptor_size;
	// represent version of descriptor
	UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap* map){
	if (map->bufferptr == NULL){
		// i dont know why if map->bufferptr equals NULL, derive that buffer is too small. 
		return EFI_BUFFER_TOO_SMALL;
	}

	// indeed, we dont need to set both map_size and buffer_size fields in this program. to describe first get memory, declare buffer_size, and to describe actually be used memory, declare memory_size. like this program, wont use buffer_size field later, dont need buffer_size field.
	map->map_size = map->buffer_size;

	// gBS->GetMemoryMap will get memorymap and write in memory area specified by second parameter and write information in each parameters
	return gBS->GetMemoryMap(
			&map->map_size,
			// EFI_MEMORY_DESCRIPTOR means individual row of memorymap. 
			(EFI_MEMORY_DESCRIPTOR*)map->bufferptr,
			&map->map_key,
			&map->descriptor_size,
			&map->descriptor_version);
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type){
	switch(type) {
		case EfiReservedMemoryType: return L"EfiReservedMemoryType";
		case EfiLoaderCode: return L"EfiLoaderCode";
		case EfiLoaderData: return L"EfiLoaderData";
		case EfiBootServicesCode: return L"EfiBootServicesCode";
		case EfiBootServicesData: return L"EfiBootServicesData";
		case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
		case EfiRuntimeServicesData: return L"EfiRuntimeServicesData"
		case EfiConventionalMemory: return L"EfiConventionalMemory";
		case EfiUnusableMemory: return L"EfiUnusableMemory";
		case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
		case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
		case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
		case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
		case EfiPalCode: return L"EfiPalCode";
		case EfiPersistentMemory: return L"EfiPersistentMemory";
		case EfiMaxMemoryType: return L"EfiMaxMemoryType";
       		default: return L"InvalidMemoryType";
	}
}	

// in c language, if struct is big, copying occur when struct is deleverd as parameter. so however we dont have to declare first parameter as pointer, we declare pointer to efficiency of memory. 
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file){
	CHAR8 buf[256];
	UINTN len;

	// headerptr is memoryAddress that point memory area stored string "Index, Type, Type(name), ..."
	CHAR8* headerptr = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
	// AsciiStrLen() calculate length of string in parameter
	len = AsciiStrLen(headerptr);
	// file->Write() write first second parameter character of third parameter to first parameter. and write indeed number of character written to second parameter.
	file->Write(file, &len, headerptr);
	// %08lx is format specifier. each characters have meaning. 0 means zero padding, 8 means more than 8...
	Print(L"map->bufferptr = %08lx, map->map_size = %08lx\n", map->bufferptr, map->map_size);

	// EFI_PHYSICAL_ADDRESS means physical memory address. UINT64 type
	EFI_PHYSICAL_ADDRESS iter;
	int i;

	for(iter = (EFI_PHYSICAL_ADDRESS)map->bufferptr, i = 0;
 	    iter < (EFI_PHYSICAL_ADDRESS)map->bufferptr + map_>map_size;
	    iter += map->descriptor_size, i ++){
		EFI_MEMORY_DESCRIPTOR* descptr = (EFI_MEMORY_DESCRIPTOR*) iter;
		// AsciiSPrint() write third parameter to second parameter size of char[] specified by first parameter
		// in C language, when [] variable is used as a method parameter, its variable become pointer of its [] 
		len = AsciiSPrint(
			buf,sizeof(buf),"%u,%x,%-ls,%08lx,%lx,%lx\n",
			i, descptr->Type, GetMemoryTypeUnicode(descptr->Type),
			descptr->PhysicalStart, descptr->NumberOfPages,
			// reason of & 0xffffflu is that extract lower 20 bit. lu means type unsigned long. AND calculation is only 1 and 1, become 1. so higher than 20 bit of 0xffffflu is all 0, higer than 20 bit of AND calculation result is always 0.  
			descptr->Attribute & 0xffffflu);
		file->Write(file, &len, buf);		
	}
		
	return EFI_SUCCESS;
}

// root directory means that root directory of device where this application runs. if this applicationis stored in usb drive, root directory means root directory of usb drive. if hdd, root directory of hdd. 
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** rootptrptr){
	// EFI_LOADED_IMAGE_PROTOCOL is protocol that provide some information of loaded image(application or driver)
	// loaded_imageptr is protocl to access devicehandle to open EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
	EFI_LOADED_IMAGE_PROTOCOL* loaded_imageptr;
	// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL is protocol that provide access to simple file system.
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fsptr;

	// in UEFI, handle is cluster of protocol. protocol is data structure that have data and method pointer and guid. Handle means UEFI application, driver, firmware. protocol means function tied handle.
	// gBS->OpenProtocol() is method that open protocol to use other protocol. first parameter is handle to search the protocol. second parameter is guid of the protocol. third parameter is pointer of pointer of the protocol. fourth parameter is handle to use the protocol
	gBS->OpenProtocol(
		image_handle,
		&gEfiLoadedImageProtocolGuid,
		(VOID**)&loaded_imageptr,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	
	gBs->OpenProtocol(
		loaded_imageptr->DeviceHandle,
		&gEfiSimpleFileSystemProtocolGuid,
		(VOID**)&fsptr,
		image_handle,
		NULL,
		EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

	// EFI_SIMPLE_FILE_SYSTEM->OpenVolume() is method that open volume of specified file system protocol and get pointer of root directory of that volume.	
	fsptr->OpenVolume(fsptr, rootptrptr);

	return EFI_SUCCESS;
}
	
// *system_table is pointer of EFI_SYSTEM_TABLE variable
// EFI_HANDLE is handle that specify various resource managed by UEFI. in this case, image_handle represent this application.
EFI_STATUS EFIAPI UefiMain(
		EFI_HANDLE image_handle,
		EFI_SYSTEM_TABLE* system_table){
	Print(L"Saiko no Egao de Kirinukeruyo\n");
	
	CHAR8 memmap_buf[4096 * 4];
	struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
	GetMemoryMap(&memmap);

	// in this time, root_dirptr is not pointer of root directory.
	EFI_FILE_PROTOCOL* root_dirptr;
	// this time, root_dirptr become pointer of root directory. to update pointer, we should use pointer of pointer.
	OpenRootDir(image_handle, &root_dirptr);

	EFI_FILE_PROTOCOL* memmap_fileptr;
	// EFI_FILE_PROTOCOL->Open() means open file. first parameter is pointer of current file instance. second parameter is pointer of pointer of file newly open. third parameter is name of file newly open. in this case, memmap is below root directory. fourth parameter is mode. return pointer of newly opened file as second parameter.
	root_dirptr->Open(
		root_dirptr, &memmap_fileptr, L"\\memmap",
		// | means OR calculation. each rights are represented by other bit row. so OR calculation makes all rights on.  
		EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

	SaveMemoryMap(&memmap, memmap_fileptr);
	memmap_file->Close(memmap_fileptr);

	Print(L"All done\n");

	while(1);
	return EFI_SUCCESS;
}
