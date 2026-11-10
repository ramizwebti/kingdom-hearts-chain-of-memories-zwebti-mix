#!/usr/bin/env python3
"""ROM-free execution of the production task/list pools and attack ownership.

Only the battle context and EWRAM allocator are fixtures. The real Task and
ListPool declarations run with host pointers; cartridge ABI assertions are
disabled in this host-only test, since they require 32-bit pointers.
"""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="rekhcom-task-source-") as temporary:
        work = Path(temporary)
        (work / "macros.h").write_text(
            "#define GUARD_MACROS_H\n#define STATIC_ASSERT(expression, name)\n", encoding="utf-8")
        (work / "main.h").write_text(
            '#include "types.h"\nvoid* GetEwramHeapStart(void);\n'
            'u32 GetEwramHeapSize(void);\n', encoding="utf-8")
        (work / "btl_collision.h").write_text(
            '#ifndef FIXTURE_COLLISION_H\n#define FIXTURE_COLLISION_H\n#include "types.h"\n'
            'typedef struct BtlObj { int id; } BtlObj;\n'
            'BtlObj* BtlGetAttackSource(void);\n'
            'BtlObj* BtlSetAttackSource(BtlObj* source);\n#endif\n', encoding="utf-8")
        (work / "harness.c").write_text(r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "malloc.h"
#include "taskpool.h"
#include "btl_collision.h"
static union { long double align; unsigned char bytes[EWRAM_HEAP_SIZE]; } heap;
static size_t position;
static u32 heapSize = EWRAM_HEAP_SIZE;
static int freed;
static BtlObj actors[3], *current;
void* GetEwramHeapStart(void) { return heap.bytes; }
u32 GetEwramHeapSize(void) { return heapSize; }
void* EwramAlloc(u32 size) {
    size_t start = (position + 31) & ~(size_t)31;
    if (start + size > heapSize) return NULL;
    position = start + size;
    return heap.bytes + start;
}
void EwramFree(const void* pointer) { if (pointer) ++freed; }
BtlObj* BtlGetAttackSource(void) { return current; }
BtlObj* BtlSetAttackSource(BtlObj* source) {
    BtlObj* previous = current; current = source; return previous;
}
#include "PRODUCTION_LISTPOOL"
#include "PRODUCTION_TASKPOOL"

typedef struct Fixture {
    BtlObj* expected;
    int createChild;
    int keep;
} Fixture;
static TaskPool children, nested;
static int initialized, updated, drawn, destroyed;
static Task* child;
static TaskDesc descriptor;
static void init(void* work, const void* arg) {
    Fixture* fixture = work;
    *fixture = *(const Fixture*)arg;
    assert(current == fixture->expected);
    ++initialized;
    if (fixture->createChild) {
        Fixture next = {fixture->expected, 0, 1};
        child = TaskCreate(&children, &descriptor, &next);
        assert(child && TaskAttackSource(child) == fixture->expected);
    }
    /* Caller context must be restored even if a callback changes it. */
    current = &actors[2];
}
static u8 updateSecond(void* work, Task* task) {
    Fixture* fixture = work;
    (void)task;
    assert(current == fixture->expected);
    ++updated;
    current = &actors[2];
    return fixture->keep;
}
static u8 update(void* work, Task* task) {
    Fixture* fixture = work;
    assert(current == fixture->expected);
    if (fixture->createChild) {
        /* A legacy nested task must run with NULL instead of inheriting the
         * owning parent update's temporary context. */
        TaskPoolUpdate(&nested);
        assert(current == fixture->expected);
    }
    SetTaskUpdate(task, updateSecond);
    return updateSecond(work, task);
}
static void draw(void* work) {
    Fixture* fixture = work;
    assert(current == fixture->expected);
    ++drawn;
    current = &actors[2];
}
static void destroy(void* work) {
    Fixture* fixture = work;
    assert(current == fixture->expected);
    ++destroyed;
    current = &actors[2];
}
static TaskDesc descriptor = {"fixture", init, update, draw, destroy, sizeof(Fixture)};
static u8 crowdedUpdate(void* work, Task* task) {
    (void)work; (void)task;
    assert(current == &actors[0]);
    ++updated;
    return 1;
}
static TaskDesc crowded = {"crowded", NULL, crowdedUpdate, NULL, NULL, 0};
static void resetHeap(void) {
    position = 0; freed = 0; current = NULL;
    memset(heap.bytes, 0, sizeof(heap.bytes));
}
int main(void) {
    TaskPool pool, many, failure;
    Task* task;
    Task* reused;
    Fixture owned = {&actors[0], 1, 1};
    Fixture legacy = {NULL, 0, 1};
    int before;
    int index;
    resetHeap();
    TaskPoolInit(&pool, 2);
    TaskPoolInit(&children, 2);
    TaskPoolInit(&nested, 1);
    assert(TaskCreate(&nested, &descriptor, &legacy));
    assert(current == NULL);
    current = &actors[0];
    task = TaskCreate(&pool, &descriptor, &owned);
    assert(task && child && current == &actors[0]);
    assert(TaskAttackSource(task) == &actors[0]);
    assert(TaskAttackSource(child) == &actors[0]);
    current = &actors[1];
    TaskPoolUpdate(&pool);
    assert(current == &actors[1]);
    TaskPoolUpdate(&children);
    assert(current == &actors[1]);
    TaskPoolDraw(&pool);
    assert(current == &actors[1] && drawn == 1);
    /* SetTaskUpdate preserves ownership across coroutine state changes. */
    TaskPoolUpdate(&pool);
    assert(current == &actors[1]);
    ((Fixture*)task->work)->keep = 0;
    before = destroyed;
    TaskPoolUpdate(&pool);
    assert(current == &actors[1] && destroyed == before + 1);
    assert(!IsTaskActive(task) && !TaskAttackSource(task));
    current = NULL;
    reused = TaskCreate(&pool, &descriptor, &legacy);
    assert(reused && !TaskAttackSource(reused));
    current = &actors[1];
    TaskKill(&pool, reused);
    assert(current == &actors[1] && !TaskAttackSource(reused));
    TaskPoolDestroy(&pool);
    TaskPoolDestroy(&children);
    TaskPoolDestroy(&nested);
    assert(current == &actors[1] && freed > 0);

    /* A heap reset can reuse every Task address. Initializing a new pool
     * explicitly clears the previous epoch's ownership before any create. */
    resetHeap();
    TaskPoolInit(&pool, 2);
    current = &actors[0];
    task = TaskCreate(&pool, &crowded, NULL);
    assert(TaskAttackSource(task) == &actors[0]);
    resetHeap();
    TaskPoolInit(&pool, 2);
    assert(!TaskAttackSource((Task*)pool.tasks));
    task = TaskCreate(&pool, &descriptor, &legacy);
    assert(task && !TaskAttackSource(task));
    TaskPoolDestroy(&pool);

    /* More simultaneous owners than a 1024-entry fixed registry supports:
     * address indexing has no capacity exhaustion or hash eviction. */
    resetHeap();
    TaskPoolInit(&many, 1500);
    assert(many.tasks);
    current = &actors[0];
    for (index = 0; index < 1500; ++index) {
        task = TaskCreate(&many, &crowded, NULL);
        assert(task && TaskAttackSource(task) == &actors[0]);
    }
    before = updated;
    current = &actors[1];
    TaskPoolUpdate(&many);
    assert(updated == before + 1500 && current == &actors[1]);
    TaskPoolDestroy(&many);

    /* Work allocation failure must clear ownership on the still-free slot. */
    resetHeap();
    TaskPoolInit(&failure, 1);
    task = ListPoolFirstFree(&failure);
    SetTaskAttackSource(task, &actors[0]);
    position = heapSize;
    assert(!TaskCreate(&failure, &descriptor, &legacy));
    assert(!TaskAttackSource(task) && !IsTaskActive(task));
    TaskPoolDestroy(&failure);

    assert(!TaskAttackSourceSlot((Task*)(heap.bytes + EWRAM_HEAP_SIZE)));
    assert(!TaskAttackSourceSlot((Task*)(heap.bytes + 1)));
    heapSize = 64;
    assert(!TaskAttackSourceSlot((Task*)(heap.bytes + 64)));
    puts("PASS: real TaskPool ownership inheritance, nested legacy tasks, callback restoration, coroutine changes, destroy/kill/reuse/reset, 1500 simultaneous owners, allocation failure, bounds");
    return 0;
}
'''.replace("PRODUCTION_LISTPOOL", str(ROOT / "src/listpool.c"))
         .replace("PRODUCTION_TASKPOOL", str(ROOT / "src/taskpool.c")), encoding="utf-8")
        executable = work / "test"
        subprocess.run([
            os.environ.get("CC", "gcc"), "-std=gnu11", "-O2", "-Wall", "-Wextra", "-Werror",
            "-Wno-unused-parameter", "-include", str(work / "macros.h"),
            "-DPLATFORM_PC", "-iquote", str(work), "-iquote", str(ROOT / "include"),
            str(work / "harness.c"), "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
