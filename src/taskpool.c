/**
 * taskpool.c
 * Task Pool Management
 */

#include "listpool.h"
#include "taskpool.h"
#include "malloc.h"
#include "main.h"
#include <stddef.h>
#include "types.h"

#ifdef PLATFORM_PC
#include "btl_collision.h"

/* Tasks live in the 0x34000-byte EWRAM heap and are four-byte aligned.
 * Index ownership by heap address rather than changing the cartridge Task
 * layout or using a bounded hash table that could silently lose an owner.
 * This is native storage, outside EWRAM; it costs 208 KiB on the game ABI.
 * Like the task itself, the owning battle actor must outlive its callbacks. */
static BtlObj* sTaskAttackSources[EWRAM_HEAP_SIZE / 4];

static BtlObj** TaskAttackSourceSlot(Task* task) {
    __UINTPTR_TYPE__ address = (__UINTPTR_TYPE__)task;
    __UINTPTR_TYPE__ start = (__UINTPTR_TYPE__)GetEwramHeapStart();
    __UINTPTR_TYPE__ offset;

    if (address < start) {
        return NULL;
    }

    offset = address - start;

    if (offset >= GetEwramHeapSize() || offset >= EWRAM_HEAP_SIZE || (offset & 3) != 0) {
        return NULL;
    }

    return &sTaskAttackSources[offset / 4];
}

static BtlObj* TaskAttackSource(Task* task) {
    BtlObj** slot = TaskAttackSourceSlot(task);
    return slot != NULL ? *slot : NULL;
}

static void SetTaskAttackSource(Task* task, BtlObj* source) {
    BtlObj** slot = TaskAttackSourceSlot(task);

    if (slot != NULL) {
        *slot = source;
    }
}
#endif

Task* TaskDestroy(TaskPool* pool, Task* task) {
#ifdef PLATFORM_PC
    BtlObj* previous = BtlSetAttackSource(TaskAttackSource(task));
    SetTaskAttackSource(task, NULL);
#endif
    if (task->desc->destroy != NULL) {
        task->desc->destroy(task->work);
    }
#ifdef PLATFORM_PC
    BtlSetAttackSource(previous);
#endif

    EwramFree(task->work);

    return ListPoolRelease(&task->node, pool);
}

void TaskKill(TaskPool* pool, Task* task) {
#ifdef PLATFORM_PC
    BtlObj* previous = BtlSetAttackSource(TaskAttackSource(task));
    SetTaskAttackSource(task, NULL);
#endif
    if (task->desc->destroy != NULL) {
        task->desc->destroy(task->work);
    }
#ifdef PLATFORM_PC
    BtlSetAttackSource(previous);
#endif

    EwramFree(task->work);

    ListPoolRelease(&task->node, pool);
}

Task* TaskCreate(TaskPool* pool, TaskDesc* desc, const void* arg) {
    Task* task;

    task = ListPoolFirstFree(pool);

    if (task == NULL) {
        return NULL;
    }

#ifdef PLATFORM_PC
    /* Free slots may have survived a heap reset or a previous allocation. */
    SetTaskAttackSource(task, NULL);
#endif

    if (desc->workSize > 0) {
        task->work = EwramAlloc(desc->workSize);

        if (task->work == NULL) {
            return NULL;
        }
    } else {
        task->work = NULL;
    }

    task->desc = desc;
    task->update = desc->update;
#ifdef PLATFORM_PC
    SetTaskAttackSource(task, BtlGetAttackSource());
#endif
    ListPoolActivate(&task->node, pool);

    if (desc->init != NULL) {
#ifdef PLATFORM_PC
        BtlObj* previous = BtlSetAttackSource(TaskAttackSource(task));
#endif
        desc->init(task->work, arg);
#ifdef PLATFORM_PC
        BtlSetAttackSource(previous);
#endif
    }

    return task;
}

void TaskPoolInit(TaskPool* pool, s32 count) {
    Task* task;
    s32 i;

    pool->tasks = EwramAlloc(count * sizeof(Task));

    if (pool->tasks == NULL) {
        return;
    }

    ListPoolInit(pool);

    for (i = 0; i < count; i++) {
        task = &((Task*)pool->tasks)[i];
#ifdef PLATFORM_PC
        SetTaskAttackSource(task, NULL);
#endif
        ListPoolAddFree(&task->node, pool, task);
    }
}

void TaskPoolUpdate(TaskPool* pool) {
    Task* task;
#ifdef PLATFORM_PC
    BtlObj* previous;
    u8 keep;
#endif

    task = ListPoolFirst(&pool->head);

    while (task != NULL) {
#ifdef PLATFORM_PC
        /* Clear a surrounding owner for legacy tasks too: nested pool updates
         * must not turn an unowned enemy task into a player's projectile. */
        previous = BtlSetAttackSource(TaskAttackSource(task));
        keep = task->update == NULL || task->update(task->work, task) != 0;
        BtlSetAttackSource(previous);

        if (!keep) {
#else
        if (task->update != NULL && task->update(task->work, task) == 0) {
#endif
            task = TaskDestroy(pool, task);
        } else {
            task = ListPoolNext(&task->node);
        }
    }
}

void TaskPoolDraw(TaskPool* pool) {
    Task* task;
#ifdef PLATFORM_PC
    BtlObj* previous;
#endif

    task = ListPoolFirst(&pool->head);

    while (task != NULL) {
        if (task->desc->draw != NULL) {
#ifdef PLATFORM_PC
            previous = BtlSetAttackSource(TaskAttackSource(task));
#endif
            task->desc->draw(task->work);
#ifdef PLATFORM_PC
            BtlSetAttackSource(previous);
#endif
        }

        task = ListPoolNext(&task->node);
    }
}

void TaskPoolDestroy(TaskPool* pool) {
    Task* task;

    task = ListPoolFirst(&pool->head);

    while (task != NULL) {
        task = TaskDestroy(pool, task);
    }

    EwramFree(pool->tasks);
}

void func_08000F30(TaskPool* pool) {
    Task* task;

    task = ListPoolFirst(&pool->head);

    if (task != NULL) {
        do {
            task = ListPoolNext(&task->node);
        } while (task != NULL);
    }
}

u8 IsTaskActive(Task* task) {
    if (task == NULL || (task->node.flags & LIST_NODE_FLAG_ACTIVE) == 0) {
        return FALSE;
    }

    return TRUE;
}

u8 IsTaskActiveNamed(Task* task, const char* name) {
    if (task == NULL || name == NULL || task->desc->name != name || (task->node.flags & LIST_NODE_FLAG_ACTIVE) == 0) {
        return FALSE;
    }

    return TRUE;
}

const char* GetTaskName(Task* task) {
    return task->desc->name;
}

void SetTaskUpdate(Task* task, TaskUpdateFunc update) {
    task->update = update;
}

s32 func_08000F90() {
    return 0;
}
