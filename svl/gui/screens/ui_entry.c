#include <ui.h>

#define CHART_POINT_CNT          300
#define CHART_WIDTH              880
#define CHART_HEIGHT             190  // 调整图表高度以适应三个图表垂直排列
#define UPDATE_PERIOD_MS         500
#define SIDEBAR_WIDTH            (LV_HOR_RES / 4)
#define CONTENT_LEFT_MARGIN      (SIDEBAR_WIDTH + 50)  // 内容区域左边距
#define SIDEBAR_MARGIN           20   // 侧边栏上下边距
#define SIDEBAR_LEFT_MARGIN      30 // 侧边栏左边距
#define CHANNEL_CONTAINER_HEIGHT 220  // 增加通道容器高度以提供更多内部空间，从180增加到220
#define CONTENT_VERTICAL_SPACING 25   // 内容区域垂直间距
#define CHANNEL_CONTAINER_WIDTH  (SIDEBAR_WIDTH - 40) // 通道容器宽度，留出足够的边距避免滚动条

// 添加统计数据结构
typedef struct
{
    int32_t min_value;
    int32_t max_value;
    int32_t avg_value;
    uint32_t count;
    int64_t sum;
    int32_t latest_value;     // 最新值
    int32_t range;            // 范围 (最大值-最小值)
    int32_t median;           // 中位数 (简化的计算方式)
} channel_stats_t;

static lv_obj_t *main_cont;
static lv_obj_t *title_label;
static lv_obj_t *sidebar;
static lv_obj_t *channel_containers[3];
static lv_obj_t *value_labels[3];
static lv_obj_t *min_labels[3];
static lv_obj_t *max_labels[3];
static lv_obj_t *avg_labels[3];
static lv_obj_t *latest_labels[3];  // 最新值标签
static lv_obj_t *range_labels[3];   // 范围标签
static lv_obj_t *median_labels[3];  // 中位数标签
static lv_obj_t *charts[3];  // 三个图表对象数组
static lv_chart_series_t *series[3];  // 三个图表系列数组
static lv_timer_t *update_timer;
static channel_stats_t stats[3];

// 更新统计数据
void update_channel_stats(uint8_t channel, int32_t value)
{
    // 更新最新值
    stats[channel].latest_value = value;

    // 更新计数和总和
    stats[channel].count++;
    stats[channel].sum += value;

    // 更新最小值和最大值
    if (stats[channel].count == 1)
    {
        stats[channel].min_value = value;
        stats[channel].max_value = value;
    }
    else
    {
        if (value < stats[channel].min_value)
        {
            stats[channel].min_value = value;
        }
        if (value > stats[channel].max_value)
        {
            stats[channel].max_value = value;
        }
    }

    // 计算平均值
    stats[channel].avg_value = stats[channel].sum / stats[channel].count;

    // 计算范围
    stats[channel].range = stats[channel].max_value - stats[channel].min_value;

    // 简化的中位数计算（取平均值作为近似）
    stats[channel].median = stats[channel].avg_value;
}

void update_chart_timer_cb(lv_timer_t *timer)
{
    // 更新三个图表的数据
    for (int i = 0; i < 3; i++)
    {
        int32_t new_value = lv_rand(40, 60);
        lv_chart_set_next_value(charts[i], series[i], new_value);

        // 更新统计数据
        update_channel_stats(i, new_value);

        // 更新界面显示
        lv_label_set_text_fmt(value_labels[i], "Value: %ld", new_value);
        lv_label_set_text_fmt(min_labels[i], "Min: %ld", stats[i].min_value);
        lv_label_set_text_fmt(max_labels[i], "Max: %ld", stats[i].max_value);
        lv_label_set_text_fmt(avg_labels[i], "Average: %ld", stats[i].avg_value);
        lv_label_set_text_fmt(latest_labels[i], "Latest: %ld", stats[i].latest_value);
        lv_label_set_text_fmt(range_labels[i], "Range: %ld", stats[i].range);
        lv_label_set_text_fmt(median_labels[i], "Median: %ld", stats[i].median);
    }
    lv_obj_invalidate(main_cont);
}

void create_chart_ui(lv_obj_t *screen)
{
    // 初始化统计数据
    lv_memset(stats, 0, sizeof(stats));

    // 创建主容器
    main_cont = lv_obj_create(screen);
    lv_obj_remove_style_all(main_cont);
    lv_obj_set_size(main_cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(main_cont);

    // 创建侧边栏容器
    sidebar = lv_obj_create(main_cont);
    lv_obj_set_size(sidebar, SIDEBAR_WIDTH, LV_VER_RES - 2 * SIDEBAR_MARGIN);
    lv_obj_align(sidebar, LV_ALIGN_TOP_LEFT, SIDEBAR_LEFT_MARGIN, SIDEBAR_MARGIN);

    // 设置侧边栏样式
    lv_obj_set_style_bg_color(sidebar, lv_color_make(240, 240, 240), 0);
    lv_obj_set_style_border_width(sidebar, 2, 0);
    lv_obj_set_style_border_color(sidebar, lv_color_make(200, 200, 200), 0);
    lv_obj_set_style_radius(sidebar, 10, 0);
    lv_obj_set_style_pad_all(sidebar, 15, 0);  // 增加内边距

    // 在侧边栏中创建通道信息显示区域
    for (int i = 0; i < 3; i++)
    {
        // 为每个通道创建一个容器
        channel_containers[i] = lv_obj_create(sidebar);
        lv_obj_set_size(channel_containers[i], CHANNEL_CONTAINER_WIDTH, CHANNEL_CONTAINER_HEIGHT);
        lv_obj_set_style_bg_color(channel_containers[i], lv_color_make(255, 255, 255), 0);
        lv_obj_set_style_border_width(channel_containers[i], 1, 0);
        lv_obj_set_style_border_color(channel_containers[i], lv_color_make(180, 180, 180), 0);
        lv_obj_set_style_radius(channel_containers[i], 8, 0);
        lv_obj_set_style_pad_all(channel_containers[i], 20, 0);  // 增加内边距

        if (i == 0)
        {
            lv_obj_align(channel_containers[i], LV_ALIGN_TOP_MID, 0, 20);
        }
        else
        {
            lv_obj_align_to(channel_containers[i], channel_containers[i - 1],
                            LV_ALIGN_OUT_BOTTOM_MID, 0, 20);  // 增加容器间间距
        }

        // 创建通道标题
        lv_obj_t *channel_title = lv_label_create(channel_containers[i]);
        lv_label_set_text_fmt(channel_title, "Channel %d", i);
        lv_obj_set_style_text_font(channel_title, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(channel_title, lv_color_make(0, 100, 200), 0);
        lv_obj_align(channel_title, LV_ALIGN_TOP_MID, 0, 0);

        // 创建当前值标签
        value_labels[i] = lv_label_create(channel_containers[i]);
        lv_label_set_text(value_labels[i], "Value: --");
        lv_obj_align_to(value_labels[i], channel_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

        // 创建最小值标签
        min_labels[i] = lv_label_create(channel_containers[i]);
        lv_label_set_text(min_labels[i], "Min: --");
        lv_obj_align_to(min_labels[i], value_labels[i], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

        // 创建最大值标签
        max_labels[i] = lv_label_create(channel_containers[i]);
        lv_label_set_text(max_labels[i], "Max: --");
        lv_obj_align_to(max_labels[i], min_labels[i], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

        // 创建平均值标签
        avg_labels[i] = lv_label_create(channel_containers[i]);
        lv_label_set_text(avg_labels[i], "Average: --");
        lv_obj_align_to(avg_labels[i], max_labels[i], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

        // 创建最新值标签
        latest_labels[i] = lv_label_create(channel_containers[i]);
        lv_label_set_text(latest_labels[i], "Latest: --");
        lv_obj_align_to(latest_labels[i], avg_labels[i], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

        // 创建范围标签
        range_labels[i] = lv_label_create(channel_containers[i]);
        lv_label_set_text(range_labels[i], "Range: --");
        lv_obj_align_to(range_labels[i], latest_labels[i], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

        // 创建中位数标签
        median_labels[i] = lv_label_create(channel_containers[i]);
        lv_label_set_text(median_labels[i], "Median: --");
        lv_obj_align_to(median_labels[i], range_labels[i], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    }

    // 创建标题
    title_label = lv_label_create(main_cont);
    lv_label_set_text(title_label, "Lasense");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(title_label, lv_color_make(0, 100, 200), 0);
    // 调整标题位置，使其位于左侧栏右侧的中央
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, CONTENT_LEFT_MARGIN, 20);

    // 创建三个图表
    for (uint8_t i = 0; i < 3; i++)
    {
        // 创建图表标签
        lv_obj_t *chart_label = lv_label_create(main_cont);
        lv_label_set_text_fmt(chart_label, "Channel %u", i);
        lv_obj_set_style_text_font(chart_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(chart_label, lv_color_make(50, 50, 50), 0);

        // 计算图表和标签的位置
        if (i == 0)
        {
            // 第一个图表标签位于主标题下方
            lv_obj_align_to(chart_label, title_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                            CONTENT_VERTICAL_SPACING);
        }
        else
        {
            // 其他图表标签位于前一个图表下方
            lv_obj_align_to(chart_label, charts[i - 1], LV_ALIGN_OUT_BOTTOM_LEFT, 0,
                            CONTENT_VERTICAL_SPACING);
        }

        // 创建图表
        charts[i] = lv_chart_create(main_cont);
        lv_obj_set_size(charts[i], CHART_WIDTH, CHART_HEIGHT);
        // 将图表位置调整到左侧栏右侧
        lv_obj_align_to(charts[i], chart_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_obj_set_style_size(charts[i], 0, 0, LV_PART_INDICATOR);

        // 设置图表样式
        lv_obj_set_style_bg_color(charts[i], lv_color_make(250, 250, 250), 0);
        lv_obj_set_style_border_width(charts[i], 1, 0);
        lv_obj_set_style_border_color(charts[i], lv_color_make(200, 200, 200), 0);
        lv_obj_set_style_radius(charts[i], 8, 0);
        lv_obj_set_style_pad_all(charts[i], 10, 0);

        lv_chart_set_point_count(charts[i], CHART_POINT_CNT);
        lv_chart_set_axis_range(charts[i], LV_CHART_AXIS_PRIMARY_Y, 0, 100);
        lv_chart_set_update_mode(charts[i], LV_CHART_UPDATE_MODE_SHIFT);

        // 添加数据系列，使用不同颜色区分
        lv_color_t colors[3] = { LV_COLOR_MAKE(0xFF, 0x00, 0x00),  // 红色
                                 LV_COLOR_MAKE(0x00, 0xFF, 0x00),  // 绿色
                                 LV_COLOR_MAKE(0x00, 0x00, 0xFF) }; // 蓝色

        series[i] = lv_chart_add_series(charts[i], colors[i], LV_CHART_AXIS_PRIMARY_Y);

        // 初始化图表数据
        for (int j = 0; j < CHART_POINT_CNT; j++)
        {
            lv_chart_set_series_value_by_id(charts[i], series[i], j, lv_rand(40, 60));
        }
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
