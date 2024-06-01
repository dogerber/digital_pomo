#include <WiFi.h>
#include <time.h>

#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
// #include <Adafruit_FT6206.h> // Or your specific touchscreen library
#include "User_setup.h"
#include <TouchDrvCSTXXX.hpp>

TFT_eSPI tft = TFT_eSPI();
// Adafruit_FT6206 ts = Adafruit_FT6206();
TouchDrvCSTXXX ts;

#define SCREEN_WIDTH 170
#define SCREEN_HEIGHT 320

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 10];

lv_obj_t *time_label;
lv_obj_t *start_button;
lv_obj_t *set_button;
lv_obj_t *hour_spinbox;
lv_obj_t *min_spinbox;
lv_obj_t *sec_spinbox;

int countdown_seconds = 0;
bool countdown_running = false;

const char *ssid = "FILL ME";
const char *password = "FILL ME";

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint16_t c;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
    for (int y = area->y1; y <= area->y2; y++)
    {
        for (int x = area->x1; x <= area->x2; x++)
        {
            c = color_p->full;
            tft.pushColor(c);
            color_p++;
        }
    }
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    int16_t x[5], y[5];

    if (!ts.getPoint(x, y, ts.getSupportTouchPoint()))
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        // TS_Point p = ts.getPoint();
        data->state = LV_INDEV_STATE_PR;
        data->point.x = *x;
        data->point.y = *y;
    }
}

void update_time(lv_timer_t *timer)
{
    if (countdown_running)
    {
        countdown_seconds--;
        if (countdown_seconds <= 0)
        {
            countdown_seconds = 0;
            countdown_running = false;
            lv_label_set_text(time_label, "Time's up!");
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

void start_button_event_handler(lv_event_t *e)
{
    countdown_running = true;
}

void set_button_event_handler(lv_event_t *e)
{
    int hours = lv_spinbox_get_value(hour_spinbox);
    int minutes = lv_spinbox_get_value(min_spinbox);
    int seconds = lv_spinbox_get_value(sec_spinbox);
    countdown_seconds = (hours * 3600) + (minutes * 60) + seconds;
    countdown_running = false;
    update_time(NULL);
}

void create_countdown_screen()
{
    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Countdown Timer");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    time_label = lv_label_create(scr);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -40);

    hour_spinbox = lv_spinbox_create(scr);
    lv_spinbox_set_range(hour_spinbox, 0, 99);
    lv_spinbox_set_digit_format(hour_spinbox, 2, 0);
    lv_obj_align(hour_spinbox, LV_ALIGN_CENTER, -80, 40);

    min_spinbox = lv_spinbox_create(scr);
    lv_spinbox_set_range(min_spinbox, 0, 59);
    lv_spinbox_set_digit_format(min_spinbox, 2, 0);
    lv_obj_align(min_spinbox, LV_ALIGN_CENTER, 0, 40);

    sec_spinbox = lv_spinbox_create(scr);
    lv_spinbox_set_range(sec_spinbox, 0, 59);
    lv_spinbox_set_digit_format(sec_spinbox, 2, 0);
    lv_obj_align(sec_spinbox, LV_ALIGN_CENTER, 80, 40);

    set_button = lv_btn_create(scr);
    lv_obj_align(set_button, LV_ALIGN_CENTER, -60, 100);
    lv_obj_t *set_label = lv_label_create(set_button);
    lv_label_set_text(set_label, "Set Time");
    lv_obj_add_event_cb(set_button, set_button_event_handler, LV_EVENT_CLICKED, NULL);

    start_button = lv_btn_create(scr);
    lv_obj_align(start_button, LV_ALIGN_CENTER, 60, 100);
    lv_obj_t *start_label = lv_label_create(start_button);
    lv_label_set_text(start_label, "Start");
    lv_obj_add_event_cb(start_button, start_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_timer_create(update_time, 1000, NULL); // Update time every second
}

void setup()
{
    Serial.begin(115200);
    tft.begin();
    tft.setRotation(0); // 0: Portrait, 1: Landscape, 2: Inverted Portrait, 3: Inverted Landscape

    tft.setColorDepth(16);

    // Initialize capacitive touch
    ts.setPins(BOARD_TOUCH_RST, BOARD_TOUCH_IRQ);

    if (!ts.begin(Wire, CST328_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL))
    {
        Serial.println("Failed init CST328 Device!");
        if (!ts.begin(Wire, CST816_SLAVE_ADDRESS, BOARD_I2C_SDA, BOARD_I2C_SCL))
        {
            Serial.println("Failed init CST816 Device!");
            while (1)
            {
                Serial.println("Not find touch device!");
                delay(1000);
            }
        }
    }
    Serial.println("passed touch screen init");

    // fix orientation
    ts.setMaxCoordinates(SCREEN_HEIGHT, SCREEN_WIDTH);
    ts.setMirrorXY(true, false);
    ts.setSwapXY(false);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    if (false)
    {
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("WiFi connected.");

        configTime(0, 0, "time.windows.com", "time.nist.gov"); // Get time from NTP servers
        if (!getLocalTime(NULL))
        {
            Serial.println("Failed to obtain time");
        }
        else
        {
            Serial.println("Time obtained");
        }
    }
    else
    {
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
    }

    create_countdown_screen();
}

void loop()
{
    lv_task_handler();
    delay(5);
}
