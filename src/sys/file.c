/**
 * @file sys/file.c
 *
 * @copyright 2015-2016 Bill Zissimopoulos
 */
/*
 * This file is part of WinFsp.
 *
 * You can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License version 3 as published by the
 * Free Software Foundation.
 *
 * Licensees holding a valid commercial license may use this file in
 * accordance with the commercial license agreement provided with the
 * software.
 */

#include <sys/driver.h>

NTSTATUS FspFileNodeCopyList(PDEVICE_OBJECT DeviceObject,
    FSP_FILE_NODE ***PFileNodes, PULONG PFileNodeCount);
VOID FspFileNodeDeleteList(FSP_FILE_NODE **FileNodes, ULONG FileNodeCount);
NTSTATUS FspFileNodeCreate(PDEVICE_OBJECT DeviceObject,
    ULONG ExtraSize, FSP_FILE_NODE **PFileNode);
VOID FspFileNodeDelete(FSP_FILE_NODE *FileNode);
VOID FspFileNodeAcquireSharedF(FSP_FILE_NODE *FileNode, ULONG Flags);
BOOLEAN FspFileNodeTryAcquireSharedF(FSP_FILE_NODE *FileNode, ULONG Flags, BOOLEAN Wait);
VOID FspFileNodeAcquireExclusiveF(FSP_FILE_NODE *FileNode, ULONG Flags);
BOOLEAN FspFileNodeTryAcquireExclusiveF(FSP_FILE_NODE *FileNode, ULONG Flags, BOOLEAN Wait);
VOID FspFileNodeConvertExclusiveToSharedF(FSP_FILE_NODE *FileNode, ULONG Flags);
VOID FspFileNodeSetOwnerF(FSP_FILE_NODE *FileNode, ULONG Flags, PVOID Owner);
VOID FspFileNodeReleaseF(FSP_FILE_NODE *FileNode, ULONG Flags);
VOID FspFileNodeReleaseOwnerF(FSP_FILE_NODE *FileNode, ULONG Flags, PVOID Owner);
FSP_FILE_NODE *FspFileNodeOpen(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject,
    UINT32 GrantedAccess, UINT32 ShareAccess, NTSTATUS *PResult);
VOID FspFileNodeCleanup(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject,
    PBOOLEAN PDeletePending);
VOID FspFileNodeCleanupComplete(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject);
VOID FspFileNodeClose(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject);
NTSTATUS FspFileNodeFlushAndPurgeCache(FSP_FILE_NODE *FileNode,
    UINT64 FlushOffset64, ULONG FlushLength, BOOLEAN FlushAndPurge);
VOID FspFileNodeRename(FSP_FILE_NODE *FileNode, PUNICODE_STRING NewFileName);
BOOLEAN FspFileNodeHasOpenHandles(PDEVICE_OBJECT FsvolDeviceObject,
    PUNICODE_STRING FileName, BOOLEAN SubpathOnly);
VOID FspFileNodeGetFileInfo(FSP_FILE_NODE *FileNode, FSP_FSCTL_FILE_INFO *FileInfo);
BOOLEAN FspFileNodeTryGetFileInfo(FSP_FILE_NODE *FileNode, FSP_FSCTL_FILE_INFO *FileInfo);
VOID FspFileNodeSetFileInfo(FSP_FILE_NODE *FileNode, PFILE_OBJECT CcFileObject,
    const FSP_FSCTL_FILE_INFO *FileInfo);
BOOLEAN FspFileNodeTrySetFileInfo(FSP_FILE_NODE *FileNode, PFILE_OBJECT CcFileObject,
    const FSP_FSCTL_FILE_INFO *FileInfo, ULONG InfoChangeNumber);
BOOLEAN FspFileNodeReferenceSecurity(FSP_FILE_NODE *FileNode, PCVOID *PBuffer, PULONG PSize);
VOID FspFileNodeSetSecurity(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size);
BOOLEAN FspFileNodeTrySetSecurity(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size,
    ULONG SecurityChangeNumber);
BOOLEAN FspFileNodeReferenceDirInfo(FSP_FILE_NODE *FileNode, PCVOID *PBuffer, PULONG PSize);
VOID FspFileNodeSetDirInfo(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size);
BOOLEAN FspFileNodeTrySetDirInfo(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size,
    ULONG DirInfoChangeNumber);
static VOID FspFileNodeInvalidateDirInfo(FSP_FILE_NODE *FileNode);
VOID FspFileNodeNotifyChange(FSP_FILE_NODE *FileNode,
    ULONG Filter, ULONG Action);
NTSTATUS FspFileNodeProcessLockIrp(FSP_FILE_NODE *FileNode, PIRP Irp);
static NTSTATUS FspFileNodeCompleteLockIrp(PVOID Context, PIRP Irp);
NTSTATUS FspFileDescCreate(FSP_FILE_DESC **PFileDesc);
VOID FspFileDescDelete(FSP_FILE_DESC *FileDesc);
NTSTATUS FspFileDescResetDirectoryPattern(FSP_FILE_DESC *FileDesc,
    PUNICODE_STRING FileName, BOOLEAN Reset);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FspFileNodeCopyList)
#pragma alloc_text(PAGE, FspFileNodeDeleteList)
#pragma alloc_text(PAGE, FspFileNodeCreate)
#pragma alloc_text(PAGE, FspFileNodeDelete)
#pragma alloc_text(PAGE, FspFileNodeAcquireSharedF)
#pragma alloc_text(PAGE, FspFileNodeTryAcquireSharedF)
#pragma alloc_text(PAGE, FspFileNodeAcquireExclusiveF)
#pragma alloc_text(PAGE, FspFileNodeTryAcquireExclusiveF)
#pragma alloc_text(PAGE, FspFileNodeConvertExclusiveToSharedF)
#pragma alloc_text(PAGE, FspFileNodeSetOwnerF)
#pragma alloc_text(PAGE, FspFileNodeReleaseF)
#pragma alloc_text(PAGE, FspFileNodeReleaseOwnerF)
#pragma alloc_text(PAGE, FspFileNodeOpen)
#pragma alloc_text(PAGE, FspFileNodeCleanup)
#pragma alloc_text(PAGE, FspFileNodeCleanupComplete)
#pragma alloc_text(PAGE, FspFileNodeClose)
#pragma alloc_text(PAGE, FspFileNodeFlushAndPurgeCache)
#pragma alloc_text(PAGE, FspFileNodeRename)
#pragma alloc_text(PAGE, FspFileNodeHasOpenHandles)
#pragma alloc_text(PAGE, FspFileNodeGetFileInfo)
#pragma alloc_text(PAGE, FspFileNodeTryGetFileInfo)
#pragma alloc_text(PAGE, FspFileNodeSetFileInfo)
#pragma alloc_text(PAGE, FspFileNodeTrySetFileInfo)
#pragma alloc_text(PAGE, FspFileNodeReferenceSecurity)
#pragma alloc_text(PAGE, FspFileNodeSetSecurity)
#pragma alloc_text(PAGE, FspFileNodeTrySetSecurity)
// !#pragma alloc_text(PAGE, FspFileNodeReferenceDirInfo)
// !#pragma alloc_text(PAGE, FspFileNodeSetDirInfo)
// !#pragma alloc_text(PAGE, FspFileNodeTrySetDirInfo)
// !#pragma alloc_text(PAGE, FspFileNodeInvalidateDirInfo)
#pragma alloc_text(PAGE, FspFileNodeNotifyChange)
#pragma alloc_text(PAGE, FspFileNodeProcessLockIrp)
#pragma alloc_text(PAGE, FspFileNodeCompleteLockIrp)
#pragma alloc_text(PAGE, FspFileDescCreate)
#pragma alloc_text(PAGE, FspFileDescDelete)
#pragma alloc_text(PAGE, FspFileDescResetDirectoryPattern)
#endif

#define FSP_FILE_NODE_GET_FLAGS()       \
    PIRP Irp = IoGetTopLevelIrp();      \
    BOOLEAN IrpValid = (PIRP)FSRTL_MAX_TOP_LEVEL_IRP_FLAG < Irp &&\
        IO_TYPE_IRP == Irp->Type;       \
    if (IrpValid)                       \
        Flags &= ~FspIrpTopFlags(Irp)
#define FSP_FILE_NODE_ASSERT_FLAGS_CLR()\
    ASSERT(IrpValid ? (0 == (FspIrpFlags(Irp) & Flags)) : TRUE)
#define FSP_FILE_NODE_ASSERT_FLAGS_SET()\
    ASSERT(IrpValid ? (Flags == (FspIrpFlags(Irp) & Flags)) : TRUE)
#define FSP_FILE_NODE_SET_FLAGS()       \
    if (IrpValid)                       \
        FspIrpSetFlags(Irp, FspIrpFlags(Irp) | Flags)
#define FSP_FILE_NODE_CLR_FLAGS()       \
    if (IrpValid)                       \
        FspIrpSetFlags(Irp, FspIrpFlags(Irp) & (~Flags & 3))

NTSTATUS FspFileNodeCopyList(PDEVICE_OBJECT DeviceObject,
    FSP_FILE_NODE ***PFileNodes, PULONG PFileNodeCount)
{
    PAGED_CODE();

    NTSTATUS Result;
    ULONG Index;

    FspFsvolDeviceLockContextTable(DeviceObject);
    Result = FspFsvolDeviceCopyContextByNameList(DeviceObject, PFileNodes, PFileNodeCount);
    if (NT_SUCCESS(Result))
    {
        for (Index = 0; *PFileNodeCount > Index; Index++)
            FspFileNodeReference((*PFileNodes)[Index]);
    }
    FspFsvolDeviceUnlockContextTable(DeviceObject);

    return Result;
}

VOID FspFileNodeDeleteList(FSP_FILE_NODE **FileNodes, ULONG FileNodeCount)
{
    PAGED_CODE();

    ULONG Index;

    for (Index = 0; FileNodeCount > Index; Index++)
        FspFileNodeDereference(FileNodes[Index]);

    FspFsvolDeviceDeleteContextByNameList(FileNodes, FileNodeCount);
}

NTSTATUS FspFileNodeCreate(PDEVICE_OBJECT DeviceObject,
    ULONG ExtraSize, FSP_FILE_NODE **PFileNode)
{
    PAGED_CODE();

    *PFileNode = 0;

    FSP_FILE_NODE_NONPAGED *NonPaged = FspAllocNonPaged(sizeof *NonPaged);
    if (0 == NonPaged)
        return STATUS_INSUFFICIENT_RESOURCES;

    FSP_FILE_NODE *FileNode = FspAlloc(sizeof *FileNode + ExtraSize);
    if (0 == FileNode)
    {
        FspFree(NonPaged);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(NonPaged, sizeof *NonPaged);
    ExInitializeResourceLite(&NonPaged->Resource);
    ExInitializeResourceLite(&NonPaged->PagingIoResource);
    ExInitializeFastMutex(&NonPaged->HeaderFastMutex);
    KeInitializeSpinLock(&NonPaged->DirInfoSpinLock);

    RtlZeroMemory(FileNode, sizeof *FileNode + ExtraSize);
    FileNode->Header.NodeTypeCode = FspFileNodeFileKind;
    FileNode->Header.NodeByteSize = sizeof *FileNode;
    FileNode->Header.IsFastIoPossible = FastIoIsNotPossible;
    FileNode->Header.Resource = &NonPaged->Resource;
    FileNode->Header.PagingIoResource = &NonPaged->PagingIoResource;
    FileNode->Header.ValidDataLength.QuadPart = MAXLONGLONG;
        /* disable ValidDataLength functionality */
    FsRtlSetupAdvancedHeader(&FileNode->Header, &NonPaged->HeaderFastMutex);
    FileNode->NonPaged = NonPaged;
    FileNode->RefCount = 1;
    FileNode->FsvolDeviceObject = DeviceObject;
    FspDeviceReference(FileNode->FsvolDeviceObject);
    RtlInitEmptyUnicodeString(&FileNode->FileName, FileNode->FileNameBuf, (USHORT)ExtraSize);

    FsRtlInitializeFileLock(&FileNode->FileLock, FspFileNodeCompleteLockIrp, 0);

    *PFileNode = FileNode;

    return STATUS_SUCCESS;
}

VOID FspFileNodeDelete(FSP_FILE_NODE *FileNode)
{
    PAGED_CODE();

    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension =
        FspFsvolDeviceExtension(FileNode->FsvolDeviceObject);

    FsRtlUninitializeFileLock(&FileNode->FileLock);

    FsRtlTeardownPerStreamContexts(&FileNode->Header);

    FspMetaCacheInvalidateItem(FsvolDeviceExtension->DirInfoCache, FileNode->NonPaged->DirInfo);
    FspMetaCacheInvalidateItem(FsvolDeviceExtension->SecurityCache, FileNode->Security);

    FspDeviceDereference(FileNode->FsvolDeviceObject);

    if (0 != FileNode->ExternalFileName)
        FspFree(FileNode->ExternalFileName);

    ExDeleteResourceLite(&FileNode->NonPaged->PagingIoResource);
    ExDeleteResourceLite(&FileNode->NonPaged->Resource);
    FspFree(FileNode->NonPaged);

    FspFree(FileNode);
}

VOID FspFileNodeAcquireSharedF(FSP_FILE_NODE *FileNode, ULONG Flags)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();
    FSP_FILE_NODE_ASSERT_FLAGS_CLR();

    if (Flags & FspFileNodeAcquireMain)
        ExAcquireResourceSharedLite(FileNode->Header.Resource, TRUE);

    if (Flags & FspFileNodeAcquirePgio)
        ExAcquireResourceSharedLite(FileNode->Header.PagingIoResource, TRUE);

    FSP_FILE_NODE_SET_FLAGS();
}

BOOLEAN FspFileNodeTryAcquireSharedF(FSP_FILE_NODE *FileNode, ULONG Flags, BOOLEAN Wait)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();
    FSP_FILE_NODE_ASSERT_FLAGS_CLR();

    BOOLEAN Result = TRUE;

    if (Flags & FspFileNodeAcquireMain)
    {
        Result = ExAcquireResourceSharedLite(FileNode->Header.Resource, Wait);
        if (!Result)
            return FALSE;
    }

    if (Flags & FspFileNodeAcquirePgio)
    {
        Result = ExAcquireResourceSharedLite(FileNode->Header.PagingIoResource, Wait);
        if (!Result)
        {
            if (Flags & FspFileNodeAcquireMain)
                ExReleaseResourceLite(FileNode->Header.Resource);
            return FALSE;
        }
    }

    if (Result)
        FSP_FILE_NODE_SET_FLAGS();

    return Result;
}

VOID FspFileNodeAcquireExclusiveF(FSP_FILE_NODE *FileNode, ULONG Flags)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();
    FSP_FILE_NODE_ASSERT_FLAGS_CLR();

    if (Flags & FspFileNodeAcquireMain)
        ExAcquireResourceExclusiveLite(FileNode->Header.Resource, TRUE);

    if (Flags & FspFileNodeAcquirePgio)
        ExAcquireResourceExclusiveLite(FileNode->Header.PagingIoResource, TRUE);

    FSP_FILE_NODE_SET_FLAGS();
}

BOOLEAN FspFileNodeTryAcquireExclusiveF(FSP_FILE_NODE *FileNode, ULONG Flags, BOOLEAN Wait)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();
    FSP_FILE_NODE_ASSERT_FLAGS_CLR();

    BOOLEAN Result = TRUE;

    if (Flags & FspFileNodeAcquireMain)
    {
        Result = ExAcquireResourceExclusiveLite(FileNode->Header.Resource, Wait);
        if (!Result)
            return FALSE;
    }

    if (Flags & FspFileNodeAcquirePgio)
    {
        Result = ExAcquireResourceExclusiveLite(FileNode->Header.PagingIoResource, Wait);
        if (!Result)
        {
            if (Flags & FspFileNodeAcquireMain)
                ExReleaseResourceLite(FileNode->Header.Resource);
            return FALSE;
        }
    }

    if (Result)
        FSP_FILE_NODE_SET_FLAGS();

    return Result;
}

VOID FspFileNodeConvertExclusiveToSharedF(FSP_FILE_NODE *FileNode, ULONG Flags)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();

    if (Flags & FspFileNodeAcquirePgio)
        ExConvertExclusiveToSharedLite(FileNode->Header.PagingIoResource);

    if (Flags & FspFileNodeAcquireMain)
        ExConvertExclusiveToSharedLite(FileNode->Header.Resource);
}

VOID FspFileNodeSetOwnerF(FSP_FILE_NODE *FileNode, ULONG Flags, PVOID Owner)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();

    Owner = (PVOID)((UINT_PTR)Owner | 3);

    if (Flags & FspFileNodeAcquireMain)
        ExSetResourceOwnerPointer(FileNode->Header.Resource, Owner);

    if (Flags & FspFileNodeAcquirePgio)
        ExSetResourceOwnerPointer(FileNode->Header.PagingIoResource, Owner);
}

VOID FspFileNodeReleaseF(FSP_FILE_NODE *FileNode, ULONG Flags)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();
    FSP_FILE_NODE_ASSERT_FLAGS_SET();

    if (Flags & FspFileNodeAcquirePgio)
        ExReleaseResourceLite(FileNode->Header.PagingIoResource);

    if (Flags & FspFileNodeAcquireMain)
        ExReleaseResourceLite(FileNode->Header.Resource);

    FSP_FILE_NODE_CLR_FLAGS();
}

VOID FspFileNodeReleaseOwnerF(FSP_FILE_NODE *FileNode, ULONG Flags, PVOID Owner)
{
    PAGED_CODE();

    FSP_FILE_NODE_GET_FLAGS();
    FSP_FILE_NODE_ASSERT_FLAGS_SET();

    Owner = (PVOID)((UINT_PTR)Owner | 3);

    if (Flags & FspFileNodeAcquirePgio)
    {
        if (ExIsResourceAcquiredLite(FileNode->Header.PagingIoResource))
            ExReleaseResourceLite(FileNode->Header.PagingIoResource);
        else
            ExReleaseResourceForThreadLite(FileNode->Header.PagingIoResource, (ERESOURCE_THREAD)Owner);
    }

    if (Flags & FspFileNodeAcquireMain)
    {
        if (ExIsResourceAcquiredLite(FileNode->Header.Resource))
            ExReleaseResourceLite(FileNode->Header.Resource);
        else
            ExReleaseResourceForThreadLite(FileNode->Header.Resource, (ERESOURCE_THREAD)Owner);
    }

    FSP_FILE_NODE_CLR_FLAGS();
}

FSP_FILE_NODE *FspFileNodeOpen(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject,
    UINT32 GrantedAccess, UINT32 ShareAccess, NTSTATUS *PResult)
{
    /*
     * Attempt to insert our FileNode into the volume device's generic table.
     * If an FileNode with the same UserContext already exists, then use that
     * FileNode instead.
     *
     * There is no FileNode that can be acquired when calling this function.
     */

    PAGED_CODE();

    PDEVICE_OBJECT FsvolDeviceObject = FileNode->FsvolDeviceObject;
    FSP_FILE_NODE *OpenedFileNode;
    BOOLEAN Inserted, DeletePending;
    NTSTATUS Result;

    FspFsvolDeviceLockContextTable(FsvolDeviceObject);

    OpenedFileNode = FspFsvolDeviceInsertContextByName(FsvolDeviceObject,
        &FileNode->FileName, FileNode, &FileNode->ContextByNameElementStorage, &Inserted);
    ASSERT(0 != OpenedFileNode);

    if (Inserted)
    {
        /*
         * The new FileNode was inserted into the Context table. Set its share access
         * and reference and open it. There should be (at least) two references to this
         * FileNode, one from our caller and one from the Context table.
         */
        ASSERT(OpenedFileNode == FileNode);

        IoSetShareAccess(GrantedAccess, ShareAccess, FileObject,
            &OpenedFileNode->ShareAccess);
    }
    else
    {
        /*
         * The new FileNode was NOT inserted into the Context table. Instead we are
         * opening a prior FileNode that we found in the table.
         */
        ASSERT(OpenedFileNode != FileNode);

        DeletePending = 0 != OpenedFileNode->DeletePending;
        MemoryBarrier();
        if (DeletePending)
        {
            Result = STATUS_DELETE_PENDING;
            goto exit;
        }

        /*
         * FastFat says to do the following on Vista and above.
         *
         * Quote:
         *     Do an extra test for writeable user sections if the user did not allow
         *     write sharing - this is neccessary since a section may exist with no handles
         *     open to the file its based against.
         */
        if (!FlagOn(ShareAccess, FILE_SHARE_WRITE) &&
            FlagOn(GrantedAccess,
                FILE_EXECUTE | FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA | DELETE) &&
            MmDoesFileHaveUserWritableReferences(&OpenedFileNode->NonPaged->SectionObjectPointers))
        {
            Result = STATUS_SHARING_VIOLATION;
            goto exit;
        }

        /* share access check */
        Result = IoCheckShareAccess(GrantedAccess, ShareAccess, FileObject,
            &OpenedFileNode->ShareAccess, TRUE);

    exit:
        if (!NT_SUCCESS(Result))
        {
            if (0 != PResult)
                *PResult = Result;

            OpenedFileNode = 0;
        }
    }

    if (0 != OpenedFileNode)
    {
        FspFileNodeReference(OpenedFileNode);
        OpenedFileNode->OpenCount++;
        OpenedFileNode->HandleCount++;
    }

    FspFsvolDeviceUnlockContextTable(FsvolDeviceObject);

    return OpenedFileNode;
}

VOID FspFileNodeCleanup(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject,
    PBOOLEAN PDeletePending)
{
    /*
     * Determine whether a FileNode should be deleted. Note that when FileNode->DeletePending
     * is set, the OpenCount/HandleCount cannot be increased because FspFileNodeOpen() will
     * return STATUS_DELETE_PENDING.
     *
     * The FileNode must be acquired exclusive (Main) when calling this function.
     */

    PAGED_CODE();

    PDEVICE_OBJECT FsvolDeviceObject = FileNode->FsvolDeviceObject;
    FSP_FILE_DESC *FileDesc = FileObject->FsContext2;
    BOOLEAN DeletePending, SingleHandle;

    FspFsvolDeviceLockContextTable(FsvolDeviceObject);

    if (FileDesc->DeleteOnClose)
        FileNode->DeletePending = TRUE;
    DeletePending = 0 != FileNode->DeletePending;
    MemoryBarrier();

    SingleHandle = 1 == FileNode->HandleCount;

    FspFsvolDeviceUnlockContextTable(FsvolDeviceObject);

    if (0 != PDeletePending)
        *PDeletePending = SingleHandle && DeletePending;
}

VOID FspFileNodeCleanupComplete(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject)
{
    /*
     * Complete the cleanup of a FileNode. Remove its share access and
     * finalize its cache.
     *
     * NOTE: If the FileNode is not being deleted (!FileNode->DeletePending)
     * the FileNode REMAINS in the Context table until Close time!
     * This is so that if there are mapped views or write behind's pending
     * when a file gets reopened the FileNode will be correctly reused.
     *
     * The FileNode must be acquired exclusive (Main) when calling this function.
     */

    PAGED_CODE();

    PDEVICE_OBJECT FsvolDeviceObject = FileNode->FsvolDeviceObject;
    LARGE_INTEGER TruncateSize = { 0 }, *PTruncateSize = 0;
    BOOLEAN DeletePending;
    BOOLEAN DeletedFromContextTable = FALSE;

    FspFsvolDeviceLockContextTable(FsvolDeviceObject);

    IoRemoveShareAccess(FileObject, &FileNode->ShareAccess);

    if (0 == --FileNode->HandleCount)
    {
        DeletePending = 0 != FileNode->DeletePending;
        MemoryBarrier();

        if (DeletePending)
        {
            PTruncateSize = &TruncateSize;

            if (0 == --FileNode->OpenCount)
                FspFsvolDeviceDeleteContextByName(FsvolDeviceObject, &FileNode->FileName,
                    &DeletedFromContextTable);
        }
        else if (FileNode->TruncateOnClose && FlagOn(FileObject->Flags, FO_CACHE_SUPPORTED))
        {
            /*
             * Even when the FileInfo is expired, this is the best guess for a file size
             * without asking the user-mode file system.
             */
            TruncateSize = FileNode->Header.FileSize;
            PTruncateSize = &TruncateSize;
        }
    }

    FspFsvolDeviceUnlockContextTable(FsvolDeviceObject);

    CcUninitializeCacheMap(FileObject, PTruncateSize, 0);

    if (DeletedFromContextTable)
        FspFileNodeDereference(FileNode);
}

VOID FspFileNodeClose(FSP_FILE_NODE *FileNode, PFILE_OBJECT FileObject)
{
    /*
     * Close the FileNode. If the OpenCount becomes zero remove it
     * from the Context table.
     *
     * The FileNode may or may not be acquired when calling this function.
     */

    PAGED_CODE();

    PDEVICE_OBJECT FsvolDeviceObject = FileNode->FsvolDeviceObject;
    BOOLEAN DeletedFromContextTable = FALSE;

    FspFsvolDeviceLockContextTable(FsvolDeviceObject);

    if (0 < FileNode->OpenCount && 0 == --FileNode->OpenCount)
        FspFsvolDeviceDeleteContextByName(FsvolDeviceObject, &FileNode->FileName,
            &DeletedFromContextTable);

    FspFsvolDeviceUnlockContextTable(FsvolDeviceObject);

    if (DeletedFromContextTable)
        FspFileNodeDereference(FileNode);
}

NTSTATUS FspFileNodeFlushAndPurgeCache(FSP_FILE_NODE *FileNode,
    UINT64 FlushOffset64, ULONG FlushLength, BOOLEAN FlushAndPurge)
{
    /*
     * The FileNode must be acquired exclusive (Full) when calling this function.
     */

    PAGED_CODE();

    LARGE_INTEGER FlushOffset;
    PLARGE_INTEGER PFlushOffset = &FlushOffset;
    FSP_FSCTL_FILE_INFO FileInfo;
    IO_STATUS_BLOCK IoStatus = { STATUS_SUCCESS };

    FlushOffset.QuadPart = FlushOffset64;
    if (FILE_WRITE_TO_END_OF_FILE == FlushOffset.LowPart && -1L == FlushOffset.HighPart)
    {
        if (FspFileNodeTryGetFileInfo(FileNode, &FileInfo))
            FlushOffset.QuadPart = FileInfo.FileSize;
        else
            PFlushOffset = 0; /* we don't know how big the file is, so flush it all! */
    }

    /* it is unclear if the Cc* calls below can raise or not; wrap them just in case */
    try
    {
        if (0 != FspMvCcCoherencyFlushAndPurgeCache)
        {
            /* if we are on Win7+ use CcCoherencyFlushAndPurgeCache */
            FspMvCcCoherencyFlushAndPurgeCache(
                &FileNode->NonPaged->SectionObjectPointers, PFlushOffset, FlushLength, &IoStatus,
                FlushAndPurge ? 0 : CC_FLUSH_AND_PURGE_NO_PURGE);

            if (STATUS_CACHE_PAGE_LOCKED == IoStatus.Status)
                IoStatus.Status = STATUS_SUCCESS;
        }
        else
        {
            /* do it the old-fashioned way; non-cached and mmap'ed I/O are non-coherent */
            CcFlushCache(&FileNode->NonPaged->SectionObjectPointers, PFlushOffset, FlushLength, &IoStatus);
            if (NT_SUCCESS(IoStatus.Status))
            {
                if (FlushAndPurge)
                    CcPurgeCacheSection(&FileNode->NonPaged->SectionObjectPointers, PFlushOffset, FlushLength, FALSE);

                IoStatus.Status = STATUS_SUCCESS;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        IoStatus.Status = GetExceptionCode();
    }

    return IoStatus.Status;
}

VOID FspFileNodeRename(FSP_FILE_NODE *FileNode, PUNICODE_STRING NewFileName)
{
    PAGED_CODE();

    PDEVICE_OBJECT FsvolDeviceObject = FileNode->FsvolDeviceObject;
    BOOLEAN Deleted, Inserted;

    FspFsvolDeviceLockContextTable(FsvolDeviceObject);

    FspFsvolDeviceDeleteContextByName(FsvolDeviceObject, &FileNode->FileName, &Deleted);
    ASSERT(Deleted);

    if (0 != FileNode->ExternalFileName)
        FspFree(FileNode->ExternalFileName);
    FileNode->FileName = *NewFileName;
    FileNode->ExternalFileName = NewFileName->Buffer;

    FspFsvolDeviceInsertContextByName(FsvolDeviceObject, &FileNode->FileName, FileNode,
        &FileNode->ContextByNameElementStorage, &Inserted);
    ASSERT(Inserted);

    FspFsvolDeviceUnlockContextTable(FsvolDeviceObject);
}

BOOLEAN FspFileNodeHasOpenHandles(PDEVICE_OBJECT FsvolDeviceObject,
    PUNICODE_STRING FileName, BOOLEAN SubpathOnly)
{
    /*
     * The ContextByNameTable must be already locked.
     */

    PAGED_CODE();

    FSP_FILE_NODE *FileNode;
    PVOID RestartKey = 0;

    for (;;)
    {
        FileNode = FspFsvolDeviceEnumerateContextByName(FsvolDeviceObject, FileName, SubpathOnly,
            &RestartKey);
        if (0 == FileNode)
            return FALSE;
        if (0 < FileNode->HandleCount)
            return TRUE;
    }
}

VOID FspFileNodeGetFileInfo(FSP_FILE_NODE *FileNode, FSP_FSCTL_FILE_INFO *FileInfo)
{
    PAGED_CODE();

    FileInfo->AllocationSize = FileNode->Header.AllocationSize.QuadPart;
    FileInfo->FileSize = FileNode->Header.FileSize.QuadPart;

    FileInfo->FileAttributes = FileNode->FileAttributes;
    FileInfo->ReparseTag = FileNode->ReparseTag;
    FileInfo->CreationTime = FileNode->CreationTime;
    FileInfo->LastAccessTime = FileNode->LastAccessTime;
    FileInfo->LastWriteTime = FileNode->LastWriteTime;
    FileInfo->ChangeTime = FileNode->ChangeTime;
}

BOOLEAN FspFileNodeTryGetFileInfo(FSP_FILE_NODE *FileNode, FSP_FSCTL_FILE_INFO *FileInfo)
{
    PAGED_CODE();

    BOOLEAN Result;

    if (FspExpirationTimeValid(FileNode->InfoExpirationTime))
    {
        FileInfo->AllocationSize = FileNode->Header.AllocationSize.QuadPart;
        FileInfo->FileSize = FileNode->Header.FileSize.QuadPart;

        FileInfo->FileAttributes = FileNode->FileAttributes;
        FileInfo->ReparseTag = FileNode->ReparseTag;
        FileInfo->CreationTime = FileNode->CreationTime;
        FileInfo->LastAccessTime = FileNode->LastAccessTime;
        FileInfo->LastWriteTime = FileNode->LastWriteTime;
        FileInfo->ChangeTime = FileNode->ChangeTime;
        Result = TRUE;
    }
    else
        Result = FALSE;

    return Result;
}

VOID FspFileNodeSetFileInfo(FSP_FILE_NODE *FileNode, PFILE_OBJECT CcFileObject,
    const FSP_FSCTL_FILE_INFO *FileInfo)
{
    PAGED_CODE();

    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension =
        FspFsvolDeviceExtension(FileNode->FsvolDeviceObject);
    UINT64 AllocationSize = FileInfo->AllocationSize > FileInfo->FileSize ?
        FileInfo->AllocationSize : FileInfo->FileSize;
    UINT64 AllocationUnit;

    AllocationUnit = FsvolDeviceExtension->VolumeParams.SectorSize *
        FsvolDeviceExtension->VolumeParams.SectorsPerAllocationUnit;
    AllocationSize = (AllocationSize + AllocationUnit - 1) / AllocationUnit * AllocationUnit;

    FileNode->Header.AllocationSize.QuadPart = AllocationSize;
    FileNode->Header.FileSize.QuadPart = FileInfo->FileSize;

    FileNode->FileAttributes = FileInfo->FileAttributes;
    FileNode->ReparseTag = FileInfo->ReparseTag;
    FileNode->CreationTime = FileInfo->CreationTime;
    FileNode->LastAccessTime = FileInfo->LastAccessTime;
    FileNode->LastWriteTime = FileInfo->LastWriteTime;
    FileNode->ChangeTime = FileInfo->ChangeTime;
    FileNode->InfoExpirationTime = FspExpirationTimeFromMillis(
        FsvolDeviceExtension->VolumeParams.FileInfoTimeout);
    FileNode->InfoChangeNumber++;

    if (0 != CcFileObject)
    {
        NTSTATUS Result = FspCcSetFileSizes(
            CcFileObject, (PCC_FILE_SIZES)&FileNode->Header.AllocationSize);
        if (!NT_SUCCESS(Result))
        {
            DEBUGLOG("FspCcSetFileSizes error: %s", NtStatusSym(Result));
            DEBUGBREAK_CRIT();
            CcUninitializeCacheMap(CcFileObject, 0, 0);
        }
    }
}

BOOLEAN FspFileNodeTrySetFileInfo(FSP_FILE_NODE *FileNode, PFILE_OBJECT CcFileObject,
    const FSP_FSCTL_FILE_INFO *FileInfo, ULONG InfoChangeNumber)
{
    PAGED_CODE();

    if (FileNode->InfoChangeNumber != InfoChangeNumber)
        return FALSE;

    FspFileNodeSetFileInfo(FileNode, CcFileObject, FileInfo);
    return TRUE;
}

BOOLEAN FspFileNodeReferenceSecurity(FSP_FILE_NODE *FileNode, PCVOID *PBuffer, PULONG PSize)
{
    PAGED_CODE();

    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension =
        FspFsvolDeviceExtension(FileNode->FsvolDeviceObject);

    return FspMetaCacheReferenceItemBuffer(FsvolDeviceExtension->SecurityCache,
        FileNode->Security, PBuffer, PSize);
}

VOID FspFileNodeSetSecurity(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size)
{
    PAGED_CODE();

    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension =
        FspFsvolDeviceExtension(FileNode->FsvolDeviceObject);

    FspMetaCacheInvalidateItem(FsvolDeviceExtension->SecurityCache, FileNode->Security);
    FileNode->Security = 0 != Buffer ?
        FspMetaCacheAddItem(FsvolDeviceExtension->SecurityCache, Buffer, Size) : 0;
    FileNode->SecurityChangeNumber++;
}

BOOLEAN FspFileNodeTrySetSecurity(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size,
    ULONG SecurityChangeNumber)
{
    PAGED_CODE();

    if (FileNode->SecurityChangeNumber != SecurityChangeNumber)
        return FALSE;

    FspFileNodeSetSecurity(FileNode, Buffer, Size);
    return TRUE;
}

BOOLEAN FspFileNodeReferenceDirInfo(FSP_FILE_NODE *FileNode, PCVOID *PBuffer, PULONG PSize)
{
    // !PAGED_CODE();

    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension =
        FspFsvolDeviceExtension(FileNode->FsvolDeviceObject);
    FSP_FILE_NODE_NONPAGED *NonPaged = FileNode->NonPaged;
    UINT64 DirInfo;

    /* no need to acquire the DirInfoSpinLock as the FileNode is acquired */
    DirInfo = NonPaged->DirInfo;

    return FspMetaCacheReferenceItemBuffer(FsvolDeviceExtension->DirInfoCache,
        DirInfo, PBuffer, PSize);
}

VOID FspFileNodeSetDirInfo(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size)
{
    // !PAGED_CODE();

    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension =
        FspFsvolDeviceExtension(FileNode->FsvolDeviceObject);
    FSP_FILE_NODE_NONPAGED *NonPaged = FileNode->NonPaged;
    KIRQL Irql;
    UINT64 DirInfo;

    /* no need to acquire the DirInfoSpinLock as the FileNode is acquired */
    DirInfo = NonPaged->DirInfo;

    FspMetaCacheInvalidateItem(FsvolDeviceExtension->DirInfoCache, DirInfo);
    DirInfo = 0 != Buffer ?
        FspMetaCacheAddItem(FsvolDeviceExtension->DirInfoCache, Buffer, Size) : 0;
    FileNode->DirInfoChangeNumber++;

    /* acquire the DirInfoSpinLock to protect against concurrent FspFileNodeInvalidateDirInfo */
    KeAcquireSpinLock(&NonPaged->DirInfoSpinLock, &Irql);
    NonPaged->DirInfo = DirInfo;
    KeReleaseSpinLock(&NonPaged->DirInfoSpinLock, Irql);
}

BOOLEAN FspFileNodeTrySetDirInfo(FSP_FILE_NODE *FileNode, PCVOID Buffer, ULONG Size,
    ULONG DirInfoChangeNumber)
{
    // !PAGED_CODE();

    if (FileNode->DirInfoChangeNumber != DirInfoChangeNumber)
        return FALSE;

    FspFileNodeSetDirInfo(FileNode, Buffer, Size);
    return TRUE;
}

static VOID FspFileNodeInvalidateDirInfo(FSP_FILE_NODE *FileNode)
{
    // !PAGED_CODE();

    PDEVICE_OBJECT FsvolDeviceObject = FileNode->FsvolDeviceObject;
    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension = FspFsvolDeviceExtension(FsvolDeviceObject);
    FSP_FILE_NODE_NONPAGED *NonPaged = FileNode->NonPaged;
    KIRQL Irql;
    UINT64 DirInfo;

    /* acquire the DirInfoSpinLock to protect against concurrent FspFileNodeSetDirInfo */
    KeAcquireSpinLock(&NonPaged->DirInfoSpinLock, &Irql);
    DirInfo = NonPaged->DirInfo;
    KeReleaseSpinLock(&NonPaged->DirInfoSpinLock, Irql);

    FspMetaCacheInvalidateItem(FsvolDeviceExtension->DirInfoCache, DirInfo);
}

VOID FspFileNodeNotifyChange(FSP_FILE_NODE *FileNode,
    ULONG Filter, ULONG Action)
{
    /* FileNode must be acquired (exclusive or shared) Main */

    PAGED_CODE();

    PDEVICE_OBJECT FsvolDeviceObject = FileNode->FsvolDeviceObject;
    FSP_FSVOL_DEVICE_EXTENSION *FsvolDeviceExtension = FspFsvolDeviceExtension(FsvolDeviceObject);
    UNICODE_STRING Parent, Suffix;
    FSP_FILE_NODE *ParentNode;

    FspUnicodePathSuffix(&FileNode->FileName, &Parent, &Suffix);

    switch (Action)
    {
    case FILE_ACTION_ADDED:
    case FILE_ACTION_REMOVED:
    //case FILE_ACTION_MODIFIED:
    case FILE_ACTION_RENAMED_OLD_NAME:
    case FILE_ACTION_RENAMED_NEW_NAME:
        FspFsvolDeviceInvalidateVolumeInfo(FsvolDeviceObject);

        FspFsvolDeviceLockContextTable(FsvolDeviceObject);
        ParentNode = FspFsvolDeviceLookupContextByName(FsvolDeviceObject, &Parent);
        if (0 != ParentNode)
            FspFileNodeReference(ParentNode);
        FspFsvolDeviceUnlockContextTable(FsvolDeviceObject);

        if (0 != ParentNode)
        {
            FspFileNodeInvalidateDirInfo(ParentNode);
            FspFileNodeDereference(ParentNode);
        }
        break;
    }

    FspNotifyReportChange(
        FsvolDeviceExtension->NotifySync, &FsvolDeviceExtension->NotifyList,
        &FileNode->FileName,
        (USHORT)((PUINT8)Suffix.Buffer - (PUINT8)FileNode->FileName.Buffer),
        0, Filter, Action);
}

NTSTATUS FspFileNodeProcessLockIrp(FSP_FILE_NODE *FileNode, PIRP Irp)
{
    PAGED_CODE();

    IoMarkIrpPending(Irp);
    FspFileNodeSetOwnerF(FileNode, FspIrpFlags(Irp), Irp);

    try
    {
        FsRtlProcessFileLock(&FileNode->FileLock, Irp, FileNode);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Irp->IoStatus.Status = GetExceptionCode();
        Irp->IoStatus.Information = 0;

        FspFileNodeCompleteLockIrp(FileNode, Irp);
    }

    return STATUS_PENDING;
}

static NTSTATUS FspFileNodeCompleteLockIrp(PVOID Context, PIRP Irp)
{
    PAGED_CODE();

    FSP_FILE_NODE *FileNode = Context;
    NTSTATUS Result = Irp->IoStatus.Status;

    FspFileNodeReleaseOwnerF(FileNode, FspIrpFlags(Irp), Irp);

    DEBUGLOGIRP(Irp, Result);

    FspIopCompleteIrp(Irp, Result);

    return Result;
}

NTSTATUS FspFileDescCreate(FSP_FILE_DESC **PFileDesc)
{
    PAGED_CODE();

    *PFileDesc = FspAlloc(sizeof(FSP_FILE_DESC));
    if (0 == *PFileDesc)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(*PFileDesc, sizeof(FSP_FILE_DESC));

    return STATUS_SUCCESS;
}

VOID FspFileDescDelete(FSP_FILE_DESC *FileDesc)
{
    PAGED_CODE();

    if (0 != FileDesc->DirectoryPattern.Buffer &&
        FspFileDescDirectoryPatternMatchAll != FileDesc->DirectoryPattern.Buffer)
    {
        if (FileDesc->CaseSensitive)
            FspFree(FileDesc->DirectoryPattern.Buffer);
        else
            RtlFreeUnicodeString(&FileDesc->DirectoryPattern);
    }

    FspFree(FileDesc);
}

NTSTATUS FspFileDescResetDirectoryPattern(FSP_FILE_DESC *FileDesc,
    PUNICODE_STRING FileName, BOOLEAN Reset)
{
    PAGED_CODE();

    if (Reset || 0 == FileDesc->DirectoryPattern.Buffer)
    {
        UNICODE_STRING DirectoryPattern;

        if (0 == FileName || (sizeof(WCHAR) == FileName->Length && L'*' == FileName->Buffer[0]))
        {
            DirectoryPattern.Length = DirectoryPattern.MaximumLength = sizeof(WCHAR); /* L"*" */
            DirectoryPattern.Buffer = FspFileDescDirectoryPatternMatchAll;
        }
        else
        {
            if (FileDesc->CaseSensitive)
            {
                DirectoryPattern.Length = DirectoryPattern.MaximumLength = FileName->Length;
                DirectoryPattern.Buffer = FspAlloc(FileName->Length);
                if (0 == DirectoryPattern.Buffer)
                    return STATUS_INSUFFICIENT_RESOURCES;
                RtlCopyMemory(DirectoryPattern.Buffer, FileName->Buffer, FileName->Length);
            }
            else
            {
                NTSTATUS Result = RtlUpcaseUnicodeString(&DirectoryPattern, FileName, TRUE);
                if (!NT_SUCCESS(Result))
                    return Result;
            }
        }

        if (0 != FileDesc->DirectoryPattern.Buffer &&
            FspFileDescDirectoryPatternMatchAll != FileDesc->DirectoryPattern.Buffer)
        {
            if (FileDesc->CaseSensitive)
                FspFree(FileDesc->DirectoryPattern.Buffer);
            else
                RtlFreeUnicodeString(&FileDesc->DirectoryPattern);
        }

        FileDesc->DirectoryPattern = DirectoryPattern;
    }

    return STATUS_SUCCESS;
}

WCHAR FspFileDescDirectoryPatternMatchAll[] = L"*";
