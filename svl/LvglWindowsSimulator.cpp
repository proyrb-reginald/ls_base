#include <Windows.h>
#include <LvglWindowsIconResource.h>

#include "lvgl/lvgl.h"
#include "lvgl/src/widgets/chart/lv_chart.h"    // 图表组件
#include "lvgl/src/widgets/button/lv_button.h"      // 按钮组件
#include "lvgl/src/widgets/label/lv_label.h"      // 标签组件
#include "lvgl/src/misc/lv_timer.h"       // 定时器组件
#include "lvgl/src/misc/lv_event.h"
#include "lvgl/src/draw/lv_image_dsc.h"

#include "lvgl/demos/lv_demos.h"
#include <time.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
#define LEFT_WIDTH (SCREEN_WIDTH / 5)
#define RIGHT_WIDTH (SCREEN_WIDTH - LEFT_WIDTH)
#define CHART_POINT_COUNT 60  // 图表数据点数量（与刻度数量一致）
#define X_SCALE_HEIGHT 50     // X轴刻度高度
#define Y_AXIS_WIDTH 70       // Y轴区域宽度

// 模拟数据结构
typedef struct {
    time_t timestamp;
    float co2;
    float ch4;
    float h2o;
} GasData_t;

// 定义范围结构体
typedef struct {
    float x_min;
    float x_max;
    float y_min;
    float y_max;
} ChartRange;

// 定义结构体保存图表状态（便于更新）
typedef struct {
    lv_obj_t* chart;         // 图表对象
    lv_obj_t* x_scale;       // X轴刻度对象
    time_t start_time;       // 当前X轴起始时间
    uint32_t interval_sec;   // 时间间隔（秒）
    const char** time_labels;// 当前时间标签数组
    lv_chart_series_t* series;// 图表数据系列
    float y_min;
    float y_max;
} chart_state_t;

// 全局或静态变量保存状态（多个图表可使用数组）
static chart_state_t co2_chart_state, ch4_chart_state, h2o_chart_state;

static GasData_t gas_data[60]; // 存储60个数据点
static uint8_t data_idx = 0;
static bool is_measuring = true;

// 控件句柄
static lv_obj_t* label_co2_val, * label_ch4_val, * label_h2o_val;
static lv_obj_t* btn_measure, * btn_clear;
static lv_obj_t* tabview;
static lv_obj_t* chart_co2, * chart_ch4, * chart_h2o;
static lv_obj_t* tab_history;
static lv_chart_series_t* ser_co2, * ser_ch4, * ser_h2o;
static bool app_quit = false; // 自定义退出标志

extern "C" {
    LV_IMAGE_DECLARE(logo);  // 与C文件中定义的图像名称完全一致
}

// 生成时间标签的辅助函数
static void generate_time_labels(char labels[][30], uint16_t cnt, bool with_date) {
    time_t now = time(NULL);
    struct tm* tm_info;
    for (int i = 0; i < cnt; i++) {
        time_t t = now - (cnt - 1 - i) * 10; // 模拟过去10分钟的数据，每10秒一个点
        tm_info = localtime(&t);
        if (with_date) {
            strftime(labels[i], 30, "%H:%M:%S %Y/%m/%d", tm_info);
        }
        else {
            strftime(labels[i], 30, "%H:%M:%S", tm_info);
        }
    }
}

// 按钮事件回调
static void btn_event_handler(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (btn == btn_measure) {
            is_measuring = !is_measuring;
            if (is_measuring) {
                lv_label_set_text(lv_obj_get_child(btn, 0), "停止测量");
                // lv_obj_set_style_text_font(btn, &lv_font_source_han_sans_sc_16_cjk, 0);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF5555), 0);
            }
            else {
                lv_label_set_text(lv_obj_get_child(btn, 0), "开始测量");
                // lv_obj_set_style_text_font(btn, &lv_font_source_han_sans_sc_16_cjk, 0);
                lv_obj_set_style_bg_color(btn, lv_color_hex(0x55FF55), 0);
            }
        }
        else if (btn == btn_clear) {
            lv_chart_set_all_values(chart_co2, ser_co2, 0);
            lv_chart_set_all_values(chart_ch4, ser_ch4, 0);
            lv_chart_set_all_values(chart_h2o, ser_h2o, 0);
            lv_label_set_text(label_co2_val, "0.0000");
            lv_label_set_text(label_ch4_val, "0.00000");
            lv_label_set_text(label_h2o_val, "0.0000");
        }
    }
}

// 消息框按钮点击回调（用于关闭消息框）
static void msgbox_btn_handler(lv_event_t* e) {
    lv_obj_t* msgbox = (lv_obj_t*)lv_event_get_user_data(e); // 获取消息框对象
    lv_obj_del(msgbox); // 点击按钮后删除消息框
}

// 窗口关闭事件回调（Windows API）
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CLOSE:
        app_quit = true; // 用户点击关闭按钮，设置退出标志
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// 在初始化显示时替换默认窗口过程
static void set_window_close_handler(lv_display_t* display) {
    HWND hwnd = lv_windows_get_display_window_handle(display);
    if (hwnd) {
        // 替换窗口过程，监听关闭事件
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
    }
}

// 文字链接点击事件（修正版）
static void link_event_handler(lv_event_t* e) {
    lv_obj_t* label = (lv_obj_t*)lv_event_get_target(e);
    const char* text = lv_label_get_text(label);

    if (strcmp(text, "退出程序") == 0) {
        app_quit = true;
        return;
    }

    // 1. 创建消息框基础对象（仅传入父容器）
    lv_obj_t* msgbox = lv_msgbox_create(lv_scr_act()); // 父容器设为屏幕

    // 2. 设置标题
    lv_msgbox_add_title(msgbox, "提示");

    // 3. 设置消息内容
    if (strcmp(text, "调试面板") == 0) {
        lv_msgbox_add_text(msgbox, "调试面板功能待实现");
    }
    else if (strcmp(text, "保存参数") == 0) {
        lv_msgbox_add_text(msgbox, "保存参数功能待实现");
    }
    else if (strcmp(text, "联系我们") == 0) {
        lv_msgbox_add_text(msgbox, "联系我们功能待实现");
    }

    lv_msgbox_add_close_button(msgbox);

    // 6. 居中显示消息框
    lv_obj_center(msgbox);
}

// 初始化左侧显示区域
static void init_left_side(lv_obj_t* parent) {
    // 左侧容器整体设置（确保高度填满父容器，禁用默认内边距避免挤压）
    lv_obj_set_size(parent, LEFT_WIDTH, SCREEN_HEIGHT - 60);
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 5, 0);    // 减少内边距
    lv_obj_set_style_pad_row(parent, 10, 0);  // 子控件间垂直间距

    // 2. 显示区域（占左侧剩余高度的60%，放置三个气体显示框）
    lv_obj_t* display_area = lv_obj_create(parent);
    lv_obj_set_size(display_area, LV_PCT(100), LV_PCT(55));  // 宽度填满，高度占60%
    lv_obj_set_layout(display_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(display_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(display_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(display_area, 20, 0);  // 三个显示框之间的间距
    lv_obj_set_style_pad_top(display_area, 15, 0);  // 顶部留白

    // 通用函数：创建气体显示框（避免重复代码）
    auto create_gas_box = [&](lv_obj_t* parent, const char* title, lv_obj_t** val_label, const char* unit) {
        lv_obj_t* box = lv_obj_create(parent);
        lv_obj_set_size(box, LV_PCT(90), LV_SIZE_CONTENT);  // 宽度占90%，高度自适应内容
        lv_obj_set_layout(box, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(box, 10, 0);

        // 标题（加粗左对齐）
        lv_obj_t* title_lbl = lv_label_create(box);
        lv_label_set_text(title_lbl, title);
        lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_LEFT, 0);

        // 数值+单位行（数值左对齐，单位右对齐）
        lv_obj_t* val_row = lv_obj_create(box);
        lv_obj_set_size(val_row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_layout(val_row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(val_row, LV_FLEX_FLOW_ROW);

        // 数值标签（加粗左对齐）
        *val_label = lv_label_create(val_row);
        lv_label_set_text(*val_label, "0.0000");
        lv_obj_set_style_text_color(*val_label, lv_color_hex(0x0000FF), 0);  // 蓝色

        lv_obj_set_style_text_align(*val_label, LV_TEXT_ALIGN_LEFT, 0);

        // 单位标签（右对齐）
        lv_obj_t* unit_lbl = lv_label_create(val_row);
        lv_label_set_text(unit_lbl, unit);
        lv_obj_set_flex_grow(unit_lbl, 1);  // 占满剩余宽度，实现右对齐
        lv_obj_set_style_text_align(unit_lbl, LV_TEXT_ALIGN_RIGHT, 0);

        return box;
        };

    // 创建三个气体显示框
    create_gas_box(display_area, "二氧化碳", &label_co2_val, "ppm");
    create_gas_box(display_area, "甲烷", &label_ch4_val, "ppm");
    create_gas_box(display_area, "水", &label_h2o_val, "%");


    // 3. 按钮区域（高度100px，固定，放置两个按钮）
    lv_obj_t* btn_area = lv_obj_create(parent);
    lv_obj_set_size(btn_area, LV_PCT(100), 80);  // 宽度填满，高度固定
    lv_obj_set_layout(btn_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_area, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(btn_area, 10, 0);
    lv_obj_set_style_pad_right(btn_area, 10, 0);

    // 开始/停止测量按钮
    btn_measure = lv_btn_create(btn_area);
    lv_obj_set_size(btn_measure, 100, 40);  // 固定尺寸
    lv_obj_set_style_bg_color(btn_measure, lv_color_hex(0xFF5555), 0);  // 初始红色
    lv_obj_add_event_cb(btn_measure, btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_t* btn_measure_label = lv_label_create(btn_measure);
    lv_label_set_text(btn_measure_label, "停止测量");
    lv_obj_center(btn_measure_label);

    // 清空图表按钮
    btn_clear = lv_btn_create(btn_area);
    lv_obj_set_size(btn_clear, 100, 40);  // 固定尺寸
    lv_obj_add_event_cb(btn_clear, btn_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_t* btn_clear_label = lv_label_create(btn_clear);
    lv_label_set_text(btn_clear_label, "清空图表");
    lv_obj_center(btn_clear_label);


    // 3. 文字链接区域（占剩余高度，确保宽度填满左侧容器）
    lv_obj_t* link_area = lv_obj_create(parent);
    lv_obj_set_size(link_area, LV_PCT(100), LV_PCT(100));  // 占剩余100%高度
    lv_obj_set_flex_grow(link_area, 1);  // 关键：强制拉伸填满剩余空间
    lv_obj_set_layout(link_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(link_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(link_area, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);  // 内容顶部对齐
    lv_obj_set_style_pad_row(link_area, 20, 0);  // 增大行间距，减少底部空白
    lv_obj_set_style_pad_top(link_area, 10, 0);

    // 第一行链接（调试面板、保存参数）
    lv_obj_t* link_row1 = lv_obj_create(link_area);
    lv_obj_set_size(link_row1, LV_PCT(90), LV_SIZE_CONTENT);  // 宽度100%
    lv_obj_set_layout(link_row1, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(link_row1, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(link_row1, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);  // 两端对齐
    lv_obj_set_style_pad_column(link_row1, 40, 0);  // 增大两个链接的水平间距（关键）

    // 第二行链接（联系我们、退出程序）
    lv_obj_t* link_row2 = lv_obj_create(link_area);
    lv_obj_set_size(link_row2, LV_PCT(90), LV_SIZE_CONTENT);  // 宽度100%
    lv_obj_set_layout(link_row2, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(link_row2, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(link_row2, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);  // 两端对齐
    lv_obj_set_style_pad_column(link_row2, 40, 0);  // 增大水平间距（关键）

    // 创建链接（保持文字大小适中）
    auto create_link = [](lv_obj_t* parent, const char* text) {
        lv_obj_t* link = lv_label_create(parent);
        lv_label_set_text(link, text);
        lv_obj_set_style_text_color(link, lv_color_hex(0x0000FF), 0);  // 蓝色
        lv_obj_add_event_cb(link, link_event_handler, LV_EVENT_CLICKED, NULL);
        return link;
        };

    // 添加链接
    create_link(link_row1, "调试面板");
    create_link(link_row1, "保存参数");
    create_link(link_row2, "联系我们");
    create_link(link_row2, "退出程序");
}

/**
 * 生成X轴时间标签数组（格式：fmt，基于初始时间和间隔）
 * @param start_time 初始时间
 * @param interval_sec 刻度间隔（秒）
 * @param fmt 时间格式（如"%H:%M:%S"）
 * @return 时间标签数组（以NULL结尾，需手动释放）
 */
static const char** generate_time_labels(time_t start_time, uint32_t interval_sec, const char* fmt) {
    const char** labels = (const char**)lv_malloc(sizeof(const char*) * (CHART_POINT_COUNT + 1));
    for (uint32_t i = 0; i < CHART_POINT_COUNT; i++) {
        char* buf = (char*)lv_malloc(32);
        time_t curr_time = start_time + i * interval_sec;
        struct tm* t = localtime(&curr_time);
        strftime(buf, 32, fmt, t);
        labels[i] = buf;
    }
    labels[CHART_POINT_COUNT] = NULL;
    return labels;
}

/**
 * 创建Y轴垂直刻度及标题
 * @param parent 父容器（Y轴区域）
 * @param title Y轴标题（如"CO2(ppm)"）
 * @param y_min/y_max Y轴范围
 * @return Y轴刻度对象
 */
static lv_obj_t* create_y_scale(lv_obj_t* parent, const char* title, float y_min, float y_max) {
    // 1. 垂直刻度
    lv_obj_t* y_scale = lv_scale_create(parent);
    lv_scale_set_mode(y_scale, LV_SCALE_MODE_VERTICAL_LEFT);  // 垂直模式
    lv_scale_set_range(y_scale, y_min, y_max);      // Y轴范围
    lv_scale_set_total_tick_count(y_scale, 30);
    lv_scale_set_major_tick_every(y_scale, 5);  // 5个主刻度
    lv_scale_set_label_show(y_scale, true);         // 显示刻度值
    lv_obj_set_style_text_font(y_scale, &lv_font_montserrat_8, 0);

    // 2. 样式设置
    lv_obj_set_size(y_scale, LV_PCT(100), LV_PCT(100));  // 占满Y轴区域
    lv_obj_set_style_line_color(y_scale, lv_color_hex(0x666666), LV_PART_ITEMS);  // 刻度线颜色

    // 3. Y轴标题（旋转270度，居左显示）
    lv_obj_t* y_title = lv_label_create(parent);
    lv_label_set_text(y_title, title);
    lv_obj_set_style_text_font(y_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_transform_angle(y_title, 2700, LV_PART_MAIN);  // 旋转270度

    return y_scale;
}

/**
 * 创建X轴水平刻度（时间轴）
 * @param parent 父容器（X轴区域）
 * @param time_labels 时间标签数组（以NULL结尾）
 * @return X轴刻度对象
 */
static lv_obj_t* create_x_scale(lv_obj_t* parent, const char** time_labels, lv_obj_t** x_scale) {
    *x_scale = lv_scale_create(parent);
    lv_scale_set_mode(*x_scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_scale_set_range(*x_scale, 0, CHART_POINT_COUNT - 1);
    lv_scale_set_total_tick_count(*x_scale, CHART_POINT_COUNT);
    lv_scale_set_major_tick_every(*x_scale, 5);
    lv_scale_set_label_show(*x_scale, true);

    // 绑定时间标签数组
    lv_scale_set_text_src(*x_scale, time_labels);

    // 尺寸与样式
    lv_obj_set_size(*x_scale, LV_PCT(100), X_SCALE_HEIGHT);
    lv_obj_set_style_text_font(*x_scale, &lv_font_montserrat_8, 0);
    lv_obj_set_style_line_color(*x_scale, lv_color_hex(0x666666), LV_PART_ITEMS);

    return *x_scale;
}

/**
 * 创建气体图表（包含X轴刻度，参考示例布局）
 * @param parent 父容器
 * @param title Y轴标题（如"CO2(ppm)"）
 * @param y_min/y_max Y轴范围
 * @param x_fmt X轴时间格式
 * @param x_start 初始时间
 * @param x_interval 刻度间隔（秒）
 */
static lv_obj_t* create_gas_chart(lv_obj_t* parent, const char* title,
    float y_min, float y_max,
    const char* x_fmt, time_t x_start, uint32_t x_interval,
    lv_obj_t** chart, lv_chart_series_t** ser, chart_state_t* state) {
    /* 1. 创建包裹容器（参考示例中的wrapper，确保图表与刻度对齐） */
    lv_obj_t* chart_col = lv_obj_create(parent);
    lv_obj_remove_style_all(chart_col);  // 移除默认样式，避免干扰
    lv_obj_set_size(chart_col, LV_PCT(100), LV_PCT(33));  // 占父容器1/3高度
    lv_obj_set_flex_flow(chart_col, LV_FLEX_FLOW_COLUMN);  // 改为垂直布局：图表在上，X轴在下
    lv_obj_set_flex_align(chart_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(chart_col, 0, LV_PART_MAIN);  // 内边距0，紧凑布局

    // 2. 图表子行容器（横向Flex：Y轴 + 图表）
    lv_obj_t* chart_subrow = lv_obj_create(chart_col);
    lv_obj_set_size(chart_subrow, LV_PCT(100), LV_PCT(100) - X_SCALE_HEIGHT);  // 扣除X轴高度
    lv_obj_set_flex_flow(chart_subrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_grow(chart_subrow, 1);  // 占满chart_row剩余高度

    // 3. Y轴区域（宽度固定，包含垂直刻度和标题）
    lv_obj_t* y_axis_cont = lv_obj_create(chart_subrow);
    lv_obj_set_size(y_axis_cont, Y_AXIS_WIDTH, LV_PCT(100));
    lv_obj_set_flex_flow(y_axis_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(y_axis_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN);

    create_y_scale(y_axis_cont, title, y_min, y_max);

    /* 4. 创建图表（参考示例中的chart） */
    *chart = lv_chart_create(chart_subrow);
    lv_obj_set_flex_grow(*chart, 1);  // 占满剩余宽度
    lv_obj_set_height(*chart, LV_PCT(100));  // 扣除X轴高度
    lv_chart_set_type(*chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(*chart, CHART_POINT_COUNT);  // 数据点数量=刻度数量
    lv_chart_set_update_mode(*chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_axis_range(*chart, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    lv_obj_set_style_radius(*chart, 0, LV_PART_MAIN);  // 无边框圆角，与示例一致

    // 网格线
    lv_obj_set_style_line_color(*chart, lv_color_hex(0xDDDDDD), LV_PART_MAIN);

    /* 5. 添加曲线（淡蓝色线，红色点） */
    *ser = lv_chart_add_series(*chart, lv_color_hex(0xADD8E6), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(*chart, 2, LV_PART_ITEMS);


    /* 5. 创建X轴刻度（参考示例中的scale_bottom） */
    const char** time_labels = generate_time_labels(x_start, x_interval, x_fmt);
    lv_obj_t* x_scale;
    lv_obj_t* x_scale_cont = lv_obj_create(chart_col);
    lv_obj_set_size(x_scale_cont, LV_PCT(100), X_SCALE_HEIGHT);
    lv_obj_align(x_scale_cont, LV_ALIGN_BOTTOM_LEFT, Y_AXIS_WIDTH, 0);  // 与上半部分左对齐
    lv_obj_set_style_pad_left(x_scale_cont, Y_AXIS_WIDTH + 24, 0);   // 左侧留白
    create_x_scale(x_scale_cont, time_labels, &x_scale);  // 创建X轴刻度

    state->chart = *chart;
    state->x_scale = x_scale;
    state->interval_sec = x_interval;         // 间隔1秒
    state->start_time = x_start;
    state->series = *ser;                      // 数据系列
    state->y_min = y_min;                      // 数据系列
    state->y_max = y_max;                      // 数据系列

    /* 8. 初始化图表数据 */
    for (uint32_t i = 0; i < CHART_POINT_COUNT; i++) {
        // 生成范围内的随机数据（示例中用lv_rand，此处适配气体范围）
        float val = y_min + (y_max - y_min) * ((float)rand() / RAND_MAX);
        lv_chart_set_series_value_by_id(*chart, *ser, i, val);
    }
    lv_chart_refresh(*chart);

    return chart_col;
}


// 初始化右侧TabView
static void init_right_side(lv_obj_t* parent) {
    // 右侧容器整体设置（填满父容器，无内边距）
    lv_obj_set_size(parent, RIGHT_WIDTH, SCREEN_HEIGHT - 60);
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(parent, 0, 0);

    // 创建TabView（填满右侧容器）
    tabview = lv_tabview_create(parent);
    lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
    //lv_tabview_set_btns_pos(tabview, LV_TABVIEW_BTNS_TOP);  // Tab按钮在顶部


    // 1. 第一个Tab：历史浓度（显示三个图表，上下排列）
    tab_history = lv_tabview_add_tab(tabview, "历史浓度");
    lv_obj_set_size(tab_history, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(tab_history, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab_history, LV_FLEX_FLOW_COLUMN);
    //lv_obj_set_style_pad_row(tab_history, 15, 0);  // 图表之间的间距
    lv_obj_set_style_pad_all(tab_history, 10, 0);  // 边缘留白

    // 初始时间（当前时间）
    time_t start_time = time(NULL);

    // 1. CO2图表：X轴格式 "%H:%M:%S"，间隔1秒
    create_gas_chart(tab_history, "CO2(ppm)", 735.0f, 765.0f,
        "%H:%M:%S", start_time, 1, &chart_co2, &ser_co2, &co2_chart_state);

    // 2. CH4图表：X轴格式 "%H:%M:%S"，间隔1秒
    create_gas_chart(tab_history, "CH4(ppm)", 2.05f * 100, 2.30f * 100,
        "%H:%M:%S", start_time, 1, &chart_ch4, &ser_ch4, &ch4_chart_state);

    // 3. H2O图表：X轴格式 "%H:%M:%S %Y/%m/%d"，间隔1秒
    create_gas_chart(tab_history, "H2O(%)", 0.625f * 1000, 0.775f * 1000,
        "%Y/%m/%d\n%H:%M:%S", start_time, 1, &chart_h2o, &ser_h2o, &h2o_chart_state);

    // 2. 第二个Tab：操作页面（示例内容）
    lv_obj_t* tab_operate = lv_tabview_add_tab(tabview, "操作页面");
    lv_obj_set_size(tab_operate, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(tab_operate, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(tab_operate, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 示例内容：参数设置区域
    lv_obj_t* param_cont = lv_obj_create(tab_operate);
    lv_obj_set_size(param_cont, LV_PCT(80), LV_PCT(80));
    lv_obj_set_layout(param_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(param_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(param_cont, 20, 0);
    lv_obj_set_style_pad_row(param_cont, 30, 0);

    lv_obj_t* operate_title = lv_label_create(param_cont);
    lv_label_set_text(operate_title, "设备参数设置");

    lv_obj_t* param_label = lv_label_create(param_cont);
    lv_label_set_text(param_label, "采样频率:1次/秒\n数据保存:开启\n报警阈值:CO2 > 760ppm");
}

static void chart_update_timer(chart_state_t* state) {
    // 1. 更新起始时间（+1秒，实现整体移动）
    state->start_time += 1;  // 每次更新，起始时间+1秒

    // 2. 释放旧的时间标签数组（避免内存泄漏）
    if (state->time_labels != NULL) {
        for (int i = 0; state->time_labels[i] != NULL; i++) {
            lv_free((void*)state->time_labels[i]);  // 释放每个标签
        }
        lv_free(state->time_labels);  // 释放数组本身
    }

    // 3. 生成新的时间标签（基于新的起始时间）
    state->time_labels = generate_time_labels(
        state->start_time,
        state->interval_sec,
        "%H:%M:%S"  // 时间格式（与创建时一致）
    );

    // 4. 更新X轴刻度标签
    lv_scale_set_text_src(state->x_scale, state->time_labels);

    // 5. 同步更新图表数据（左移并添加新数据）
    // 左移数据：将所有点向前移动1位
    int32_t *origin_y_array = lv_chart_get_series_y_array(state->chart, state->series);
    for (int i = 0; i < CHART_POINT_COUNT - 1; i++) {
        lv_chart_set_series_value_by_id(state->chart, state->series, i, origin_y_array[i+1]);
    }
    // 添加新数据（示例：随机生成在[y_min, y_max]范围内的值）
    float new_val = state->y_min + (state->y_max - state->y_min) * ((float)rand() / RAND_MAX);
    lv_chart_set_series_value_by_id(state->chart, state->series, CHART_POINT_COUNT - 1, new_val);

    // 6. 刷新图表
    lv_chart_refresh(state->chart);
}

// 数据更新定时器
static void update_data_task(lv_timer_t* timer) {
    if (!is_measuring) return;

    // 生成新数据（确保甲烷和水的数值在图表Y轴范围内）
    data_idx = (data_idx + 1) % 60;
    gas_data[data_idx].timestamp = time(NULL);

    // CO2：735-765 ppm
    gas_data[data_idx].co2 = 735 + (rand() % 30) + ((float)rand() / RAND_MAX) * 1.0;

    // 甲烷：2.05-2.30 ppm（修正随机数范围）
    gas_data[data_idx].ch4 = 2.05 + ((float)rand() / RAND_MAX) * 0.25;  // 0.25的范围确保在2.05-2.30之间

    // 水：0.625-0.775 %（修正随机数范围）
    gas_data[data_idx].h2o = 0.625 + ((float)rand() / RAND_MAX) * 0.15;  // 0.15的范围确保在0.625-0.775之间

    // 更新显示标签
    lv_label_set_text_fmt(label_co2_val, "%.4f", gas_data[data_idx].co2);
    lv_label_set_text_fmt(label_ch4_val, "%.5f", gas_data[data_idx].ch4);
    lv_label_set_text_fmt(label_h2o_val, "%.4f", gas_data[data_idx].h2o);

    // 更新图表数据
    lv_chart_set_next_value(chart_co2, ser_co2, gas_data[data_idx].co2);
    lv_chart_set_next_value(chart_ch4, ser_ch4, gas_data[data_idx].ch4*100);
    lv_chart_set_next_value(chart_h2o, ser_h2o, gas_data[data_idx].h2o*1000);

    //更新刻度
    chart_update_timer(&co2_chart_state);
    chart_update_timer(&ch4_chart_state);
    chart_update_timer(&h2o_chart_state);
}

int main() {
    lv_init();

    // 初始化Windows显示
    lv_display_t* display = lv_windows_create_display(
        L"气体浓度分析仪", SCREEN_WIDTH, SCREEN_HEIGHT, 100, false, true);
    if (!display) return -1;

    // 配置UTF-8控制台输出（可选）
#if LV_TXT_ENC == LV_TXT_ENC_UTF8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    // 设置窗口图标
    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (window_handle) {
        HICON icon_handle = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_LVGL_WINDOWS));
        if (icon_handle) {
            SendMessageW(window_handle, WM_SETICON, TRUE, (LPARAM)icon_handle);
            SendMessageW(window_handle, WM_SETICON, FALSE, (LPARAM)icon_handle);
        }
    }

    // 获取输入设备（鼠标/键盘）
    lv_indev_t* pointer_indev = lv_windows_acquire_pointer_indev(display);
    if (!pointer_indev) return -1;
    lv_indev_t* keypad_indev = lv_windows_acquire_keypad_indev(display);
    if (!keypad_indev) return -1;
    lv_indev_t* encoder_indev = lv_windows_acquire_encoder_indev(display);
    if (!encoder_indev) return -1;

    // 设置窗口关闭事件处理（关键：监听用户点击关闭按钮）
    //set_window_close_handler(display);

    // 4. 创建最上方的标题栏（独立于左右分栏）
    lv_obj_t* top_title_bar = lv_obj_create(lv_scr_act());  // 父容器为屏幕
    lv_obj_set_size(top_title_bar, LV_PCT(100), 80);  // 固定高度80px（根据需求调整），宽度占满
    lv_obj_set_flex_flow(top_title_bar, LV_FLEX_FLOW_ROW);  // 横向布局
    lv_obj_set_flex_align(
        top_title_bar,
        LV_FLEX_ALIGN_CENTER,    // 垂直方向居中（解决高度对齐问题）
        LV_FLEX_ALIGN_SPACE_BETWEEN,  // 水平方向：标题居中，Logo靠右
        LV_FLEX_ALIGN_CENTER
    );
    lv_obj_set_style_pad_left(top_title_bar, 300, 0);   // 左侧留白

    // 标题文本（居中加粗）
    lv_obj_t* main_title = lv_label_create(top_title_bar);
    lv_label_set_text(main_title, "CO2/CH4/H2O气体浓度分析仪");
    lv_obj_set_flex_grow(main_title, 1);  // 占满中间空间，实现居中
    lv_obj_set_style_text_align(main_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(main_title, &SourceHanSansCN_24, 0);
    lv_obj_set_style_height(main_title, LV_SIZE_CONTENT, 0);  // 高度自适应文本，不撑开容器

    // 右侧logo
    lv_obj_t* main_logo = lv_img_create(top_title_bar);
    lv_image_set_src(main_logo, &logo);
    // Logo高度不超过标题栏（80px - 上下内边距），宽度按比例自适应
    lv_obj_set_style_max_height(main_logo, 60, 0);  // 最大高度60px（小于标题栏高度80px）
    lv_obj_set_style_max_width(main_logo, LV_SIZE_CONTENT, 0);  // 宽度按比例自适应
    lv_obj_set_flex_grow(main_logo, 0);  // 不占用额外空间
    lv_obj_align(main_logo, LV_ALIGN_CENTER, 0, 0);  // 相对自身垂直居中（与标题对齐）

    // 3. 创建全局主容器（左右分栏的根容器）
    lv_obj_t* main_cont = lv_obj_create(lv_scr_act());  // 父容器为屏幕
    lv_obj_set_size(main_cont, SCREEN_WIDTH, SCREEN_HEIGHT - 60);  // 高度800-60=740px
    lv_obj_set_y(main_cont, 60);  // 位于标题栏下方（Y坐标=60）
    lv_obj_set_layout(main_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_ROW);  // 水平排列（左右分栏）
    lv_obj_set_style_pad_all(main_cont, 0, 0);  // 无边距，避免浪费空间

    // 4. 创建左侧容器（占1/5宽度），并初始化左侧内容
    lv_obj_t* left_cont = lv_obj_create(main_cont);  // 父容器为main_cont
    lv_obj_set_size(left_cont, LEFT_WIDTH, SCREEN_HEIGHT - 60);  // 宽度256px，高度800px
    lv_obj_set_flex_grow(left_cont, 0);  // 禁止拉伸，固定宽度
    init_left_side(left_cont);  // 调用左侧初始化函数，传入左侧容器

    // 5. 创建右侧容器（占4/5宽度），并初始化右侧内容
    lv_obj_t* right_cont = lv_obj_create(main_cont);  // 父容器为main_cont
    lv_obj_set_size(right_cont, RIGHT_WIDTH, SCREEN_HEIGHT - 60);  // 宽度1024px，高度800px
    lv_obj_set_flex_grow(right_cont, 1);  // 允许拉伸，占满剩余宽度
    init_right_side(right_cont);  // 调用右侧初始化函数，传入右侧容器

    // 启动数据更新定时器
    lv_timer_create(update_data_task, 1000, NULL);

    // 主循环
    while (!app_quit) {
        lv_timer_handler();
        lv_delay_ms(10);
    }

    return 0;
}
