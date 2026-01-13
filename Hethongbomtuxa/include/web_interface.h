#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>

// Mã nguồn Giao diện Web (HTML/CSS/JS) được lưu trong bộ nhớ chương trình (PROGMEM)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>Hệ Thống Bơm Từ Xa</title>
    <style>
        :root {
            --bg-color: #121212;
            --card-bg: #1e1e1e;
            --text-color: #e0e0e0;
            --accent-color: #2196F3;
            --danger-color: #f44336;
            --success-color: #00e676;
            --led-on: #00ff00;
            --led-off: #333;
        }

        body {
            font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            margin: 0;
            padding: 10px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            -webkit-tap-highlight-color: transparent;
        }

        h2 { margin: 10px 0 20px; text-transform: uppercase; letter-spacing: 1px; font-weight: 300; text-align: center; }

        /* --- TABS NAVIGATION --- */
        .tabs {
            display: flex;
            background: #2c2c2c;
            padding: 5px;
            border-radius: 50px;
            margin-bottom: 20px;
            box-shadow: 0 4px 15px rgba(0,0,0,0.3);
            width: 100%;
            max-width: 400px;
            justify-content: space-between;
        }

        .tab-btn {
            flex: 1;
            background: transparent;
            border: none;
            color: #888;
            padding: 10px 0;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            border-radius: 40px;
            transition: all 0.3s ease;
            outline: none;
        }

        .tab-btn.active {
            background: var(--accent-color);
            color: white;
            box-shadow: 0 2px 10px rgba(33, 150, 243, 0.4);
        }

        /* --- CONTENT SECTIONS --- */
        .content {
            display: none;
            width: 100%;
            max-width: 600px;
            animation: fadeIn 0.4s ease-out;
        }
        .content.active { display: block; }

        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }

        /* --- STATUS BAR --- */
        .status-bar {
            background: #252525;
            padding: 15px;
            border-radius: 15px;
            margin-bottom: 20px;
            text-align: center;
            border: 1px solid #333;
        }
        .time-display { font-size: 2em; font-weight: bold; color: var(--accent-color); }
        .mode-btn {
            width: 100%;
            padding: 12px;
            margin-top: 10px;
            border: none;
            border-radius: 10px;
            font-weight: bold;
            font-size: 16px;
            cursor: pointer;
            transition: transform 0.2s;
        }
        .mode-btn:active { transform: scale(0.98); }
        .mode-auto { background: var(--success-color); color: #000; }
        .mode-manual { background: #ff9800; color: #000; }

        /* --- CONTROL GRID --- */
        .grid-container {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }

        .card {
            background: var(--card-bg);
            padding: 15px;
            border-radius: 15px;
            display: flex;
            flex-direction: column;
            align-items: center;
            box-shadow: 0 4px 10px rgba(0,0,0,0.2);
            transition: transform 0.2s;
            cursor: pointer;
            user-select: none;
        }
        .card:active { transform: scale(0.98); }

        /* --- BIG LED INDICATOR --- */
        .led {
            width: 50px;
            height: 50px;
            border-radius: 50%;
            background: var(--led-off);
            border: 3px solid #444;
            margin-bottom: 10px;
            transition: all 0.4s cubic-bezier(0.175, 0.885, 0.32, 1.275);
            box-shadow: inset 0 0 15px rgba(0,0,0,0.8);
        }

        .led.on {
            background: var(--led-on);
            border-color: #00cc00;
            box-shadow: 0 0 20px var(--led-on), inset 0 0 10px rgba(255,255,255,0.5);
            transform: scale(1.1);
        }

        .label { font-size: 14px; font-weight: 500; text-align: center; }
        .status-text { font-size: 12px; color: #888; margin-top: 5px; }

        /* --- FORMS --- */
        .form-group { margin-bottom: 15px; }
        .form-label { display: block; margin-bottom: 5px; color: #aaa; font-size: 14px; }
        .form-control {
            width: 100%;
            padding: 12px;
            background: #2c2c2c;
            border: 1px solid #444;
            border-radius: 8px;
            color: white;
            box-sizing: border-box;
            font-size: 16px;
        }
        .btn-submit {
            width: 100%;
            padding: 15px;
            background: var(--accent-color);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            margin-top: 10px;
        }

        /* --- LOGS --- */
        .log-container {
            background: #000;
            color: #00e676;
            font-family: monospace;
            padding: 15px;
            border-radius: 10px;
            height: 300px;
            overflow-y: auto;
            font-size: 12px;
            border: 1px solid #333;
        }

        @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.02); } 100% { transform: scale(1); } }
        .alert-box {
            display: none; background: #d32f2f; color: white; padding: 20px; border-radius: 15px; 
            margin-bottom: 20px; text-align: center; border: 2px solid #ff5252; animation: pulse 2s infinite;
        }
    </style>
</head>
<body>

    <h2>Hệ Thống Bơm</h2>

    <div class="tabs">
        <button class="tab-btn active" onclick="showTab('control')">Điều Khiển</button>
        <button class="tab-btn" onclick="showTab('settings')">Cài Đặt</button>
        <button class="tab-btn" onclick="showTab('logs')">Nhật Ký</button>
    </div>

    <!-- CẢNH BÁO LỖI -->
    <div id="fault-alert" class="alert-box">
        <h2 style="margin-top:0; color: #fff;">⚠️ SỰ CỐ ÁP SUẤT!</h2>
        <p>Hệ thống đã dừng khẩn cấp để bảo vệ.</p>
        <button class="btn-submit" style="background: white; color: #d32f2f; font-weight: bold;" onclick="resetFault()">RESET HỆ THỐNG</button>
    </div>

    <!-- TAB ĐIỀU KHIỂN -->
    <div id="control" class="content active">
        <div class="status-bar">
            <div class="time-display" id="clock">--:--</div>
            <button id="btn-mode" class="mode-btn mode-manual" onclick="toggleMode()">ĐANG CHẠY THỦ CÔNG</button>
        </div>

        <h4 style="margin-left: 10px; color: #888;">THIẾT BỊ CHÍNH</h4>
        <div class="grid-container">
            <div class="card" onclick="controlDev('pump')">
                <div id="led-pump" class="led"></div>
                <span class="label">MÁY BƠM</span>
                <span class="status-text" id="txt-pump">Tắt</span>
            </div>
            <div class="card" onclick="controlDev('source')">
                <div id="led-source" class="led"></div>
                <span class="label">NGUỒN VAN</span>
                <span class="status-text" id="txt-source">Tắt</span>
            </div>
        </div>

        <h4 style="margin-left: 10px; color: #888;">DANH SÁCH VAN</h4>
        <div class="grid-container" style="grid-template-columns: repeat(4, 1fr);">
            <script>
                for(let i=1; i<=8; i++) {
                    document.write(`<div class="card" onclick="controlValve(${i})"><div id="led-v${i}" class="led" style="width: 40px; height: 40px;"></div><span class="label">Van ${i}</span></div>`);
                }
            </script>
        </div>
    </div>

    <!-- TAB CÀI ĐẶT -->
    <div id="settings" class="content">
        <form action="/save" method="GET">
            <div class="card" style="width: 100%; box-sizing: border-box; margin-bottom: 20px;">
                <h3 style="margin-top:0">Giờ chạy tự động</h3>
                <div style="display: flex; gap: 10px;"><div style="flex:1"><label class="form-label">Sáng (Giờ)</label><input type="number" class="form-control" name="h1" id="h1"></div><div style="flex:1"><label class="form-label">Phút</label><input type="number" class="form-control" name="m1" id="m1"></div></div>
                <div style="display: flex; gap: 10px; margin-top: 10px;"><div style="flex:1"><label class="form-label">Chiều (Giờ)</label><input type="number" class="form-control" name="h2" id="h2"></div><div style="flex:1"><label class="form-label">Phút</label><input type="number" class="form-control" name="m2" id="m2"></div></div>
            </div>
            <div class="card" style="width: 100%; box-sizing: border-box;">
                <h3 style="margin-top:0">Thời gian tưới (giây)</h3>
                <div class="grid-container" style="grid-template-columns: repeat(2, 1fr); gap: 10px; margin-bottom: 0;">
                    <script>for(let i=1; i<=8; i++) { document.write(`<div><label class="form-label">Van ${i}</label><input type="number" class="form-control" name="t${i}" id="t${i}"></div>`); }</script>
                </div>
            </div>
            <button type="submit" class="btn-submit">LƯU CÀI ĐẶT</button>
        </form>
    </div>

    <!-- TAB LOGS -->
    <div id="logs" class="content">
        <div class="log-container" id="log-display">Đang tải dữ liệu...</div>
        <button class="btn-submit" style="background: #444; margin-top: 10px;" onclick="fetch('/control?dev=clear_fault_log&state=1')">Xóa Log</button>
    </div>

    <script>
        function showTab(id){document.querySelectorAll('.content').forEach(e=>e.classList.remove('active'));document.querySelectorAll('.tab-btn').forEach(e=>e.classList.remove('active'));document.getElementById(id).classList.add('active');event.target.classList.add('active');}
        function toggleMode(){fetch('/toggle_mode');}
        function resetFault(){fetch('/control?dev=reset_fault&state=1').then(()=>{location.reload();});}
        function controlDev(d){let e=document.getElementById('led-'+d);let s=e.classList.contains('on')?0:1;fetch('/control?dev='+d+'&state='+s+'&ajax=1');}
        function controlValve(i){let e=document.getElementById('led-v'+i);let s=e.classList.contains('on')?0:1;fetch('/control?dev=valve&id='+i+'&state='+s+'&ajax=1');if(s)e.classList.add('on');else e.classList.remove('on');}
        function setLed(n,s){let e=document.getElementById('led-'+n),t=document.getElementById('txt-'+n);if(s){e.classList.add('on');t.innerText='Đang chạy';t.style.color='#00ff00';}else{e.classList.remove('on');t.innerText='Tắt';t.style.color='#888';}}
        function update(){fetch('/status').then(r=>r.json()).then(d=>{if(d.fault){document.getElementById('fault-alert').style.display='block';document.getElementById('control').style.display='none';}else{document.getElementById('fault-alert').style.display='none';document.getElementById('control').style.display='block';}document.getElementById('clock').innerText=d.time;let b=document.getElementById('btn-mode');if(d.isAuto){b.className='mode-btn mode-auto';b.innerText='ĐANG CHẠY TỰ ĐỘNG (AUTO)';}else{b.className='mode-btn mode-manual';b.innerText='ĐANG CHẠY THỦ CÔNG (MANUAL)';}setLed('pump',d.pump);setLed('source',d.source);if(d.isAuto){for(let i=1;i<=8;i++){let e=document.getElementById('led-v'+i);if(d.activeValve==i)e.classList.add('on');else e.classList.remove('on');}}document.getElementById('log-display').innerHTML=d.log;});}
        setInterval(update,1000);update();
    </script>
</body>
</html>
)rawliteral";

#endif