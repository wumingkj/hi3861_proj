#include "buzzer.h"

typedef enum {
    BUZ_IDLE = 0,
    BUZ_ON,
    BUZ_OFF_GAP,
    BUZ_MUSIC_NOTE,
    BUZ_MUSIC_REST,
} buz_state_t;

// 音符定义结构
typedef struct {
    uint16_t freq_hz;    // 频率
    uint16_t duration_ms; // 持续时间
} note_t;

// 小星星乐谱
static const note_t little_star_notes[] = {
    {NOTE_C4, 500}, {NOTE_C4, 500}, {NOTE_G4, 500}, {NOTE_G4, 500},
    {NOTE_A4, 500}, {NOTE_A4, 500}, {NOTE_G4, 1000},
    {NOTE_F4, 500}, {NOTE_F4, 500}, {NOTE_E4, 500}, {NOTE_E4, 500},
    {NOTE_D4, 500}, {NOTE_D4, 500}, {NOTE_C4, 1000},
    {NOTE_G4, 500}, {NOTE_G4, 500}, {NOTE_F4, 500}, {NOTE_F4, 500},
    {NOTE_E4, 500}, {NOTE_E4, 500}, {NOTE_D4, 1000},
    {NOTE_G4, 500}, {NOTE_G4, 500}, {NOTE_F4, 500}, {NOTE_F4, 500},
    {NOTE_E4, 500}, {NOTE_E4, 500}, {NOTE_D4, 1000},
    {NOTE_C4, 500}, {NOTE_C4, 500}, {NOTE_G4, 500}, {NOTE_G4, 500},
    {NOTE_A4, 500}, {NOTE_A4, 500}, {NOTE_G4, 1000},
    {NOTE_F4, 500}, {NOTE_F4, 500}, {NOTE_E4, 500}, {NOTE_E4, 500},
    {NOTE_D4, 500}, {NOTE_D4, 500}, {NOTE_C4, 1000}
};

#define LITTLE_STAR_NOTES_COUNT (sizeof(little_star_notes) / sizeof(little_star_notes[0]))

typedef struct {
    buz_state_t st;

    uint16_t remaining;      // 剩余响的次数（一次响算一次）
    uint16_t on_ms;
    uint16_t off_ms;

    uint32_t t_ref;          // 上次切换时间戳(ms)

    bool busy;
    
    // 音乐播放相关
    uint16_t current_note;   // 当前播放的音符索引
    uint16_t total_notes;    // 总音符数
    const note_t *music_notes; // 乐谱指针
} buz_ctx_t;

static buz_ctx_t g_buz;

// 计算PWM周期基于频率
static uint32_t calculate_period_from_freq(uint32_t freq_hz) {
    if (freq_hz == 0) return BEEP_PWM_PERIOD;
    return 1000000 / freq_hz; // 1MHz / freq_hz = 周期(us)
}

static inline void pwm_on_with_freq(uint32_t freq_hz)
{
    uint32_t period = calculate_period_from_freq(freq_hz);
    (void)hi_pwm_start(BEEP_PWM_PORT, period / 2, period); // 50%占空比
    log_d("BUZZER", "PWM ON: freq=%dHz, period=%d", freq_hz, period);
}

static inline void pwm_on(void)
{
    pwm_on_with_freq(1000000 / BEEP_PWM_PERIOD); // 使用默认频率
}

static inline void pwm_off(void)
{
    (void)hi_pwm_stop(BEEP_PWM_PORT);
    log_d("BUZZER", "PWM OFF");
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
    g_buz.current_note = 0;
    g_buz.total_notes = 0;
    g_buz.music_notes = NULL;
    pwm_off();
    
    log_i("BUZZER", "Buzzer initialized successfully");
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
    g_buz.current_note = 0;
    g_buz.total_notes = 0;
    g_buz.music_notes = NULL;
    pwm_off();
    log_i("BUZZER", "Buzzer stopped");
}

// 新的PWM控制API
void Buzzer_SetFrequency(uint32_t freq_hz) {
    if (freq_hz > 0) {
        uint32_t period = calculate_period_from_freq(freq_hz);
        (void)hi_pwm_start(BEEP_PWM_PORT, period / 2, period);
        log_i("BUZZER", "Frequency set to %d Hz", freq_hz);
    }
}

void Buzzer_SetDuty(uint16_t duty) {
    // 保持当前频率，只改变占空比
    uint32_t current_period = BEEP_PWM_PERIOD; // 简化处理
    uint32_t duty_cycle = (current_period * duty) / 100;
    (void)hi_pwm_start(BEEP_PWM_PORT, duty_cycle, current_period);
    log_i("BUZZER", "Duty cycle set to %d%%", duty);
}

void Buzzer_StartPWM(void) {
    pwm_on();
}

void Buzzer_StopPWM(void) {
    pwm_off();
}

// 播放小星星
void Buzzer_PlayLittleStar(void) {
    pwm_off(); // 打断当前播放

    g_buz.music_notes = little_star_notes;
    g_buz.total_notes = LITTLE_STAR_NOTES_COUNT;
    g_buz.current_note = 0;
    
    g_buz.st = BUZ_MUSIC_NOTE;
    g_buz.busy = true;
    g_buz.t_ref = 0xFFFFFFFFu; // 标记"等待首个 Tick"

    // 开始播放第一个音符
    if (g_buz.current_note < g_buz.total_notes) {
        pwm_on_with_freq(g_buz.music_notes[g_buz.current_note].freq_hz);
    }
    
    log_i("BUZZER", "Little Star music started, total notes: %d", g_buz.total_notes);
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
        log_w("BUZZER", "Invalid parameters: cnt=%d, on_ms=%d", cnt, on_ms);
        return;
    }

    g_buz.remaining = cnt;
    g_buz.on_ms = on_ms;
    g_buz.off_ms = off_ms;

    g_buz.st = BUZ_ON;
    g_buz.busy = true;

    // t_ref 由外部 Tick 首次调用时刷新也行
    // 这里不拿 now_ms，避免依赖 time 库
    g_buz.t_ref = 0xFFFFFFFFu; // 标记"等待首个 Tick"
    pwm_on();
    
    log_i("BUZZER", "Alarm started: cnt=%d, on_ms=%d, off_ms=%d", cnt, on_ms, off_ms);
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

            // 一个"响"结束，消耗一次
            if (g_buz.remaining > 0) g_buz.remaining--;

            if (g_buz.remaining == 0) {
                g_buz.st = BUZ_IDLE;
                g_buz.busy = false;
                log_i("BUZZER", "Alarm completed");
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

    case BUZ_MUSIC_NOTE:
        if (dt >= g_buz.music_notes[g_buz.current_note].duration_ms) {
            // 当前音符播放结束
            g_buz.current_note++;
            g_buz.t_ref = now_ms;

            if (g_buz.current_note >= g_buz.total_notes) {
                // 音乐播放完成
                pwm_off();
                g_buz.st = BUZ_IDLE;
                g_buz.busy = false;
                log_i("BUZZER", "Music completed");
                return;
            }

            // 播放下一个音符
            pwm_on_with_freq(g_buz.music_notes[g_buz.current_note].freq_hz);
            log_d("BUZZER", "Playing note %d/%d: freq=%dHz, duration=%dms", 
                  g_buz.current_note + 1, g_buz.total_notes,
                  g_buz.music_notes[g_buz.current_note].freq_hz,
                  g_buz.music_notes[g_buz.current_note].duration_ms);
        }
        break;

    default:
        // 安全兜底
        Buzzer_Stop();
        log_w("BUZZER", "Unknown state, buzzer stopped");
        break;
    }
}