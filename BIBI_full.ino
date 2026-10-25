#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// =====================================================
// БИБИ
// Обычное управление БЕЗ балансировки
// =====================================================


// =====================================================
// ПИНЫ
// =====================================================

// LED MATRIX
#define LED_PIN 33
#define NUM_LEDS 64
#define BRIGHTNESS 40

// МОТОРЫ
#define MOTOR_1A  27
#define MOTOR_A1B 14

#define MOTOR_B1A 16
#define MOTOR_B2A 17

// РУКИ
#define LEFT_ARM_PIN  32
#define RIGHT_ARM_PIN 12

// I2C
#define SDA_PIN 21
#define SCL_PIN 22


// =====================================================
// НАСТРОЙКИ ДВИЖЕНИЯ
// =====================================================

// 0 = стоп
// 255 = максимальная команда PWM
//
// Начинаем со 170.
#define DRIVE_SPEED 170

String currentDirection = "stop";


// =====================================================
// VL53L0X
// =====================================================

#define STOP_DISTANCE_MM 90

Adafruit_VL53L0X lox;

bool distanceSensorOK = false;

int lastDistanceMM = -1;

unsigned long lastDistanceCheck = 0;


// =====================================================
// MPU6050
// =====================================================

// MPU теперь ТОЛЬКО показывает данные.
// Моторы он НЕ контролирует.

Adafruit_MPU6050 mpu;

bool mpuOK = false;

float pitch = 0.0;
float roll = 0.0;

float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;

unsigned long lastMPUCheck = 0;


// =====================================================
// LED MATRIX
// =====================================================

CRGB leds[NUM_LEDS];


// =====================================================
// РУКИ
// =====================================================

Servo leftArm;
Servo rightArm;


// =====================================================
// ПОДМИГИВАНИЕ БЕЗ DELAY
// =====================================================

bool winkActive = false;
uint8_t winkStage = 0;
unsigned long winkTimer = 0;


// =====================================================
// WI-FI
// =====================================================

const char* ssid = "BIBI_AP";
const char* password = "12345678";

WebServer server(80);


// =====================================================
// HTML
// =====================================================
//
// Сюда можешь вставить свой старый полный HTML.
// Команды должны идти:
//
// /cmd/forward
// /cmd/back
// /cmd/left
// /cmd/right
// /cmd/stop
//
// /cmd/leftarm/90
// /cmd/rightarm/90
//
// /cmd/face/default
// /cmd/face/wink
// и т.д.
//

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover,user-scalable=no">
    <meta name="theme-color" content="#05060d">
    <meta name="mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <title>Биби</title>
    <style>
        * {
            box-sizing: border-box;
            -webkit-tap-highlight-color: transparent
        }

        :root {
            --bg: #05060d;
            --card: #161b24;
            --card2: #1c2330;
            --line: #2a3242;
            --txt: #e9eef7;
            --dim: #8b96a8;
            --acc: #4f8cff;
            --ok: #3ddc97;
            --err: #ff5c5c;
            --motion: #3b82f6;
            --emotion: #ec4899;
            --var: #f59e0b;
            --ctrl: #22c55e;
            --misc: #64748b;
            --math: #a855f7;
        }

        html, body {
            margin: 0;
            height: 100%;
            overflow: hidden
        }

        body {
            background: #05060d;
            color: var(--txt);
            font-family: -apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Arial,sans-serif;
            font-size: 15px;
            display: flex;
            flex-direction: column;
            padding-top: env(safe-area-inset-top);
            -webkit-user-select: none;
            user-select: none;
            position: relative;
            isolation: isolate;
        }

            /* ================= КОСМИЧЕСКИЙ ФОН ================= */

            /* Слой 1: туманности через радиальные градиенты */
            body::before {
                content: "";
                position: fixed;
                inset: 0;
                z-index: -3;
                background: radial-gradient(ellipse 60% 40% at 12% 18%, rgba(107,70,255,.55), transparent 70%), radial-gradient(ellipse 50% 60% at 88% 22%, rgba(0,180,220,.42), transparent 70%), radial-gradient(ellipse 70% 50% at 72% 88%, rgba(255,40,140,.32), transparent 70%), radial-gradient(ellipse 55% 55% at 25% 92%, rgba(60,90,255,.30), transparent 70%), radial-gradient(circle at 50% 50%, #0a1024 0%, #05060d 65%, #000000 100%);
                filter: blur(0.5px);
                animation: nebulaDrift 32s ease-in-out infinite alternate;
            }

        @keyframes nebulaDrift {
            0% {
                background-position: 0 0, 0 0, 0 0, 0 0, 0 0
            }

            100% {
                background-position: 20px 10px, -15px 12px, 10px -14px, -12px -8px, 0 0
            }
        }

        /* Слой 2: звёзды-точки (мелкая сетка «пыли») */
        body::after {
            content: "";
            position: fixed;
            inset: 0;
            z-index: -2;
            background-image: radial-gradient(1px 1px at 12px 24px, #ffffff, transparent 60%), radial-gradient(1px 1px at 88px 68px, #c9d8ff, transparent 60%), radial-gradient(1px 1px at 152px 12px, #ffffff, transparent 60%), radial-gradient(1px 1px at 210px 90px, #ffd0e8, transparent 60%), radial-gradient(1px 1px at 60px 130px,#a8c6ff, transparent 60%), radial-gradient(1px 1px at 180px 160px,#ffffff, transparent 60%);
            background-size: 240px 220px;
            background-repeat: repeat;
            opacity: .35;
            animation: starDrift 140s linear infinite;
        }

        @keyframes starDrift {
            from {
                background-position: 0 0
            }

            to {
                background-position: 240px 220px
            }
        }

        /* Слой 3: активные звёзды (canvas, рисуются в JS) */
        #spaceStars {
            position: fixed;
            inset: 0;
            z-index: -1;
            width: 100%;
            height: 100%;
            pointer-events: none;
            display: block;
        }

        /* ================= ОСТАЛЬНОЕ ================= */

        .topbar {
            flex: 0 0 auto;
            display: flex;
            align-items: center;
            justify-content: space-between;
            padding: 12px 16px;
            border-bottom: 1px solid rgba(120,140,200,.18);
            background: rgba(10,14,28,.55);
            backdrop-filter: blur(14px);
            -webkit-backdrop-filter: blur(14px);
            position: relative;
            z-index: 2
        }

        .logo {
            font-weight: 700;
            font-size: 17px;
            text-shadow: 0 0 12px rgba(120,150,255,.7)
        }

        .status {
            font-size: 12px;
            color: var(--dim)
        }

            .status.ok {
                color: var(--ok);
                text-shadow: 0 0 8px rgba(61,220,151,.8)
            }

            .status.err {
                color: var(--err);
                text-shadow: 0 0 8px rgba(255,92,92,.8)
            }

        main {
            flex: 1 1 auto;
            overflow-y: auto;
            -webkit-overflow-scrolling: touch;
            padding: 14px 14px 104px;
            position: relative;
            z-index: 1
        }

        h2 {
            font-size: 13px;
            text-transform: uppercase;
            letter-spacing: .09em;
            color: var(--dim);
            margin: 0 0 10px;
            font-weight: 600
        }

        .dance-grid {
            display: grid;
            grid-template-columns: repeat(2,1fr);
            gap: 12px
        }

        .dance-card {
            display: flex;
            flex-direction: column;
            align-items: flex-start;
            gap: 2px;
            padding: 15px 14px;
            border-radius: 18px;
            border: 1px solid rgba(120,140,200,.22);
            background: linear-gradient(160deg, rgba(29,36,51,.85), rgba(15,20,35,.85));
            backdrop-filter: blur(10px);
            -webkit-backdrop-filter: blur(10px);
            color: var(--txt);
            font-family: inherit;
            text-align: left;
            transition: transform .1s, box-shadow .2s;
            box-shadow: 0 4px 20px rgba(0,0,0,.35)
        }

            .dance-card:active {
                transform: scale(.97);
                box-shadow: 0 0 24px rgba(120,150,255,.35)
            }

            .dance-card .emoji {
                font-size: 26px;
                line-height: 1.2;
                filter: drop-shadow(0 0 8px rgba(150,180,255,.6))
            }

            .dance-card .name {
                font-size: 17px;
                font-weight: 700
            }

            .dance-card .tag {
                font-size: 10.5px;
                color: var(--dim);
                text-transform: uppercase;
                letter-spacing: .07em
            }

        .builder-head {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 8px;
            margin-bottom: 8px
        }

            .builder-head h2 {
                margin: 0
            }

        .btn {
            border: 1px solid rgba(120,140,200,.22);
            background: rgba(28,35,48,.75);
            backdrop-filter: blur(8px);
            -webkit-backdrop-filter: blur(8px);
            color: var(--txt);
            font-family: inherit;
            font-size: 13px;
            padding: 8px 12px;
            border-radius: 11px
        }

            .btn:active {
                transform: scale(.96)
            }

            .btn.primary {
                background: var(--acc);
                border-color: var(--acc);
                color: #08101f;
                font-weight: 700;
                box-shadow: 0 0 18px rgba(79,140,255,.5)
            }

            .btn.danger {
                color: var(--err);
                border-color: #4a2a30
            }

        .program {
            min-height: 80px;
            padding-bottom: 6px
        }

        .block {
            border: 1px solid rgba(120,140,200,.22);
            border-left: 4px solid var(--cat,var(--misc));
            background: rgba(28,35,48,.85);
            backdrop-filter: blur(8px);
            -webkit-backdrop-filter: blur(8px);
            border-radius: 12px;
            padding: 7px 8px;
            margin-bottom: 7px;
            box-shadow: 0 2px 10px rgba(0,0,0,.3)
        }

        .block-head {
            display: flex;
            align-items: center;
            gap: 6px
        }

        .block-label {
            flex: 1;
            display: flex;
            flex-wrap: wrap;
            align-items: center;
            gap: 5px;
            font-size: 13.5px;
            line-height: 1.55;
            min-width: 0
        }

        .block-tools {
            display: flex;
            gap: 3px;
            flex: 0 0 auto
        }

        .icon-btn {
            width: 26px;
            height: 26px;
            border-radius: 8px;
            border: 1px solid rgba(120,140,200,.2);
            background: rgba(14,20,32,.85);
            color: var(--dim);
            font-size: 12px;
            line-height: 1;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 0;
            font-family: inherit
        }

            .icon-btn.danger {
                color: var(--err)
            }

        .nested {
            margin: 7px 0 0 4px;
            padding-left: 9px;
            border-left: 2px dashed rgba(120,140,200,.25)
        }

        .else-row {
            font-size: 11.5px;
            color: var(--dim);
            margin: 8px 0 0 4px;
            padding-left: 9px;
            border-left: 2px dashed rgba(120,140,200,.25);
            line-height: 1
        }

        .add-btn {
            width: 100%;
            margin: 2px 0 8px;
            padding: 9px;
            border-radius: 11px;
            border: 1px dashed rgba(120,140,200,.3);
            background: rgba(255,255,255,.02);
            color: var(--dim);
            font-family: inherit;
            font-size: 12.5px
        }

            .add-btn:active {
                background: rgba(120,150,255,.1)
            }

        .inp {
            background: rgba(8,12,20,.85);
            border: 1px solid rgba(120,140,200,.22);
            color: var(--txt);
            border-radius: 8px;
            padding: 4px 6px;
            font-size: 13px;
            font-family: inherit;
            min-width: 0;
            -webkit-appearance: none;
            appearance: none
        }

            .inp.num {
                width: 60px;
                text-align: center
            }

            .inp.expr {
                width: 64px;
                text-align: center
            }

            .inp.name {
                width: 62px;
                text-align: center;
                color: var(--var)
            }

        input[type="number"].inp::-webkit-outer-spin-button,
        input[type="number"].inp::-webkit-inner-spin-button {
            -webkit-appearance: none;
            margin: 0
        }

        input[type="number"].inp {
            -moz-appearance: textfield
        }

        select.inp {
            padding-right: 4px
        }

        .cond {
            display: inline-flex;
            gap: 4px;
            align-items: center
        }

        .hint {
            font-size: 11.5px;
            color: var(--dim);
            line-height: 1.5;
            margin: 10px 2px 0
        }

            .hint b {
                color: var(--txt);
                font-weight: 600
            }

        .cat-motion {
            --cat: var(--motion)
        }

        .cat-emotion {
            --cat: var(--emotion)
        }

        .cat-var {
            --cat: var(--var)
        }

        .cat-ctrl {
            --cat: var(--ctrl)
        }

        .cat-misc {
            --cat: var(--misc)
        }

        .cat-math {
            --cat: var(--math)
        }

        .dpad {
            display: grid;
            grid-template-columns: repeat(3,1fr);
            grid-template-rows: repeat(3,1fr);
            gap: 10px;
            width: min(78vw,320px);
            margin: 26px auto;
            aspect-ratio: 1
        }

        .dp {
            border-radius: 22px;
            border: 1px solid rgba(120,140,200,.22);
            background: linear-gradient(160deg,rgba(29,36,51,.9),rgba(14,22,35,.9));
            backdrop-filter: blur(8px);
            -webkit-backdrop-filter: blur(8px);
            color: var(--txt);
            font-size: 24px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-family: inherit;
            transition: transform .08s, background .15s, color .15s, box-shadow .2s;
            box-shadow: 0 4px 16px rgba(0,0,0,.3)
        }

            .dp.on {
                background: var(--acc);
                color: #08101f;
                border-color: var(--acc);
                transform: scale(.96);
                box-shadow: 0 0 28px rgba(79,140,255,.7)
            }

            .dp.up {
                grid-area: 1/2
            }

            .dp.left {
                grid-area: 2/1
            }

            .dp.right {
                grid-area: 2/3
            }

            .dp.down {
                grid-area: 3/2
            }

            .dp.center {
                grid-area: 2/2;
                font-size: 15px;
                font-weight: 700;
                background: rgba(42,23,25,.9);
                color: var(--err);
                border-color: #5a3038
            }

                .dp.center:active {
                    background: #3a1d22;
                    transform: scale(.96)
                }

        .tabbar {
            position: fixed;
            left: 0;
            right: 0;
            bottom: 0;
            display: flex;
            z-index: 20;
            background: rgba(10,14,28,.72);
            backdrop-filter: blur(16px);
            -webkit-backdrop-filter: blur(16px);
            border-top: 1px solid rgba(120,140,200,.18);
            padding-bottom: env(safe-area-inset-bottom)
        }

            .tabbar button {
                flex: 1;
                background: none;
                border: 0;
                color: var(--dim);
                font-family: inherit;
                font-size: 19px;
                padding: 9px 0 7px;
                display: flex;
                flex-direction: column;
                align-items: center;
                gap: 3px;
                transition: color .15s, text-shadow .2s
            }

                .tabbar button span {
                    font-size: 10.5px;
                    letter-spacing: .03em
                }

                .tabbar button.active {
                    color: var(--acc);
                    text-shadow: 0 0 12px rgba(79,140,255,.85)
                }

        .nowplaying {
            position: fixed;
            left: 12px;
            right: 12px;
            bottom: calc(70px + env(safe-area-inset-bottom));
            z-index: 19;
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 10px 12px;
            border-radius: 14px;
            background: rgba(20,28,45,.85);
            backdrop-filter: blur(14px);
            -webkit-backdrop-filter: blur(14px);
            border: 1px solid rgba(120,140,200,.25);
            box-shadow: 0 8px 30px rgba(0,0,0,.55), 0 0 24px rgba(79,140,255,.15)
        }

        .np-name {
            flex: 1;
            font-size: 14px;
            font-weight: 600
        }

        .np-dot {
            width: 9px;
            height: 9px;
            border-radius: 50%;
            background: var(--ok);
            box-shadow: 0 0 10px var(--ok);
            animation: pulse 1s infinite
        }

        @keyframes pulse {
            50% {
                opacity: .25;
                box-shadow: 0 0 4px var(--ok)
            }
        }

        .np-stop {
            border: 1px solid #4a2a30;
            background: rgba(42,23,25,.9);
            color: var(--err);
            border-radius: 10px;
            padding: 7px 14px;
            font-family: inherit;
            font-size: 13px;
            font-weight: 700
        }

        .backdrop {
            position: fixed;
            inset: 0;
            background: rgba(0,0,0,.62);
            z-index: 30;
            backdrop-filter: blur(2px);
            -webkit-backdrop-filter: blur(2px)
        }

        .sheet {
            position: fixed;
            left: 0;
            right: 0;
            bottom: 0;
            z-index: 31;
            background: rgba(18,24,38,.92);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border-radius: 20px 20px 0 0;
            padding: 16px 14px calc(20px + env(safe-area-inset-bottom));
            max-height: 76vh;
            overflow-y: auto;
            box-shadow: 0 -10px 40px rgba(0,0,0,.6), 0 0 60px rgba(120,150,255,.15);
            border-top: 1px solid rgba(120,140,200,.28)
        }

        .sheet-title {
            font-weight: 700;
            margin-bottom: 4px
        }

        .sheet-sub {
            font-size: 12px;
            color: var(--dim);
            margin-bottom: 8px
        }

        .pal-group {
            font-size: 10.5px;
            text-transform: uppercase;
            letter-spacing: .09em;
            color: var(--dim);
            margin: 13px 0 6px
        }

        .pal-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px
        }

        .pal-item {
            padding: 11px 10px;
            border-radius: 11px;
            background: rgba(28,35,48,.85);
            border: 1px solid rgba(120,140,200,.22);
            border-left: 4px solid var(--cat,var(--misc));
            color: var(--txt);
            font-family: inherit;
            font-size: 13px;
            text-align: left
        }

            .pal-item:active {
                background: rgba(37,48,63,.95)
            }

        [hidden] {
            display: none !important
        }
    </style>
</head>
<body>

    <canvas id="spaceStars"></canvas>

    <div class="topbar">
        <div class="logo">🤖 Биби</div>
        <div class="status" id="status">нет связи</div>
    </div>

    <main>
        <section class="view" id="view-dances">
            <h2>Танцы</h2>
            <div class="dance-grid" id="danceGrid"></div>
        </section>

        <section class="view" id="view-builder" hidden>
            <div class="builder-head">
                <h2>Мой танец</h2>
                <div style="display:flex;gap:6px">
                    <button class="btn primary" id="btnRun">▶</button>
                    <button class="btn danger" id="btnStop">■</button>
                    <button class="btn" id="btnClear">✕</button>
                </div>
            </div>
            <div class="program" id="programRoot"></div>
            <p class="hint">
                Углы <b>0–180</b>: для рук 0 — опущена, 180 — поднята; для поворота 90 — нейтраль.<br>
            </p>
        </section>

        <section class="view" id="view-drive" hidden>
            <h2>Движение</h2>
            <div class="dpad">
                <button class="dp up" data-dir="forward">▲</button>
                <button class="dp left" data-dir="left">◀</button>
                <button class="dp center" data-dir="stop">СТОП</button>
                <button class="dp right" data-dir="right">▶</button>
                <button class="dp down" data-dir="back">▼</button>
            </div>
            <p class="hint">Нажми направление — робот едет, пока не нажмёшь это же направление ещё раз, другое направление или СТОП.</p>
        </section>
    </main>

    <div class="nowplaying" id="nowPlaying" hidden>
        <div class="np-dot"></div>
        <div class="np-name">—</div>
        <button class="np-stop" id="npStop">СТОП</button>
    </div>

    <nav class="tabbar">
        <button data-view="dances" class="active">🎵<span>Танцы</span></button>
        <button data-view="builder">🧩<span>Конструктор</span></button>
        <button data-view="drive">🕹<span>Движение</span></button>
    </nav>

    <div class="backdrop" id="backdrop" hidden></div>
    <div class="sheet" id="sheet" hidden>
        <div class="sheet-title">Добавить блок</div>
        <div class="sheet-sub">Куда добавляем — там и появится</div>
        <div id="sheetContent"></div>
    </div>

    <script>
        "use strict";

        /* =========================================================
           КОСМИЧЕСКИЙ ФОН — звёзды на canvas
           ========================================================= */
        (function spaceCanvas() {
            const canvas = document.getElementById('spaceStars');
            const ctx = canvas.getContext('2d');
            let W = 0, H = 0, dpr = 1;
            let stars = [];
            let shooting = null;
            let lastShoot = 0;

            function resize() {
                dpr = Math.min(window.devicePixelRatio || 1, 2);
                W = window.innerWidth;
                H = window.innerHeight;
                canvas.width = W * dpr;
                canvas.height = H * dpr;
                canvas.style.width = W + 'px';
                canvas.style.height = H + 'px';
                ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
                spawnStars();
            }

            function spawnStars() {
                const count = Math.min(160, Math.floor((W * H) / 7000));
                stars = [];
                for (let i = 0; i < count; i++) {
                    const depth = Math.random();          // 0..1
                    stars.push({
                        x: Math.random() * W,
                        y: Math.random() * H,
                        r: 0.3 + depth * 1.3,
                        base: 0.35 + depth * 0.55,
                        twinkleSpeed: 0.5 + Math.random() * 1.8,
                        phase: Math.random() * Math.PI * 2,
                        drift: (0.02 + depth * 0.06),
                        hue: Math.random() < 0.15 ? 200 : (Math.random() < 0.1 ? 320 : 0)
                    });
                }
            }

            function spawnShootingStar() {
                const fromLeft = Math.random() < 0.5;
                shooting = {
                    x: fromLeft ? -40 : W + 40,
                    y: 40 + Math.random() * (H * 0.35),
                    vx: fromLeft ? (7 + Math.random() * 4) : -(7 + Math.random() * 4),
                    vy: 1.5 + Math.random() * 1.5,
                    life: 0,
                    maxLife: 60 + Math.random() * 30
                };
            }

            let t = 0;
            function draw() {
                t += 0.016;
                ctx.clearRect(0, 0, W, H);

                for (const s of stars) {
                    // мерцание
                    const tw = 0.6 + 0.4 * Math.sin(t * s.twinkleSpeed + s.phase);
                    const alpha = s.base * tw;

                    // очень медленный дрейф вниз-вправо
                    s.x += s.drift * 0.15;
                    s.y += s.drift * 0.05;
                    if (s.x > W + 4) s.x = -4;
                    if (s.y > H + 4) s.y = -4;

                    // цвет
                    let fill;
                    if (s.hue === 0) fill = `rgba(255,255,255,${alpha})`;
                    else if (s.hue === 200) fill = `rgba(170,210,255,${alpha})`;
                    else fill = `rgba(255,180,225,${alpha})`;

                    // ореол для крупных
                    if (s.r > 1.1) {
                        const grd = ctx.createRadialGradient(s.x, s.y, 0, s.x, s.y, s.r * 4);
                        grd.addColorStop(0, fill);
                        grd.addColorStop(1, 'rgba(0,0,0,0)');
                        ctx.fillStyle = grd;
                        ctx.beginPath();
                        ctx.arc(s.x, s.y, s.r * 4, 0, Math.PI * 2);
                        ctx.fill();
                    }

                    ctx.fillStyle = fill;
                    ctx.beginPath();
                    ctx.arc(s.x, s.y, s.r, 0, Math.PI * 2);
                    ctx.fill();
                }

                // падающая звезда
                if (shooting) {
                    const sh = shooting;
                    sh.x += sh.vx;
                    sh.y += sh.vy;
                    sh.life++;

                    const fade = Math.max(0, 1 - sh.life / sh.maxLife);
                    const tail = 60;
                    const grd = ctx.createLinearGradient(
                        sh.x, sh.y,
                        sh.x - sh.vx * (tail / 8), sh.y - sh.vy * (tail / 8)
                    );
                    grd.addColorStop(0, `rgba(255,255,255,${0.9 * fade})`);
                    grd.addColorStop(0.3, `rgba(160,200,255,${0.5 * fade})`);
                    grd.addColorStop(1, 'rgba(0,0,0,0)');

                    ctx.strokeStyle = grd;
                    ctx.lineWidth = 2;
                    ctx.lineCap = 'round';
                    ctx.beginPath();
                    ctx.moveTo(sh.x, sh.y);
                    ctx.lineTo(sh.x - sh.vx * (tail / 8), sh.y - sh.vy * (tail / 8));
                    ctx.stroke();

                    if (sh.life >= sh.maxLife || sh.x < -100 || sh.x > W + 100 || sh.y > H + 100) {
                        shooting = null;
                    }
                } else if (performance.now() - lastShoot > 6500 + Math.random() * 8000) {
                    spawnShootingStar();
                    lastShoot = performance.now();
                }

                requestAnimationFrame(draw);
            }

            window.addEventListener('resize', resize);
            resize();
            lastShoot = performance.now();
            requestAnimationFrame(draw);
        })();

        /* =========================================================
           Протокол: GET /cmd/<command>[/<arg>]
             /cmd/forward  /cmd/back  /cmd/left  /cmd/right  /cmd/stop  /cmd/ping
             /cmd/leftarm/<0-180>   /cmd/rightarm/<0-180>
             /cmd/face/<id>

           Углы — АБСОЛЮТНЫЕ, 0..180, без отрицательных.
           Для arm:  0 = рука опущена, 180 = поднята.
           ========================================================= */

        const $ = id => document.getElementById(id);

        /* ---------------- ТРАНСПОРТ ---------------- */
        let connTimer = null;
        async function send(...tokens) {
            const path = '/cmd/' + tokens.map(t => encodeURIComponent(t)).join('/');
            try {
                const res = await fetch(path, { cache: 'no-store' });
                setStatus(res.ok);
                return res.ok;
            } catch (e) {
                setStatus(false);
                return false;
            }
        }
        function setStatus(ok) {
            const el = $('status');
            el.textContent = ok ? '● на связи' : '● нет связи';
            el.className = 'status ' + (ok ? 'ok' : 'err');
            clearTimeout(connTimer);
            if (ok) connTimer = setTimeout(() => {
                el.textContent = '● на связи'; el.className = 'status ok';
            }, 4000);
        }

        /* ---------------- ОБЩЕЕ СОСТОЯНИЕ ---------------- */
        const state = { running: false, stop: false };
        const sleep = ms => new Promise(r => setTimeout(r, ms));
        const num = v => { const n = parseFloat(String(v).replace(',', '.')); return isFinite(n) ? n : 0; };
        const clampAngle = v => Math.max(0, Math.min(180, Math.round(num(v))));

        /* ---------------- ЭМОЦИИ ---------------- */
        const FACES = [
            { id: 'default', name: 'Обычное лицо' },
            { id: 'left', name: 'Левый глаз закрыт' },
            { id: 'right', name: 'Правый глаз закрыт' },
            { id: 'both', name: 'Оба глаза закрыты' },
            { id: 'wink', name: 'Подмигивание' },
            { id: 'tendernese', name: 'Улыбка' },
            { id: 'sadness', name: 'Грусть' },
            { id: 'angry', name: 'Злость' },
            { id: 'love', name: 'Любовь' }
        ];

        /* ---------------- ВКЛАДКИ ---------------- */
        document.querySelectorAll('.tabbar button').forEach(btn => {
            btn.addEventListener('click', () => {
                document.querySelectorAll('.view').forEach(v => v.hidden = (v.id !== 'view-' + btn.dataset.view));
                document.querySelectorAll('.tabbar button').forEach(b => b.classList.toggle('active', b === btn));
            });
        });

        /* =========================================================
                                ТАНЦЫ
           ========================================================= */
        const DANCES = [
            {
                id: '67', name: '67', emoji: '🔥', tag: 'Reels', seq: [
                    { t: 'face', face: 'default', d: 200 },
                    { t: 'arm', side: 'right', angle: 110, d: 380 },
                    { t: 'arm', side: 'left', angle: 110, d: 380 },
                    { t: 'face', face: 'wink', d: 300 },
                    { t: 'arm', side: 'right', angle: 25, d: 300 },
                    { t: 'arm', side: 'left', angle: 25, d: 300 },
                    { t: 'face', face: 'both', d: 350 },
                    { t: 'arm', side: 'right', angle: 125, d: 400 },
                    { t: 'arm', side: 'left', angle: 125, d: 400 },
                    { t: 'face', face: 'tendernese', d: 350 },
                    { t: 'arm', side: 'right', angle: 0, d: 300 },
                    { t: 'arm', side: 'left', angle: 0, d: 300 },
                    { t: 'face', face: 'default', d: 200 }
                ]
            },
            {
                id: 'mognu', name: 'Могну', emoji: '💃', tag: 'Reels', seq: [
                    { t: 'face', face: 'tendernese', d: 250 },
                    { t: 'arm', side: 'right', angle: 90, d: 350 },
                    { t: 'arm', side: 'right', angle: 0, d: 300 },
                    { t: 'arm', side: 'left', angle: 90, d: 350 },
                    { t: 'arm', side: 'left', angle: 0, d: 300 },
                    { t: 'face', face: 'wink', d: 350 },
                    { t: 'arm', side: 'right', angle: 130, d: 380 },
                    { t: 'arm', side: 'left', angle: 130, d: 380 },
                    { t: 'face', face: 'both', d: 400 },
                    { t: 'arm', side: 'right', angle: 0, d: 300 },
                    { t: 'arm', side: 'left', angle: 0, d: 300 },
                    { t: 'face', face: 'default', d: 200 }
                ]
            },
            {
                id: 'otvinta', name: 'От винта!', emoji: '🚀', tag: 'Смешарики', seq: [
                    { t: 'face', face: 'default', d: 250 },
                    { t: 'arm', side: 'right', angle: 45, d: 250 },
                    { t: 'arm', side: 'left', angle: 45, d: 250 },
                    { t: 'face', face: 'wink', d: 300 },
                    { t: 'arm', side: 'right', angle: 180, d: 200 },
                    { t: 'arm', side: 'left', angle: 180, d: 200 },
                    { t: 'face', face: 'tendernese', d: 300 },
                    { t: 'arm', side: 'right', angle: 135, d: 250 },
                    { t: 'arm', side: 'left', angle: 135, d: 250 },
                    { t: 'face', face: 'love', d: 400 },
                    { t: 'arm', side: 'right', angle: 180, d: 300 },
                    { t: 'arm', side: 'left', angle: 180, d: 300 },
                    { t: 'face', face: 'wink', d: 350 },
                    { t: 'arm', side: 'right', angle: 0, d: 300 },
                    { t: 'arm', side: 'left', angle: 0, d: 300 },
                    { t: 'face', face: 'tendernese', d: 350 },
                    { t: 'face', face: 'default', d: 200 }
                ]
            }
        ];

        function buildDanceGrid() {
            const grid = $('danceGrid');
            grid.innerHTML = '';
            DANCES.forEach(d => {
                const b = document.createElement('button');
                b.className = 'dance-card';
                b.innerHTML = `<span class="emoji">${d.emoji}</span>
                       <span class="name">${d.name}</span>
                       <span class="tag">${d.tag}</span>`;
                b.addEventListener('click', () => playDance(d));
                grid.appendChild(b);
            });
        }

        async function execStep(s) {
            switch (s.t) {
                case 'arm': return send(s.side + 'arm', clampAngle(s.angle));
                case 'face': return send('face', s.face);
                case 'move': return send(s.dir);
                case 'stop': return send('stop');
            }
        }

        async function playDance(d) {
            if (state.running) stopAll();
            await sleep(120);
            state.running = true;
            state.stop = false;
            showNowPlaying(d.name);
            for (const s of d.seq) {
                if (state.stop) break;
                await execStep(s);
                await sleep(s.d ?? 250);
            }
            state.running = false;
            hideNowPlaying();
        }

        /* =========================================================
                             КОНСТРУКТОР
           ========================================================= */
        let program = [];
        let addTarget = null;

        const PALETTE = [
            {
                title: 'Движение', cat: 'motion', items: [
                    { type: 'arm', label: 'Поднять руку' },
                ]
            },
            { title: 'Эмоции', cat: 'emotion', items: [{ type: 'face', label: 'Эмоция' }] },
            {
                title: 'Переменные', cat: 'var', items: [
                    { type: 'setvar', label: 'Задать переменную' },
                    { type: 'addvar', label: 'Изменить переменную' },
                    { type: 'arith', label: 'Формула A (+,−,×,÷) B' }
                ]
            },
            {
                title: 'Управление', cat: 'ctrl', items: [
                    { type: 'if', label: 'Если … то' },
                    { type: 'for', label: 'Повторить N раз' },
                    { type: 'while', label: 'Пока … делать' },
                    { type: 'dowhile', label: 'Делать, пока …' }
                ]
            },
            { title: 'Прочее', cat: 'misc', items: [{ type: 'wait', label: 'Ждать' }] }
        ];

        function createBlock(type) {
            switch (type) {
                case 'arm': return { type, side: 'right', angle: '90' };
                case 'face': return { type, face: 'default' };
                case 'wait': return { type, ms: '500' };
                case 'setvar': return { type, name: 'x', value: '0' };
                case 'addvar': return { type, name: 'x', value: '1' };
                case 'arith': return { type, name: 'x', a: '0', op: '+', b: '0' };
                case 'if': return { type, cond: { left: 'x', op: '>', right: '0' }, body: [], elseBody: [] };
                case 'for': return { type, times: '3', body: [] };
                case 'while': return { type, cond: { left: 'x', op: '<', right: '3' }, body: [] };
                case 'dowhile': return { type, cond: { left: 'x', op: '<', right: '3' }, body: [] };
            }
        }

        /* ---- фабрики DOM ---- */
        function mkInput(value, cls, onChange, inputMode) {
            const el = document.createElement('input');
            el.type = 'text';
            el.className = 'inp ' + cls;
            el.value = value;
            if (inputMode) el.inputMode = inputMode;
            el.autocomplete = 'off';
            el.spellcheck = false;
            el.addEventListener('input', () => onChange(el.value));
            el.addEventListener('focus', () => el.select());
            return el;
        }

        function mkAngle(value, onChange) {
            const el = document.createElement('input');
            el.type = 'number';
            el.min = '0';
            el.max = '180';
            el.step = '1';
            el.className = 'inp num';
            el.value = String(clampAngle(value));
            el.inputMode = 'numeric';
            el.autocomplete = 'off';
            el.addEventListener('input', () => onChange(el.value));
            el.addEventListener('blur', () => {
                const v = clampAngle(el.value);
                el.value = String(v);
                onChange(String(v));
            });
            el.addEventListener('focus', () => el.select());
            return el;
        }

        function mkSelect(options, value, onChange) {
            const s = document.createElement('select');
            s.className = 'inp';
            options.forEach(([v, n]) => {
                const o = document.createElement('option');
                o.value = v; o.textContent = n; s.appendChild(o);
            });
            s.value = value;
            s.addEventListener('change', () => onChange(s.value));
            return s;
        }
        function txt(t) { const s = document.createElement('span'); s.textContent = t; return s; }

        function sideSelect(b, key) {
            return mkSelect([['right', 'правую'], ['left', 'левую']], b[key], v => b[key] = v);
        }
        function faceSelect(b, key) {
            return mkSelect(FACES.map(f => [f.id, f.name]), b[key], v => b[key] = v);
        }
        function nameInput(b, key) {
            return mkInput(b[key], 'name', v => {
                b[key] = v.replace(/[^A-Za-z0-9_]/g, '').replace(/^[0-9]/, '');
            }, 'text');
        }
        function condEditor(b) {
            const wrap = document.createElement('span');
            wrap.className = 'cond';
            wrap.appendChild(mkInput(b.cond.left, 'expr', v => b.cond.left = v, 'text'));
            wrap.appendChild(mkSelect(
                [['==', '='], ['!=', '≠'], ['>', '>'], ['<', '<'], ['>=', '≥'], ['<=', '≤']],
                b.cond.op, v => b.cond.op = v
            ));
            wrap.appendChild(mkInput(b.cond.right, 'expr', v => b.cond.right = v, 'text'));
            return wrap;
        }

        function catOf(type) {
            if (type === 'face') return 'emotion';
            if (type === 'setvar' || type === 'addvar' || type === 'arith') return 'var';
            if (type === 'if' || type === 'for' || type === 'while' || type === 'dowhile') return 'ctrl';
            return 'misc';
        }

        const ARITH_OPS = [
            ['+', '+'], ['-', '−'], ['*', '×'], ['/', '÷']
        ];

        function buildLabel(el, b) {
            switch (b.type) {
                case 'arm':
                    el.append(txt('Поднять руку'), sideSelect(b, 'side'), txt('на'),
                        mkAngle(b.angle, v => b.angle = v), txt('° (0=низ, 180=верх)'));
                    break;
                case 'face':
                    el.append(txt('Эмоция'), faceSelect(b, 'face'));
                    break;
                case 'wait':
                    el.append(txt('Ждать'), mkInput(b.ms, 'num', v => b.ms = v, 'numeric'), txt('мс'));
                    break;
                case 'setvar':
                    el.append(txt('Задать'), nameInput(b, 'name'), txt('='),
                        mkInput(b.value, 'expr', v => b.value = v, 'text'));
                    break;
                case 'addvar':
                    el.append(txt('Изменить'), nameInput(b, 'name'), txt('на'),
                        mkInput(b.value, 'expr', v => b.value = v, 'text'));
                    break;
                case 'arith':
                    el.append(
                        txt('Задать'), nameInput(b, 'name'), txt('='),
                        mkInput(b.a ?? '0', 'expr', v => b.a = v, 'text'),
                        mkSelect(ARITH_OPS, b.op, v => b.op = v),
                        mkInput(b.b ?? '0', 'expr', v => b.b = v, 'text')
                    );
                    break;
                case 'if': el.append(txt('Если'), condEditor(b), txt('то')); break;
                case 'for': el.append(txt('Повторить'), mkInput(b.times, 'num', v => b.times = v, 'numeric'), txt('раз')); break;
                case 'while': el.append(txt('Пока'), condEditor(b), txt('делать')); break;
                case 'dowhile': el.append(txt('Делать, пока'), condEditor(b)); break;
            }
        }

        function iconBtn(label, onClick, extra) {
            const b = document.createElement('button');
            b.className = 'icon-btn' + (extra ? ' ' + extra : '');
            b.textContent = label;
            b.addEventListener('click', onClick);
            return b;
        }

        function renderList(parent, list) {
            list.forEach((b, i) => parent.appendChild(renderBlock(b, list, i)));
            const add = document.createElement('button');
            add.className = 'add-btn';
            add.textContent = '+ блок';
            add.addEventListener('click', () => openSheet(list, list.length));
            parent.appendChild(add);
        }

        function renderBlock(b, list, i) {
            const wrap = document.createElement('div');
            const el = document.createElement('div');
            el.className = 'block cat-' + catOf(b.type);
            const head = document.createElement('div');
            head.className = 'block-head';
            const label = document.createElement('div');
            label.className = 'block-label';
            buildLabel(label, b);
            head.appendChild(label);
            const tools = document.createElement('div');
            tools.className = 'block-tools';
            tools.appendChild(iconBtn('↑', () => moveBlock(list, i, -1)));
            tools.appendChild(iconBtn('↓', () => moveBlock(list, i, 1)));
            tools.appendChild(iconBtn('✕', () => removeBlock(list, i), 'danger'));
            head.appendChild(tools);
            el.appendChild(head);
            if (b.body) {
                const nested = document.createElement('div');
                nested.className = 'nested';
                renderList(nested, b.body);
                el.appendChild(nested);
            }
            if (b.elseBody) {
                const er = document.createElement('div');
                er.className = 'else-row'; er.textContent = 'иначе';
                el.appendChild(er);
                const nested2 = document.createElement('div');
                nested2.className = 'nested';
                renderList(nested2, b.elseBody);
                el.appendChild(nested2);
            }
            wrap.appendChild(el);
            return wrap;
        }

        function renderProgram() {
            const root = $('programRoot');
            root.innerHTML = '';
            renderList(root, program);
            saveProgram();
        }
        function moveBlock(list, i, dir) {
            const j = i + dir;
            if (j < 0 || j >= list.length) return;
            const t = list[i]; list[i] = list[j]; list[j] = t;
            renderProgram();
        }
        function removeBlock(list, i) { list.splice(i, 1); renderProgram(); }

        function openSheet(list, index) {
            addTarget = { list, index };
            const c = $('sheetContent');
            c.innerHTML = '';
            PALETTE.forEach(group => {
                const h = document.createElement('div');
                h.className = 'pal-group'; h.textContent = group.title;
                c.appendChild(h);
                const g = document.createElement('div');
                g.className = 'pal-grid';
                group.items.forEach(it => {
                    const b = document.createElement('button');
                    b.className = 'pal-item cat-' + group.cat;
                    b.textContent = it.label;
                    b.addEventListener('click', () => { addBlockOfType(it.type); closeSheet(); });
                    g.appendChild(b);
                });
                c.appendChild(g);
            });
            $('sheet').hidden = false;
            $('backdrop').hidden = false;
        }
        function closeSheet() {
            $('sheet').hidden = true;
            $('backdrop').hidden = true;
            addTarget = null;
        }
        function addBlockOfType(type) {
            if (!addTarget) return;
            addTarget.list.splice(addTarget.index, 0, createBlock(type));
            renderProgram();
        }

        function saveProgram() {
            try { localStorage.setItem('bibi_program', JSON.stringify(program)); } catch (e) { }
        }
        function loadProgram() {
            try {
                const raw = localStorage.getItem('bibi_program');
                if (raw) {
                    const p = JSON.parse(raw);
                    if (Array.isArray(p)) program = p;
                }
            } catch (e) { }
        }

        /* =========================================================
                            ИНТЕРПРЕТАТОР
           ========================================================= */
        function evalExpr(src, vars) {
            const s = String(src ?? '0').trim();
            if (s === '') return 0;
            if (/^-?\d+(\.\d+)?$/.test(s)) return parseFloat(s);
            try {
                const names = Object.keys(vars).filter(n => /^[A-Za-z_]\w*$/.test(n));
                const fn = new Function(...names, '"use strict"; return (' + s + ');');
                const r = Number(fn(...names.map(n => vars[n] ?? 0)));
                return isFinite(r) ? r : 0;
            } catch (e) { return 0; }
        }
        function evalCond(c, vars) {
            const l = evalExpr(c.left, vars);
            const r = evalExpr(c.right, vars);
            switch (c.op) {
                case '==': return l == r;
                case '!=': return l != r;
                case '>': return l > r;
                case '<': return l < r;
                case '>=': return l >= r;
                case '<=': return l <= r;
            }
            return false;
        }

        async function execList(list, vars) {
            for (const b of list) {
                if (state.stop) return;
                await execBlock(b, vars);
            }
        }

        async function execBlock(b, vars) {
            if (state.stop) return;
            switch (b.type) {
                case 'arm': {
                    const a = clampAngle(b.angle);
                    await send(b.side + 'arm', a);
                    await sleep(Math.max(250, a * 7));
                    break;
                }
                case 'face':
                    await send('face', b.face);
                    await sleep(250);
                    break;
                case 'wait':
                    await sleep(Math.max(0, Math.min(60000, num(b.ms))));
                    break;
                case 'setvar': vars[b.name] = evalExpr(b.value, vars); break;
                case 'addvar': vars[b.name] = (vars[b.name] ?? 0) + evalExpr(b.value, vars); break;
                case 'arith': {
                    const a = evalExpr(b.a ?? '0', vars);
                    const c = evalExpr(b.b ?? '0', vars);
                    let r;
                    switch (b.op) {
                        case '+': r = a + c; break;
                        case '-': r = a - c; break;
                        case '*': r = a * c; break;
                        case '/': r = (c === 0) ? 0 : a / c; break;
                        default: r = 0;
                    }
                    vars[b.name] = isFinite(r) ? r : 0;
                    break;
                }
                case 'if':
                    if (evalCond(b.cond, vars)) await execList(b.body, vars);
                    else await execList(b.elseBody, vars);
                    break;
                case 'for': {
                    const n = Math.max(0, Math.min(1000, Math.floor(evalExpr(b.times, vars))));
                    for (let i = 0; i < n && !state.stop; i++) await execList(b.body, vars);
                    break;
                }
                case 'while': {
                    let guard = 0;
                    while (!state.stop && evalCond(b.cond, vars) && guard++ < 1000) await execList(b.body, vars);
                    break;
                }
                case 'dowhile': {
                    let guard = 0;
                    do { await execList(b.body, vars); }
                    while (!state.stop && evalCond(b.cond, vars) && guard++ < 1000);
                    break;
                }
            }
        }

        async function runProgram() {
            if (state.running) return;
            state.running = true;
            state.stop = false;
            showNowPlaying('Мой танец');
            const vars = {};
            try { await execList(program, vars); }
            catch (e) { console.error(e); }
            state.running = false;
            hideNowPlaying();
        }

        /* =========================================================
                            ДВИЖЕНИЕ (toggle)
           ========================================================= */
        let activeDir = null;

        function updateDpad() {
            document.querySelectorAll('.dp').forEach(b => {
                b.classList.toggle('on', b.dataset.dir === activeDir);
            });
        }
        async function toggleMove(dir) {
            if (activeDir === dir) {
                activeDir = null; updateDpad();
                await send('stop');
                return;
            }
            const prev = activeDir;
            activeDir = dir; updateDpad();
            if (prev) await send('stop');
            await send(dir);
        }
        function stopMove() {
            activeDir = null;
            updateDpad();
            send('stop');
        }
        document.querySelectorAll('.dp').forEach(btn => {
            const dir = btn.dataset.dir;
            btn.addEventListener('click', () => {
                if (dir === 'stop') stopMove();
                else toggleMove(dir);
            });
            btn.addEventListener('contextmenu', e => e.preventDefault());
        });

        /* =========================================================
                            СТОП / NOW PLAYING
           ========================================================= */
        function showNowPlaying(name) {
            $('nowPlaying').hidden = false;
            $('nowPlaying').querySelector('.np-name').textContent = name;
        }
        function hideNowPlaying() { $('nowPlaying').hidden = true; }
        function stopAll() {
            state.stop = true;
            state.running = false;
            stopMove();
            hideNowPlaying();
        }

        /* =========================================================
                            ИНИЦИАЛИЗАЦИЯ
           ========================================================= */
        $('backdrop').addEventListener('click', closeSheet);
        $('npStop').addEventListener('click', stopAll);
        $('btnStop').addEventListener('click', stopAll);
        $('btnRun').addEventListener('click', runProgram);
        $('btnClear').addEventListener('click', () => {
            if (program.length && confirm('Очистить скрипт?')) {
                program = [];
                renderProgram();
            }
        });
        $('programRoot').addEventListener('input', saveProgram);
         
        buildDanceGrid();
        loadProgram();
        renderProgram();
        updateDpad();
        send('ping');
    </script>
</body>
</html>
)rawliteral";


// =====================================================
// МОТОРЫ
// =====================================================

void setOneMotor(
  int pinA,
  int pinB,
  int power
) {

  power = constrain(
    power,
    -255,
    255
  );


  // ВПЕРЁД
  if (power > 0) {

    analogWrite(
      pinA,
      power
    );

    analogWrite(
      pinB,
      0
    );
  }


  // НАЗАД
  else if (power < 0) {

    analogWrite(
      pinA,
      0
    );

    analogWrite(
      pinB,
      -power
    );
  }


  // СТОП
  else {

    analogWrite(
      pinA,
      0
    );

    analogWrite(
      pinB,
      0
    );
  }
}


// =====================================================
// ДВА МОТОРА
// =====================================================

void setMotors(
  int leftPower,
  int rightPower
) {

  setOneMotor(
    MOTOR_1A,
    MOTOR_A1B,
    leftPower
  );


  setOneMotor(
    MOTOR_B1A,
    MOTOR_B2A,
    rightPower
  );
}


// =====================================================
// СТОП МОТОРОВ
// =====================================================

void stopMotors() {

  setMotors(
    0,
    0
  );
}


// =====================================================
// ОБЫЧНОЕ ДВИЖЕНИЕ
// =====================================================

void moveRobot(
  String dir
) {

  // ---------------------------------------------------
  // ВПЕРЁД
  // ---------------------------------------------------

  if (dir == "forward") {

    setMotors(
      DRIVE_SPEED,
      DRIVE_SPEED
    );

    currentDirection =
      "forward";
  }


  // ---------------------------------------------------
  // НАЗАД
  // ---------------------------------------------------

  else if (dir == "back") {

    setMotors(
      -DRIVE_SPEED,
      -DRIVE_SPEED
    );

    currentDirection =
      "back";
  }


  // ---------------------------------------------------
  // ВЛЕВО
  // ---------------------------------------------------

  else if (dir == "left") {

    setMotors(
      -DRIVE_SPEED,
      DRIVE_SPEED
    );

    currentDirection =
      "left";
  }


  // ---------------------------------------------------
  // ВПРАВО
  // ---------------------------------------------------

  else if (dir == "right") {

    setMotors(
      DRIVE_SPEED,
      -DRIVE_SPEED
    );

    currentDirection =
      "right";
  }


  // ---------------------------------------------------
  // СТОП
  // ---------------------------------------------------

  else {

    stopMotors();

    currentDirection =
      "stop";
  }
}


// =====================================================
// MPU6050
// =====================================================

void updateMPU() {

  if (!mpuOK) {

    return;
  }


  sensors_event_t a;
  sensors_event_t g;
  sensors_event_t temp;


  mpu.getEvent(
    &a,
    &g,
    &temp
  );


  // ROLL

  roll =
    atan2(
      a.acceleration.y,
      a.acceleration.z
    )
    * 180.0 / PI;


  // PITCH

  pitch =
    atan2(
      -a.acceleration.x,

      sqrt(
        a.acceleration.y *
        a.acceleration.y +

        a.acceleration.z *
        a.acceleration.z
      )
    )
    * 180.0 / PI;


  // GYRO

  gyroX =
    g.gyro.x *
    180.0 / PI;


  gyroY =
    g.gyro.y *
    180.0 / PI;


  gyroZ =
    g.gyro.z *
    180.0 / PI;
}


// =====================================================
// VL53L0X
// =====================================================

int getDistanceMM() {

  if (!distanceSensorOK) {

    return -1;
  }


  VL53L0X_RangingMeasurementData_t measure;


  lox.rangingTest(
    &measure,
    false
  );


  if (
    measure.RangeStatus != 4
  ) {

    return
      measure.RangeMilliMeter;
  }


  return -1;
}


// =====================================================
// ПРОВЕРКА ПРЕПЯТСТВИЯ
// =====================================================

void checkObstacle() {

  // Датчик вмешивается только при движении вперёд.

  if (
    currentDirection !=
    "forward"
  ) {

    return;
  }


  int distance =
    getDistanceMM();


  lastDistanceMM =
    distance;


  if (distance <= 0) {

    return;
  }


  if (
    distance <=
    STOP_DISTANCE_MM
  ) {

    stopMotors();

    currentDirection =
      "stop";


    Serial.print(
      "СТОП! Препятствие: "
    );

    Serial.print(
      distance
    );

    Serial.println(
      " мм"
    );
  }
}


// =====================================================
// LED MATRIX
// =====================================================

uint16_t XY(
  uint8_t x,
  uint8_t y
) {

  // Змейка

  if (
    y % 2 == 0
  ) {

    return
      y * 8 + x;
  }


  return
    y * 8 +
    (7 - x);
}


// =====================================================
// PIXEL
// =====================================================

void px(
  int x,
  int y,
  CRGB color
) {

  if (
    x >= 0 &&
    x < 8 &&
    y >= 0 &&
    y < 8
  ) {

    leds[
      XY(x, y)
    ] = color;
  }
}


// =====================================================
// CLEAR FACE
// =====================================================

void clearFace() {

  FastLED.clear();
}


// =====================================================
// ОБЫЧНОЕ ЛИЦО
// =====================================================

void faceDefault() {

  clearFace();

  CRGB c =
    CRGB::White;


  // ЛЕВЫЙ ГЛАЗ

  px(0,2,c);
  px(1,2,c);
  px(2,2,c);

  px(0,3,c);
  px(2,3,c);

  px(0,4,c);
  px(2,4,c);

  px(0,5,c);
  px(1,5,c);
  px(2,5,c);


  // ПРАВЫЙ ГЛАЗ

  px(5,2,c);
  px(6,2,c);
  px(7,2,c);

  px(5,3,c);
  px(7,3,c);

  px(5,4,c);
  px(7,4,c);

  px(5,5,c);
  px(6,5,c);
  px(7,5,c);


  FastLED.show();
}


// =====================================================
// ЛЕВЫЙ ГЛАЗ ЗАКРЫТ
// =====================================================

void faceLeftClosed() {

  clearFace();

  CRGB c =
    CRGB::White;


  px(0,4,c);
  px(1,4,c);
  px(2,4,c);


  px(5,2,c);
  px(6,2,c);
  px(7,2,c);

  px(5,3,c);
  px(7,3,c);

  px(5,4,c);
  px(7,4,c);

  px(5,5,c);
  px(6,5,c);
  px(7,5,c);


  FastLED.show();
}


// =====================================================
// ПРАВЫЙ ГЛАЗ ЗАКРЫТ
// =====================================================

void faceRightClosed() {

  clearFace();

  CRGB c =
    CRGB::White;


  px(0,2,c);
  px(1,2,c);
  px(2,2,c);

  px(0,3,c);
  px(2,3,c);

  px(0,4,c);
  px(2,4,c);

  px(0,5,c);
  px(1,5,c);
  px(2,5,c);


  px(5,4,c);
  px(6,4,c);
  px(7,4,c);


  FastLED.show();
}


// =====================================================
// ОБА ГЛАЗА ЗАКРЫТЫ
// =====================================================

void faceBothClosed() {

  clearFace();

  CRGB c =
    CRGB::White;


  px(0,4,c);
  px(1,4,c);
  px(2,4,c);


  px(5,4,c);
  px(6,4,c);
  px(7,4,c);


  FastLED.show();
}


// =====================================================
// ПОДМИГИВАНИЕ
// БЕЗ delay()
// =====================================================

void faceWink() {

  faceDefault();


  winkActive =
    true;


  winkStage =
    0;


  winkTimer =
    millis();
}


// =====================================================
// ОБНОВЛЕНИЕ ПОДМИГИВАНИЯ
// =====================================================

void updateFaceAnimation() {

  if (!winkActive) {

    return;
  }


  unsigned long now =
    millis();


  // Через 100 мс закрываем глаз

  if (
    winkStage == 0 &&
    now - winkTimer >= 100
  ) {

    faceLeftClosed();


    winkStage =
      1;


    winkTimer =
      now;


    return;
  }


  // Через 350 мс открываем

  if (
    winkStage == 1 &&
    now - winkTimer >= 350
  ) {

    faceDefault();


    winkActive =
      false;


    winkStage =
      0;
  }
}


// =====================================================
// НЕЖНОСТЬ
// =====================================================

void faceTender() {

  clearFace();

  CRGB c =
    CRGB::Yellow;


  px(0,4,c);
  px(1,3,c);
  px(2,4,c);


  px(5,4,c);
  px(6,3,c);
  px(7,4,c);


  FastLED.show();
}


// =====================================================
// ГРУСТЬ
// =====================================================

void faceSad() {

  clearFace();

  CRGB c =
    CRGB::Blue;


  px(0,3,c);
  px(1,2,c);
  px(2,2,c);

  px(0,4,c);
  px(1,4,c);
  px(2,4,c);


  px(5,2,c);
  px(6,2,c);
  px(7,3,c);

  px(5,4,c);
  px(6,4,c);
  px(7,4,c);


  FastLED.show();
}


// =====================================================
// ЗЛОСТЬ
// =====================================================

void faceAngry() {

  clearFace();

  CRGB c =
    CRGB::Red;


  px(0,2,c);
  px(1,3,c);
  px(2,3,c);


  px(5,3,c);
  px(6,3,c);
  px(7,2,c);


  px(0,4,c);
  px(1,4,c);
  px(2,4,c);


  px(5,4,c);
  px(6,4,c);
  px(7,4,c);


  FastLED.show();
}


// =====================================================
// LOVE
// =====================================================

void faceLove() {

  clearFace();

  CRGB c =
    CRGB::Red;


  px(3,2,c);
  px(4,2,c);


  for (
    int x = 2;
    x <= 5;
    x++
  ) {

    px(x,3,c);
  }


  for (
    int x = 1;
    x <= 6;
    x++
  ) {

    px(x,4,c);
  }


  for (
    int x = 0;
    x < 8;
    x++
  ) {

    px(x,5,c);
    px(x,6,c);
  }


  px(1,7,c);
  px(2,7,c);

  px(5,7,c);
  px(6,7,c);


  FastLED.show();
}


// =====================================================
// ВЫБОР ЛИЦА
// =====================================================

void showFace(
  String faceId
) {

  // Если пришло другое лицо,
  // отменяем текущий wink.

  if (
    faceId != "wink"
  ) {

    winkActive =
      false;

    winkStage =
      0;
  }


  if (
    faceId == "default"
  ) {

    faceDefault();
  }


  else if (
    faceId == "left"
  ) {

    faceLeftClosed();
  }


  else if (
    faceId == "right"
  ) {

    faceRightClosed();
  }


  else if (
    faceId == "both"
  ) {

    faceBothClosed();
  }


  else if (
    faceId == "wink"
  ) {

    faceWink();
  }


  else if (
    faceId == "tendernese" ||
    faceId == "happy"
  ) {

    faceTender();
  }


  else if (
    faceId == "sadness"
  ) {

    faceSad();
  }


  else if (
    faceId == "angry"
  ) {

    faceAngry();
  }


  else if (
    faceId == "love"
  ) {

    faceLove();
  }
}


// =====================================================
// WEB — ГЛАВНАЯ
// =====================================================

void handleRoot() {

  server.send_P(
    200,
    "text/html; charset=utf-8",
    INDEX_HTML
  );
}


// =====================================================
// WEB — КОМАНДЫ
// =====================================================

void handleCommand() {

  String uri =
    server.uri();


  Serial.print(
    "Команда: "
  );

  Serial.println(
    uri
  );


  if (
    !uri.startsWith(
      "/cmd/"
    )
  ) {

    server.send(
      404,
      "text/plain",
      "Not found"
    );

    return;
  }


  String command =
    uri.substring(5);


  // ===================================================
  // PING
  // ===================================================

  if (
    command == "ping"
  ) {

    server.send(
      200,
      "text/plain",
      "OK"
    );

    return;
  }


  // ===================================================
  // ВПЕРЁД
  // ===================================================

  if (
    command == "forward"
  ) {

    // Перед движением проверяем препятствие.

    if (distanceSensorOK) {

      int distance =
        getDistanceMM();


      lastDistanceMM =
        distance;


      if (
        distance > 0 &&
        distance <=
        STOP_DISTANCE_MM
      ) {

        stopMotors();

        currentDirection =
          "stop";


        server.send(
          200,
          "text/plain",
          "BLOCKED"
        );

        return;
      }
    }


    moveRobot(
      "forward"
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );

    return;
  }


  // ===================================================
  // НАЗАД
  // ===================================================

  if (
    command == "back"
  ) {

    moveRobot(
      "back"
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );

    return;
  }


  // ===================================================
  // ВЛЕВО
  // ===================================================

  if (
    command == "left"
  ) {

    moveRobot(
      "left"
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );

    return;
  }


  // ===================================================
  // ВПРАВО
  // ===================================================

  if (
    command == "right"
  ) {

    moveRobot(
      "right"
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );

    return;
  }


  // ===================================================
  // STOP
  // ===================================================

  if (
    command == "stop"
  ) {

    moveRobot(
      "stop"
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );

    return;
  }


  // ===================================================
  // DISTANCE
  // ===================================================

  if (
    command == "distance"
  ) {

    int distance =
      getDistanceMM();


    lastDistanceMM =
      distance;


    if (
      distance > 0
    ) {

      server.send(
        200,
        "text/plain",
        String(distance)
      );
    }

    else {

      server.send(
        503,
        "text/plain",
        "-1"
      );
    }


    return;
  }


  // ===================================================
  // MPU
  // ===================================================

  if (
    command == "gyro"
  ) {

    if (!mpuOK) {

      server.send(
        503,
        "application/json",
        "{\"ok\":false}"
      );

      return;
    }


    String json =
      "{";


    json +=
      "\"ok\":true,";


    json +=
      "\"pitch\":";

    json +=
      String(
        pitch,
        1
      );


    json +=
      ",\"roll\":";

    json +=
      String(
        roll,
        1
      );


    json +=
      ",\"gx\":";

    json +=
      String(
        gyroX,
        1
      );


    json +=
      ",\"gy\":";

    json +=
      String(
        gyroY,
        1
      );


    json +=
      ",\"gz\":";

    json +=
      String(
        gyroZ,
        1
      );


    json +=
      "}";


    server.send(
      200,
      "application/json",
      json
    );


    return;
  }


  // ===================================================
  // ЛЕВАЯ РУКА
  // ===================================================

  if (
    command.startsWith(
      "leftarm/"
    )
  ) {

    int angle =
      command
      .substring(8)
      .toInt();


    angle =
      constrain(
        angle,
        0,
        180
      );


    leftArm.write(
      angle
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );


    return;
  }


  // ===================================================
  // ПРАВАЯ РУКА
  // ===================================================

  if (
    command.startsWith(
      "rightarm/"
    )
  ) {

    int angle =
      command
      .substring(9)
      .toInt();


    angle =
      constrain(
        angle,
        0,
        180
      );


    rightArm.write(
      angle
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );


    return;
  }


  // ===================================================
  // FACE
  // ===================================================

  if (
    command.startsWith(
      "face/"
    )
  ) {

    String faceId =
      command.substring(5);


    showFace(
      faceId
    );


    server.send(
      200,
      "text/plain",
      "OK"
    );


    return;
  }


  // ===================================================
  // НЕИЗВЕСТНАЯ КОМАНДА
  // ===================================================

  server.send(
    404,
    "text/plain",
    "Command Not Found"
  );
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );


  delay(
    500
  );


  Serial.println();

  Serial.println(
    "======================"
  );

  Serial.println(
    "     БИБИ START"
  );

  Serial.println(
    "======================"
  );


  // ===================================================
  // МОТОРЫ
  // ===================================================

  pinMode(
    MOTOR_1A,
    OUTPUT
  );

  pinMode(
    MOTOR_A1B,
    OUTPUT
  );

  pinMode(
    MOTOR_B1A,
    OUTPUT
  );

  pinMode(
    MOTOR_B2A,
    OUTPUT
  );


  stopMotors();


  Serial.println(
    "Моторы готовы"
  );


  // ===================================================
  // I2C
  // ===================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  // ===================================================
  // MPU6050
  // ===================================================

  Serial.println(
    "Поиск MPU6050..."
  );


  if (
    mpu.begin()
  ) {

    mpuOK =
      true;


    Serial.println(
      "MPU6050 найден"
    );


    mpu.setAccelerometerRange(
      MPU6050_RANGE_8_G
    );


    mpu.setGyroRange(
      MPU6050_RANGE_500_DEG
    );


    mpu.setFilterBandwidth(
      MPU6050_BAND_21_HZ
    );
  }

  else {

    mpuOK =
      false;


    Serial.println(
      "MPU6050 НЕ найден"
    );
  }


  // ===================================================
  // VL53L0X
  // ===================================================

  Serial.println(
    "Поиск VL53L0X..."
  );


  if (
    lox.begin()
  ) {

    distanceSensorOK =
      true;


    Serial.println(
      "VL53L0X найден"
    );
  }

  else {

    distanceSensorOK =
      false;


    Serial.println(
      "VL53L0X НЕ найден"
    );
  }


  // ===================================================
  // LED MATRIX
  // ===================================================

  FastLED.addLeds<
    WS2812B,
    LED_PIN,
    GRB
  >(
    leds,
    NUM_LEDS
  );


  FastLED.setBrightness(
    BRIGHTNESS
  );


  FastLED.clear();

  FastLED.show();


  faceDefault();


  Serial.println(
    "Лицо готово"
  );


  // ===================================================
  // РУКИ
  // ===================================================

  leftArm.setPeriodHertz(
    50
  );


  rightArm.setPeriodHertz(
    50
  );


  leftArm.attach(
    LEFT_ARM_PIN,
    500,
    2400
  );


  rightArm.attach(
    RIGHT_ARM_PIN,
    500,
    2400
  );


  leftArm.write(
    0
  );


  rightArm.write(
    0
  );


  Serial.println(
    "Руки готовы"
  );


  // ===================================================
  // WI-FI
  // ===================================================

  Serial.println(
    "Запуск Wi-Fi..."
  );


  WiFi.mode(
    WIFI_AP
  );


  if (
    WiFi.softAP(
      ssid,
      password
    )
  ) {

    Serial.println(
      "Wi-Fi запущен"
    );
  }

  else {

    Serial.println(
      "Ошибка Wi-Fi"
    );
  }


  Serial.print(
    "SSID: "
  );

  Serial.println(
    ssid
  );


  Serial.print(
    "IP: "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  // ===================================================
  // WEB SERVER
  // ===================================================

  server.on(
    "/",
    HTTP_GET,
    handleRoot
  );


  server.onNotFound(
    handleCommand
  );


  server.begin();


  Serial.println(
    "HTTP сервер запущен"
  );


  Serial.println(
    "======================"
  );

  Serial.println(
    "Биби готов!"
  );

  Serial.println(
    "======================"
  );
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // WEB
  // ===================================================

  server.handleClient();


  // ===================================================
  // ЛИЦО
  // ===================================================

  updateFaceAnimation();


  // ===================================================
  // MPU6050
  //
  // Только читаем датчик.
  // На моторы MPU НЕ влияет.
  // ===================================================

  if (
    millis() -
    lastMPUCheck >= 50
  ) {

    lastMPUCheck =
      millis();


    updateMPU();
  }


  // ===================================================
  // VL53L0X
  //
  // Если Биби едет вперёд и перед ним препятствие,
  // останавливаем моторы.
  // ===================================================

  if (
    millis() -
    lastDistanceCheck >= 100
  ) {

    lastDistanceCheck =
      millis();


    checkObstacle();
  }
}