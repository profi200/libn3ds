/**
 * @file fcram.h
 * @brief FCRAM allocator.
 */
#pragma once



/**
 * @brief Allocates a 8-byte aligned buffer.
 * @param size Size of the buffer to allocate.
 * @return The allocated buffer.
 */
void* fcramAlloc(size_t size);

/**
 * @brief Allocates a buffer aligned to the given size.
 * @param size Size of the buffer to allocate.
 * @param alignment Alignment to use.
 * @return The allocated buffer.
 */
void* fcramMemAlign(size_t size, size_t alignment);

/**
 * @brief Reallocates a buffer.
 * Note: Not implemented yet.
 * @param mem Buffer to reallocate.
 * @param size Size of the buffer to allocate.
 * @return The reallocated buffer.
 */
//void* fcramRealloc(void* mem, size_t size);

/**
 * @brief Retrieves the allocated size of a buffer.
 * @return The size of the buffer.
 */
size_t fcramGetSize(void* mem);

/**
 * @brief Frees a buffer.
 * @param mem Buffer to free.
 */
void fcramFree(void* mem);

/**
 * @brief Gets the current FCRAM free space.
 * @return The current FCRAM free space.
 */
u32 fcramSpaceFree(void);