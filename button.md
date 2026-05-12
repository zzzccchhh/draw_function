# 矩阵键盘功能说明

## 硬件布局

```
┌─────────────────────────────┐
│  1    2    3    A (长按)   │
│  4    5    6    B (长按)   │
│  7    8    9    C (长按)   │
│  0   [.]   E    F (长按)   │
└─────────────────────────────┘
```

## short_press_map 索引映射

代码中 `short_press_map` 的定义：

```c
static const uint8 short_press_map[MATRIX_ROW_NUM][MATRIX_COL_NUM] = {
    {1, 2, 3,  4},    // 第1行: 短按输出 1,2,3,4 (长按输出 A,B,C,D)
    {5, 6, 7,  8},    // 第2行: 短按输出 5,6,7,8 (长按输出 E,F,G,H)
    {9, 10, 11, 12},  // 第3行: 短按输出 9,10,11,12 (长按输出 I,J,K,L)
    {13, 14, 15, 16}  // 第4行: 短按输出 13,14,15,16 (长按输出 M,N,O,P)
};
```

长按时返回 `'A' + index - 1`，例如 index=4 则返回 `'A'`，index=8 则返回 `'E'`。

## 短按功能（数字输入）

| 物理位置 | key_index | 功能 | 实现函数 |
|---------|-----------|------|---------|
| 第1行第1列 | 1 | 数字 1 | `handle_digit_input(1)` |
| 第1行第2列 | 2 | 数字 2 | `handle_digit_input(2)` |
| 第1行第3列 | 3 | 数字 3 | `handle_digit_input(3)` |
| 第1行第4列 | 4 | 数字 4 | `handle_digit_input(4)` |
| 第2行第1列 | 5 | 数字 5 | `handle_digit_input(5)` |
| 第2行第2列 | 6 | 数字 6 | `handle_digit_input(6)` |
| 第2行第3列 | 7 | 数字 7 | `handle_digit_input(7)` |
| 第2行第4列 | 8 | 数字 8 | `handle_digit_input(8)` |
| 第3行第1列 | 9 | 数字 9 | `handle_digit_input(9)` |
| 第3行第2列 | 10 | 数字 0 | `handle_digit_input(0)` |
| 第3行第3列 | 11 | 小数点 `.` | `handle_decimal_input()` |
| 第3行第4列 | 12 | 清空当前系数 | `handle_clear_current()` |
| 第4行第1列 | 13 | 切换上一个系数 (E) | `handle_navigate_prev()` |
| 第4行第2列 | 14 | 切换下一个系数 (F) | `handle_navigate_next()` |
| 第4行第3列 | 15 | 退格 | `handle_backspace()` |
| 第4行第4列 | 16 | 无操作 | 无 |

## 长按功能（A-P 输出）

| 物理位置 | key_index | 长按输出 | 功能 | 实现函数 |
|---------|-----------|---------|------|---------|
| 第1行第4列 | 4 | `'A'` | 切换当前系数符号 (+/-) | `handle_sign_toggle()` |
| 第2行第4列 | 8 | `'E'` | 取消输入，返回显示 | `handle_cancel()` |
| 第3行第4列 | 12 | `'L'` | 清空所有系数 | `handle_clear_all()` |
| 第4行第4列 | 16 | `'P'` | 确认输入，绘制图像 | `handle_confirm()` |

## 实现代码

### 矩阵键盘回调处理 (main.c:153-168)

```c
void matrix_key_event_handler(uint8_t key_index, matrix_key_event_enum event, uint8_t key_value)
{
    app_state_t state = app_state_get_current();

    if(state != STATE_INPUT_COEFFICIENTS) return;
    if(event != MATRIX_EVENT_SHORT_PRESS && event != MATRIX_EVENT_LONG_PRESS) return;

    last_input_time = 0;

    if(event == MATRIX_EVENT_LONG_PRESS) {
        switch(key_value) {
            case 'A': handle_sign_toggle(); break;  // 第1行第4列长按
            case 'E': handle_cancel(); break;        // 第2行第4列长按
            case 'L': handle_clear_all(); break;     // 第3行第4列长按
            case 'P': handle_confirm(); break;       // 第4行第4列长按
            default: break;
        }
    }
    else if(event == MATRIX_EVENT_SHORT_PRESS) {
        switch(key_index) {
            case 1: case 2: case 3:
            case 4: case 5: case 6:
            case 7: case 8: case 9:
                handle_digit_input(key_index); break;
            case 10: handle_digit_input(0); break;
            case 11: handle_decimal_input(); break;
            case 12: handle_clear_current(); break;
            case 13: handle_navigate_prev(); break;
            case 14: handle_navigate_next(); break;
            case 15: handle_backspace(); break;
            case 16: break;
            default: break;
        }
    }
}
```

### 数字输入处理 (main.c:291-303)

```c
void handle_digit_input(uint8_t digit)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;
    uint8_t pos = ctx->cursor_pos;

    if(pos < 10) {
        ctx->input_buffer[idx][pos] = '0' + digit;
        ctx->input_buffer[idx][pos + 1] = '\0';
        ctx->cursor_pos++;
        update_display();
    }
}
```

### 小数点输入 (main.c:305-317)

```c
void handle_decimal_input(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    if(!ctx->has_decimal && ctx->cursor_pos > 0 && ctx->cursor_pos < 9) {
        ctx->input_buffer[idx][ctx->cursor_pos] = '.';
        ctx->input_buffer[idx][ctx->cursor_pos + 1] = '\0';
        ctx->cursor_pos++;
        ctx->has_decimal = 1;
        update_display();
    }
}
```

### 退格处理 (main.c:319-332)

```c
void handle_backspace(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    if(ctx->cursor_pos > 0) {
        ctx->cursor_pos--;
        if(ctx->input_buffer[idx][ctx->cursor_pos] == '.') {
            ctx->has_decimal = 0;
        }
        ctx->input_buffer[idx][ctx->cursor_pos] = '\0';
        update_display();
    }
}
```

### 清空当前系数 (main.c:334-344)

```c
void handle_clear_current(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    ctx->input_buffer[idx][0] = '\0';
    ctx->cursor_pos = 0;
    ctx->has_decimal = 0;
    ctx->sign = 1;
    update_display();
}
```

### 系数导航 (main.c:352-377)

```c
void handle_navigate_prev(void)
{
    coef_input_context_t *ctx = coef_input_get_context();

    if(ctx->current_coef_index > 0) {
        ctx->current_coef_index--;
        ctx->cursor_pos = 0;
        ctx->has_decimal = 0;
        ctx->sign = 1;
        update_display();
    }
}

void handle_navigate_next(void)
{
    coef_input_context_t *ctx = coef_input_get_context();

    ctx->current_coef_index++;
    if(ctx->current_coef_index >= ctx->coef_count) {
        ctx->current_coef_index = 0;
    }
    ctx->cursor_pos = 0;
    ctx->has_decimal = 0;
    ctx->sign = 1;
    update_display();
}
```

### 符号切换 (main.c:379-401)

```c
void handle_sign_toggle(void)
{
    coef_input_context_t *ctx = coef_input_get_context();
    uint8_t idx = ctx->current_coef_index;

    if(ctx->cursor_pos > 0) {
        if(ctx->input_buffer[idx][0] == '-') {
            for(uint8_t i = 0; i < ctx->cursor_pos - 1; i++) {
                ctx->input_buffer[idx][i] = ctx->input_buffer[idx][i + 1];
            }
            ctx->input_buffer[idx][ctx->cursor_pos - 1] = '\0';
            ctx->cursor_pos--;
        } else {
            for(uint8_t i = ctx->cursor_pos; i > 0; i--) {
                ctx->input_buffer[idx][i] = ctx->input_buffer[idx][i - 1];
            }
            ctx->input_buffer[idx][0] = '-';
            ctx->input_buffer[idx][ctx->cursor_pos + 1] = '\0';
            ctx->cursor_pos++;
        }
        update_display();
    }
}
```

### 清空所有系数 (main.c:346-350)

```c
void handle_clear_all(void)
{
    coef_input_reset();
    update_display();
}
```

### 取消输入 (main.c:419-428)

```c
void handle_cancel(void)
{
    coef_input_reset();
    matrix_keyboard_set_enabled(0);
    app_state_set_next(STATE_IDLE);

    coef_input_context_t *ctx = coef_input_get_context();
    function_plot_custom(ctx->func_type, ctx->coef);
    USART1_SendString("\r\nCancelled - displaying graph\r\n");
}
```

### 确认输入 (main.c:403-417)

```c
void handle_confirm(void)
{
    coef_input_context_t *ctx = coef_input_get_context();

    for(uint8_t i = 0; i < ctx->coef_count; i++) {
        if(ctx->input_buffer[i][0] != '\0') {
            ctx->coef[i] = parse_coefficient(ctx->input_buffer[i], 1);
        }
    }

    matrix_keyboard_set_enabled(0);
    app_state_set_next(STATE_DISPLAY);
    display_graph();
    USART1_Printf("\r\nConfirmed and displaying graph\r\n");
}
```

## 状态机流程

```
IDLE ─[长按PE8]─→ INPUT_COEFFICIENTS
                        │
                        ├──[短按数字]──→ 输入系数
                        ├──[短按E/F]──→ 切换系数
                        ├──[长按A]──→ 切换符号
                        ├──[短按.]──→ 输入小数点
                        ├──[短按退格]──→ 删除字符
                        ├──[短按清空]──→ 清空当前系数
                        ├──[长按清空]──→ 清空所有系数
                        ├──[长按取消]──→ 取消返回IDLE
                        └──[长按确认]──→ 绘制图像
```