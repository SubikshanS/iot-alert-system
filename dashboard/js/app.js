/**
 * Smart Fire, Smoke & Temperature IoT Monitoring Dashboard
 * Frontend Application Engine & Telemetry Visualizer
 */

class IoTMonitoringApp {
    constructor() {
        this.isSimMode = true;
        this.targetEspIp = "";
        this.pollInterval = null;
        this.chart = null;
        this.maxChartPoints = 30;
        this.chartLabels = [];
        this.tempHistory = [];
        this.smokeHistory = [];
        this.eventHistory = [];
        this.previousState = "NORMAL";
    }

    init() {
        this.initTheme();
        this.initChart();
        this.startPolling();
        console.log("[DASHBOARD] Monitoring application engine initialized.");
    }

    initTheme() {
        const savedTheme = localStorage.getItem("iot_dashboard_theme") || "dark";
        if (savedTheme === "light") {
            document.body.classList.remove("dark-theme");
            document.body.classList.add("light-theme");
        }
    }

    toggleTheme() {
        if (document.body.classList.contains("dark-theme")) {
            document.body.classList.remove("dark-theme");
            document.body.classList.add("light-theme");
            localStorage.setItem("iot_dashboard_theme", "light");
        } else {
            document.body.classList.remove("light-theme");
            document.body.classList.add("dark-theme");
            localStorage.setItem("iot_dashboard_theme", "dark");
        }
        if (this.chart) {
            this.chart.update();
        }
    }

    initChart() {
        const ctx = document.getElementById('telemetryChart').getContext('2d');
        
        // Seed initial historical buffer
        const now = new Date();
        for (let i = this.maxChartPoints - 1; i >= 0; i--) {
            const t = new Date(now.getTime() - i * 1000);
            this.chartLabels.push(t.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' }));
            this.tempHistory.push(26.5);
            this.smokeHistory.push(580);
        }

        this.chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: this.chartLabels,
                datasets: [
                    {
                        label: 'Temperature (°C)',
                        data: this.tempHistory,
                        borderColor: '#f97316',
                        backgroundColor: 'rgba(249, 115, 22, 0.1)',
                        borderWidth: 2.5,
                        tension: 0.3,
                        yAxisID: 'yTemp',
                        pointRadius: 2
                    },
                    {
                        label: 'Smoke Sensor (ADC)',
                        data: this.smokeHistory,
                        borderColor: '#38bdf8',
                        backgroundColor: 'rgba(56, 189, 248, 0.1)',
                        borderWidth: 2,
                        tension: 0.3,
                        yAxisID: 'ySmoke',
                        pointRadius: 2
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: { duration: 300 },
                interaction: { mode: 'index', intersect: false },
                plugins: {
                    legend: { display: false },
                    tooltip: { enabled: true }
                },
                scales: {
                    x: {
                        grid: { color: 'rgba(255, 255, 255, 0.05)' },
                        ticks: { color: '#94a3b8', maxTicksLimit: 8 }
                    },
                    yTemp: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        min: 10,
                        max: 65,
                        title: { display: true, text: 'Temp (°C)', color: '#f97316' },
                        grid: { color: 'rgba(255, 255, 255, 0.05)' },
                        ticks: { color: '#f97316' }
                    },
                    ySmoke: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        min: 0,
                        max: 4095,
                        title: { display: true, text: 'Smoke (ADC)', color: '#38bdf8' },
                        grid: { drawOnChartArea: false },
                        ticks: { color: '#38bdf8' }
                    }
                }
            }
        });
    }

    startPolling() {
        if (this.pollInterval) clearInterval(this.pollInterval);
        this.fetchTelemetry();
        this.pollInterval = setInterval(() => this.fetchTelemetry(), 1000);
    }

    async fetchTelemetry() {
        let endpoint = '/api/status';

        if (!this.isSimMode && this.targetEspIp) {
            // If user connected to hardware, query either direct or via local relay
            endpoint = `http://${this.targetEspIp}/api/status`;
        }

        try {
            const res = await fetch(endpoint, { cache: 'no-store' });
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            this.renderTelemetry(data);
            this.updateConnectionBadge(true);
        } catch (err) {
            console.warn(`[DASHBOARD] Fetch failed from ${endpoint}:`, err.message);
            this.updateConnectionBadge(false);
        }
    }

    updateConnectionBadge(isOnline) {
        const badge = document.getElementById('connBadge');
        if (this.isSimMode) {
            badge.className = 'badge badge-sim';
            badge.textContent = '🟡 SIMULATION DEMO MODE';
        } else if (isOnline) {
            badge.className = 'badge badge-live';
            badge.textContent = `🟢 LIVE NODE: ${this.targetEspIp}`;
        } else {
            badge.className = 'badge badge-offline';
            badge.textContent = `🔴 OFFLINE: ${this.targetEspIp}`;
        }
    }

    connectHardware() {
        const ipInput = document.getElementById('espIpInput');
        const ip = ipInput.value.trim();
        if (!ip) {
            alert("Please enter the ESP32 IP address (e.g. 192.168.1.105)");
            return;
        }
        this.targetEspIp = ip;
        this.isSimMode = false;
        console.log(`[DASHBOARD] Switched to live hardware monitoring at: ${ip}`);
        this.fetchTelemetry();
    }

    toggleSimMode() {
        this.isSimMode = true;
        this.targetEspIp = "";
        document.getElementById('espIpInput').value = "";
        console.log("[DASHBOARD] Switched to local simulated telemetry mode.");
        this.fetchTelemetry();
    }

    formatUptime(seconds) {
        const h = Math.floor(seconds / 3600).toString().padStart(2, '0');
        const m = Math.floor((seconds % 3600) / 60).toString().padStart(2, '0');
        const s = (seconds % 60).toString().padStart(2, '0');
        return `${h}:${m}:${s}`;
    }

    renderTelemetry(data) {
        const sys = data.system || {};
        const sensors = data.sensors || {};
        const health = data.health || {};
        const gps = data.gps || {};

        // 1. System Overview
        const stateStr = sys.state || 'NORMAL';
        document.getElementById('sysStateVal').textContent = `STATE_${stateStr}`;
        document.getElementById('sysThreatVal').textContent = sys.threat || 'NONE';
        document.getElementById('sysUptimeVal').textContent = this.formatUptime(sys.uptime_seconds || 0);
        document.getElementById('sysHeapVal').textContent = `${Math.round((sys.free_heap || 0) / 1024)} KB`;

        const statePill = document.getElementById('sysStatePill');
        statePill.textContent = stateStr;
        statePill.className = 'pill ' + (stateStr === 'NORMAL' ? 'pill-success' :
                                         stateStr === 'WARNING' ? 'pill-warning' : 'pill-danger');

        // 2. Alert Banner & Actions
        this.updateAlertBanner(stateStr, sys.threat, sensors);

        // 3. Temperature & Humidity
        const temp = parseFloat(sensors.temperature) || 0.0;
        const hum = parseFloat(sensors.humidity) || 0.0;
        document.getElementById('tempValue').textContent = temp.toFixed(1);
        document.getElementById('humValue').textContent = `${Math.round(hum)}%`;

        const tempBar = document.getElementById('tempBar');
        const tempPercent = Math.min(100, Math.max(0, (temp / 60.0) * 100));
        tempBar.style.width = `${tempPercent}%`;
        tempBar.style.backgroundColor = temp >= 45.0 ? 'var(--color-critical)' :
                                       temp >= 38.0 ? 'var(--color-warning)' : 'var(--color-normal)';

        const tempPill = document.getElementById('tempStatusPill');
        tempPill.textContent = temp >= 45.0 ? 'CRITICAL HEAT' : temp >= 38.0 ? 'ELEVATED' : 'NOMINAL';
        tempPill.className = 'pill ' + (temp >= 45.0 ? 'pill-danger' : temp >= 38.0 ? 'pill-warning' : 'pill-success');

        // 4. Smoke Sensor
        const smoke = parseInt(sensors.smoke_raw) || 0;
        const smokeVolt = parseFloat(sensors.smoke_voltage) || 0.0;
        document.getElementById('smokeRawValue').textContent = smoke;
        document.getElementById('smokeVoltValue').textContent = `${smokeVolt.toFixed(2)} V`;

        const smokeBar = document.getElementById('smokeBar');
        const smokePercent = Math.min(100, (smoke / 4095.0) * 100);
        smokeBar.style.width = `${smokePercent}%`;
        smokeBar.style.backgroundColor = smoke >= 2200 ? 'var(--color-critical)' :
                                        smoke >= 1200 ? 'var(--color-warning)' : 'var(--color-normal)';

        const smokePill = document.getElementById('smokeStatusPill');
        smokePill.textContent = smoke >= 2200 ? 'DENSE SMOKE' : smoke >= 1200 ? 'ELEVATED' : 'CLEAN';
        smokePill.className = 'pill ' + (smoke >= 2200 ? 'pill-danger' : smoke >= 1200 ? 'pill-warning' : 'pill-success');

        // 5. Flame Sensor
        const flameDetected = sensors.flame_detected === true;
        const flameIcon = document.getElementById('flameIcon');
        const flameText = document.getElementById('flameText');
        const flamePill = document.getElementById('flameStatusPill');
        const flamePinState = document.getElementById('flamePinState');

        if (flameDetected) {
            flameIcon.textContent = '🔥';
            flameIcon.className = 'large-status-icon icon-fire';
            flameText.textContent = 'OPTICAL FLAME DETECTED!';
            flamePill.textContent = 'FIRE TRIGGERED';
            flamePill.className = 'pill pill-danger';
            flamePinState.textContent = 'LM393 DO Low (Active)';
        } else {
            flameIcon.textContent = '🛡️';
            flameIcon.className = 'large-status-icon icon-clear';
            flameText.textContent = 'NO FLAME DETECTED';
            flamePill.textContent = 'CLEAR';
            flamePill.className = 'pill pill-success';
            flamePinState.textContent = 'LM393 DO High (Idle)';
        }

        // 6. GPS Geolocation
        const hasGpsFix = gps.has_fix === true;
        const gpsPill = document.getElementById('gpsFixPill');
        const gpsLat = document.getElementById('gpsLatVal');
        const gpsLng = document.getElementById('gpsLngVal');
        const gpsSats = document.getElementById('gpsSatsVal');
        const gpsTime = document.getElementById('gpsTimeVal');
        const gpsLink = document.getElementById('gpsMapLink');

        if (hasGpsFix) {
            gpsPill.textContent = '3D FIX ACTIVE';
            gpsPill.className = 'pill pill-success';
            gpsLat.textContent = `${parseFloat(gps.latitude).toFixed(6)}° N`;
            gpsLng.textContent = `${parseFloat(gps.longitude).toFixed(6)}° E`;
            gpsSats.textContent = `${gps.satellites} Satellites (HDOP: ${gps.hdop || '1.0'})`;
            gpsTime.textContent = gps.timestamp || 'Synchronized';
            gpsLink.href = gps.maps_url || `https://maps.google.com/?q=${gps.latitude},${gps.longitude}`;
            gpsLink.style.display = 'inline-block';
        } else {
            gpsPill.textContent = 'SEARCHING / INDOOR';
            gpsPill.className = 'pill pill-warning';
            gpsLat.textContent = 'Indoor (Unacquired)';
            gpsLng.textContent = 'Indoor (Unacquired)';
            gpsSats.textContent = `${gps.satellites || 0} Satellites visible`;
            gpsTime.textContent = 'Awaiting Time Lock';
            gpsLink.style.display = 'none';
        }

        // 7. Network Connectivity
        document.getElementById('wifiIpVal').textContent = sys.ip_address || '0.0.0.0';
        document.getElementById('wifiRssiVal').textContent = `${sys.rssi || 0} dBm (${sys.rssi > -65 ? 'Strong' : 'Moderate'})`;

        // 8. Sensor Health
        this.updateHealthBadge('healthDHT', health.dht_valid, 'DHT11');
        this.updateHealthBadge('healthMQ2', health.mq2_valid, 'MQ-2');
        this.updateHealthBadge('healthFlame', health.flame_valid, 'Flame');
        this.updateHealthBadge('healthGPS', hasGpsFix, 'GPS');

        // 9. Update Chart.js Series
        this.updateChart(temp, smoke);

        // 10. Check State Transition for Event Log
        if (stateStr !== this.previousState) {
            this.logEvent(stateStr, sys.threat);
            this.previousState = stateStr;
        }

        // Render events from server if present
        if (Array.isArray(data.event_log) && data.event_log.length > 0) {
            this.renderEventLog(data.event_log);
        }
    }

    updateHealthBadge(elemId, isValid, name) {
        const el = document.getElementById(elemId);
        if (isValid) {
            el.className = 'badge badge-success';
            el.textContent = 'HEALTHY (OK)';
        } else {
            el.className = 'badge badge-danger';
            el.textContent = 'FAULT / DISCONNECTED';
        }
    }

    updateAlertBanner(state, threat, sensors) {
        const banner = document.getElementById('alertBanner');
        const icon = document.getElementById('alertIcon');
        const title = document.getElementById('alertTitle');
        const details = document.getElementById('alertDetails');
        const btnReset = document.getElementById('btnResetAlarm');

        if (state === 'CRITICAL' || state === 'EMERGENCY') {
            banner.className = 'alert-banner alert-critical';
            icon.textContent = '🚨';
            title.textContent = state === 'EMERGENCY' ?
                'MANUAL EMERGENCY ALARM ACTIVE!' : 'CRITICAL FIRE ALARM TRIGGERED!';
            details.textContent = `Evacuate area immediately! Trigger: ${threat}. Temp: ${sensors.temperature}°C, Smoke: ${sensors.smoke_raw} ADC.`;
            btnReset.style.display = 'inline-block';
        } else if (state === 'WARNING') {
            banner.className = 'alert-banner alert-warning';
            icon.textContent = '⚠️';
            title.textContent = 'ELEVATED ENVIRONMENTAL WARNING';
            details.textContent = `Investigate area for potential hazards. Trigger: ${threat}. Smoke: ${sensors.smoke_raw} ADC, Temp: ${sensors.temperature}°C.`;
            btnReset.style.display = 'none';
        } else if (state === 'SENSOR_ERROR') {
            banner.className = 'alert-banner alert-warning';
            icon.textContent = '🔧';
            title.textContent = 'HARDWARE SENSOR DIAGNOSTIC ALERT';
            details.textContent = 'One or more sensors are disconnected or reading out-of-bounds.';
            btnReset.style.display = 'none';
        } else {
            banner.className = 'alert-banner alert-normal';
            icon.textContent = '🛡️';
            title.textContent = 'SYSTEM NOMINAL - ALL SENSORS CLEAR';
            details.textContent = 'No fire, smoke, or elevated heat signatures detected in monitoring zone.';
            btnReset.style.display = 'none';
        }
    }

    updateChart(temp, smoke) {
        if (!this.chart) return;
        const nowStr = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });

        this.chartLabels.push(nowStr);
        this.tempHistory.push(temp);
        this.smokeHistory.push(smoke);

        if (this.chartLabels.length > this.maxChartPoints) {
            this.chartLabels.shift();
            this.tempHistory.shift();
            this.smokeHistory.shift();
        }

        this.chart.update('none'); // Update smoothly without full redraw
    }

    logEvent(state, threat) {
        const timeStr = new Date().toLocaleTimeString();
        let type = 'INFO';
        let msg = `System transitioned to ${state}`;

        if (state === 'CRITICAL') {
            type = 'CRITICAL';
            msg = `Fire condition confirmed: ${threat}`;
        } else if (state === 'EMERGENCY') {
            type = 'EMERGENCY';
            msg = `Manual emergency button engaged`;
        } else if (state === 'WARNING') {
            type = 'WARNING';
            msg = `Advisory threshold exceeded: ${threat}`;
        } else if (state === 'NORMAL') {
            type = 'NORMAL';
            msg = `Conditions returned to normal`;
        }

        this.eventHistory.unshift({ timestamp: timeStr, type: type, message: msg });
        if (this.eventHistory.length > 20) this.eventHistory.pop();
        this.renderEventLog(this.eventHistory);
    }

    renderEventLog(events) {
        const tbody = document.getElementById('eventLogBody');
        if (!tbody) return;
        tbody.innerHTML = '';

        events.forEach(ev => {
            const tr = document.createElement('tr');
            const pillClass = ev.type === 'CRITICAL' || ev.type === 'EMERGENCY' ? 'badge-danger' :
                              ev.type === 'WARNING' ? 'badge-sim' : 'badge-success';
            tr.innerHTML = `
                <td><code>${ev.timestamp}</code></td>
                <td><span class="badge ${pillClass}">${ev.type}</span></td>
                <td>${ev.message}</td>
            `;
            tbody.appendChild(tr);
        });
    }

    clearEventLog() {
        this.eventHistory = [];
        this.renderEventLog([]);
    }

    async resetEmergency() {
        let endpoint = '/api/emergency/reset';
        if (!this.isSimMode && this.targetEspIp) {
            endpoint = `http://${this.targetEspIp}/api/emergency/reset`;
        }

        try {
            const res = await fetch(endpoint, { method: 'POST' });
            if (res.ok) {
                console.log("[DASHBOARD] Emergency reset request acknowledged.");
                this.fetchTelemetry();
            }
        } catch (err) {
            console.error("[DASHBOARD] Failed to reset emergency:", err);
        }
    }

    async triggerSim(type) {
        try {
            await fetch(`/api/simulate/trigger?type=${type}`, { method: 'POST' });
            this.fetchTelemetry();
        } catch (err) {
            console.error("[DASHBOARD] Sim trigger error:", err);
        }
    }
}

// Global App Instance
const app = new IoTMonitoringApp();
window.addEventListener('DOMContentLoaded', () => app.init());

