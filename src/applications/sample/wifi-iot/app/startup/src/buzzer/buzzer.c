#include "buzzer.h"

typedef enum {
    BUZ_IDLE = 0,
    BUZ_ON,
    BUZ_OFF_GAP,
} buz_state_t;

typedef struct {
    buz_state_t st;

    uint16_t remaining;      // 剩余响的次数（一次响算一次）
    uint16_t on_ms;
    uint16_t off_ms;

    uint32_t t_ref;          // 上次切换时间戳(ms)

    bool busy;
} buz_ctx_t;

static buz_ctx_t g_buz;

static inline void pwm_on(void)
{
    (void)hi_pwm_start(BEEP_PWM_PORT, BEEP_PWM_DUTY, BEEP_PWM_PERIOD);
}
static inline void pwm_off(void)
{
    (void)hi_pwm_stop(BEEP_PWM_PORT);
}

void Buzzer_Init(void)
{
    hi_gpio_init();
    hi_io_set_pull(BEEP_PIN, HI_IO_PULL_UP);
    hi_io_set_func(BEEP_PIN, BEEP_PIN_PWM_FUNC);
    hi_gpio_set_dir(BEEP_PIN, HI_GPIO_DIR_OUT);

    (void)hi_pwm_init(BEEP_PWM_PORT);

    g_buz.st = BUZ_IDLE;
    g_buz.busy = false;
    g_buz.remaining = 0;
    pwm_off();
}

bool Buzzer_IsBusy(void)
{
    return g_buz.busy;
}

void Buzzer_Stop(void)
{
    g_buz.st = BUZ_IDLE;
    g_buz.busy = false;
    g_buz.remaining = 0;
    pwm_off();
}

void Buzzer_BeepMs(uint16_t ms)
{
    // 等价于 cnt=1, off=0
    Buzzer_Alarm(1, ms, 0);
}

void Buzzer_Alarm(uint16_t cnt, uint16_t on_ms, uint16_t off_ms)
{
    // 打断当前
    pwm_off();

    if (cnt == 0 || on_ms == 0) {
        Buzzer_Stop();
        return;
    }

    g_buz.remaining = cnt;
    g_buz.on_ms = on_ms;
    g_buz.off_ms = off_ms;

    g_buz.st = BUZ_ON;
    g_buz.busy = true;

    // t_ref 由外部 Tick 首次调用时刷新也行
    // 这里不拿 now_ms，避免依赖 time 库
    g_buz.t_ref = 0xFFFFFFFFu; // 标记“等待首个 Tick”
    pwm_on();
}

void Buzzer_Tick(uint32_t now_ms)
{
    if (!g_buz.busy) return;

    // 第一次进来：补上基准时间
    if (g_buz.t_ref == 0xFFFFFFFFu) {
        g_buz.t_ref = now_ms;
        return;
    }

    uint32_t dt = now_ms - g_buz.t_ref;

    switch (g_buz.st) {
    case BUZ_ON:
        if (dt >= g_buz.on_ms) {
            pwm_off();
            g_buz.t_ref = now_ms;

            // 一个“响”结束，消耗一次
            if (g_buz.remaining > 0) g_buz.remaining--;

            if (g_buz.remaining == 0) {
                g_buz.st = BUZ_IDLE;
                g_buz.busy = false;
                return;
            }

            // 进入停的间隙
            if (g_buz.off_ms > 0) {
                g_buz.st = BUZ_OFF_GAP;
            } else {
                // off_ms=0：立刻继续响
                g_buz.st = BUZ_ON;
                pwm_on();
                g_buz.t_ref = now_ms;
            }
        }
        break;

    case BUZ_OFF_GAP:
        if (dt >= g_buz.off_ms) {
            g_buz.st = BUZ_ON;
            g_buz.t_ref = now_ms;
            pwm_on();
        }
        break;

    default:
        // 安全兜底
        Buzzer_Stop();
        break;
    }
}
