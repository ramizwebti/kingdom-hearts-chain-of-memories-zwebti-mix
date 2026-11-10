/**
 * btl_lockon.c
 * Battle Lock-On Cursor and Area Marker
 */

#include "task_descriptors.h"
#include "obj_api.h"
#include "btl.h"
#include "btl_api.h"
#include "sprites_btl.h"
#include "btl_tasks.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "listpool.h"
#include "obj.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"
#ifdef PLATFORM_PC
#include "port/port_action_game.h"
#endif

TaskDesc gTaskDescBtlLockon = {
    "task_btl_lockon",
    (TaskInitFunc)task_btl_lockon_0,
    (TaskUpdateFunc)task_btl_lockon_1,
    (TaskDrawFunc)task_btl_lockon_2,
    (TaskDestroyFunc)task_btl_lockon_3,
    sizeof(BtlLockonWork),
};

TaskDesc gTaskDescBtlArea = {
    "task_btl_area",
    (TaskInitFunc)task_btl_area_0,
    (TaskUpdateFunc)task_btl_area_1,
    (TaskDrawFunc)task_btl_area_2,
    (TaskDestroyFunc)task_btl_area_3,
    sizeof(BtlAreaWork),
};

void task_btl_lockon_0(BtlLockonWork* work) {
    work->tiles = LoadObjTiles(gBtlLockonTiles, sizeof(gBtlLockonTiles));
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    AnimInit(&work->anim, gBtlLockonAnims, gBtlLockonFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->timer = 0;
    gBtlWork->actor2 = NULL;
}

void SelectLockonTarget() {
    BtlObj* player;
    BtlObj* enemy;
    s32 minDist;

#ifdef PLATFORM_PC
    if (PortActionLockMeleeTarget()) return;
#endif

    player = gBtlWork->actor;
    minDist = 0x40000;
    gBtlWork->actor2 = NULL;
    enemy = ListPoolFirst(&gBtlWork->pool);

    if (player->flags & BTLOBJ_FLAG_FACING_LEFT) {
        for (; enemy != NULL; enemy = ListPoolNext(&enemy->node)) {
            if (player->x < enemy->x || player->x - enemy->x > 0x9600 ||
                (player->y - enemy->y >= 0 ? player->y - enemy->y > 0x1800
                                              : enemy->y - player->y > 0x1800) ||
                (player->z - enemy->z >= 0 ? player->z - enemy->z > 0x6400
                                              : enemy->z - player->z > 0x6400) ||
                (enemy->flags & BTLOBJ_FLAG_UNHITTABLE) || player->x - enemy->x >= minDist) {
                continue;
            }

            gBtlWork->actor2 = enemy;
            minDist = player->x - enemy->x;
        }

        if (gBtlWork->actor2 == NULL) {
            minDist = 0x40000;
            enemy = ListPoolFirst(&gBtlWork->pool);

            for (; enemy != NULL; enemy = ListPoolNext(&enemy->node)) {
                if (player->x > enemy->x || enemy->x - player->x > 0x5A00 ||
                    (player->y - enemy->y >= 0 ? player->y - enemy->y > 0x1800
                                                  : enemy->y - player->y > 0x1800) ||
                    (player->z - enemy->z >= 0 ? player->z - enemy->z > 0x6400
                                                  : enemy->z - player->z > 0x6400) ||
                    (enemy->flags & BTLOBJ_FLAG_UNHITTABLE) || enemy->x - player->x >= minDist) {
                    continue;
                }

                gBtlWork->actor2 = enemy;
                minDist = enemy->x - player->x;
            }
        }
    } else {
        for (; enemy != NULL; enemy = ListPoolNext(&enemy->node)) {
            if (player->x > enemy->x || enemy->x - player->x > 0x9600 ||
                (player->y - enemy->y >= 0 ? player->y - enemy->y > 0x1800
                                              : enemy->y - player->y > 0x1800) ||
                (player->z - enemy->z >= 0 ? player->z - enemy->z > 0x6400
                                              : enemy->z - player->z > 0x6400) ||
                (enemy->flags & BTLOBJ_FLAG_UNHITTABLE) || enemy->x - player->x >= minDist) {
                continue;
            }

            gBtlWork->actor2 = enemy;
            minDist = enemy->x - player->x;
        }

        if (gBtlWork->actor2 == NULL) {
            minDist = 0x40000;
            enemy = ListPoolFirst(&gBtlWork->pool);

            for (; enemy != NULL; enemy = ListPoolNext(&enemy->node)) {
                if (player->x < enemy->x || player->x - enemy->x > 0x5A00 ||
                    (player->y - enemy->y >= 0 ? player->y - enemy->y > 0x1800
                                                  : enemy->y - player->y > 0x1800) ||
                    (player->z - enemy->z >= 0 ? player->z - enemy->z > 0x6400
                                                  : enemy->z - player->z > 0x6400) ||
                    (enemy->flags & BTLOBJ_FLAG_UNHITTABLE) || player->x - enemy->x >= minDist) {
                    continue;
                }

                gBtlWork->actor2 = enemy;
                minDist = player->x - enemy->x;
            }
        }
    }
}

u8 task_btl_lockon_1(BtlLockonWork* work) {
    if ((gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) == 0) {
        SelectLockonTarget();
    }

    if (gBtlWork->actor2 != NULL) {
        work->gfx = AnimUpdate(&work->anim);

        if (gBtlWork->actor2->flags & BTLOBJ_FLAG_UNHITTABLE) {
            gBtlWork->actor2 = NULL;
        }
    }

    if (work->timer != 0) {
        work->timer--;
    }

    return 1;
}

void task_btl_lockon_2(BtlLockonWork* work) {
    BtlObj* target;
    s16 x;
    s16 y;

    target = gBtlWork->actor2;

    if (target != NULL) {
        WorldToScreen(&x, &y, target->x + (target->centerOffsetX << 8), target->y,
                      target->z - (target->centerHeight << 8));
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_FLAG_NO_MOSAIC, 16);
    }
}

void task_btl_lockon_3(BtlLockonWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_btl_area_0(BtlAreaWork* work) {
    work->visible = FALSE;
    work->palette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    work->tiles = LoadObjTiles(gBtlAreaTiles, sizeof(gBtlAreaTiles));
    work->timer = 0;
    work->enabled = TRUE;
}

u8 task_btl_area_1(BtlAreaWork* work) {
    if (!work->enabled) {
        work->visible = FALSE;
        return 1;
    }

    if (gBtlWork->areaUpdated) {
        work->timer = 20;
        gBtlWork->areaUpdated = FALSE;
    }

    if (work->timer > 0) {
        work->visible = TRUE;
    } else {
        work->visible = FALSE;
    }

    if (work->timer > 0) {
        work->timer--;
    }

    return 1;
}

void task_btl_area_2(BtlAreaWork* work) {
    BtlObj* obj;
    s16 x;
    s16 y;

    if (!work->visible) {
        return;
    }

    WorldToScreen(&x, &y, gBtlWork->x3 - (gBtlWork->areaHalfX << 8),
                  gBtlWork->y3 - (gBtlWork->areaHalfY << 8), gBtlWork->z3);
    DrawSprite(x, y, gBtlAreaFrame0, work->tiles, work->palette, NULL, 0, 0x101);
    WorldToScreen(&x, &y, gBtlWork->x3 + (gBtlWork->areaHalfX << 8),
                  gBtlWork->y3 - (gBtlWork->areaHalfY << 8), gBtlWork->z3);
    DrawSprite(x, y, gBtlAreaFrame1, work->tiles, work->palette, NULL, 0, 0x101);
    WorldToScreen(&x, &y, gBtlWork->x3 - (gBtlWork->areaHalfX << 8),
                  gBtlWork->y3 + (gBtlWork->areaHalfY << 8), gBtlWork->z3);
    DrawSprite(x, y, gBtlAreaFrame3, work->tiles, work->palette, NULL, 0, 0x101);
    WorldToScreen(&x, &y, gBtlWork->x3 + (gBtlWork->areaHalfX << 8),
                  gBtlWork->y3 + (gBtlWork->areaHalfY << 8), gBtlWork->z3);
    DrawSprite(x, y, gBtlAreaFrame2, work->tiles, work->palette, NULL, 0, 0x101);
    WorldToScreen(&x, &y, gBtlWork->x3, gBtlWork->y3, gBtlWork->z3);
    DrawSprite(x, y, gBtlAreaFrame5, work->tiles, work->palette, NULL, 0, 0x101);
    WorldToScreen(&x, &y, gBtlWork->x3, gBtlWork->y3,
                  gBtlWork->z3 - (gBtlWork->areaHalfZ << 8));
    DrawSprite(x, y, gBtlAreaFrame4, work->tiles, work->palette, NULL, 0, 0x101);
    WorldToScreen(&x, &y, gBtlWork->x3, gBtlWork->y3,
                  gBtlWork->z3 + (gBtlWork->areaHalfZ << 8));
    DrawSprite(x, y, gBtlAreaFrame4, work->tiles, work->palette, NULL, SPRITE_FLAG_VFLIP, 0x101);

    if (gBtlWork->soraOwnsPlay) {
        obj = ListPoolFirst(&gBtlWork->pool);

        while (obj != NULL) {
            WorldToScreen(&x, &y, obj->x - (obj->radiusX << 8),
                          obj->y - (obj->radiusY << 8), obj->z);
            DrawSprite(x, y, gBtlAreaFrame0, work->tiles, work->palette, NULL, 0, 0x101);
            WorldToScreen(&x, &y, obj->x + (obj->radiusX << 8),
                          obj->y - (obj->radiusY << 8), obj->z);
            DrawSprite(x, y, gBtlAreaFrame1, work->tiles, work->palette, NULL, 0, 0x101);
            WorldToScreen(&x, &y, obj->x - (obj->radiusX << 8),
                          obj->y + (obj->radiusY << 8), obj->z);
            DrawSprite(x, y, gBtlAreaFrame3, work->tiles, work->palette, NULL, 0, 0x101);
            WorldToScreen(&x, &y, obj->x + (obj->radiusX << 8),
                          obj->y + (obj->radiusY << 8), obj->z);
            DrawSprite(x, y, gBtlAreaFrame2, work->tiles, work->palette, NULL, 0, 0x101);
            WorldToScreen(&x, &y, obj->x, obj->y, obj->z);
            DrawSprite(x, y, gBtlAreaFrame5, work->tiles, work->palette, NULL, 0, 0x101);
            WorldToScreen(&x, &y, obj->x, obj->y,
                          obj->z - (obj->height << 8));
            DrawSprite(x, y, gBtlAreaFrame4, work->tiles, work->palette, NULL, 0, 0x101);
            obj = ListPoolNext(&obj->node);
        }
    } else {
        obj = gBtlWork->actor;
        WorldToScreen(&x, &y, obj->x - (obj->radiusX << 8),
                      obj->y - (obj->radiusY << 8), obj->z);
        DrawSprite(x, y, gBtlAreaFrame0, work->tiles, work->palette, NULL, 0, 0x101);
        WorldToScreen(&x, &y, obj->x + (obj->radiusX << 8),
                      obj->y - (obj->radiusY << 8), obj->z);
        DrawSprite(x, y, gBtlAreaFrame1, work->tiles, work->palette, NULL, 0, 0x101);
        WorldToScreen(&x, &y, obj->x - (obj->radiusX << 8),
                      obj->y + (obj->radiusY << 8), obj->z);
        DrawSprite(x, y, gBtlAreaFrame3, work->tiles, work->palette, NULL, 0, 0x101);
        WorldToScreen(&x, &y, obj->x + (obj->radiusX << 8),
                      obj->y + (obj->radiusY << 8), obj->z);
        DrawSprite(x, y, gBtlAreaFrame2, work->tiles, work->palette, NULL, 0, 0x101);
        WorldToScreen(&x, &y, obj->x, obj->y, obj->z);
        DrawSprite(x, y, gBtlAreaFrame5, work->tiles, work->palette, NULL, 0, 0x101);
        WorldToScreen(&x, &y, obj->x, obj->y,
                      obj->z - (obj->height << 8));
        DrawSprite(x, y, gBtlAreaFrame4, work->tiles, work->palette, NULL, 0, 0x101);
    }
}

void task_btl_area_3(BtlAreaWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}
