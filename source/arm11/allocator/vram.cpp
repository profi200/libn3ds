#include "types.h"
extern "C"
{
	#include "mem_map.h"
	#include "arm11/allocator/vram.h"
	#include "arm11/util/rbtree.h"
}

#include "mem_pool.h"
#include "addrmap.h"

static MemPool sVramPoolA, sVramPoolB;

static bool vramInit()
{
	if (sVramPoolA.Ready() || sVramPoolB.Ready())
		return true;

	auto blkA = MemBlock::Create((u8*)VRAM_BANK0, VRAM_BANK_SIZE);
	if (!blkA)
		return false;

	auto blkB = MemBlock::Create((u8*)VRAM_BANK1, VRAM_BANK_SIZE);
	if (!blkB)
	{
		free(blkA);
		return false;
	}

	sVramPoolA.AddBlock(blkA);
	sVramPoolB.AddBlock(blkB);
	rbtree_init(&sAddrMap, addrMapNodeComparator);
	return true;
}

static MemPool* vramPoolForAddr(void* addr)
{
	uintptr_t addr_ = (uintptr_t)addr;
	if (addr_ < VRAM_BASE)
		return nullptr;
	if (addr_ < VRAM_BANK1)
		return &sVramPoolA;
	if (addr_ < VRAM_BASE + VRAM_SIZE)
		return &sVramPoolB;
	return nullptr;
}

void* vramAlloc(size_t size)
{
	return vramMemAlignAt(size, 0x80, VRAM_ALLOC_ANY);
}

void* vramAllocAt(size_t size, vramAllocPos pos)
{
	return vramMemAlignAt(size, 0x80, pos);
}

void* vramMemAlign(size_t size, size_t alignment)
{
	return vramMemAlignAt(size, alignment, VRAM_ALLOC_ANY);
}

void* vramMemAlignAt(size_t size, size_t alignment, vramAllocPos pos)
{
	// Convert alignment to shift
	int shift = alignmentToShift(alignment);
	if (shift < 0)
		return nullptr;

	// Initialize the allocator if it is not ready
	if (!vramInit())
		return nullptr;

	// Allocate the chunk
	MemChunk chunk;
	bool didAlloc = false;
	switch (pos & VRAM_ALLOC_ANY)
	{
		default:
			break;
		case VRAM_ALLOC_A:
			didAlloc = sVramPoolA.Allocate(chunk, size, shift);
			break;
		case VRAM_ALLOC_B:
			didAlloc = sVramPoolB.Allocate(chunk, size, shift);
			break;
		case VRAM_ALLOC_ANY:
		{
			// Crude attempt at "load balancing" VRAM A and B
			bool prefer_a = sVramPoolA.GetFreeSpace() >= sVramPoolB.GetFreeSpace();
			MemPool& firstPool = prefer_a ? sVramPoolA : sVramPoolB;
			MemPool& secondPool = prefer_a ? sVramPoolB : sVramPoolA;

			didAlloc = firstPool.Allocate(chunk, size, shift);
			if (!didAlloc) didAlloc = secondPool.Allocate(chunk, size, shift);
			break;
		}
	}

	if (!didAlloc)
		return nullptr;

	auto node = newNode(chunk);
	if (!node)
	{
		vramPoolForAddr(chunk.addr)->Deallocate(chunk);
		return nullptr;
	}
	if (rbtree_insert(&sAddrMap, &node->node)) {}
	return chunk.addr;
}

void* vramRealloc(void* mem, size_t size)
{
	(void)mem;
	(void)size;

	// TODO
	return NULL;
}

size_t vramGetSize(void* mem)
{
	auto node = getNode(mem);
	return node ? node->chunk.size : 0;
}

void vramFree(void* mem)
{
	auto node = getNode(mem);
	if (!node) return;

	// Free the chunk
	vramPoolForAddr(mem)->Deallocate(node->chunk);

	// Free the node
	delNode(node);
}

u32 vramSpaceFree()
{
	return sVramPoolA.GetFreeSpace() + sVramPoolB.GetFreeSpace();
}