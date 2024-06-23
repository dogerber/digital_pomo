
#include <Arduino.h>

#include "lv_conf.h"
#include "lvgl.h" /* https://github.com/lvgl/lvgl.git */
#include "Arduino.h"
#include "Wire.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "pin_config.h"
// #include "lv_styles.h"

#include <TouchDrvCSTXXX.hpp>

#define HORIZONTAL_RESOLUTION 320
#define VERTICAL_RESOLUTION 170

#define COUNTDOWN_DEFAULT_SECONDS 12

typedef enum
{
    PROGRESS_BAR,
    LAST_VISUALISATION_MODE
} visualisation_mode;

visualisation_mode visualisation_mode_active = PROGRESS_BAR;

TouchDrvCSTXXX touch;
int16_t x[5], y[5];

esp_lcd_panel_io_handle_t io_handle = NULL;
static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;      // contains callback functions
static lv_color_t *lv_disp_buf;
static bool is_initialized_lvgl = false;

lv_obj_t *time_label;
lv_obj_t *start_button;
lv_obj_t *set_button;
lv_obj_t *popup;
lv_obj_t *confirm_button;
lv_obj_t *confirm_label;
lv_obj_t *label_popup;

// styles
static lv_style_t style_btn_norm;

// visualisation objects
lv_obj_t *lv_visualisation_object;

int countdown_seconds = 10;
int countdown_starttime_seconds = countdown_seconds;
bool countdown_running = false;

// ----- Structures -----
typedef struct
{
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},

};

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    if (is_initialized_lvgl)
    {
        lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
        lv_disp_flush_ready(disp_driver);
    }
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void lv_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{

    if (touch.getPoint(x, y, touch.getSupportTouchPoint()))
    {
        data->point.x = *x;
        data->point.y = *y;
        data->state = LV_INDEV_STATE_PR;
    }
    else
        data->state = LV_INDEV_STATE_REL;
}

// ----- Button event handlers -----
void start_button_event_handler(lv_event_t *e)
{
    countdown_starttime_seconds = countdown_seconds;
    countdown_running = true;
}

void set_button_event_handler(lv_event_t *e)
{
    // countdown_seconds = (hours * 3600) + (minutes * 60) + seconds;
    countdown_running = false;
    update_time(NULL);
}

void confirm_button_event_handler(lv_event_t *e)
{
    lv_obj_add_flag(popup, LV_OBJ_FLAG_HIDDEN); // Hide the popup
}

void button_1_pressed()
{
    Serial.println("button 1 pressed");
    countdown_seconds = countdown_seconds + 10;
    update_time(NULL); // to update the screen
}

void button_2_pressed()
{
    Serial.println("button 2 pressed");
    countdown_seconds = countdown_seconds - 10;
    update_time(NULL); // to update the screen
}

void inc_button_event_handler(lv_event_t *e)
{
    countdown_seconds += 60; // Increment by 1 minute
    update_time(NULL);       // Update the display with the new time
}

void dec_button_event_handler(lv_event_t *e)
{
    if (countdown_seconds > 60)
    {                            // Prevent negative countdown time
        countdown_seconds -= 60; // Decrement by 1 minute
        update_time(NULL);       // Update the display with the new time
    }
}

// ----- styles -----

void lv_set_button_style(void)
{
    lv_style_init(&style_btn_norm);
    lv_style_set_bg_color(&style_btn_norm, (lv_color_black())); // Set background color
    lv_style_set_pad_all(&style_btn_norm, 10);
    lv_style_set_bg_opa(&style_btn_norm, LV_OPA_COVER);             // Set background opacity
    lv_style_set_border_width(&style_btn_norm, 2);                  // Set border width
    lv_style_set_border_color(&style_btn_norm, (lv_color_white())); // Set border color
    lv_style_set_radius(&style_btn_norm, 5);                        // Set corner radius for rounded corners
    // lv_style_set_text_color(&style_btn_norm, lv_palette_main(LV_PALETTE_WHITE));   // Set text color
}

// ----- create functions -----

/**
 * @brief Create the countdown screen.
 *
 * This function creates the countdown screen, which includes a label, a time label,
 * set and start buttons, increment and decrement buttons, and a visualisation.
 */
void create_countdown_screen()
{
    lv_obj_t *scr = lv_scr_act();

    // Create the label at the top of the screen
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Countdown Timer");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    // Create the time label in the center of the screen
    time_label = lv_label_create(scr);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -20);

    // Create the set button in the center of the screen
    set_button = lv_btn_create(scr);
    lv_obj_remove_style_all(set_button); /*Remove the style coming from the theme*/
    lv_obj_add_style(set_button, &style_btn_norm, 0);
    lv_obj_align(set_button, LV_ALIGN_CENTER, 60, 60);
    lv_obj_t *set_label = lv_label_create(set_button);
    lv_label_set_text(set_label, "Set Time");
    lv_obj_add_event_cb(set_button, set_button_event_handler, LV_EVENT_CLICKED, NULL);

    // Create the start button in the center of the screen
    start_button = lv_btn_create(scr);
    lv_obj_remove_style_all(start_button);
    lv_obj_add_style(start_button, &style_btn_norm, 0);
    lv_obj_align(start_button, LV_ALIGN_CENTER, -60, 60);
    lv_obj_t *start_label = lv_label_create(start_button);
    lv_label_set_text(start_label, "Start");
    lv_obj_add_event_cb(start_button, start_button_event_handler, LV_EVENT_CLICKED, NULL);

    // Increment Button
    lv_obj_t *inc_button = lv_btn_create(scr);
    lv_obj_remove_style_all(inc_button);
    lv_obj_add_style(inc_button, &style_btn_norm, 0);
    lv_obj_align(inc_button, LV_ALIGN_CENTER, 120, -30); // Adjust position as needed
    lv_obj_t *inc_label = lv_label_create(inc_button);
    lv_label_set_text(inc_label, "+");
    lv_obj_add_event_cb(inc_button, inc_button_event_handler, LV_EVENT_CLICKED, NULL);

    // Decrement Button
    lv_obj_t *dec_button = lv_btn_create(scr);
    lv_obj_remove_style_all(dec_button);
    lv_obj_add_style(dec_button, &style_btn_norm, 0);
    lv_obj_align(dec_button, LV_ALIGN_CENTER, 120, 30); // Adjust position as needed
    lv_obj_t *dec_label = lv_label_create(dec_button);
    lv_label_set_text(dec_label, "-");
    lv_obj_add_event_cb(dec_button, dec_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_timer_create(update_time, 1000, NULL); // Update time every second

    create_popup(); // Displayed when countdown is done

    create_visualisation();
}

void create_visualisation()
{

    lv_obj_t *scr = lv_scr_act();

    if (visualisation_mode_active == PROGRESS_BAR)
    {
        lv_visualisation_object = lv_bar_create(scr);
        lv_obj_set_size(lv_visualisation_object, 200, 20);
        lv_obj_align(lv_visualisation_object, LV_ALIGN_CENTER, 0, 0);
        lv_bar_set_range(lv_visualisation_object, 0, 100);
        lv_bar_set_value(lv_visualisation_object, 100, LV_ANIM_OFF);
    }
}

void create_popup()
{
    popup = lv_obj_create(lv_scr_act()); // todo use messagebox instead? https://docs.lvgl.io/7.11/widgets/msgbox.html
    lv_obj_set_size(popup, 250, 120);
    lv_obj_center(popup);

    label_popup = lv_label_create(popup);
    lv_label_set_text(label_popup, "Time's up!\nPlease confirm.");
    lv_obj_align(label_popup, LV_ALIGN_TOP_MID, 0, 40);

    confirm_button = lv_btn_create(popup);
    lv_obj_align(confirm_button, LV_ALIGN_BOTTOM_MID, 0, -50);
    confirm_label = lv_label_create(confirm_button);
    lv_label_set_text(confirm_label, "Confirm");
    lv_obj_add_event_cb(confirm_button, confirm_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(popup, LV_OBJ_FLAG_HIDDEN); // Hide the popup initially
}

// ----- Update functions -----
void update_visualisation()
{

    // todo disable update if countdown is done?
    if (visualisation_mode_active == PROGRESS_BAR)
    {
        lv_bar_set_value(lv_visualisation_object, (100 * (countdown_starttime_seconds - countdown_seconds)) / countdown_starttime_seconds, LV_ANIM_ON);
    }
}

void update_time(lv_timer_t *timer)
{
    if (countdown_running)
    {
        countdown_seconds--;
        update_visualisation();
        if (countdown_seconds <= 0)
        {
            countdown_seconds = 0;
            countdown_running = false;
            lv_label_set_text(time_label, "Time's up!");
            lv_obj_move_foreground(popup);                // Move the popup to the front
            lv_obj_clear_flag(popup, LV_OBJ_FLAG_HIDDEN); // Show the popup when the countdown is done
            return;
        }
    }

    int hours = countdown_seconds / 3600;
    int minutes = (countdown_seconds % 3600) / 60;
    int seconds = countdown_seconds % 60;
    char buf[64];
    sprintf(buf, "%02d:%02d:%02d", hours, minutes, seconds);
    lv_label_set_text(time_label, buf);
}

// ----- Setup and Loop -----

void setup()
{
    Serial.begin(115200);
    Serial.println("started setup()");

    // ----- Input Buttons Init -----
    pinMode(PIN_BUTTON_1, INPUT_PULLUP);
    pinMode(PIN_BUTTON_2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_1), button_1_pressed, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON_2), button_2_pressed, RISING);

    // ----- Display Init -----
    pinMode(PIN_POWER_ON, OUTPUT); // Turn on display power
    digitalWrite(PIN_POWER_ON, HIGH);

    pinMode(PIN_LCD_RD, OUTPUT);
    digitalWrite(PIN_LCD_RD, HIGH);
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = PIN_LCD_DC,
        .wr_gpio_num = PIN_LCD_WR,
        .clk_src = LCD_CLK_SRC_PLL160M,
        .data_gpio_nums =
            {
                PIN_LCD_D0,
                PIN_LCD_D1,
                PIN_LCD_D2,
                PIN_LCD_D3,
                PIN_LCD_D4,
                PIN_LCD_D5,
                PIN_LCD_D6,
                PIN_LCD_D7,
            },
        .bus_width = 8,
        .max_transfer_bytes = LVGL_LCD_BUF_SIZE * sizeof(uint16_t),
    };
    esp_lcd_new_i80_bus(&bus_config, &i80_bus);

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 20,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels =
            {
                .dc_idle_level = 0,
                .dc_cmd_level = 0,
                .dc_dummy_level = 0,
                .dc_data_level = 1,
            },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RES,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);

    esp_lcd_panel_swap_xy(panel_handle, true); // screen display Orientation
    esp_lcd_panel_mirror(panel_handle, false, true);
    // the gap is LCD panel specific, even panels with the same driver IC, can
    // have different gap value
    esp_lcd_panel_set_gap(panel_handle, 0, 35);
    for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++)
    {
        esp_lcd_panel_io_tx_param(io_handle, lcd_st7789v[i].cmd, lcd_st7789v[i].data, lcd_st7789v[i].len & 0x7f);
        if (lcd_st7789v[i].len & 0x80)
            delay(120);
    }

    /* Lighten the screen with gradient */
    ledcSetup(0, 10000, 8);
    ledcAttachPin(PIN_LCD_BL, 0);
    for (uint8_t i = 0; i < 0xFF; i++)
    {
        ledcWrite(0, i);
        delay(2);
    }

    // ----- Init LVGL -----
    lv_init();
    lv_disp_buf = (lv_color_t *)heap_caps_malloc(LVGL_LCD_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    // lv_obj_clean(lv_scr_act()); // clear screen

    lv_disp_draw_buf_init(&disp_buf, lv_disp_buf, NULL, LVGL_LCD_BUF_SIZE);
    /*Initialize the display*/
    lv_disp_drv_init(&disp_drv);
    // lv_disp_set_rotation(NULL, LV_DISP_ROT_90);
    disp_drv.hor_res = HORIZONTAL_RESOLUTION;
    disp_drv.ver_res = VERTICAL_RESOLUTION;

    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);

    // ----- Initialize capacitive touch    -----
    touch.setPins(BOARD_TOUCH_RST, BOARD_TOUCH_IRQ);

    if (!touch.begin(Wire, CST328_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL))
    {
        Serial.println("Failed init CST328 Device!");
        if (!touch.begin(Wire, CST816_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL))
        {
            Serial.println("Failed init CST816 Device!");
            while (1)
            {
                Serial.println("Not find touch device!");
                delay(1000);
            }
        }
    }

    // fix orientation
    touch.setMaxCoordinates(HORIZONTAL_RESOLUTION, VERTICAL_RESOLUTION);
    touch.setMirrorXY(true, false);
    touch.setSwapXY(true);

    // ----- Link LVGL to all components -----
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    is_initialized_lvgl = true;

    // Set the initial time to Jan 1 2022 12:00:00 (you can change it as needed)
    struct tm tm;
    tm.tm_year = 2022 - 1900; // Year since 1900
    tm.tm_mon = 0;            // Month, 0 - jan
    tm.tm_mday = 1;           // Day of the month
    tm.tm_hour = 12;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    time_t t = mktime(&tm);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);

    // set styles
    lv_set_button_style();

    create_countdown_screen();
    Serial.println("Reached end of setup()");
}

void loop()
{
    lv_timer_handler();
    delay(2);
}
