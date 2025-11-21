#include <ui.h>

#define CHART_POINT_CNT  300
#define CHART_WIDTH      1200
#define CHART_HEIGHT     700
#define UPDATE_PERIOD_MS 1000

static lv_obj_t *main_cont;
static lv_obj_t *title_label;
static lv_obj_t *chart;
static lv_chart_series_t *ser;
static lv_timer_t *update_timer;

void update_chart_timer_cb(lv_timer_t *timer)
{
    LV_LOG_USER("timer");
    lv_chart_set_next_value(chart, ser, lv_rand(40, 60));
    lv_obj_invalidate(main_cont);
}

void create_chart_ui(lv_obj_t *screen)
{
    // 创建主容器
    main_cont = lv_obj_create(screen);
    lv_obj_remove_style_all(main_cont);
    lv_obj_set_size(main_cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(main_cont);

    // 创建标题
    lv_obj_t *title_label = lv_label_create(main_cont);
    lv_label_set_text(title_label, "Real Time Chart");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 30);

    // 创建图表
    chart = lv_chart_create(main_cont);
    lv_obj_set_size(chart, CHART_WIDTH, CHART_HEIGHT);
    lv_obj_align_to(chart, title_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);

    lv_chart_set_point_count(chart, CHART_POINT_CNT);
    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    // 添加数据系列
    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    // 修复：在SHIFT模式下，应该先用0填充所有点，而不是使用set_next_value
    lv_chart_set_all_values(chart, ser, 0);

    // 然后可以预先设置一些初始数据（可选）
    for (int i = 0; i < CHART_POINT_CNT; i++)
    {
        // lv_chart_set_next_value(chart, ser, lv_rand(40, 60));
        lv_chart_set_series_value_by_id(chart, ser, i, lv_rand(40, 60));
    }

    // 创建定时器
    update_timer = lv_timer_create(update_chart_timer_cb, UPDATE_PERIOD_MS, NULL);
    if (!update_timer)
    {
        LV_LOG_USER("Failed to create timer\n");
        lv_obj_delete(main_cont);
    }
    lv_timer_set_repeat_count(update_timer, -1);
}
