#pragma once

#include "rgy_osdep.h"

static const int disabled_filter_y_size = 42; // チェックボックスだけの高さを加算

struct CLFILTER_TRACKBAR {
public:
    int id;
    HWND label;
    HWND trackbar;
    HWND bt_left;
    HWND bt_right;
    HWND bt_text;

    CLFILTER_TRACKBAR() : id(0), label(NULL), trackbar(NULL), bt_left(NULL), bt_right(NULL), bt_text(NULL) {};

    void enable(const bool enable) {
        EnableWindow(label,    enable);
        EnableWindow(trackbar, enable);
        EnableWindow(bt_left,  enable);
        EnableWindow(bt_right, enable);
        EnableWindow(bt_text,  enable);
    }

    void show_hide(const bool show) {
        ShowWindow(label,    show ? SW_SHOW : SW_HIDE);
        ShowWindow(trackbar, show ? SW_SHOW : SW_HIDE);
        ShowWindow(bt_left,  show ? SW_SHOW : SW_HIDE);
        ShowWindow(bt_right, show ? SW_SHOW : SW_HIDE);
        ShowWindow(bt_text,  show ? SW_SHOW : SW_HIDE);
    }
};

struct CLFILTER_TRACKBAR_DATA {
    CLFILTER_TRACKBAR *tb;
    const char *labelText;
    int label_id;
    int val_min;
    int val_max;
    int val_default;
    int *ex_data_pos;
};

struct CLCU_CONTROL {
    int id;
    HWND hwnd;
    int offset_x;
    int offset_y;
};

class CLCU_FILTER_CONTROLS {
    int y_size;
    std::vector<CLCU_CONTROL> controls;
    int check_min;
    int check_max;
    int track_min;
    int track_max;
public:
    CLCU_FILTER_CONTROLS() :
        y_size(0), controls(), check_min(0), check_max(0), track_min(0), track_max(0) {};
    CLCU_FILTER_CONTROLS(const int check_min, const int check_max, const int track_min, const int track_max) :
        y_size(0), controls(), check_min(check_min), check_max(check_max), track_min(track_min), track_max(track_max) {};
    virtual ~CLCU_FILTER_CONTROLS() {};

    int first_check_idx() const {
        return check_min;
    }

    void add_control(CLCU_CONTROL& control) {
        controls.push_back(control);
    }

    void show_hide() const {
        if (controls.size() == 0) {
            return;
        }
        // フィルタの有効/無効を確認
        const int enable = SendMessage(controls[0].hwnd, BM_GETCHECK, 0, 0);
        // 最初のチェックボックスを除いて表示/非表示を切り替え
        for (size_t i = 1; i < controls.size(); i++) {
            ShowWindow(controls[i].hwnd, enable ? SW_SHOW : SW_HIDE);
        }
    }

    void move_group(int& y_pos, const int col, const int col_width) const {
        if (controls.size() == 0) {
            return;
        }
        for (const auto& control : controls) {
            SetWindowPos(control.hwnd, HWND_TOP, col * col_width + control.offset_x, y_pos + control.offset_y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
        }
        y_pos += y_actual_size();
    }

    int y_actual_size() const {
        const int enable = SendMessage(controls[0].hwnd, BM_GETCHECK, 0, 0);
        return (enable) ? y_size : disabled_filter_y_size;
    }

    void set_y_size(const int new_y_size) {
        y_size = new_y_size;
    }
};
