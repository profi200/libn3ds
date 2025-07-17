#include "types.h"
extern "C"
{
	#include "mem_map.h"
	#include "arm11/allocator/fcram.h"
	#include "arm11/util/rbtree.h"
	#include "arm11/drivers/cfg11.h"
}

#include "mem_pool.h"
#include "addrmap.h"


static MemPool g_fcramPool;



static bool fcramInit()
{
	if (g_fcramPool.Ready())
		return true;

	// N2DS and N3DS have twice as much FCRAM.
	const bool isLgr2 = !!(getCfg11Regs()->socinfo & SOCINFO_LGR2);
	auto blk = MemBlock::Create((u8*)FCRAM_BASE, (isLgr2 ? FCRAM_SIZE + FCRAM_EXT_SIZE : FCRAM_SIZE));
	if (!blk)
		return false;

	g_fcramPool.AddBlock(blk);
	rbtree_init(&sAddrMap, addrMapNodeComparator);
	return true;
}

void* fcramAlloc(size_t size)
{
	// Note: 8 bytes is also the default alignment for malloc().
	return fcramMemAlign(size, 8);
}

void* fcramMemAlign(size_t size, size_t alignment)
{
	// Convert alignment to shift
	int shift = alignmentToShift(alignment);
	if (shift < 0)
		return nullptr;

	// Initialize the allocator if it is not ready
	if (!fcramInit())
		return nullptr;

	// Allocate the chunk
	MemChunk chunk;
	if (!g_fcramPool.Allocate(chunk, size, shift))
		return nullptr;

	auto node = newNode(chunk);
	if (!node)
	{
		g_fcramPool.Deallocate(chunk);
		return nullptr;
	}
	if (rbtree_insert(&sAddrMap, &node->node)) {}
	return chunk.addr;
}

#if 0
void* fcramRealloc(void* mem, size_t size)
{
	(void)mem;
	(void)size;

	// TODO
	return NULL;
}
#endif

size_t fcramGetSize(void* mem)
{
	auto node = getNode(mem);
	return node ? node->chunk.size : 0;
}

void fcramFree(void* mem)
{
	auto node = getNode(mem);
	if (!node) return;

	// Free the chunk
	g_fcramPool.Deallocate(node->chunk);

	// Free the node
	delNode(node);
}

u32 fcramSpaceFree()
{
	return g_fcramPool.GetFreeSpace();
}