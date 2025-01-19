#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>
#include "frame_buffer_config.hpp"
#include "elf.hpp"


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
		case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
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
	EFI_STATUS status;
	CHAR8 buf[256];
	UINTN len;

	// headerptr is memoryAddress that point memory area stored string "Index, Type, Type(name), ..."
	CHAR8* headerptr = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
	// AsciiStrLen() calculate length of string in parameter
	len = AsciiStrLen(headerptr);
	// file->Write() write first second parameter character of third parameter to first parameter. and write indeed number of character written to second parameter.
	status = file->Write(file, &len, headerptr);
	if (EFI_ERROR(status)){
		return status;
	}
	// %08lx is format specifier. each characters have meaning. 0 means zero padding, 8 means more than 8...
	Print(L"map->bufferptr = %08lx, map->map_size = %08lx\n", map->bufferptr, map->map_size);

	// EFI_PHYSICAL_ADDRESS means physical memory address. UINT64 type
	EFI_PHYSICAL_ADDRESS iter;
	int i;

	for(iter = (EFI_PHYSICAL_ADDRESS)map->bufferptr, i = 0;
 	    iter < (EFI_PHYSICAL_ADDRESS)map->bufferptr + map->map_size;
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
		status = file->Write(file, &len, buf);		
		if (EFI_ERROR(status)){
			return status;
		}	
	}
		
	return EFI_SUCCESS;
}

// root directory means that root directory of device where this application runs. if this applicationis stored in usb drive, root directory means root directory of usb drive. if hdd, root directory of hdd. 
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** rootptrptr){
	EFI_STATUS status;
	// EFI_LOADED_IMAGE_PROTOCOL is protocol that provide some information of loaded image(application or driver)
	// loaded_imageptr is protocl to access devicehandle to open EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
	EFI_LOADED_IMAGE_PROTOCOL* loaded_imageptr;
	// EFI_SIMPLE_FILE_SYSTEM_PROTOCOL is protocol that provide access to simple file system.
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fsptr;

	// in UEFI, handle is cluster of protocol. protocol is data structure that have data and method pointer and guid. Handle means UEFI application, driver, firmware. protocol means function tied handle.
	// gBS->OpenProtocol() is method that open protocol to use other protocol. first parameter is handle to search the protocol. second parameter is guid of the protocol. third parameter is pointer of pointer of the protocol. fourth parameter is handle to use the protocol
	status = gBS->OpenProtocol(
			image_handle,
			&gEfiLoadedImageProtocolGuid,
			(VOID**)&loaded_imageptr,
			image_handle,
			NULL,
			EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);	
	if (EFI_ERROR(status)){
		return status;
	}
	status = gBS->OpenProtocol(
			loaded_imageptr->DeviceHandle,
			&gEfiSimpleFileSystemProtocolGuid,
			(VOID**)&fsptr,
			image_handle,
			NULL,
			EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	if (EFI_ERROR(status)){
		return status;
	}
	// EFI_SIMPLE_FILE_SYSTEM->OpenVolume() is method that open volume of specified file system protocol and get pointer of root directory of that volume.	
	return fsptr->OpenVolume(fsptr, rootptrptr);
}

EFI_STATUS OpenGOP(
		EFI_HANDLE image_handle,
		EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
	EFI_STATUS status;
	UINTN num_gop_handles = 0;
	EFI_HANDLE* gop_handles = NULL;

	// this method locate handle that can treat the specified protocol. first prameter represent way of locate. second parameter represent the protocol. fourth parameter represent number of handle. fifth parameter represent handles that can treat specified protocol 
	status = gBS->LocateHandleBuffer(
			ByProtocol,
			&gEfiGraphicsOutputProtocolGuid,
			NULL,
			&num_gop_handles,
			&gop_handles);
	if (EFI_ERROR(status)){
		return status;
	}

	status = gBS->OpenProtocol(
			gop_handles[0],
			&gEfiGraphicsOutputProtocolGuid,
			// third parameter of this method require VOID**
			(VOID**)gop,
			image_handle,
			NULL,
			EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);		
	if (EFI_ERROR(status)){
		return status;
	}
			
	// release memory buffer. memory buffer means temporary memory area
	FreePool(gop_handles);

	return EFI_SUCCESS;
}

// write const to say in this method string of result never change dynamically.
// const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT format){
//	switch(format){
//		case PixelRedGreenBlueReserved8BitPerColor:
//			return L"PixelRedGrennBlueReserved8BitPerColor";
//		case PixelBlueGreenRedReserved8BitPerColor:
//			return L"PixelBlueGreenRedReserved8BitPerColor";
//		case PixelBitMask:
//			return L"PixelBitMask";
//		case PixelBltOnly:
//			return L"PixelBltOnly";
//		case PixelFormatMax:
//			return L"PixelFormatMax";
//		default:
//			return L"InvalidPixelFormat";
//	}
// }

void Halt(void){
	// in same row, dont need {}
	while (1) __asm__("hlt");
}

void CalcLoadAddressRange(Elf64_Ehdr* ehdrptr, UINT64* firstptr, UINT64* lastptr){
	// offset means distance from base point 
	Elf64_Phdr* phdrptr = (Elf64_Phdr*)((UINT64)ehdrptr + ehdrptr->e_phoff);

	*firstptr = MAX_UINT64;
	*lastptr = 0;

	// declare i as Elf64_Half (not as UINT64) because e_phnum field declare as uint16_t( equal Elf64_Half) 
	for (Elf64_Half i = 0; i < ehdrptr->e_phnum; i++){
		if (phdrptr[i].p_type != PT_LOAD) continue;
		// virtual address require real address? If real address specified with virtual address is not empty, i wonder what will happen. 
		*firstptr = MIN(*firstptr, phdrptr[i].p_vaddr);
		*lastptr = MAX(*lastptr, phdrptr[i].p_vaddr + phdrptr[i].p_memsz);
	}
}

void CopyLoadSegments(Elf64_Ehdr* ehdrptr) {
	Elf64_Phdr* phdrptr = (Elf64_Phdr*)((UINT64)ehdrptr + ehdrptr->e_phoff);
	
	for(Elf64_Half i = 0; i<ehdrptr->e_phnum; ++i) {
		if(phdrptr[i].p_type != PT_LOAD)  continue;
		
		// base point of p_offset is head of Elf file, equal ehdrptr
		UINT64 segmentptr = (UINT64)ehdrptr + phdrptr[i].p_offset;
		// VOID means all like var in Java. first parameter represent place paste, second parameter represent plase be copied, third parameter represent size
		CopyMem((VOID*)phdrptr[i].p_vaddr, (VOID*)segmentptr, phdrptr[i].p_filesz);

		// often memsz become bigger than filesz
		UINTN remain_bytes = phdrptr[i].p_memsz - phdrptr[i].p_filesz;
		// this method fill specified area up with specified value. first parameter represent place, second parameter represent size, third parameter represent value that be used to fill up. fill up with 0 to initialize memory area before be used in runtime
		SetMem((VOID*)(phdrptr[i].p_vaddr + phdrptr[i].p_filesz), remain_bytes, 0);
	}	

}
	
// *system_table is pointer of EFI_SYSTEM_TABLE variable
// EFI_HANDLE is handle that specify various resource managed by UEFI. in this case, image_handle represent this application.
EFI_STATUS EFIAPI UefiMain(
		EFI_HANDLE image_handle,
		EFI_SYSTEM_TABLE* system_table){
	EFI_STATUS status;
	Print(L"Saiko no Egao de Kirinukeruyo\n");

	// save memory map to file start	
	CHAR8 memmap_buf[4096 * 4];
	struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
	status = GetMemoryMap(&memmap);
	if (EFI_ERROR(status)){
		Print(L"failed to get memory map: %r\n", status);
		Halt();	
	}

	// in this time, root_dirptr is not pointer of root directory.
	EFI_FILE_PROTOCOL* root_dirptr;
	// this time, root_dirptr become pointer of root directory. to update pointer, we should use pointer of pointer.
	status = OpenRootDir(image_handle, &root_dirptr);
	
	if (EFI_ERROR(status)){
		Print(L"failed to open root directory: %r\n", status);	
		Halt();
	}

	EFI_FILE_PROTOCOL* memmap_fileptr;
	// EFI_FILE_PROTOCOL->Open() means open file. first parameter is pointer of current file instance. second parameter is pointer of pointer of file newly open. third parameter is name of file newly open. in this case, memmap is below root directory. fourth parameter is mode. return pointer of newly opened file as second parameter.
	status = root_dirptr->Open(
			root_dirptr, &memmap_fileptr, L"\\memmap",
			// | means OR calculation. each rights are represented by other bit row. so OR calculation makes all rights on.  
			EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
	
	if (EFI_ERROR(status)){
		Print(L"failed to open file: %r\n", status);
		Print(L"Ignored.\n");	
	} else {
		status = SaveMemoryMap(&memmap, memmap_fileptr);
	
		if (EFI_ERROR(status)){
			Print(L"failed to save memory map: %r\n", status);
			Halt();	
		}

		status = memmap_fileptr->Close(memmap_fileptr);
	
		if (EFI_ERROR(status)){
			Print(L"failed to close memory map: %r\n", status);	
			Halt();
		}
	}
	// save memory map to file end
	
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	status = OpenGOP(image_handle, &gop);

	if (EFI_ERROR(status)){
		Print(L"failed to open GOP: %r\n", status);
		Halt();	
	}

	// Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
	//		gop->Mode->Info->HorizontalResolution,
	//		gop->Mode->Info->VerticalResolution,
	//		GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
	//		gop->Mode->Info->PixelsPerScanLine);
	// Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
	//		gop->Mode->FrameBufferBase,
	//		gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
	//		gop->Mode->FrameBufferSize);

	// declare as UINT8* because gop->Mode->FrameBufferSize is per byte
	// UINT8* frame_buffer = (UINT8*)gop->Mode->FrameBufferBase;
	// for (UINTN i = 0; i < gop->Mode->FrameBufferSize; ++i){
	//	frame_buffer[i] = 255;
	// }

	// read kernel start
	EFI_FILE_PROTOCOL* kernel_fileptr;
	status = root_dirptr->Open(root_dirptr, &kernel_fileptr, L"\\kernel.elf", EFI_FILE_MODE_READ,0);

	if (EFI_ERROR(status)){
		Print(L"failed to open file '\\kernel.elf' : %r\n", status);	
		Halt();
	}

	// EFI_FILE_PROTOCOL.GetInfo() set EFI_FILE_INFO to fourth parameter. but last field of EFI_FILE_INFO type is filename[], and default is empty. so add 12 CHAR16, \ k e r n e l . e l f + null character 
	UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
	// in c, array variable is treated as pointer of first content of its array. so later we can cast file_info_buffer to EFI_FILE_INFO* type. there are many times manipulate object per byte, so declear as UINT8. for example, file_info_size is per byte, so if declare as UINT64, we should write UINT64 file_info_buffer[file_info_size / 8]
	UINT8 file_info_buffer[file_info_size];
	status = kernel_fileptr->GetInfo(kernel_fileptr, &gEfiFileInfoGuid, &file_info_size, file_info_buffer);

	if (EFI_ERROR(status)){
		Print(L"failed to get file '\\kernel.elf'  information: %r\n", status);	
		Halt();
	}

	EFI_FILE_INFO* file_infoptr=(EFI_FILE_INFO*)file_info_buffer;
	UINTN kernel_file_size = file_infoptr->FileSize;

//	EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x110000;
	// first parameter as way of allocate memory, second parameter as type of memory area, third parameter as size, fourth parameter as pointer of allocated memory area. third parameter is number of page. in UEFI, 1 page equall 4KiB. fourth parameter wont change beacause AllocateAddress mode.
//	status = gBS->AllocatePages(
//			AllocateAddress, EfiLoaderData,
//	 		(kernel_file_size + 0xfff) / 0x1000, &kernel_base_addr);

//	if (EFI_ERROR(status)){
//		Print(L"failed to allocate pages: %r\n", status);	
//		Halt();	
//	}

	VOID* kernel_bufferptr;
	// this method allocate memory area like AllocatePages, difference is that per byte, not page, and cant specify pointer of allocated memory area
	status = gBS->AllocatePool(EfiLoaderData, kernel_file_size, &kernel_bufferptr);

	if(EFI_ERROR(status)){
		Print(L"failed to allocate pool: %r\n", status);
	}

	// first parameter as place read, second parameter as size of read, third parameter is place write
	status = kernel_fileptr->Read(kernel_fileptr, &kernel_file_size, kernel_bufferptr);
	
	if (EFI_ERROR(status)){
		Print(L"failed to read file '\\kernel.elf' : %r\n", status);
		Halt();	
	}

	// %0lx become 110000, because %0lx only output number, not prefix 
	// Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);
	
	Elf64_Ehdr* kernel_ehdrptr = (Elf64_Ehdr*)kernel_bufferptr;
	UINT64 kernel_first_addr, kernel_last_addr;
	CalcLoadAddressRange(kernel_ehdrptr, &kernel_first_addr, &kernel_last_addr);

	UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
	status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr);

	if(EFI_ERROR(status)) {
		Print(L"failed to allocate pages: %r\n", status);
		Halt();
	}

	CopyLoadSegments(kernel_ehdrptr);
	Print(L"Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addr, kernel_last_addr);

	status = gBS->FreePool(kernel_bufferptr);

	if (EFI_ERROR(status)) {
		Print(L"failed to free pool: %r\n", status);	
		Halt();
	}
	
	status = GetMemoryMap(&memmap);
	
	if (EFI_ERROR(status)){
		Print(L"failed to get memory map: %r\n", status);	
		Halt();
	}
	
	// Print method change memorymap, so below ExitBootServices is failed
	// Print(L"memmap.map_key:%llx\n", memmap.map_key);
	status = gBS->ExitBootServices(image_handle, memmap.map_key);

	if(EFI_ERROR(status)){
		Print(L"failed to exit boot service: %r\n", status);	
		Halt();
	}

// * of "*(UINT64" is dereference operator. because it is not with declaration. the reason 24 is that kernel.elf is elf for 64 bit, so entry point address starts after 24 bytes. in 64 bit architecture, memory address reoresent 64bit.
	// data that offset 24 byte of kernel.elf is address of main enry point. not main entry point 
	// UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

	// i don't know why add 24. offset 24 byte of kernel.elf is address of main entry point. but kernel_first_addr is not head of kernel.elf. it is head of LOAD segment. so i don't know.
	UINT64 entry_addr = *(UINT64*)(kernel_first_addr + 24);

	struct FrameBufferConfig config = {
		(UINT8*)gop->Mode->FrameBufferBase,
		gop->Mode->Info->PixelsPerScanLine,
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		0
	};

	switch (gop->Mode->Info->PixelFormat) {
		case PixelRedGreenBlueReserved8BitPerColor:
			config.pixel_format = kPixelRGBResv8BitPerColor;
			break;
		case PixelBlueGreenRedReserved8BitPerColor:
			config.pixel_format = kPixelBGRResv8BitPerColor;
			break;
		default:
			// cant run Print method after exit boot service
			// Print(L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
			Halt();
	}
	
	// this is type prototype. definition of c language method
	typedef void EntryPointType(const struct FrameBufferConfig*);
	EntryPointType* entry_point = (EntryPointType*)entry_addr;
	// after exit bootservices, using Print method that is one of bootservices cause freeze 
	// Print(L"korekara kernel yobidasi");
	entry_point(&config);

	Print(L"All done\n");

	while(1);
	return EFI_SUCCESS;
}
