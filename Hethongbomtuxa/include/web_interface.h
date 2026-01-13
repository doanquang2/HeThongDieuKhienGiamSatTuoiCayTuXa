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
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
        :root {
            --bg-gradient: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d); /* Gradient nền năng động */
            --bg-color: #1a1a2e;
            --card-bg: rgba(255, 255, 255, 0.1); /* Nền kính trong suốt */
            --text-color: #ffffff; /* Trắng (Chữ) */
            --accent-color: #ffc300; /* Vàng (Điểm nhấn/Tab) */
            --btn-grad: linear-gradient(45deg, #2196F3, #21CBF3); /* Gradient nút bấm */
            --danger-grad: linear-gradient(45deg, #ff512f, #dd2476);
            --success-color: #00e676;
            --led-on: #00ff00;     /* Đèn bật màu Xanh lá neon */
            --led-off: #222;       /* Đèn tắt màu tối */
        }

        body {
            font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
            background: #0f2027; /* Fallback */
            background: -webkit-linear-gradient(to right, #2c5364, #203a43, #0f2027);
            background: linear-gradient(to right, #2c5364, #203a43, #0f2027);
            background-attachment: fixed;
            color: var(--text-color);
            margin: 0;
            padding: 15px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            -webkit-tap-highlight-color: transparent;
        }

        h2 { 
            margin: 10px 0 20px; 
            text-transform: uppercase; 
            letter-spacing: 2px; 
            font-weight: 700; 
            text-align: center; 
            text-shadow: 0 2px 4px rgba(0,0,0,0.5);
        }

        /* --- TABS NAVIGATION --- */
        .tabs {
            display: flex;
            background: rgba(0, 0, 0, 0.3);
            padding: 5px;
            border-radius: 50px;
            margin-bottom: 20px;
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
            backdrop-filter: blur(5px);
            width: 100%;
            max-width: 500px;
            justify-content: space-between;
        }

        .tab-btn {
            flex: 1;
            background: transparent;
            border: none;
            color: #b0c4de;
            padding: 12px 0;
            font-size: 15px;
            font-weight: 600;
            cursor: pointer;
            border-radius: 40px;
            transition: all 0.3s ease;
            outline: none;
            display: flex; align-items: center; justify-content: center; gap: 8px;
        }

        .tab-btn.active {
            background: var(--accent-color); /* Màu Vàng */
            color: #001845; /* Chữ màu xanh đậm cho dễ đọc trên nền vàng */
            box-shadow: 0 4px 15px rgba(255, 195, 0, 0.5);
            transform: scale(1.05);
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
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            padding: 15px;
            border-radius: 20px;
            margin-bottom: 20px;
            text-align: center;
            border: 1px solid rgba(255, 255, 255, 0.1);
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        .time-display { font-size: 2.5em; font-weight: bold; color: var(--text-color); text-shadow: 0 0 10px rgba(255,255,255,0.3); }
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
        .mode-auto { background: var(--success-color); color: #fff; }
        .mode-manual { background: linear-gradient(45deg, #ff9966, #ff5e62); color: #fff; }

        /* --- CONTROL GRID --- */
        .grid-container {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(110px, 1fr)); /* Tự động chia cột, tối thiểu 110px */
            gap: 15px;
            margin-bottom: 20px;
        }

        /* Glassmorphism Card Style */
        .card {
            background: var(--card-bg);
            backdrop-filter: blur(10px);
            -webkit-backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.18);
            padding: 15px;
            border-radius: 16px;
            display: flex;
            flex-direction: column;
            align-items: center;
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.2);
            transition: transform 0.2s;
            cursor: pointer;
            user-select: none;
        }
        .card:active { transform: scale(0.95); }
        .card:hover { background: rgba(255, 255, 255, 0.15); }

        /* --- REALISTIC LED INDICATOR --- */
        .led {
            width: 45px;
            height: 45px;
            border-radius: 50%;
            background: radial-gradient(circle at 30% 30%, #555, #000);
            border: 2px solid #444;
            margin-bottom: 10px;
            transition: all 0.4s cubic-bezier(0.175, 0.885, 0.32, 1.275);
            box-shadow: 0 0 5px rgba(0,0,0,0.5), inset 0 0 5px rgba(0,0,0,1);
        }

        .led.on {
            background: radial-gradient(circle at 30% 30%, #80ff80, #00e600);
            border-color: #00ff00;
            box-shadow: 0 0 20px #00e600, inset 0 0 10px #fff;
            transform: scale(1.1);
        }

        .label { font-size: 14px; font-weight: 500; text-align: center; }
        .status-text { font-size: 12px; color: #b0c4de; margin-top: 5px; }

        /* --- FORMS --- */
        .form-group { margin-bottom: 15px; }
        .form-label { display: block; margin-bottom: 5px; color: #aaa; font-size: 14px; }
        .form-control {
            width: 100%;
            padding: 12px;
            background: rgba(0,0,0,0.4);
            border: 1px solid rgba(255,255,255,0.2);
            border-radius: 8px;
            color: white;
            box-sizing: border-box;
            font-size: 16px;
        }
        .btn-submit {
            width: 100%;
            padding: 15px;
            background: var(--btn-grad);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            margin-top: 10px;
            box-shadow: 0 4px 15px rgba(33, 150, 243, 0.4);
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
            border: 1px solid rgba(255,255,255,0.1);
        }

        @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.02); } 100% { transform: scale(1); } }
        .alert-box {
            display: none; background: #d32f2f; color: white; padding: 20px; border-radius: 15px; 
            margin-bottom: 20px; text-align: center; border: 2px solid #ff5252; animation: pulse 2s infinite;
        }

        @keyframes shake { 0% { transform: translate(1px, 1px) rotate(0deg); } 10% { transform: translate(-1px, -2px) rotate(-1deg); } 20% { transform: translate(-3px, 0px) rotate(1deg); } 30% { transform: translate(3px, 2px) rotate(0deg); } 40% { transform: translate(1px, -1px) rotate(1deg); } 50% { transform: translate(-1px, 2px) rotate(-1deg); } 60% { transform: translate(-3px, 1px) rotate(0deg); } 70% { transform: translate(3px, 1px) rotate(-1deg); } 80% { transform: translate(-1px, -1px) rotate(1deg); } 90% { transform: translate(1px, 2px) rotate(0deg); } 100% { transform: translate(1px, -2px) rotate(-1deg); } }
        .shake-icon { display: inline-block; animation: shake 0.5s infinite; margin-right: 10px; }

        /* --- TOAST NOTIFICATION --- */
        #toast-box { position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%); z-index: 1000; display: flex; flex-direction: column; gap: 10px; width: 90%; max-width: 400px; pointer-events: none; }
        .toast { background: rgba(0, 0, 0, 0.85); backdrop-filter: blur(5px); color: white; padding: 12px 20px; border-radius: 50px; box-shadow: 0 5px 15px rgba(0,0,0,0.5); display: flex; align-items: center; justify-content: center; animation: slideUp 0.3s ease-out; font-size: 14px; border: 1px solid rgba(255,255,255,0.1); }
        @keyframes slideUp { from { transform: translateY(20px); opacity: 0; } to { transform: translateY(0); opacity: 1; } }
        
        /* --- FOOTER --- */
        .footer { margin-top: auto; padding: 20px; text-align: center; font-size: 12px; color: rgba(255,255,255,0.5); }
    </style>
</head>
<body>

    <h2><i class="fa-solid fa-network-wired"></i> Hệ Thống Bơm</h2>

    <!-- TOAST CONTAINER -->
    <div id="toast-box"></div>

    <div class="tabs">
        <button class="tab-btn active" onclick="showTab('control')"><i class="fa-solid fa-gamepad"></i> Điều Khiển</button>
        <button class="tab-btn" onclick="showTab('settings')"><i class="fa-solid fa-gear"></i> Cài Đặt</button>
        <button class="tab-btn" onclick="showTab('logs')"><i class="fa-solid fa-clipboard-list"></i> Nhật Ký</button>
    </div>

    <!-- CẢNH BÁO LỖI -->
    <div id="fault-alert" class="alert-box">
        <h2 style="margin-top:0; color: #fff;"><i class="fa-solid fa-triangle-exclamation shake-icon"></i> SỰ CỐ ÁP SUẤT!</h2>
        <p>Hệ thống đã dừng khẩn cấp để bảo vệ.</p>
        <button class="btn-submit" style="background: white; color: #d32f2f; font-weight: bold;" onclick="resetFault()"><i class="fa-solid fa-rotate-right"></i> RESET HỆ THỐNG</button>
    </div>

    <!-- ĐẾM NGƯỢC PHẢN HỒI -->
    <div id="feedback-timer" class="alert-box" style="background: #0288d1; border-color: #29b6f6; animation: none;">
        <h3 style="margin:0; color: #fff;">
            <i class="fa-solid fa-spinner fa-spin"></i> ĐANG CHỜ PHẢN HỒI... <span id="timer-val" style="font-size: 1.2em; font-weight: bold; color: #ffeb3b;">0</span>s
        </h3>
    </div>

    <!-- TAB ĐIỀU KHIỂN -->
    <div id="control" class="content active">
        <div class="status-bar">
            <div class="time-display" id="clock">--:--</div>
            <button id="btn-mode" class="mode-btn mode-manual" onclick="toggleMode()"><i class="fa-solid fa-hand-pointer"></i> ĐANG CHẠY THỦ CÔNG</button>
        </div>

        <!-- SLAVE STATUS -->
        <div class="card" style="width:100%; flex-direction:row; justify-content: space-around; margin-bottom: 20px; box-sizing: border-box;">
            <div style="display:flex; align-items:center; gap:5px;"><i class="fa-solid fa-server"></i> Slave 1: <span id="s1-status" style="font-weight:bold">--</span></div>
            <div style="display:flex; align-items:center; gap:5px;"><i class="fa-solid fa-server"></i> Slave 2: <span id="s2-status" style="font-weight:bold">--</span></div>
        </div>

        <h4 style="margin-left: 10px; color: var(--accent-color); border-bottom: 1px solid rgba(255,255,255,0.1); padding-bottom: 5px;">THIẾT BỊ CHÍNH</h4>
        <div class="grid-container">
            <div class="card" onclick="controlDev('pump'); showToast('Đã gửi lệnh Bơm')">
                <div id="led-pump" class="led"></div>
                <span class="label"><i class="fa-solid fa-water"></i> MÁY BƠM</span>
                <span class="status-text" id="txt-pump">Tắt</span>
            </div>
            <div class="card" onclick="controlDev('source'); showToast('Đã gửi lệnh Nguồn Van')">
                <div id="led-source" class="led"></div>
                <span class="label"><i class="fa-solid fa-bolt"></i> NGUỒN VAN</span>
                <span class="status-text" id="txt-source">Tắt</span>
            </div>
        </div>

        <h4 style="margin-left: 10px; color: var(--accent-color); border-bottom: 1px solid rgba(255,255,255,0.1); padding-bottom: 5px;">DANH SÁCH VAN</h4>
        <div class="grid-container">
            <script>
                for(let i=1; i<=8; i++) {
                    document.write(`<div class="card" onclick="controlValve(${i}); showToast('Đã gửi lệnh Van ${i}')"><div id="led-v${i}" class="led" style="width: 40px; height: 40px;"></div><span class="label"><i class="fa-solid fa-faucet"></i> Van ${i}</span></div>`);
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
            <button type="submit" class="btn-submit" onclick="showToast('Đang lưu cài đặt...')"><i class="fa-solid fa-floppy-disk"></i> LƯU CÀI ĐẶT</button>
        </form>
        
        <form action="/set_time" method="GET" class="card" style="margin-top: 20px;">
             <h3 style="margin-top:0">Đồng bộ thời gian</h3>
             <input type="datetime-local" name="datetime" class="form-control">
             <button type="submit" class="btn-submit" style="background: linear-gradient(45deg, #007bff, #00c6ff); margin-top: 10px;" onclick="showToast('Đang cập nhật giờ...')"><i class="fa-solid fa-clock"></i> CẬP NHẬT THỜI GIAN</button>
        </form>

        <div class="card" style="margin-top: 20px; border: 1px solid var(--danger-color);">
            <h3 style="margin-top:0; color: #ff5252;">Vùng Nguy Hiểm</h3>
            <button class="btn-submit" style="background: var(--danger-grad);" onclick="if(confirm('Bạn có chắc muốn khởi động lại ESP32?')) { fetch('/control?dev=reboot&state=1'); showToast('Đang khởi động lại...'); }"><i class="fa-solid fa-power-off"></i> KHỞI ĐỘNG LẠI HỆ THỐNG</button>
        </div>
    </div>

    <!-- TAB LOGS -->
    <div id="logs" class="content">
        <div class="log-container" id="log-display">Đang tải dữ liệu...</div>
        <button class="btn-submit" style="background: #444; margin-top: 10px;" onclick="fetch('/control?dev=clear_fault_log&state=1')"><i class="fa-solid fa-trash"></i> Xóa Log</button>
    </div>

    <div class="footer">Hệ thống điều khiển tưới thông minh &copy; 2026</div>

    <script>
        function showTab(id){document.querySelectorAll('.content').forEach(e=>e.classList.remove('active'));document.querySelectorAll('.tab-btn').forEach(e=>e.classList.remove('active'));document.getElementById(id).classList.add('active');event.target.classList.add('active');if(id=='settings')loadSettings();}
        function loadSettings(){
            fetch('/get_settings').then(r=>r.json()).then(d=>{
                document.getElementById('h1').value=d.h1;
                document.getElementById('m1').value=d.m1;
                document.getElementById('h2').value=d.h2;
                document.getElementById('m2').value=d.m2;
                for(let i=1;i<=8;i++) if(document.getElementById('t'+i)) document.getElementById('t'+i).value=d['t'+i];
            });
        }
        function toggleMode(){fetch('/toggle_mode'); showToast('Đã chuyển chế độ');}
        function resetFault(){fetch('/control?dev=reset_fault&state=1').then(()=>{location.reload();});}
        function controlDev(d){let e=document.getElementById('led-'+d);let s=e.classList.contains('on')?0:1;fetch('/control?dev='+d+'&state='+s+'&ajax=1');}
        function controlValve(i){let e=document.getElementById('led-v'+i);let ic=e.nextElementSibling.querySelector('i');let s=e.classList.contains('on')?0:1;fetch('/control?dev=valve&id='+i+'&state='+s+'&ajax=1');if(s){e.classList.add('on');if(ic)ic.style.color='#ffc300';}else{e.classList.remove('on');if(ic)ic.style.color='';}}
        function setLed(n,s){let e=document.getElementById('led-'+n),t=document.getElementById('txt-'+n),ic=e.nextElementSibling.querySelector('i');if(s){e.classList.add('on');t.innerText='Đang chạy';t.style.color='#ffc300';if(ic)ic.style.color='#ffc300';}else{e.classList.remove('on');t.innerText='Tắt';t.style.color='#b0c4de';if(ic)ic.style.color='';}}
        function update(){fetch('/status').then(r=>r.json()).then(d=>{if(d.fault){document.getElementById('fault-alert').style.display='block';document.getElementById('control').style.display='none';}else{document.getElementById('fault-alert').style.display='none';document.getElementById('control').style.display='block';}if(d.countdown>0){document.getElementById('feedback-timer').style.display='block';document.getElementById('timer-val').innerText=d.countdown;}else{document.getElementById('feedback-timer').style.display='none';}document.getElementById('clock').innerText=d.time;let b=document.getElementById('btn-mode');if(d.isAuto){b.className='mode-btn mode-auto';b.innerHTML='<i class="fa-solid fa-robot"></i> ĐANG CHẠY TỰ ĐỘNG (AUTO)';}else{b.className='mode-btn mode-manual';b.innerHTML='<i class="fa-solid fa-hand-pointer"></i> ĐANG CHẠY THỦ CÔNG (MANUAL)';}setLed('pump',d.pump);setLed('source',d.source);if(d.isAuto){for(let i=1;i<=8;i++){let e=document.getElementById('led-v'+i),ic=e.nextElementSibling.querySelector('i');if(d.activeValve==i){e.classList.add('on');if(ic)ic.style.color='#ffc300';}else{e.classList.remove('on');if(ic)ic.style.color='';}}}document.getElementById('log-display').innerHTML=d.log;let s1=document.getElementById('s1-status');let s2=document.getElementById('s2-status');if(d.slave1){s1.innerText='Online';s1.style.color='#00e676';}else{s1.innerText='Offline';s1.style.color='#f44336';}if(d.slave2){s2.innerText='Online';s2.style.color='#00e676';}else{s2.innerText='Offline';s2.style.color='#f44336';}});}
        setInterval(update,1000);update();
        
        function showToast(msg) {
            let t = document.createElement('div'); t.className = 'toast'; t.innerHTML = '<i class="fa-solid fa-circle-info" style="margin-right:8px; color:#00d2ff"></i> ' + msg;
            document.getElementById('toast-box').appendChild(t);
            setTimeout(() => { t.style.opacity='0'; setTimeout(()=>t.remove(), 300); }, 3000);
        }
    </script>
</body>
</html>
)rawliteral";

#endif