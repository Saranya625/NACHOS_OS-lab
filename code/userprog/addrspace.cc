// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you are using the "stub" file system, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "synch.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
#ifdef RDATA
    noffH->readonlyData.size = WordToHost(noffH->readonlyData.size);
    noffH->readonlyData.virtualAddr =
        WordToHost(noffH->readonlyData.virtualAddr);
    noffH->readonlyData.inFileAddr = WordToHost(noffH->readonlyData.inFileAddr);
#endif
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);

#ifdef RDATA
    DEBUG(dbgAddr, "code = " << noffH->code.size
                             << " readonly = " << noffH->readonlyData.size
                             << " init = " << noffH->initData.size
                             << " uninit = " << noffH->uninitData.size << "\n");
#endif
}

static unsigned int AlignToWord(unsigned int value) {
    const unsigned int alignment = sizeof(int);
    return (value + alignment - 1) & ~(alignment - 1);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace() {
    pageTable = NULL;
    numPages = 0;
    executable = NULL;
    heapBase = 0;
    heapLimit = 0;
    brk = 0;
    bzero((char *)&noffH, sizeof(noffH));

    // pageTable = new TranslationEntry[NumPhysPages];
    // for (int i = 0; i < NumPhysPages; i++) {
    //     pageTable[i].virtualPage = i;  // for now, virt page # = phys page #
    //     pageTable[i].physicalPage = i;
    //     pageTable[i].valid = TRUE;
    //     pageTable[i].use = FALSE;
    //     pageTable[i].dirty = FALSE;
    //     pageTable[i].readOnly = FALSE;
    // }

    // // zero out the entire address space
    // bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
    int i;
    for (i = 0; i < numPages; i++) {
        if (pageTable[i].valid && pageTable[i].physicalPage >= 0) {
            kernel->gPhysPageBitMap->Clear(pageTable[i].physicalPage);
        }
    }
    delete[] pageTable;
    delete executable;
}

//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(char *fileName) {
    executable = NULL;
    pageTable = NULL;
    numPages = 0;
    heapBase = 0;
    heapLimit = 0;
    brk = 0;
    bzero((char *)&noffH, sizeof(noffH));

    executable = kernel->fileSystem->Open(fileName);
    unsigned int i, size, loadedSize;

    if (executable == NULL) {
        DEBUG(dbgFile, "\n Error opening file.");
        return;
    }
    //đọc header của file
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
    kernel->addrLock->P();
    loadedSize = noffH.code.size;
#ifdef RDATA
    loadedSize += noffH.readonlyData.size;
#endif
    loadedSize += noffH.initData.size + noffH.uninitData.size;
    heapBase = AlignToWord(loadedSize);
    heapLimit = heapBase + UserHeapSize;
    brk = heapBase;

    // how big is address space?
    size = heapLimit + UserStackSize;  // reserve heap before stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);  // check we're not trying
                                       // to run anything too big --
                                       // at least until we have
                                       // virtual memory

    // Check the available memory enough to load new process
    // debug
    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);
    cout << "[DemandPaging] Created address space for " << fileName << " with "
         << numPages << " virtual pages. All pages start invalid.\n";
    cout << "[Heap] Reserved heap region: [" << heapBase << ", "
         << heapLimit - 1 << "], initial brk = " << brk << "\n";
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;  // for now, virtual page # = phys page #
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
    }

    kernel->addrLock->V();
    return;
}

void AddrSpace::LoadSegmentPage(const Segment &segment, unsigned int vpn,
                                char *pageFrame) {
    unsigned int pageStart;
    unsigned int pageEnd;
    unsigned int segmentStart;
    unsigned int segmentEnd;
    unsigned int copyStart;
    unsigned int copyEnd;
    int bytesToRead;
    int fileOffset;
    int pageOffset;

    if (segment.size <= 0 || executable == NULL) {
        return;
    }

    pageStart = vpn * PageSize;
    pageEnd = pageStart + PageSize;
    segmentStart = segment.virtualAddr;
    segmentEnd = segment.virtualAddr + segment.size;

    if (pageEnd <= segmentStart || pageStart >= segmentEnd) {
        return;
    }

    copyStart = (pageStart > segmentStart) ? pageStart : segmentStart;
    copyEnd = (pageEnd < segmentEnd) ? pageEnd : segmentEnd;
    bytesToRead = copyEnd - copyStart;
    pageOffset = copyStart - pageStart;
    fileOffset = segment.inFileAddr + (copyStart - segmentStart);

    executable->ReadAt(pageFrame + pageOffset, bytesToRead, fileOffset);
}

bool AddrSpace::LoadPage(unsigned int vpn) {
    TranslationEntry *pte;
    int physicalPage;
    char *pageFrame;

    if (vpn >= numPages || pageTable == NULL) {
        return FALSE;
    }

    kernel->addrLock->P();

    pte = &pageTable[vpn];
    if (pte->valid) {
        kernel->addrLock->V();
        return TRUE;
    }

    physicalPage = kernel->gPhysPageBitMap->FindAndSet();
    if (physicalPage < 0) {
        DEBUG(dbgAddr, "Demand paging failed: no free physical page");
        kernel->addrLock->V();
        return FALSE;
    }

    pte->physicalPage = physicalPage;
    pte->use = FALSE;
    pte->dirty = FALSE;
    pte->readOnly = FALSE;

    pageFrame = &(kernel->machine->mainMemory[physicalPage * PageSize]);
    bzero(pageFrame, PageSize);

    cout << "[DemandPaging] Loading VPN " << vpn << " into physical page "
         << physicalPage << " (virtual bytes " << vpn * PageSize << "-"
         << (vpn + 1) * PageSize - 1 << ")\n";

    LoadSegmentPage(noffH.code, vpn, pageFrame);
#ifdef RDATA
    LoadSegmentPage(noffH.readonlyData, vpn, pageFrame);
#endif
    LoadSegmentPage(noffH.initData, vpn, pageFrame);

    pte->valid = TRUE;

    DEBUG(dbgAddr, "Demand-paged virtual page " << vpn
                                                << " into physical page "
                                                << physicalPage);
    cout << "[DemandPaging] VPN " << vpn << " is now valid.\n";

    kernel->addrLock->V();
    return TRUE;
}

int AddrSpace::Malloc(int size) {
    unsigned int allocStart;
    unsigned int allocEnd;

    if (size <= 0) {
        return 0;
    }

    kernel->addrLock->P();

    allocStart = AlignToWord(brk);
    if ((unsigned int)size > heapLimit - allocStart) {
        cout << "[Heap] malloc(" << size << ") failed. brk = " << brk
             << ", heap limit = " << heapLimit << "\n";
        kernel->addrLock->V();
        return 0;
    }
    allocEnd = AlignToWord(allocStart + size);

    if (allocEnd > heapLimit || allocEnd < allocStart) {
        cout << "[Heap] malloc(" << size << ") failed. brk = " << brk
             << ", heap limit = " << heapLimit << "\n";
        kernel->addrLock->V();
        return 0;
    }

    brk = allocEnd;

    cout << "[Heap] malloc(" << size << ") -> " << allocStart
         << ", new brk = " << brk << "\n";

    kernel->addrLock->V();
    return allocStart;
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program using the current thread
//
//      The program is assumed to have already been loaded into
//      the address space
//
//----------------------------------------------------------------------

void AddrSpace::Execute() {
    kernel->currentThread->space = this;

    this->InitRegisters();  // set the initial register values
    this->RestoreState();   // load page table register

    kernel->machine->Run();  // jump to the user progam

    ASSERTNOTREACHED();  // machine->Run never returns;
                         // the address space exits
                         // by doing the syscall "exit"
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++) machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start", which
    //  is assumed to be virtual address zero
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    // Since instructions occupy four bytes each, the next instruction
    // after start will be at virtual address four.
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}

//----------------------------------------------------------------------
// AddrSpace::Translate
//  Translate the virtual address in _vaddr_ to a physical address
//  and store the physical address in _paddr_.
//  The flag _isReadWrite_ is false (0) for read-only access; true (1)
//  for read-write access.
//  Return any exceptions caused by the address translation.
//----------------------------------------------------------------------
ExceptionType AddrSpace::Translate(unsigned int vaddr, unsigned int *paddr,
                                   int isReadWrite) {
    TranslationEntry *pte;
    int pfn;
    unsigned int vpn = vaddr / PageSize;
    unsigned int offset = vaddr % PageSize;

    if (vpn >= numPages) {
        return AddressErrorException;
    }

    pte = &pageTable[vpn];

    if (isReadWrite && pte->readOnly) {
        return ReadOnlyException;
    }

    pfn = pte->physicalPage;

    // if the pageFrame is too big, there is something really wrong!
    // An invalid translation was loaded into the page table or TLB.
    if (pfn >= NumPhysPages) {
        DEBUG(dbgAddr, "Illegal physical page " << pfn);
        return BusErrorException;
    }

    pte->use = TRUE;  // set the use, dirty bits

    if (isReadWrite) pte->dirty = TRUE;

    *paddr = pfn * PageSize + offset;

    ASSERT((*paddr < MemorySize));

    // cerr << " -- AddrSpace::Translate(): vaddr: " << vaddr <<
    //  ", paddr: " << *paddr << "\n";

    return NoException;
}
